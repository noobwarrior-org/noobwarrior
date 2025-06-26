// === noobWarrior ===
// File: HttpServerConfig.h
// Started by: Hattozo
// Started on: 6/23/2025
// Description:
#pragma once
#include "../BaseConfig.h"
#include <cstdint>
#include <queue>
#include <utility>

namespace NoobWarrior::HttpServer {
    class Config : public BaseConfig {
    public:
        Config(const std::filesystem::path &filePath, lua_State *luaState);

        uint16_t                                                Port;
        char*                                                   Password;
        std::priority_queue<std::pair<uint16_t, std::string>>   Proxies;
    private:
        void OnDeserialize() override;
        void OnSerialize() override;
    };
}
