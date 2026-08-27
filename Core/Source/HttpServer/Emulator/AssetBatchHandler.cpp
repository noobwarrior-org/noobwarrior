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
// File: AssetBatchHandler.cpp
// Started by: Hattozo
// Started on: 8/12/2026
// Description: Implements POST /v1/assets/batch and /v2/assets/batch for modern Roblox clients.
#include <NoobWarrior/HttpServer/Emulator/AssetBatchHandler.h>
#include <NoobWarrior/HttpServer/Emulator/ServerEmulator.h>
#include <NoobWarrior/HttpServer/Emulator/VideoHandler.h>
#include <NoobWarrior/NoobWarrior.h>

#include "../../algorithm/gzip.h"

#include <nlohmann/json.hpp>

#include <cstdlib>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <utility>

using namespace NoobWarrior;

static std::optional<int64_t> ReadInteger(const nlohmann::json& value) {
    if (value.is_number_unsigned()) {
        const uint64_t number = value.get<uint64_t>();
        if (number <= static_cast<uint64_t>((std::numeric_limits<int64_t>::max)()))
            return static_cast<int64_t>(number);
    }
    if (value.is_number_integer())
        return value.get<int64_t>();
    if (value.is_string()) {
        const std::string text = value.get<std::string>();
        if (text.empty())
            return std::nullopt;
        char* end = nullptr;
        const int64_t number = std::strtoll(text.c_str(), &end, 10);
        if (end != text.c_str() && *end == '\0')
            return number;
    }
    return std::nullopt;
}

static std::string ReadString(const nlohmann::json& request, const char* key) {
    if (!request.is_object())
        return {};
    const auto it = request.find(key);
    if (it == request.end())
        return {};
    if (it->is_string())
        return it->get<std::string>();
    if (it->is_boolean())
        return it->get<bool>() ? "true" : "false";
    if (it->is_number_integer())
        return std::to_string(it->get<int64_t>());
    return {};
}

static std::string UriEncode(const std::string& value) {
    char* encoded = evhttp_uriencode(value.c_str(), static_cast<ev_ssize_t>(value.size()), 0);
    if (encoded == nullptr)
        return {};
    std::string out(encoded);
    free(encoded);
    return out;
}

static nlohmann::json RequestIdFor(const nlohmann::json& request, int64_t assetId) {
    if (request.is_object()) {
        const auto it = request.find("requestId");
        if (it != request.end() && (it->is_string() || it->is_number()))
            return *it;
    }
    return std::to_string(assetId);
}

static void SendJson(evhttp_request *req, int status, const nlohmann::json& value) {
    const std::string body = value.dump();
    evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type", "application/json");
    evbuffer* output = evbuffer_new();
    evbuffer_add(output, body.data(), body.size());
    evhttp_send_reply(req, status, nullptr, output);
    evbuffer_free(output);
}

AssetBatchHandler::AssetBatchHandler(ServerEmulator* emu) :
    mEmu(emu)
{}

void AssetBatchHandler::OnRequest(evhttp_request *req, void *userdata) {
    if (evhttp_request_get_command(req) == EVHTTP_REQ_POST) {
        std::string peek;
        if (evbuffer* input = evhttp_request_get_input_buffer(req)) {
            const size_t length = evbuffer_get_length(input);
            peek.resize(length);
            if (length != 0)
                evbuffer_copyout(input, peek.data(), length); // copyout, so the body survives for us
        }
        GunzipIfNeeded(peek);

        const nlohmann::json parsed = nlohmann::json::parse(peek, nullptr, false);
        bool mentionsVideo = false;

        const char *acrKind = "absent";
        std::string acrSample;

        if (parsed.is_array()) {
            for (const nlohmann::json& entry : parsed) {
                if (!entry.is_object())
                    continue;
                const auto field = entry.find("contentRepresentationPriorityList");
                if (field == entry.end())
                    continue;
                if (std::string_view(acrKind) == "absent") {
                    acrKind = field->type_name();
                    acrSample = field->is_string() ? field->get<std::string>() : field->dump();
                    if (acrSample.size() > 96)
                        acrSample.resize(96);
                }
                const std::string acr = ReadString(entry, "contentRepresentationPriorityList");
                if (!acr.empty() && VideoHandler::AcrRequestsHls(acr.c_str())) {
                    mentionsVideo = true;
                    break;
                }
            }
        }

        const size_t layerCount = mEmu->GetProxyLayers().size();
        Out("AssetBatchHandler",
            "batch: {} entries, mentionsVideo={}, layers={}, proxying={}, acr={}:\"{}\"",
            parsed.is_array() ? parsed.size() : 0, mentionsVideo, layerCount,
            mentionsVideo && layerCount > 0, acrKind, acrSample);

        if (mentionsVideo &&
            mEmu->TryProxyRequest(req, [this](evhttp_request *r) { HandleLocally(r); })) {
            return;
        }
    }

    HandleLocally(req);
}

void AssetBatchHandler::HandleLocally(evhttp_request *req) {
    if (evhttp_request_get_command(req) != EVHTTP_REQ_POST) {
        evhttp_add_header(evhttp_request_get_output_headers(req), "Allow", "POST");
        evhttp_send_error(req, HTTP_BADMETHOD, "POST required");
        return;
    }

    std::string body;
    if (evbuffer* input = evhttp_request_get_input_buffer(req)) {
        const size_t length = evbuffer_get_length(input);
        body.resize(length);
        if (length != 0)
            evbuffer_copyout(input, body.data(), length);
    }
    GunzipIfNeeded(body);

    const nlohmann::json requests = nlohmann::json::parse(body, nullptr, false);
    if (!requests.is_array()) {
        evhttp_send_error(req, HTTP_BADREQUEST, "Expected a JSON array");
        return;
    }

    nlohmann::json response = nlohmann::json::array();
    for (const nlohmann::json& request : requests) {
        std::optional<int64_t> assetId;
        if (request.is_object()) {
            const auto id = request.find("assetId");
            if (id != request.end())
                assetId = ReadInteger(*id);
        }

        if (!assetId.has_value() || *assetId <= 0) {
            nlohmann::json result;
            result["errors"] = nlohmann::json::array({nlohmann::json{
                {"code", 400}, {"message", "Invalid asset id"}
            }});
            result["requestId"] = RequestIdFor(request, 0);
            result["isArchived"] = false;
            result["assetTypeId"] = 0;
            result["isRecordable"] = false;
            response.push_back(std::move(result));
            continue;
        }

        int version = 0;
        if (const auto it = request.find("version"); it != request.end()) {
            if (const auto value = ReadInteger(*it); value.has_value() && *value > 0 &&
                *value <= (std::numeric_limits<int>::max)()) {
                version = static_cast<int>(*value);
            }
        }

        int assetTypeId = 0;
        if (const auto summary = mEmu->GetCore()->GetEmuDbManager()->GetAssetSummary(*assetId)) {
            assetTypeId = summary->Type;
        } else if (const auto it = request.find("assetTypeId"); it != request.end()) {
            if (const auto value = ReadInteger(*it); value.has_value() && *value >= 0 &&
                *value <= (std::numeric_limits<int>::max)()) {
                assetTypeId = static_cast<int>(*value);
            }
        }

        const std::string contentRepList = ReadString(request, "contentRepresentationPriorityList");
        const std::string doNotFallback = ReadString(request, "doNotFallbackToBaselineRepresentation");
        const std::string resolutionMode = ReadString(request, "assetResolutionMode");
        const std::string accept = ReadString(request, "accept");

        std::string location = "https://assetdelivery.roblox.com/v1/asset/?id=" +
            std::to_string(*assetId);
        if (version > 0)
            location += "&version=" + std::to_string(version);
        if (!accept.empty())
            location += "&accept=" + UriEncode(accept);
        if (!contentRepList.empty())
            location += "&contentRepresentationPriorityList=" + UriEncode(contentRepList);
        if (!doNotFallback.empty())
            location += "&doNotFallbackToBaselineRepresentation=" + UriEncode(doNotFallback);
        if (!resolutionMode.empty())
            location += "&assetResolutionMode=" + UriEncode(resolutionMode);
        
        if (VideoHandler::AcrRequestsHls(contentRepList.c_str())) {
            const std::string playlist = VideoHandler::ResolvePlaylistPath(mEmu, *assetId, version);
            if (!playlist.empty()) {
                location = "https://assetdelivery.roblox.com" + playlist;
                Out("AssetBatchHandler", "  video asset {} -> {}", *assetId, location);
            } else {
                // The counterpart line. Silence here used to be indistinguishable from "not a video".
                Out("AssetBatchHandler", "  video asset {} kept the PLAIN location {} "
                                         "(no local blob, or not segmented yet)", *assetId, location);
            }
        }

        nlohmann::json result;
        result["location"] = std::move(location);
        result["requestId"] = RequestIdFor(request, *assetId);
        result["isArchived"] = false;
        result["assetTypeId"] = assetTypeId;
        result["assetMetadatas"] = nlohmann::json::array();
        result["isRecordable"] = true;
        response.push_back(std::move(result));
    }

    if (mEmu->GetCore()->GetRegistry()->GetKeyValue<bool>("debug.log_http_server_requests").value_or(false)) {
        if (!requests.empty() && requests[0].is_object()) {
            std::string keys;
            for (auto it = requests[0].begin(); it != requests[0].end(); ++it)
                keys += (keys.empty() ? "" : ", ") + it.key();
            Out("AssetBatchHandler", "  request fields: {}", keys);
        }
        {
            std::string ids;
            int shown = 0;
            for (const nlohmann::json& request : requests) {
                if (!request.is_object())
                    continue;
                const auto id = request.find("assetId");
                if (id == request.end())
                    continue;
                if (shown++ >= 24) { ids += ", ..."; break; }
                const auto value = ReadInteger(*id);
                if (value.has_value())
                    ids += (ids.empty() ? "" : ", ") + std::to_string(*value);
            }
            Out("AssetBatchHandler", "  ids: {}", ids);
        }
    }
    
    SendJson(req, HTTP_OK, response);
}
