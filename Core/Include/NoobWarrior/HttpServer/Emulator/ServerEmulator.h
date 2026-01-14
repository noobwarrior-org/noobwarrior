/*
 * Copyright (C) 2026 Hattozo
 *
 * This file is part of noobWarrior.
 *
 * noobWarrior is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * noobWarrior is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with noobWarrior; if not, see
 * <https://www.gnu.org/licenses/>.
 */
// === noobWarrior ===
// File: ServerEmulator.h
// Started by: Hattozo
// Started on: 9/2/2025
// Description:
#pragma once
#include <NoobWarrior/HttpServer/Base/HttpServer.h>
#include "ClientSettingsHandler.h"
#include "AssetHandler.h"

#include <cstdint>
#include <filesystem>
#include <vector>
#include <queue>
#include <utility>

namespace NoobWarrior {
class Core;
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
    std::unique_ptr<ClientSettingsHandler> mClientSettingsHandler;
    std::priority_queue<std::pair<uint16_t, std::string>> TemporaryProxies;
};
}