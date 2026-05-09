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

using namespace NoobWarrior;

static size_t CurlWriteToBuf(void *contents, size_t size, size_t nmemb, std::string *buffer) {
    size_t totalSize = size * nmemb;
    buffer->insert(buffer->end(), (char*)contents, (char*)contents + totalSize);
    return totalSize;
}

RbxKeychain::RbxKeychain(Registry *registry) : Keychain(registry) {}

std::string RbxKeychain::GetName() {
    return "rbx";
}

nlohmann::json RbxKeychain::GetJsonFromToken(const std::string &token) {
    CURL *handle = curl_easy_init();
    if (!handle) return nlohmann::json {};

    std::string jsonStr;
    std::string cookie = std::format(".ROBLOSECURITY={};", token);

    curl_easy_setopt(handle, CURLOPT_URL, "https://users.roblox.com/v1/users/authenticated");
    curl_easy_setopt(handle, CURLOPT_COOKIE, cookie.c_str());
    curl_easy_setopt(handle, CURLOPT_SSL_OPTIONS, CURLSSLOPT_NATIVE_CA);
    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, CurlWriteToBuf);
    curl_easy_setopt(handle, CURLOPT_WRITEDATA, &jsonStr);

    CURLcode ret = curl_easy_perform(handle);
    curl_easy_cleanup(handle);

    if (ret == CURLE_OK) {
        try {
            return nlohmann::json::parse(jsonStr);
        } catch (nlohmann::json::exception &e) {
            Out("RbxKeychain", "parse failed: {} | body: {}", e.what(), jsonStr);
        }
    }
    Out("RbxKeychain", "curl failed: {}", (int)ret);
    return nlohmann::json {};
}