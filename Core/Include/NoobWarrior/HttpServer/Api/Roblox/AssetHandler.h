// === noobWarrior ===
// File: AssetHandler.h
// Started by: Hattozo
// Started on: 6/19/2025
// Description:
#pragma once
#include "../../Handler.h"
#include "../../../Database/DatabaseManager.h"

namespace NoobWarrior::HttpServer {
class HttpServer;
class AssetHandler : public Handler {
public:
    AssetHandler(HttpServer *srv, DatabaseManager *dbm);
    int OnRequest(mg_connection *conn, void *userdata) override;
private:
    HttpServer *mHttpServer;
    DatabaseManager *mDatabaseManager;
};
}
