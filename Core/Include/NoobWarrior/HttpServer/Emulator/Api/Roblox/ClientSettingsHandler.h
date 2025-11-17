// === noobWarrior ===
// File: ClientSettingsHandler.h
// Started by: Hattozo
// Started on: 11/16/2025
// Description: Returns a JSON object containing application settings (FFlags & DFFlags)
#pragma once
#include <NoobWarrior/HttpServer/Base/Handler.h>

namespace NoobWarrior::HttpServer {
class ServerEmulator;
class ClientSettingsHandler : public Handler {
public:
    ClientSettingsHandler(ServerEmulator *server);
    int OnRequest(mg_connection *conn, void *userdata) override;
};
}