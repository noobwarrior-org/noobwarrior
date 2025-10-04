// === noobWarrior ===
// File: WebHandler.h
// Started by: Hattozo
// Started on: 6/19/2025
// Description:
#pragma once
#include "Handler.h"
#include "NoobWarrior/Config.h"

#include <filesystem>
#include <nlohmann/json_fwd.hpp>

namespace NoobWarrior::HttpServer {
class HttpServer;
class WebHandler : public Handler {
public:
    WebHandler(HttpServer *server);
    int OnRequest(mg_connection *conn, void *userdata) override;
    virtual nlohmann::json GetContextData(mg_connection *conn = nullptr);
protected:
    HttpServer *mServer;
};
}