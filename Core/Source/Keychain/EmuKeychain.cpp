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
// File: EmuKeychain.cpp
// Started by: Hattozo
// Started on: 2/11/2026
// Description:
#include <NoobWarrior/Keychain/EmuKeychain.h>

#include <ctime>

using namespace NoobWarrior;

EmuKeychain::EmuKeychain(Registry *registry) : Keychain(registry) {}

std::string EmuKeychain::GetName() {
    return "emu";
}

bool EmuKeychain::HasAccountExpired(Account &acc) {
    return acc.ExpireTimestamp > -1 && time(nullptr) > acc.ExpireTimestamp;
}

nlohmann::json EmuKeychain::GetJsonFromToken(const std::string &token) {
    return nlohmann::json {};
}
