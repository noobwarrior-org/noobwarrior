// === noobWarrior ===
// File: AssetHandler.cpp
// Started by: Hattozo
// Started on: 6/19/2025
// Description: HTTP request handler that simulates the action of getting an asset from Roblox services.
#include <NoobWarrior/HttpServer/Emulator/AssetHandler.h>
#include <NoobWarrior/NoobWarrior.h>

using namespace NoobWarrior;
using namespace NoobWarrior::HttpServer;

AssetHandler::AssetHandler(HttpServer *srv, DatabaseManager *dbm) :
    mHttpServer(srv),
    mDatabaseManager(dbm)
{}

void AssetHandler::OnRequest(evhttp_request *req, void *userdata) {
    evhttp_send_error(req, HTTP_NOTIMPLEMENTED, "WIP");
    /*
    const mg_request_info *request_info = mg_get_request_info(conn);
    const char *query_string = request_info->query_string;
    char id_string[256];

    switch (int res = mg_get_var(query_string, strlen(query_string), "id", id_string, sizeof(id_string))) {
        case -1:
            break;
        case -2:
            mg_send_http_error(conn, 500, "Error when decoding \"id\" parameter");
            return 500;
        default: break;
    }

    char* invalidChar = nullptr;
    int id = static_cast<int>(strtol(id_string, &invalidChar, 10));
    if (*invalidChar != '\0') {
        // Invalid character was assigned to by strtol, this means it found something invalid
        mg_send_http_error(conn, 400, "Invalid id number: '%c'", *invalidChar);
        return 400;
    }

    mg_send_http_error(conn, 404, "WIP");
    return 404;
    */
}
