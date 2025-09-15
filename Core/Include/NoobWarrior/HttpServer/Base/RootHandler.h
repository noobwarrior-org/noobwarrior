// === noobWarrior ===
// File: RootHandler.h
// Started by: Hattozo
// Started on: 9/13/2025
// Description:
#pragma once
#include "Handler.h"

namespace NoobWarrior::HttpServer {
class HttpServer;
class RootHandler : public Handler {
public:
    RootHandler(HttpServer *server);
    int OnRequest(mg_connection *conn, void *userdata) override;
protected:
    HttpServer *mServer;
};
}