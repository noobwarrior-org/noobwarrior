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
// File: Keychain.cpp
// Started by: Hattozo
// Started on: 2/11/2026
// Description: A list of accounts that can authenticate with a specific service
// These keys are securely stored using the appropriate API's for your operating system
#include <NoobWarrior/Keychain/Keychain.h>

using namespace NoobWarrior;

bool Keychain::HasAccountExpired(Account &acc) {
    if (acc.ExpireTimestamp > -1 && time(nullptr) > acc.ExpireTimestamp) return true; // Automatic fail if it is past the expiration date. No way that will work.
    
    nlohmann::json userInfo = GetJsonFromToken(acc.Token);
    if (!userInfo.empty() && userInfo.contains("id") && userInfo.contains("name"))
        return false;

    return true;
}

void Keychain::AddAccount(Account &acc) {
    Accounts.push_back(acc);
}