// ChatServerService.h
#pragma once

#include <winsock2.h>
#include <nlohmann/json.hpp>
#include <string>

using json = nlohmann::json;

void BroadcastUserStatus(int userId, const std::string& status);
void ForwardMessage(int senderId, int receiverId, const std::string& message);