// Register.cpp : implementation file
//

#include "pch.h"
#include "ChatApp.h"
#include "afxdialogex.h"
#include "Register.h"

#include "Login.h"
#include "AesCrypto.h"


// Register dialog

IMPLEMENT_DYNAMIC(Register, CDialogEx)

Register::Register(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_RESGISTER, pParent)
{

}

Register::~Register()
{
}

void Register::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_USERNAME, m_userName);
	DDX_Control(pDX, IDC_PASSWORD, m_passWord);
	DDX_Control(pDX, IDC_CHECK_PASSWORD, m_checkPassWord);
}


BEGIN_MESSAGE_MAP(Register, CDialogEx)
	ON_BN_CLICKED(IDC_REGISTER, &Register::OnBnClickedRegister)
	ON_BN_CLICKED(IDC_CANCEL, &Register::OnBnClickedCancel)
END_MESSAGE_MAP()


// Register message handlers

void Register::OnBnClickedRegister()
{
	// TODO: Add your control notification handler code here

    CString username, password, check;
    m_userName.GetWindowText(username);
    m_passWord.GetWindowText(password);
    m_checkPassWord.GetWindowText(check);

    if (password != check) {
        AfxMessageBox(L"Mật khẩu không khớp!");
        return;
    }
    if (username.IsEmpty() || password.IsEmpty()) {
        AfxMessageBox(L"Nhập đầy đủ!");
        return;
    }


    CClientSocket client;
    if (!client.Connect(_T("127.0.0.1"), 8888)) {
        AfxMessageBox(L"Không kết nối server!");
        return;
    }

    std::string plain = CT2A(password.GetString());
    std::string enc = AesCrypto::Encrypt(plain);

    json j = {
        {"action", "register"},
        {"username", CT2A(username.GetString())},
        {"password", enc}  // Gửi mã hóa
    };

    if (!client.SendJSON(j)) {
        AfxMessageBox(L"Gửi thất bại!");
        return;
    }

    std::string resp = client.ReceiveLine();
    try {
        json r = json::parse(resp);
        AfxMessageBox(CA2W(r["message"].get<std::string>().c_str()));
        if (r.value("success", false)) {
            EndDialog(IDOK);
            Login dlg;
            dlg.DoModal();
        }
    }
    catch (...) {
        AfxMessageBox(L"Lỗi phản hồi");
    }
}

void Register::OnBnClickedCancel()
{
	// TODO: Add your control notification handler code here
	EndDialog(IDD_DIALOG_RESGISTER);
	Login loginDgl;
	loginDgl.DoModal();
}
