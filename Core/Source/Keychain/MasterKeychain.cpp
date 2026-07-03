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
// File: MasterKeychain.cpp
// Started by: Hattozo
// Started on: 11/7/2025
// Description:
#include <NoobWarrior/Keychain/MasterKeychain.h>

#include <ctime>

using namespace NoobWarrior;

MasterKeychain::MasterKeychain(Registry *registry) : Keychain(registry) {}

std::string MasterKeychain::GetName() {
    return "master";
}

bool MasterKeychain::HasAccountExpired(Account &acc) {
    return acc.ExpireTimestamp > -1 && time(nullptr) > acc.ExpireTimestamp;
}

nlohmann::json MasterKeychain::GetJsonFromToken(const std::string &token) {
    return nlohmann::json {};
}
