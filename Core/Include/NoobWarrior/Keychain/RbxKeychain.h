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
// File: RbxKeychain.h
// Started by: Hattozo
// Started on: 11/7/2025
// Description: Manages authentication of Roblox accounts for use with the noobWarrior library
// Should also work with server emulators since the API endpoints are the same
#pragma once
#include <NoobWarrior/Config.h>
#include <NoobWarrior/Keychain/Keychain.h>

namespace NoobWarrior {
class RbxKeychain : public Keychain {
public:
    RbxKeychain(Config *config);
    std::string GetName() override;
protected:
    nlohmann::json GetJsonFromToken(const std::string &token) override;
};
}