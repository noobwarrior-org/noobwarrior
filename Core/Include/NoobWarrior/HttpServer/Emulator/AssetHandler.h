// === noobWarrior ===
// File: AssetHandler.h
// Started by: Hattozo
// Started on: 6/19/2025
// Description:
#pragma once
#include <NoobWarrior/HttpServer/Base/Handler.h>
#include <NoobWarrior/Database/DatabaseManager.h>

namespace NoobWarrior {
class HttpServer;
class AssetHandler : public Handler {
public:
    AssetHandler(HttpServer *srv, DatabaseManager *dbm);
    void OnRequest(evhttp_request *req, void *userdata) override;
private:
    HttpServer *mHttpServer;
    DatabaseManager *mDatabaseManager;
};
}
