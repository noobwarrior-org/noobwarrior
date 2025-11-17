// === noobWarrior ===
// File: ClientSettingsHandler.cpp
// Started by: Hattozo
// Started on: 11/16/2025
// Description: Returns a JSON object containing application settings (FFlags & DFFlags)
#include <NoobWarrior/HttpServer/Emulator/Api/Roblox/ClientSettingsHandler.h>

#include "FFlagJson/PCDesktopClient.json.inc"

using namespace NoobWarrior::HttpServer;

ClientSettingsHandler::ClientSettingsHandler(ServerEmulator *server) {}

int ClientSettingsHandler::OnRequest(mg_connection *conn, void *userdata) {
    mg_send_http_error(conn, 500, "WIP");
    return 500;
}