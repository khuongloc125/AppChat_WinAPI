
// ChatApp.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'pch.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols
#include "ClientSocket.h"


// ChatApp:
// See ChatApp.cpp for the implementation of this class
//

class ChatApp : public CWinApp
{
public:
	int m_nUserId = -1;
	CString m_strUsername;
	CClientSocket* m_pSocket = nullptr;
	ChatApp();

// Overrides
public:
	virtual BOOL InitInstance();

// Implementation

	DECLARE_MESSAGE_MAP()
};

extern ChatApp theApp;
