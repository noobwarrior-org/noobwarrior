// === noobWarrior ===
// File: WebHandler.h
// Started by: Hattozo
// Started on: 6/19/2025
// Description:
#pragma once
#include "Handler.h"

#include <filesystem>

namespace NoobWarrior::HttpServer {
class WebHandler : public Handler {
public:
    WebHandler(const std::filesystem::path &dir);
    int OnRequest(mg_connection *conn, void *userdata) override;
private:
    std::filesystem::path Directory;
};
}