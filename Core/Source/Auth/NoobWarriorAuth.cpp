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
// File: NoobWarriorAuth.cpp
// Started by: Hattozo
// Started on: 11/10/2025
// Description: List of accounts for connecting to servers that use noobWarrior infrastracture
#include <NoobWarrior/Auth/NoobWarriorAuth.h>

using namespace NoobWarrior;

NoobWarriorAuth::NoobWarriorAuth(Config *config) : BaseAuth(config) {}

nlohmann::json NoobWarriorAuth::AccStructToJson(Account &acc) {
    nlohmann::json accJson = BaseAuth::AccStructToJson(acc);
    accJson["url"] = acc.Url;
    return accJson;
}

Account NoobWarriorAuth::AccJsonToStruct(nlohmann::json &json) {
    Account acc = BaseAuth::AccJsonToStruct(json);
    acc.Url = json["url"];
    return acc;
}
