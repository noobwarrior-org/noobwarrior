// === noobWarrior ===
// File: ServerListHandler.cpp
// Started by: Hattozo
// Started on: 9/19/2025
// Description:
#include <NoobWarrior/HttpServer/MasterServer/Api/ServerListHandler.h>
#include <NoobWarrior/NoobWarrior.h>

#include <nlohmann/json.hpp>

using namespace NoobWarrior;
using namespace NoobWarrior::HttpServer;

ServerListHandler::ServerListHandler(HttpServer *srv) :
    mHttpServer(srv)
{}

int ServerListHandler::OnRequest(mg_connection *conn, void *userdata) {
    
    mg_printf(conn, "");
    return 200;
}