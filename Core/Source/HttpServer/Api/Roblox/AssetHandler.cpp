// === noobWarrior ===
// File: AssetHandler.cpp
// Started by: Hattozo
// Started on: 6/19/2025
// Description: HTTP request handler that simulates the action of getting an asset from Roblox services.
#include <NoobWarrior/HttpServer/Api/Roblox/AssetHandler.h>
#include <NoobWarrior/NoobWarrior.h>

using namespace NoobWarrior;
using namespace NoobWarrior::HttpServer;

AssetHandler::AssetHandler(HttpServer *srv, DatabaseManager *dbm) :
    mHttpServer(srv),
    mDatabaseManager(dbm)
{}

int AssetHandler::OnRequest(mg_connection *conn, void *userdata) {
    std::vector<unsigned char> data = mDatabaseManager->RetrieveContentData(1, IdType::Asset);

    if (data.empty()) {
        mg_send_http_error(conn, 404, "Cannot read asset");
        return 404;
    }

    mg_send_http_ok(conn, "application/octet-stream", data.size());
    mg_write(conn, data.data(), data.size());
    return 200;
}
