// ChatServerService.cpp
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <mysql_driver.h>
#include <mysql_connection.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <nlohmann/json.hpp>
#include <thread>
#include <vector>
#include <map>
#include <mutex>
#include "Globals.h"
#include "ChatServerService.h"
#include "AesCrypto.h"

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "libmysql.lib")
#pragma comment(lib, "mysqlcppconn.lib")

using json = nlohmann::json;

VOID WINAPI ServiceMain(DWORD argc, LPTSTR* argv);
VOID WINAPI ServiceCtrlHandler(DWORD Opcode);
VOID StartTcpServer(VOID);
DWORD WINAPI ClientHandler(LPVOID lpParam);
VOID Cleanup(VOID);
bool Worker();

std::thread g_ServerThread;

int main() {
#ifdef _SERVICE
    SERVICE_TABLE_ENTRY ServiceTable[] = {
        { (LPWSTR)L"ChatServerService", (LPSERVICE_MAIN_FUNCTION)ServiceMain },
        { NULL, NULL }
    };

    OutputDebugStringA("Calling StartServiceCtrlDispatcher...\n");
    if (StartServiceCtrlDispatcher(ServiceTable) == FALSE) {
        DWORD err = GetLastError();
        char buf[256];
        sprintf_s(buf, "StartServiceCtrlDispatcher failed: %lu\n", err);
        OutputDebugStringA(buf);

        OutputDebugStringA("Running in console mode...\n");
        ServiceMain(0, NULL);
        system("pause");
    }
#else
    Worker();
    while (1)
        Sleep(1000);
#endif

    return 0;
}

bool Worker()
{
    bool serverKeyLoaded = AesCrypto::LoadKey("1aa07c508ed27ad8ad902d366b0fc864");
    if (!serverKeyLoaded) {
        OutputDebugStringA("FATAL: Cannot load AES key!\n");
    }
    OutputDebugStringA("AES Key loaded successfully!\n");

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        return false;
    }

    try {
        g_Driver = sql::mysql::get_mysql_driver_instance();
        g_Con = g_Driver->connect("tcp://127.0.0.1:3306", "root", "Khuongloc125");
        g_Con->setSchema("chat_app");
    }
    catch (sql::SQLException& e) {
        OutputDebugStringA(("MySQL Error: " + std::string(e.what()) + "\n").c_str());
        WSACleanup();

        return false;
    }

    g_ServerThread = std::thread(StartTcpServer);

    return true;
}

VOID WINAPI ServiceMain(DWORD argc, LPTSTR* argv) {
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);

    g_ServiceStatusHandle = RegisterServiceCtrlHandler(L"ChatServerService", ServiceCtrlHandler);
    if (!g_ServiceStatusHandle) return;

    g_ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    g_ServiceStatus.dwCurrentState = SERVICE_START_PENDING;
    g_ServiceStatus.dwWin32ExitCode = 0;
    g_ServiceStatus.dwCheckPoint = 0;
    g_ServiceStatus.dwWaitHint = 300000;
    SetServiceStatus(g_ServiceStatusHandle, &g_ServiceStatus);

    if (!Worker())
    {
        g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
        g_ServiceStatus.dwWin32ExitCode = 1;
        SetServiceStatus(g_ServiceStatusHandle, &g_ServiceStatus);
        return;
    }

    g_ServiceStatus.dwCurrentState = SERVICE_RUNNING;
    g_ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
    g_ServiceStatus.dwCheckPoint = 0;
    SetServiceStatus(g_ServiceStatusHandle, &g_ServiceStatus);

    g_ServiceStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (!g_ServiceStopEvent) {
        g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
        g_ServiceStatus.dwWin32ExitCode = 3;
        SetServiceStatus(g_ServiceStatusHandle, &g_ServiceStatus);
        Cleanup();
        return;
    }

    while (g_ServiceStatus.dwCurrentState == SERVICE_RUNNING) {
        Sleep(500);
    }

    if (g_ServerThread.joinable()) {
        g_ServerThread.join();
    }

    Cleanup();

    g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
    SetServiceStatus(g_ServiceStatusHandle, &g_ServiceStatus);
}

VOID WINAPI ServiceCtrlHandler(DWORD Opcode) {
    switch (Opcode) {
    case SERVICE_CONTROL_STOP:
    case SERVICE_CONTROL_SHUTDOWN:
        if (g_ServiceStatus.dwCurrentState == SERVICE_RUNNING) {
            g_ServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
            g_ServiceStatus.dwCheckPoint = 0;
            g_ServiceStatus.dwWaitHint = 15000;
            SetServiceStatus(g_ServiceStatusHandle, &g_ServiceStatus);

            if (g_ListenSocket != INVALID_SOCKET) {
                shutdown(g_ListenSocket, SD_BOTH);
                closesocket(g_ListenSocket);
                g_ListenSocket = INVALID_SOCKET;
            }
            if (g_ServiceStopEvent) {
                SetEvent(g_ServiceStopEvent);  
            }
        }
        break;
    case SERVICE_CONTROL_INTERROGATE:
        SetServiceStatus(g_ServiceStatusHandle, &g_ServiceStatus);
        break;
    }
}

VOID StartTcpServer(VOID) {
    ADDRINFOW hints = { 0 }, * result = NULL;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    WCHAR portStr[16];
    wsprintfW(portStr, L"%d", 8888);

    if (GetAddrInfoW(NULL, portStr, &hints, &result) != 0) {
        OutputDebugStringA("GetAddrInfoW failed\n");
        return;
    }

    g_ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (g_ListenSocket == INVALID_SOCKET) {
        FreeAddrInfoW(result);
        OutputDebugStringA("socket() failed\n");
        return;
    }

    INT nodelay = 1;
    setsockopt(g_ListenSocket, IPPROTO_TCP, TCP_NODELAY, (CHAR*)&nodelay, sizeof(nodelay));

    if (bind(g_ListenSocket, result->ai_addr, (INT)result->ai_addrlen) == SOCKET_ERROR) {
        closesocket(g_ListenSocket);
        g_ListenSocket = INVALID_SOCKET;
        FreeAddrInfoW(result);
        OutputDebugStringA("bind() failed\n");
        return;
    }

    FreeAddrInfoW(result);

    if (listen(g_ListenSocket, SOMAXCONN) == SOCKET_ERROR) {
        closesocket(g_ListenSocket);
        g_ListenSocket = INVALID_SOCKET;
        OutputDebugStringA("listen() failed\n");
        return;
    }

    OutputDebugStringA("Server listening on port 8888...\n");

    while (g_ServiceStatus.dwCurrentState == SERVICE_RUNNING) {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(g_ListenSocket, &readfds);

        timeval timeout = { 0, 100000 }; 

        int result = select(0, &readfds, NULL, NULL, &timeout);
        if (result == SOCKET_ERROR) {
            if (WSAGetLastError() != WSAEINTR)
                OutputDebugStringA("select() error\n");
            break;
        }

        if (result > 0 && FD_ISSET(g_ListenSocket, &readfds)) {
            SOCKET client = accept(g_ListenSocket, NULL, NULL);
            if (client != INVALID_SOCKET) {
                {
                    std::lock_guard<std::mutex> lock(g_ClientsMutex);
                    g_Clients.push_back(client);
                }
                std::thread(ClientHandler, (LPVOID)(UINT_PTR)client).detach();
            }
        }
    }

    if (g_ListenSocket != INVALID_SOCKET) {
        closesocket(g_ListenSocket);
        g_ListenSocket = INVALID_SOCKET;
    }

    OutputDebugStringA("TCP Server stopped.\n");
}

std::string ReceiveLine(SOCKET sock) {
    std::string line;
    char buffer[4096];
    int len;

    while ((len = recv(sock, buffer, sizeof(buffer) - 1, MSG_PEEK)) > 0) {
        buffer[len] = '\0';
        char* p = strchr(buffer, '\n');
        if (p) {
            int toRead = p - buffer + 1;
            recv(sock, buffer, toRead, 0);
            *p = '\0';
            if (p > buffer && *(p - 1) == '\r') *(p - 1) = '\0';
            line = buffer;
            break;
        }
        recv(sock, buffer, len, 0);
        line += std::string(buffer, len);
    }
    return line;
}


DWORD WINAPI ClientHandler(LPVOID lpParam) {
    SOCKET clientSocket = (SOCKET)(UINT_PTR)lpParam;
    char buffer[4096];
    int userId = -1;
    while (g_ServiceStatus.dwCurrentState == SERVICE_RUNNING) {
        std::string raw = ReceiveLine(clientSocket);
        if (raw.empty()) break;
        OutputDebugStringA(("Raw JSON received: [" + raw + "]\n").c_str());
        try {
            json j = json::parse(raw);
            std::string action = j["action"];
            json response;
            if (action == "register") {
                std::string username = j["username"];
                std::string password = j["password"];
                std::string plain = AesCrypto::Decrypt(password);
                if (plain.empty()) {
                    response = { {"success", false}, {"message", "Decrypt failed"} };
                }
                else {
                    std::string stored = AesCrypto::Encrypt(plain); 
                    auto* check = g_Con->prepareStatement("SELECT id FROM users WHERE username = ?");
                    check->setString(1, username);
                    auto* res = check->executeQuery();
                    if (res->next()) {
                        response = { {"success", false}, {"message", "Username already exists"} };
                    }
                    else {
                        auto* insert = g_Con->prepareStatement(
                            "INSERT INTO users (username, password_hash) VALUES (?, ?)");
                        insert->setString(1, username);
                        insert->setString(2, stored); 
                        insert->execute();
                        delete insert;
                        response = { {"success", true}, {"message", "Registered successfully"} };
                    }
                    delete res; delete check;
                }
            }
            else if (action == "login") {
                std::string username = j["username"];
                std::string password = j["password"];
                std::string plain = AesCrypto::Decrypt(password);
                if (plain.empty()) {
                    response = { {"success", false}, {"message", "Invalid credentials"} };
                }
                else {
                    std::string expected = AesCrypto::Encrypt(plain);
                    auto* pstmt = g_Con->prepareStatement(
                        "SELECT id FROM users WHERE username = ? AND password_hash = ?");
                    pstmt->setString(1, username);
                    pstmt->setString(2, expected);
                    auto* res = pstmt->executeQuery();
                    if (res->next()) {
                        userId = res->getInt("id");
                        {
                            std::lock_guard<std::mutex> lock(g_ClientsMutex);
                            g_UserToSocket[userId] = clientSocket;
                            g_SocketToUser[clientSocket] = userId;
                        }
                        auto* update = g_Con->prepareStatement(
                            "UPDATE users SET status = 'online', last_login = NOW() WHERE id = ?");
                        update->setInt(1, userId);
                        update->execute();
                        delete update;
                        BroadcastUserStatus(userId, "online");
                        response = {
                            {"action", "login"},
                            {"success", true},
                            {"user_id", userId} };
                    }
                    else {
                        response = { {"success", false}, {"message", "Invalid credentials"} };
                    }
                    delete res; delete pstmt;
                }
            }
            else if (action == "send_message" && userId != -1) {
                int receiverId = j["receiver_id"];
                std::string encryptedMsg = j["message"];

                std::string decrypted = AesCrypto::Decrypt(encryptedMsg);
                if (decrypted.empty()) {
                    response = { {"success", false}, {"message", "Invalid encrypted message"} };
                }
                else {
                    auto* pstmt = g_Con->prepareStatement(
                        "INSERT INTO messages (sender_id, receiver_id, message_text) VALUES (?, ?, ?)");
                    pstmt->setInt(1, userId);
                    pstmt->setInt(2, receiverId);
                    pstmt->setString(3, encryptedMsg);
                    pstmt->execute();
                    delete pstmt;

                    ForwardMessage(userId, receiverId, encryptedMsg);
                    response = { {"success", true} };
                }
            }
            else if (action == "get_users") {
                auto* stmt = g_Con->createStatement();
                auto* res = stmt->executeQuery("SELECT id, username, status FROM users");
                json users = json::array();
                while (res->next()) {
                    std::string status = "offline";
                    if (!res->isNull("status")) {
                        status = res->getString("status");
                    }
                    users.push_back({
                        {"id", res->getInt("id")},
                        {"username", res->getString("username")},
                        {"status", status}
                        });
                }
                response = { {"success", true}, {"users", users} };
                delete res; delete stmt;
            }
            else if (action == "get_messages" && userId != -1) {
                int withUser = j.value("with_user", -1);
                json messages = json::array();

                auto* pstmt = g_Con->prepareStatement(
                    "SELECT sender_id, message_text, sent_at FROM messages "
                    "WHERE (sender_id = ? AND receiver_id = ?) OR (sender_id = ? AND receiver_id = ?) "
                    "ORDER BY id ASC");
                pstmt->setInt(1, userId);
                pstmt->setInt(2, withUser);
                pstmt->setInt(3, withUser);
                pstmt->setInt(4, userId);
                auto* res = pstmt->executeQuery();
                while (res->next()) {
                    messages.push_back({
                        {"sender_id", res->getInt("sender_id")},
                        {"message", res->getString("message_text")},
                        {"ts", res->getString("sent_at")}
                        });
                }
                delete res; delete pstmt;
                response = { {"action", "messages"}, {"messages", messages} };
                }
            else {
                response = { {"success", false}, {"message", "Unknown action"} };
            }
            std::string resp = response.dump() + "\n";

            int sent = send(clientSocket, resp.c_str(), static_cast<int>(resp.size()), 0);

            if (sent == SOCKET_ERROR) {
                int error = WSAGetLastError();
                OutputDebugStringA(("SEND FAILED! Error: " + std::to_string(error) + "\n").c_str());
            }
            else {
                OutputDebugStringA(("SEND OK: [" + resp + "]\n").c_str());
            }
        }
        catch (const std::exception& e) {
            OutputDebugStringA(("JSON/MySQL Error: " + std::string(e.what()) + "\n").c_str());
            break;
        }
        catch (...) {
            OutputDebugStringA("Unknown error in ClientHandler\n");
            break;
        }
    }
    if (userId != -1) {
        {
            std::lock_guard<std::mutex> lock(g_ClientsMutex);
            g_UserToSocket.erase(userId);
            g_SocketToUser.erase(clientSocket);
            auto* update = g_Con->prepareStatement("UPDATE users SET status = 'offline' WHERE id = ?");
            update->setInt(1, userId);
            update->execute();
            delete update;
        }
        BroadcastUserStatus(userId, "offline");
    }
    {
        std::lock_guard<std::mutex> lock(g_ClientsMutex);
        auto it = std::find(g_Clients.begin(), g_Clients.end(), clientSocket);
        if (it != g_Clients.end()) g_Clients.erase(it);
    }
    closesocket(clientSocket);
    return 0;
}
VOID Cleanup(VOID) {
    {
        std::lock_guard<std::mutex> lock(g_ClientsMutex);
        for (SOCKET s : g_Clients) {
            shutdown(s, SD_BOTH);
            closesocket(s);
        }
        g_Clients.clear();
        g_UserToSocket.clear();
        g_SocketToUser.clear();
    }

    if (g_ListenSocket != INVALID_SOCKET) {
        shutdown(g_ListenSocket, SD_BOTH);
        closesocket(g_ListenSocket);
        g_ListenSocket = INVALID_SOCKET;
    }

    WSACleanup();

    if (g_Con) {
        try { delete g_Con; }
        catch (...) {}
        g_Con = nullptr;
    }

    OutputDebugStringA("Cleanup completed.\n");
}