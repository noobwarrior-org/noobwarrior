// === noobWarrior ===
// File: ControlPanelHandler.h
// Started by: Hattozo
// Started on: 9/21/2025
// Description:
#pragma once
#include "Handler.h"

namespace NoobWarrior::HttpServer {
class HttpServer;
class ControlPanelHandler : public Handler {
public:
    ControlPanelHandler(HttpServer *server);
    int OnRequest(mg_connection *conn, void *userdata) override;
protected:
    HttpServer *mServer;
};
}