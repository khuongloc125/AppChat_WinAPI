
// ChatAppDlg.h : header file
//

#pragma once


// CChatAppDlg dialog
class CChatAppDlg : public CDialogEx
{
// Construction
public:
	CChatAppDlg(CWnd* pParent = nullptr);	// standard constructor

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_CHATAPP_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	int m_nCurrentChatId = -1;
	std::map<int, std::vector<CString>> m_mapMessages;
	void AddMessage(int userId, const CString& msg, bool isMe);
	void UpdateUserList(const json& users);
	void LoadChatHistory(int userId);
	static void OnServerMessage(const json& j, void* pParam);
	void OnDestroy();
	int FindUserItem(int userId);
	CString FindUserName(int userId);

	void SetSocket(CClientSocket* pSocket) { m_pSocket = pSocket; }
	CClientSocket* m_pSocket = nullptr;

	afx_msg LRESULT OnUserStatus(WPARAM, LPARAM);
	afx_msg LRESULT OnReceiveMessage(WPARAM, LPARAM);

	afx_msg void OnLvnItemchangedList1(NMHDR* pNMHDR, LRESULT* pResult);
	CListCtrl m_listUser;
	afx_msg void OnNMDblclkListUsers(NMHDR* pNMHDR, LRESULT* pResult);
	CEdit m_static_mess;
	CButton m_youMess;
	CEdit m_messContent;
	afx_msg void OnBnClickedSend();
	CListBox m_listMessages;
};
