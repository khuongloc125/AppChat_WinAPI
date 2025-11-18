// Login.cpp : implementation file
//

#include "pch.h"
#include "ChatApp.h"
#include "afxdialogex.h"
#include "Login.h"

#include "ChatAppDlg.h"
#include "Register.h"
#include "ChatApp.h"
#include "AesCrypto.h"


// Login dialog

IMPLEMENT_DYNAMIC(Login, CDialogEx)

Login::Login(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_LOGIN, pParent)
{

}

Login::~Login()
{
}

void Login::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_USERNAME, m_userName);
	DDX_Control(pDX, IDC_PASSWORD, m_passWord);
}


BEGIN_MESSAGE_MAP(Login, CDialogEx)
	ON_BN_CLICKED(IDC_REGISTER, &Login::OnBnClickedRegister)
	ON_BN_CLICKED(IDC_LOGIN, &Login::OnBnClickedLogin)
END_MESSAGE_MAP()


// Login message handlers

void Login::OnBnClickedRegister()
{
	// TODO: Add your control notification handler code here
	EndDialog(IDCANCEL);
	Register registerDgl;
	registerDgl.DoModal();
	
}

void Login::OnBnClickedLogin()
{
	// TODO: Add your control notification handler code here
    CString username, password;
    m_userName.GetWindowText(username);
    m_passWord.GetWindowText(password);

    if (username.IsEmpty() || password.IsEmpty()) {
        AfxMessageBox(L"Nhập đầy đủ!");
        return;
    }

    theApp.m_pSocket = new CClientSocket();
    if (!theApp.m_pSocket->Connect(_T("127.0.0.1"), 8888)) {
        AfxMessageBox(L"Không kết nối server!");
        delete theApp.m_pSocket;
        theApp.m_pSocket = nullptr;
        return;
    }

    std::string plain = CT2A(password.GetString());
    std::string enc = AesCrypto::Encrypt(plain);

    json j = {
        {"action", "login"},
        {"username", CT2A(username.GetString())},
        {"password", enc}
    };

    if (!theApp.m_pSocket->SendJSON(j)) {
        AfxMessageBox(L"Gửi thất bại!");
        delete theApp.m_pSocket;
        theApp.m_pSocket = nullptr;
        return;
    }

    std::string loginResp;
    while (true) {
        std::string line = theApp.m_pSocket->ReceiveLine();
        if (line.empty()) { Sleep(100); continue; }

        OutputDebugStringA(("RECV: " + line + "\n").c_str());

        try {
            json msg = json::parse(line);
            if (msg.value("action", "") == "login") {
                loginResp = line;
                break;
            }
        }
        catch (const std::exception& e) {
            OutputDebugStringA(("JSON parse error: " + std::string(e.what()) + "\n").c_str());
        }
    }

    if (loginResp.empty()) {
        AfxMessageBox(L"Không nhận được phản hồi login!");
        delete theApp.m_pSocket;
        theApp.m_pSocket = nullptr;
        return;
    }

    try {
        json r = json::parse(loginResp);
        if (r.is_array() && !r.empty()) r = r[0];
        //false
        if (r["action"] == "login" && r.value("success", true) == true) {
            theApp.m_nUserId = r["user_id"];
            theApp.m_strUsername = username;

            EndDialog(IDOK);
            CChatAppDlg dlg;
            dlg.SetSocket(theApp.m_pSocket);  
            theApp.m_pMainWnd = &dlg;
            dlg.DoModal();

            theApp.m_pSocket->StopReceiveThread();
            delete theApp.m_pSocket;
            theApp.m_pSocket = nullptr;
        }
        else {
            std::string msg = r.value("message", "Lỗi đăng nhập");
            AfxMessageBox(CString(msg.c_str()));
            delete theApp.m_pSocket;
            theApp.m_pSocket = nullptr;
        }
    }
    catch (const std::exception& e) {
        AfxMessageBox(L"Lỗi JSON: " + CString(e.what()));
        delete theApp.m_pSocket;
        theApp.m_pSocket = nullptr;
    }
}
