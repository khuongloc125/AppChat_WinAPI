// Globals.cpp
#include "Globals.h"

sql::mysql::MySQL_Driver* g_Driver = nullptr;
sql::Connection* g_Con = nullptr;

SOCKET g_ListenSocket = INVALID_SOCKET;
std::vector<SOCKET> g_Clients;
std::map<int, SOCKET> g_UserToSocket;
std::map<SOCKET, int> g_SocketToUser;
std::mutex g_ClientsMutex;

SERVICE_STATUS g_ServiceStatus = {0};
SERVICE_STATUS_HANDLE g_ServiceStatusHandle = NULL;
HANDLE g_ServiceStopEvent = NULL;