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
#include <queue>
#include <utility>

struct mg_context;
namespace NoobWarrior { class Core; }
namespace NoobWarrior::HttpServer {
    class HttpServer {
    public:
        HttpServer(Core *core);
        void SetRequestHandler(const char *uri, Handler *handler, void *userdata = nullptr);
        int Start(uint16_t port);
        int Stop();
    private:
        Core *mCore;
        std::filesystem::path Directory;
        mg_context *Server;

        //////////////// Handlers ////////////////
        WebHandler *mWebHandler;
        AssetHandler *mAssetHandler;

        std::vector<void**> HandlerUserdata;

        std::priority_queue<std::pair<uint16_t, std::string>> TemporaryProxies;
    };
}