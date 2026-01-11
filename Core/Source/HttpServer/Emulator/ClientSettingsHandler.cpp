// === noobWarrior ===
// File: ClientSettingsHandler.cpp
// Started by: Hattozo
// Started on: 11/16/2025
// Description: Returns a JSON object containing application settings (FFlags & DFFlags)
#include <NoobWarrior/HttpServer/Emulator/ClientSettingsHandler.h>

#include "FFlagJson/PCDesktopClient.json.inc.cpp"

using namespace NoobWarrior::HttpServer;

ClientSettingsHandler::ClientSettingsHandler(ServerEmulator *server) {}

void ClientSettingsHandler::OnRequest(evhttp_request *req, void *userdata) {
    evhttp_send_error(req, HTTP_NOTIMPLEMENTED, "WIP");
}