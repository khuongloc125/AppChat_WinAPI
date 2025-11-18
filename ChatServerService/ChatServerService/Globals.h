// Globals.h
#pragma once

#include <winsock2.h>
#include <mysql_connection.h>
#include <vector>
#include <map>
#include <mutex>

extern sql::mysql::MySQL_Driver* g_Driver;
extern sql::Connection* g_Con;

extern SOCKET g_ListenSocket;
extern std::vector<SOCKET> g_Clients;
extern std::map<int, SOCKET> g_UserToSocket;
extern std::map<SOCKET, int> g_SocketToUser;
extern std::mutex g_ClientsMutex;

extern SERVICE_STATUS g_ServiceStatus;
extern SERVICE_STATUS_HANDLE g_ServiceStatusHandle;
extern HANDLE g_ServiceStopEvent;