// === noobWarrior ===
// File: HttpServer.h
// Started by: Hattozo
// Started on: 3/10/2025
// Description:
#pragma once

#include "Client.h"

#include <vector>
#include <map>
#include <string>

namespace NoobWarrior::HttpServer {
    class HttpServer {
    public:
        HttpServer(std::string_view name = "HttpServer");
        int Open(int port, bool useipv6);

        void OnClientSentRequest(Client& client, std::string header);
        void OnClientConnected();
        void OnClientDisconnected();
    private:
        std::string_view mName;
        std::map<int, Client*> m;
        std::vector<Client> mConnectedClients;
        std::vector<int> mConnectedSockets;
        int mSocketFd;
    };
}