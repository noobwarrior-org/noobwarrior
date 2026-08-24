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
// File: ThumbnailHandler.cpp
// Started by: Hattozo
// Started on: 6/6/2026
// Description: Serves the modern thumbnails batch API (POST /v1/batch) by handing back a "Completed"
//              entry per request whose imageUrl points at this server's /emu-thumbnail image endpoint.
//              Thumbnail types retain their Roblox meaning: GameThumbnail is a place carousel image,
//              GameIcon is a universe icon, and asset/user/group thumbnails use their own item records.
#include <NoobWarrior/HttpServer/Emulator/ThumbnailHandler.h>
#include <NoobWarrior/NoobWarrior.h>

#include <nlohmann/json.hpp>

#include "../../algorithm/gzip.h"

#include <cctype>
#include <cstdlib>
#include <string>
#include <utility>

using namespace NoobWarrior;

static std::string GetQueryParam(const char* uri, const char* key) {
    std::string out;
    if (uri == nullptr) return out;
    evkeyvalq query;
    if (evhttp_parse_query(uri, &query) == 0) {
        if (const char* val = evhttp_find_header(&query, key))
            out = val;
        evhttp_clear_headers(&query);
    }
    return out;
}

static std::string NormalizeThumbnailType(std::string type) {
    for (char &c : type)
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return type;
}

static std::vector<unsigned char> RetrieveThumbnail(EmuDbManager *dbm, std::string type, int64_t id) {
    type = NormalizeThumbnailType(std::move(type));

    // These two identifiers are deliberately different. CoreScripts requests GameThumbnail with a
    // place id for its 768x432 loading backdrop, but GameIcon with a universe id for the square tile.
    if (type == "gamethumbnail")
        return dbm->RetrievePlaceThumbnailData(id);
    if (type == "gameicon") {
        // PlaceUniverseHandler intentionally uses place==universe as a compatibility fallback when a
        // standalone place has no Universe row. Preserve its icon in that case instead of returning
        // the unknown-universe placeholder.
        const std::optional<int64_t> startPlaceId = dbm->GetStartPlaceIdForUniverse(id);
        return dbm->RetrieveImageData(ItemType::Asset,
            startPlaceId.has_value() && startPlaceId.value() > 0 ? startPlaceId.value() : id);
    }

    if (type.find("avatar") != std::string::npos || type.find("headshot") != std::string::npos || type.find("bust") != std::string::npos)
        return dbm->RetrieveImageData(ItemType::User, id);
    if (type == "groupicon")
        return dbm->RetrieveImageData(ItemType::Group, id);
    if (type == "badgeicon")
        return dbm->RetrieveImageData(ItemType::Badge, id);
    if (type == "bundlethumbnail")
        return dbm->RetrieveImageData(ItemType::Bundle, id);
    if (type == "outfit")
        return dbm->RetrieveImageData(ItemType::Outfit, id);
    if (type == "gamepass")
        return dbm->RetrieveImageData(ItemType::Pass, id);
    if (type == "developerproduct")
        return dbm->RetrieveImageData(ItemType::DevProduct, id);
    return dbm->RetrieveImageData(ItemType::Asset, id);
}

ThumbnailHandler::ThumbnailHandler(EmuDbManager *dbm) : mEmuDbManager(dbm) {}

void ThumbnailHandler::OnRequest(evhttp_request *req, void *userdata) {
    const char* uri = evhttp_request_get_uri(req);
    std::string path = uri ? uri : "";
    if (auto q = path.find('?'); q != std::string::npos)
        path.resize(q);

    if (path.find("emu-thumbnail") != std::string::npos)
        ServeImage(req);
    else
        ServeBatch(req);
}

void ThumbnailHandler::ServeBatch(evhttp_request *req) {
    // The request body is a JSON array of { requestId, targetId, type, size, format, ... }.
    std::string bodyStr;
    if (evbuffer* in = evhttp_request_get_input_buffer(req)) {
        size_t len = evbuffer_get_length(in);
        bodyStr.resize(len);
        if (len > 0)
            evbuffer_copyout(in, bodyStr.data(), len);
    }

    GunzipIfNeeded(bodyStr);

    nlohmann::json requests = nlohmann::json::parse(bodyStr, nullptr, false);

    nlohmann::json data = nlohmann::json::array();
    if (requests.is_array()) {
        for (const auto& r : requests) {
            std::string requestId = r.value("requestId", std::string());
            std::string type = r.value("type", std::string("Asset"));

            int64_t targetId = 0;
            if (auto it = r.find("targetId"); it != r.end()) {
                if (it->is_number_integer()) targetId = it->get<int64_t>();
                else if (it->is_string()) targetId = strtoll(it->get<std::string>().c_str(), nullptr, 10);
            }

            // Point at a real thumbnail CDN host (the connect-hook redirects it back to us). Studio's
            // image fetcher appears to only accept thumbnail urls on rbxcdn hosts.
            nlohmann::json entry;
            entry["requestId"] = requestId;
            entry["errorCode"] = 0;
            entry["errorMessage"] = "";
            entry["targetId"] = targetId;
            entry["state"] = "Completed";
            entry["imageUrl"] = "https://t0.rbxcdn.com/emu-thumbnail?assetId=" + std::to_string(targetId) + "&type=" + type;
            entry["version"] = "TN3.5";
            data.push_back(std::move(entry));
        }
    }

    nlohmann::json j;
    j["data"] = std::move(data);

    const std::string out = j.dump();
    evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type", "application/json");
    evbuffer* buf = evbuffer_new();
    evbuffer_add(buf, out.data(), out.size());
    evhttp_send_reply(req, HTTP_OK, nullptr, buf);
    evbuffer_free(buf);
}

void ThumbnailHandler::ServeImage(evhttp_request *req) {
    const char* uri = evhttp_request_get_uri(req);
    std::string idStr = GetQueryParam(uri, "assetId");
    if (idStr.empty())
        idStr = GetQueryParam(uri, "id");

    char* endPtr;
    int64_t id = strtoll(idStr.c_str(), &endPtr, 10);
    if (idStr.empty() || *endPtr != '\0') {
        evhttp_send_error(req, 400, "Invalid asset id");
        return;
    }

    std::vector<unsigned char> image = RetrieveThumbnail(mEmuDbManager, GetQueryParam(uri, "type"), id);
    if (image.empty()) {
        evhttp_send_error(req, 404, "No image");
        return;
    }

    evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type", "image/png");
    evbuffer* buf = evbuffer_new();
    evbuffer_add(buf, image.data(), image.size());
    evhttp_send_reply(req, HTTP_OK, nullptr, buf);
    evbuffer_free(buf);
}
