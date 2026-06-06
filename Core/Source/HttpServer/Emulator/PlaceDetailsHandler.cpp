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
// File: PlaceDetailsHandler.cpp
// Started by: Hattozo
// Started on: 6/6/2026
// Description:
#include <NoobWarrior/HttpServer/Emulator/PlaceDetailsHandler.h>
#include <nlohmann/json.hpp>

#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

using namespace NoobWarrior;

PlaceDetailsHandler::PlaceDetailsHandler(EmuDbManager *dbm) : mEmuDbManager(dbm) {}

// Pulls every integer value for a query key out of the raw URI. Handles both the repeated
// (placeIds=1&placeIds=2) and comma-separated (placeIds=1,2) forms, and percent-decodes each value.
static std::vector<int64_t> ParseIdParams(const char* uri, const char* key) {
    std::vector<int64_t> out;
    if (uri == nullptr) return out;
    const char* q = strchr(uri, '?');
    if (q == nullptr) return out;

    std::string query = q + 1;
    size_t pos = 0;
    while (pos <= query.size()) {
        size_t amp = query.find('&', pos);
        std::string pair = query.substr(pos, amp == std::string::npos ? std::string::npos : amp - pos);

        if (size_t eq = pair.find('='); eq != std::string::npos && pair.compare(0, eq, key) == 0) {
            std::string value = pair.substr(eq + 1);

            size_t decodedSize = 0;
            char* decoded = evhttp_uridecode(value.c_str(), 1, &decodedSize);
            std::string ids = decoded ? std::string(decoded, decodedSize) : value;
            if (decoded) free(decoded);

            size_t start = 0;
            while (start <= ids.size()) {
                size_t comma = ids.find(',', start);
                std::string token = ids.substr(start, comma == std::string::npos ? std::string::npos : comma - start);
                if (!token.empty()) {
                    char* endPtr;
                    int64_t id = strtoll(token.c_str(), &endPtr, 10);
                    if (*endPtr == '\0')
                        out.push_back(id);
                }
                if (comma == std::string::npos) break;
                start = comma + 1;
            }
        }

        if (amp == std::string::npos) break;
        pos = amp + 1;
    }
    return out;
}

void PlaceDetailsHandler::OnRequest(evhttp_request *req, void *userdata) {
    std::vector<int64_t> placeIds = ParseIdParams(evhttp_request_get_uri(req), "placeIds");

    // Fall back to a single placeholder place so callers without a placeIds list still get a row.
    if (placeIds.empty())
        placeIds.push_back(1818);

    nlohmann::json j = nlohmann::json::array();
    for (int64_t placeId : placeIds) {
        int64_t universeId = mEmuDbManager->GetUniverseIdForPlace(placeId).value_or(placeId);
        int64_t rootPlaceId = mEmuDbManager->GetStartPlaceIdForUniverse(universeId).value_or(placeId);
        std::string name = mEmuDbManager->GetItemName(ItemType::Asset, placeId).value_or("noobWarrior Place");

        int64_t builderId = mEmuDbManager->GetCreatorUserId(ItemType::Asset, placeId).value_or(1);
        std::string builder = mEmuDbManager->GetItemName(ItemType::User, builderId).value_or("Player");

        nlohmann::json place;
        place["placeId"] = placeId;
        place["name"] = name;
        place["description"] = "";
        place["sourceName"] = name;
        place["sourceDescription"] = "";
        place["url"] = "";
        place["builder"] = builder;
        place["builderId"] = builderId;
        place["hasVerifiedBadge"] = false;
        place["isPlayable"] = true;
        place["reasonProhibited"] = "None";
        place["reasonProhibitedMessage"] = "";
        place["universeId"] = universeId;
        place["universeRootPlaceId"] = rootPlaceId;
        place["price"] = 0;
        place["imageToken"] = "";
        j.push_back(std::move(place));
    }

    const std::string body = j.dump();
    evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type", "application/json");
    evbuffer* buf = evbuffer_new();
    evbuffer_add(buf, body.data(), body.size());
    evhttp_send_reply(req, HTTP_OK, nullptr, buf);
    evbuffer_free(buf);
}
