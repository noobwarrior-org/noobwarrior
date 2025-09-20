// === noobWarrior ===
// File: ServerListHandler.h
// Started by: Hattozo
// Started on: 9/19/2025
// Description:
#pragma once
#include <NoobWarrior/HttpServer/Base/Handler.h>

namespace NoobWarrior::HttpServer {
class HttpServer;
class ServerListHandler : public Handler {
public:
    ServerListHandler(HttpServer *srv);
    int OnRequest(mg_connection *conn, void *userdata) override;
private:
    HttpServer *mHttpServer;
};
}
