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
// File: EmuKeychain.h
// Started by: Hattozo
// Started on: 2/11/2026
// Description: Manages authentication of server emulator accounts for use with the noobWarrior library
// This is based off the Roblox keychain since the API endpoints are the same
#pragma once
#include <NoobWarrior/Config.h>
#include <NoobWarrior/Keychain/RbxKeychain.h>

namespace NoobWarrior {
class EmuKeychain : public RbxKeychain {
public:
    EmuKeychain(Config *config);
    std::string GetName() override;
};
}