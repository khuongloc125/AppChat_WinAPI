// Utils.cpp
#include "Globals.h"
#include "ChatServerService.h"

void BroadcastUserStatus(int userId, const std::string& status) {
    json msg = {
        {"action", "user_status"},
        {"user_id", userId},
        {"status", status}
    };
    std::string data = msg.dump() + "\n";

    std::lock_guard<std::mutex> lock(g_ClientsMutex);
    for (const auto& pair : g_SocketToUser) {
        send(pair.first, data.c_str(), static_cast<int>(data.size()), 0);
    }
}

void ForwardMessage(int senderId, int receiverId, const std::string& encryptedMsg) {
    json msg = { {"action", "receive_message"}, {"sender_id", senderId}, {"message", encryptedMsg} };
    std::string data = msg.dump() + "\n";

    std::lock_guard<std::mutex> lock(g_ClientsMutex);
    auto it = g_UserToSocket.find(receiverId);
    if (it != g_UserToSocket.end()) {
        send(it->second, data.c_str(), data.size(), 0);
    }
}