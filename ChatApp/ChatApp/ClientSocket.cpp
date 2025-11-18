// ClientSocket.cpp
#include "pch.h"
#include "ClientSocket.h"

CClientSocket::CClientSocket() { AfxSocketInit(); }
CClientSocket::~CClientSocket() { StopReceiveThread(); Close(); }

bool CClientSocket::Connect(LPCTSTR lpszHost, UINT nPort)
{
    if (!Create()) return false;
    return CSocket::Connect(lpszHost, nPort);
}

bool CClientSocket::SendJSON(const json& j)
{
    std::string data = j.dump() + "\n";
    return Send(data.c_str(), (int)data.size()) == (int)data.size();
}

std::string CClientSocket::ReceiveLine()
{
    std::string line;
    char buffer[4096];
    int len;

    SOCKET sock = m_hSocket;
    if (sock == INVALID_SOCKET) return "";

    u_long mode = 1;
    ioctlsocket(sock, FIONBIO, &mode);

    while (true) {
        len = ::recv(sock, buffer, sizeof(buffer) - 1, MSG_PEEK);
        if (len > 0) {
            buffer[len] = '\0';
            char* p = strchr(buffer, '\n');
            if (p) {
                int toRead = p - buffer + 1;
                ::recv(sock, buffer, toRead, 0);
                *p = '\0';
                if (p > buffer && *(p - 1) == '\r') *(p - 1) = '\0';
                line = buffer;
                break;
            }
            ::recv(sock, buffer, len, 0);
            line += std::string(buffer, len);
        }
        else if (len == 0) break;
        else {
            if (WSAGetLastError() == WSAEWOULDBLOCK) {
                Sleep(10);
                continue;
            }
            break;
        }
    }

    mode = 0;
    ioctlsocket(sock, FIONBIO, &mode);

    return line;
}

void CClientSocket::Close() { CSocket::Close(); }

void CClientSocket::SetMessageCallback(MessageCallback cb, void* pParam)
{
    m_pCallback = cb;
    m_pCallbackParam = pParam;
}

void CClientSocket::StartReceiveThread()
{
    if (m_hThread) return;
    m_bStop = false;
    m_hThread = (HANDLE)_beginthreadex(NULL, 0, ReceiveThread, this, 0, nullptr);
}

void CClientSocket::StopReceiveThread()
{
    m_bStop = true;
    if (m_hThread) {
        WaitForSingleObject(m_hThread, 5000);
        CloseHandle(m_hThread);
        m_hThread = nullptr;
    }
}

UINT WINAPI CClientSocket::ReceiveThread(LPVOID pParam)
{
    CClientSocket* pThis = (CClientSocket*)pParam;
    while (!pThis->m_bStop) {
        std::string line = pThis->ReceiveLine();
        if (line.empty()) { Sleep(10); continue; }
        try {
            json j = json::parse(line);
            if (pThis->m_pCallback)
                pThis->m_pCallback(j, pThis->m_pCallbackParam);
        }
        catch (...) {}
    }
    return 0;
}