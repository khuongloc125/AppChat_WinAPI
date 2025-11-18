#pragma once
#include "afxdialogex.h"


// Register dialog

class Register : public CDialogEx
{
	DECLARE_DYNAMIC(Register)

public:
	Register(CWnd* pParent = nullptr);   // standard constructor
	virtual ~Register();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_RESGISTER };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedRegister();
	CEdit m_userName;
	CEdit m_passWord;
	CEdit m_checkPassWord;
	afx_msg void OnBnClickedCancel();
};
