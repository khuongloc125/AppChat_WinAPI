// ClientSocket.h
#pragma once
#include <afxsock.h>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

class CClientSocket : public CSocket
{
public:
    CClientSocket();
    ~CClientSocket();
    SOCKET GetSafeSocket() const { return m_hSocket; }
    bool Connect(LPCTSTR lpszHost, UINT nPort);
    bool SendJSON(const json& j);
    std::string ReceiveLine();
    void Close();

    void StartReceiveThread();
    void StopReceiveThread();

    typedef void (*MessageCallback)(const json& j, void* pParam);
    void SetMessageCallback(MessageCallback cb, void* pParam = nullptr);

private:
    static UINT WINAPI ReceiveThread(LPVOID pParam);
    HANDLE m_hThread = nullptr;
    bool m_bStop = false;
    MessageCallback m_pCallback = nullptr;
    void* m_pCallbackParam = nullptr;
};