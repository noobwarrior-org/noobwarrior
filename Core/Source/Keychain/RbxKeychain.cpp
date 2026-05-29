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
// File: RbxKeychain.cpp
// Started by: Hattozo
// Started on: 11/7/2025
// Description: Manages authentication of Roblox accounts for use with the noobWarrior library
#include <NoobWarrior/Keychain/RbxKeychain.h>

#include <curl/curl.h>
#include <cpr/cpr.h>

using namespace NoobWarrior;

RbxKeychain::RbxKeychain(Registry *registry) : Keychain(registry) {}

std::string RbxKeychain::GetName() {
    return "rbx";
}

nlohmann::json RbxKeychain::GetJsonFromToken(const std::string &token) {
    std::string cookie = std::format(".ROBLOSECURITY={};", token);

    cpr::Session session;
    session.SetUrl(cpr::Url{"https://users.roblox.com/v1/users/authenticated"});
    session.SetHeader(cpr::Header{{"Cookie", cookie}});
    // Verify against the OS certificate store.
    curl_easy_setopt(session.GetCurlHolder()->handle, CURLOPT_SSL_OPTIONS, (long)CURLSSLOPT_NATIVE_CA);

    cpr::Response res = session.Get();

    if (res.error.code == cpr::ErrorCode::OK) {
        try {
            return nlohmann::json::parse(res.text);
        } catch (nlohmann::json::exception &e) {
            Out("RbxKeychain", "parse failed: {} | body: {}", e.what(), res.text);
        }
    }
    Out("RbxKeychain", "curl failed: {}", (int)res.error.code);
    return nlohmann::json {};
}