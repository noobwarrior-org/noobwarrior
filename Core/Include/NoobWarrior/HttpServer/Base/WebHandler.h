// === noobWarrior ===
// File: WebHandler.h
// Started by: Hattozo
// Started on: 6/19/2025
// Description:
#pragma once
#include "Handler.h"
#include "NoobWarrior/Config.h"

#include <filesystem>

namespace NoobWarrior::HttpServer {
class HttpServer;
class WebHandler : public Handler {
public:
    WebHandler(HttpServer *server);
    int OnRequest(mg_connection *conn, void *userdata) override;
private:
    Config *mConfig;
    std::filesystem::path Directory;
};
}