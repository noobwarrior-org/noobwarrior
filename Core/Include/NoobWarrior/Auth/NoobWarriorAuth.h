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
// File: NoobWarriorAuth.h
// Started by: Hattozo
// Started on: 11/10/2025
// Description: List of accounts for connecting to servers that use noobWarrior infrastracture
#pragma once
#include <NoobWarrior/Config.h>
#include <NoobWarrior/Auth/BaseAuth.h>

namespace NoobWarrior {
struct Account {
    int64_t Id;
    std::string Name;
    std::string Token;
    std::string Url;
    long ExpireTimestamp { -1 }; // Defaults to -1 if the expiration date is unknown.
};

class NoobWarriorAuth : public BaseAuth<Account> {
public:
    NoobWarriorAuth(Config *config);
    std::string GetName() override = 0;
    nlohmann::json AccStructToJson(Account &acc) override;
    Account AccJsonToStruct(nlohmann::json &json) override;
protected:
    nlohmann::json GetJsonFromToken(const std::string &token) override;
};
}