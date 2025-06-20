// === noobWarrior ===
// File: HttpServer.h
// Started by: Hattozo
// Started on: 3/10/2025
// Description:
#pragma once
#include "Handler.h"
#include "WebHandler.h"
#include "Api/Roblox/AssetHandler.h"

#include <cstdint>
#include <filesystem>
#include <vector>

struct mg_context;

namespace NoobWarrior::HttpServer {
    class HttpServer {
    public:
        HttpServer(const std::filesystem::path &dir);
        void SetRequestHandler(const char *uri, Handler *handler, void *userdata = nullptr);
        int Start(uint16_t port);
        int Stop();
    private:
        mg_context *Server;
        WebHandler *mWebHandler;
        AssetHandler *mAssetHandler;
        std::vector<void**> HandlerUserdata;
        std::filesystem::path Directory;
    };
}