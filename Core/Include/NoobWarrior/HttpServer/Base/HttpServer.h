// === noobWarrior ===
// File: HttpServer.h
// Started by: Hattozo
// Started on: 3/10/2025
// Description:
#pragma once
#include "Handler.h"

#include <cstdint>
#include <filesystem>
#include <vector>
#include <queue>
#include <utility>

#define NOOBWARRIOR_SET_URI(uri, handler)
#define NOOBWARRIOR_LINK_URI_TO_TEMPLATE(uri, fileName) SetRequestHandler(uri, mWebHandler, (void*)fileName);

struct mg_context;
namespace NoobWarrior { class Core; }
namespace NoobWarrior::HttpServer {
    class WebHandler;
    class HttpServer {
    friend class WebHandler;
    public:
        HttpServer(Core *core, std::string name = "HttpServer");
        
        virtual int Start(uint16_t port);
        virtual int Stop();

        bool        IsRunning();
        void        SetRequestHandler(const char *uri, Handler *handler, void *userdata = nullptr);
    protected:
        bool Running;

        std::string Name;
        Core *mCore;
        std::filesystem::path Directory;
        mg_context *Server;

        //////////////// Handlers ////////////////
        WebHandler *mWebHandler;

        std::vector<void**> HandlerUserdata;
    };
}