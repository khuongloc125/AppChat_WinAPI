
// ChatAppDlg.cpp : implementation file
//

#include "pch.h"
#include "framework.h"
#include "ChatApp.h"
#include "ChatAppDlg.h"
#include "afxdialogex.h"
#include "AesCrypto.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CAboutDlg dialog used for App About

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CChatAppDlg dialog



CChatAppDlg::CChatAppDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_CHATAPP_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CChatAppDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST1, m_listUser);
	DDX_Control(pDX, IDC_STATIC_MESSAGE, m_static_mess);
	DDX_Control(pDX, IDC_SEND, m_youMess);
	DDX_Control(pDX, IDC_MESSAGE_CONTENT, m_messContent);
	DDX_Control(pDX, IDC_LIST_MESSAGES, m_listMessages);
}

BEGIN_MESSAGE_MAP(CChatAppDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST1, &CChatAppDlg::OnLvnItemchangedList1)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST1, &CChatAppDlg::OnNMDblclkListUsers)
	
	ON_MESSAGE(WM_USER + 1, &CChatAppDlg::OnUserStatus)
	ON_MESSAGE(WM_USER + 2, &CChatAppDlg::OnReceiveMessage)

	ON_BN_CLICKED(IDC_SEND, &CChatAppDlg::OnBnClickedSend)
END_MESSAGE_MAP()

LRESULT CChatAppDlg::OnUserStatus(WPARAM wParam, LPARAM lParam)
{
	int userId = (int)wParam;
	CString* pStatus = (CString*)lParam;
	for (int i = 0; i < m_listUser.GetItemCount(); i++) {
		if (_ttoi(m_listUser.GetItemText(i, 0)) == userId) {
			m_listUser.SetItemText(i, 1, *pStatus);
			break;
		}
	}
	delete pStatus;
	return 0;
}

LRESULT CChatAppDlg::OnReceiveMessage(WPARAM wParam, LPARAM lParam)
{
	int senderId = (int)wParam;
	CString* pMsg = (CString*)lParam;
	if (!pMsg) return 0;

	bool isMe = false;
	CString actualMsg = *pMsg;
	if (!actualMsg.IsEmpty() && actualMsg.GetAt(0) == '\x01') {
		isMe = true;
		actualMsg = actualMsg.Mid(1);
	}

	// Chỉ dùng AddMessage cho tin nhắn mới, map sẽ lưu theo thứ tự cũ->mới
	AddMessage(senderId, actualMsg, isMe);

	delete pMsg;
	return 0;
}

// CChatAppDlg message handlers

BOOL CChatAppDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here

	m_listUser.InsertColumn(0, L"Username", LVCFMT_LEFT, 100);
	m_listUser.InsertColumn(1, L"Status", LVCFMT_LEFT, 70);

	if (m_pSocket) {
		m_pSocket->SetMessageCallback(OnServerMessage, this);
		m_pSocket->StartReceiveThread();

		json getUsers = { {"action", "get_users"} };
		m_pSocket->SendJSON(getUsers);
	}
	if (!AesCrypto::LoadKey("1aa07c508ed27ad8ad902d366b0fc864")) {
		AfxMessageBox(L"Không load được key mã hóa!");
		return FALSE;
	}
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CChatAppDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CChatAppDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CChatAppDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CChatAppDlg::OnLvnItemchangedList1(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	// TODO: Add your control notification handler code here
	*pResult = 0;
}

void CChatAppDlg::OnNMDblclkListUsers(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMITEMACTIVATE pNMItem = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	int nItem = pNMItem->iItem;
	if (nItem == -1) return;

	m_nCurrentChatId = (int)m_listUser.GetItemData(nItem);
	CString username = m_listUser.GetItemText(nItem, 0);

	m_static_mess.SetWindowText(username + L"\r\n");

	LoadChatHistory(m_nCurrentChatId);
	if (theApp.m_pSocket) {
        json req = {
            {"action", "get_messages"},
            {"with_user", m_nCurrentChatId}
        };
        theApp.m_pSocket->SendJSON(req);
    }
	*pResult = 0;
}

void CChatAppDlg::OnServerMessage(const json& j, void* pParam)
{
	CChatAppDlg* pThis = (CChatAppDlg*)pParam;
	if (!pThis) return;

	std::string action = j.value("action", "");
	//false
	if (j.contains("success") && j.value("success", true) && j.contains("users")) {
		const auto& users = j["users"];
		if (!users.is_null() && users.is_array()) {
			pThis->UpdateUserList(users);
		}
		return;
	}

	if (action == "user_status") {
		int uid = j.value("user_id", -1);
		std::string status = j.value("status", "offline");
		if (uid != -1) {
			CString csStatus = CA2W(status.c_str());
			pThis->PostMessage(WM_USER + 1, uid, (LPARAM)new CString(csStatus));
		}
		return;
	}

	if (action == "receive_message") {
		int senderId = j.value("sender_id", -1);
		std::string encrypted = j.value("message", "");

		std::string decrypted = AesCrypto::Decrypt(encrypted);
		if (decrypted.empty()) {
			decrypted = "[Tin nhắn lỗi hoặc không thể giải mã]";
		}

		CString msg = CA2W(decrypted.c_str());
		pThis->PostMessage(WM_USER + 2, senderId, (LPARAM)new CString(msg));
		return;
	}

	if (action == "messages") {
		if (!j.contains("messages") || !j["messages"].is_array()) return;

		int withUser = pThis->m_nCurrentChatId;
		auto& arr = j["messages"];

		pThis->m_mapMessages[withUser].clear();

		for (const auto& m : arr) {
			int senderId = m.value("sender_id", -1);
			std::string encryptedMsg = m.value("message", "");

			std::string decrypted = AesCrypto::Decrypt(encryptedMsg);
			if (decrypted.empty()) {
				decrypted = "[Lỗi giải mã]";
			}

			CString plainText = CA2W(decrypted.c_str());
			bool isMe = (senderId == theApp.m_nUserId);
			CString senderName = isMe ? L"You" : pThis->FindUserName(senderId);
			CString line = senderName + L": " + plainText;

			pThis->m_mapMessages[withUser].push_back(line);
		}

		pThis->LoadChatHistory(withUser);
		return;
	}
}

void CChatAppDlg::UpdateUserList(const json& users)
{
	m_listUser.DeleteAllItems();
	int index = 0;
	for (const auto& u : users) {
		int id = u["id"];
		if (id == theApp.m_nUserId) continue;

		std::string uname = u.value("username", "Unknown");
		CString name = CA2W(uname.c_str());

		CString status = CA2W(u["status"].get<std::string>().c_str());

		int n = m_listUser.InsertItem(index, name);
		m_listUser.SetItemText(n, 1, status);
		m_listUser.SetItemData(n, id);
		index++;
	}
}

void CChatAppDlg::OnBnClickedSend()
{
	if (m_nCurrentChatId == -1) {
		AfxMessageBox(L"Chọn người để chat!");
		return;
	}

	CString msg;
	m_messContent.GetWindowTextW(msg);
	if (msg.IsEmpty()) return;

	std::string plain = CT2A(msg.GetString());

	std::string encrypted = AesCrypto::Encrypt(plain);

	json j = {
		{"action", "send_message"},
		{"receiver_id", m_nCurrentChatId},
		{"message", encrypted}  
	};

	if (theApp.m_pSocket->SendJSON(j)) {
		AddMessage(m_nCurrentChatId, msg, true);
		m_messContent.SetWindowTextW(L"");
	}
}

int CChatAppDlg::FindUserItem(int userId)
{
	for (int i = 0; i < m_listUser.GetItemCount(); i++) {
		if ((int)m_listUser.GetItemData(i) == userId)
			return i;
	}
	return -1;
}

CString CChatAppDlg::FindUserName(int userId)
{
	int idx = FindUserItem(userId);
	if (idx != -1) return m_listUser.GetItemText(idx, 0);
	return L"Unknown";
}

void CChatAppDlg::AddMessage(int userId, const CString& msg, bool isMe)
{
	CString senderName = isMe ? L"You" : FindUserName(userId);

	CString line;
	line.Format(L"%s: %s", senderName, msg);

	m_mapMessages[userId].push_back(line);

	if (m_nCurrentChatId == userId) {
		m_listMessages.AddString(line);

		int count = m_listMessages.GetCount();
		if (count > 0) {
			m_listMessages.SetCurSel(count - 1);
			m_listMessages.SetTopIndex(count - 1);
		}
	}
}

void CChatAppDlg::LoadChatHistory(int userId)
{
	m_listMessages.ResetContent();

	auto it = m_mapMessages.find(userId);
	if (it != m_mapMessages.end()) {
		for (const auto& line : it->second) {
			m_listMessages.AddString(line);
		}
		int count = m_listMessages.GetCount();
		if (count > 0) {
			m_listMessages.SetCurSel(count - 1);
			m_listMessages.SetTopIndex(count - 1);
		}
	}
}

void CChatAppDlg::OnDestroy()
{
	if (theApp.m_pSocket) {
		theApp.m_pSocket->StopReceiveThread();
		delete theApp.m_pSocket;
		theApp.m_pSocket = nullptr;
	}
	CDialogEx::OnDestroy();
}

