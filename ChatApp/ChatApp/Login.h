#pragma once
#include "afxdialogex.h"


// Login dialog

class Login : public CDialogEx
{
	DECLARE_DYNAMIC(Login)

public:
	Login(CWnd* pParent = nullptr);   // standard constructor
	virtual ~Login();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_LOGIN };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedRegister();
	afx_msg void OnBnClickedLogin();
	CEdit m_userName;
	CEdit m_passWord;
};
