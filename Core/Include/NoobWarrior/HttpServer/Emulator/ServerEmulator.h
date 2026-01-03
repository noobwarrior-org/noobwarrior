// === noobWarrior ===
// File: ServerEmulator.h
// Started by: Hattozo
// Started on: 9/2/2025
// Description:
#pragma once
#include <NoobWarrior/HttpServer/Base/HttpServer.h>
#include "AssetHandler.h"

#include <cstdint>
#include <filesystem>
#include <vector>
#include <queue>
#include <utility>

namespace NoobWarrior { class Core; }
namespace NoobWarrior::HttpServer {
class ServerEmulator : public HttpServer {
    friend class ContentPageHandler;
public:
    ServerEmulator(Core *core);
    ~ServerEmulator();

    int Start(uint16_t port) override;
    int Stop() override;
    nlohmann::json GetBaseContextData(evhttp_request *req = nullptr) override;
private:
    //////////////// Handlers ////////////////
    std::unique_ptr<AssetHandler> mAssetHandler;
    std::priority_queue<std::pair<uint16_t, std::string>> TemporaryProxies;
};
}