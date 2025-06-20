// === noobWarrior ===
// File: AssetHandler.h
// Started by: Hattozo
// Started on: 6/19/2025
// Description:
#pragma once
#include <NoobWarrior/HttpServer/Handler.h>

namespace NoobWarrior::HttpServer {
class AssetHandler : public Handler {
public:
    int OnRequest(mg_connection *conn, void *userdata) override;
};
}