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
// File: ToolboxServiceHandler.cpp
// Started by: Hattozo
// Started on: 6/6/2026
// Description: Modern /toolbox-service/v1 endpoints (home configuration, category search, item details).
//              Search returns asset ids; Studio then asks /items/details for the full records, both
//              served straight from the mounted databases.
#include <NoobWarrior/HttpServer/Emulator/ToolboxServiceHandler.h>
#include <NoobWarrior/HttpServer/Emulator/ToolboxAssetCategory.h>
#include <NoobWarrior/HttpServer/Base/HttpServer.h>
#include <NoobWarrior/NoobWarrior.h>

#include <nlohmann/json.hpp>

#include <cstdlib>
#include <cstring>
#include <ctime>
#include <string>
#include <vector>

using namespace NoobWarrior;

// Pulls every integer value for a query key out of the raw URI, handling both the repeated
// (assetIds=1&assetIds=2) and comma-separated (assetIds=1,2) forms, percent-decoding each value.
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

// Formats a unix timestamp as the ISO-8601 UTC string the toolbox expects; 0 maps to a stable default.
static std::string UnixToIso(int64_t t) {
    if (t <= 0) return "2015-01-01T00:00:00Z";
    std::time_t tt = static_cast<std::time_t>(t);
    std::tm tmv{};
#if defined(_WIN32)
    gmtime_s(&tmv, &tt);
#else
    gmtime_r(&tt, &tmv);
#endif
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &tmv);
    return buf;
}

static nlohmann::json BuildCreator(EmuDbManager* dbm, const EmuDb::AssetSummary& summary) {
    nlohmann::json creator;
    if (summary.GroupId.has_value()) {
        creator["id"] = summary.GroupId.value();
        creator["name"] = dbm->GetItemName(ItemType::Group, summary.GroupId.value()).value_or("Group");
        creator["type"] = 2; // Group
    } else {
        int64_t userId = summary.UserId.value_or(1);
        creator["id"] = userId;
        creator["name"] = dbm->GetItemName(ItemType::User, userId).value_or("Player");
        creator["type"] = 1; // User
    }
    creator["isVerifiedCreator"] = false;
    return creator;
}

static void SendJson(evhttp_request* req, const nlohmann::json& j) {
    const std::string body = j.dump();
    evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type", "application/json");
    evbuffer* buf = evbuffer_new();
    evbuffer_add(buf, body.data(), body.size());
    evhttp_send_reply(req, HTTP_OK, nullptr, buf);
    evbuffer_free(buf);
}

ToolboxServiceHandler::ToolboxServiceHandler(HttpServer *srv, EmuDbManager *dbm) :
    mHttpServer(srv),
    mEmuDbManager(dbm)
{}

void ToolboxServiceHandler::OnRequest(evhttp_request *req, void *userdata) {
    const char* uri = evhttp_request_get_uri(req);
    std::string path = uri ? uri : "";
    if (auto q = path.find('?'); q != std::string::npos)
        path.resize(q);

    if (path.ends_with("/configuration") && path.find("/home/") != std::string::npos)
        HandleConfiguration(req);
    else if (path.ends_with("/assets") && path.find("/section/") != std::string::npos)
        HandleSection(req);
    else if (path.ends_with("/items/details"))
        HandleItemDetails(req);
    else
        HandleSearch(req);
}

void ToolboxServiceHandler::HandleConfiguration(evhttp_request *req) {
    static constexpr const char* kConfig = R"({"sections":[{"displayName":"Essential","name":"essential"}]})";

    evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type", "application/json");
    evbuffer* buf = evbuffer_new();
    evbuffer_add(buf, kConfig, strlen(kConfig));
    evhttp_send_reply(req, HTTP_OK, nullptr, buf);
    evbuffer_free(buf);
}

// Asset ids shown for a toolbox category. The Decals tab (type 13) also surfaces raw Image assets
// (type 1), since grabbed images usually have no Decal wrapper and would otherwise never appear.
static std::vector<int64_t> SearchToolboxAssets(EmuDbManager* dbm, Roblox::AssetType type,
                                                const std::string& keyword, int limit) {
    std::vector<int64_t> ids = dbm->SearchAssetIds(type, keyword, limit, 0);
    if (type == Roblox::AssetType::Decal) {
        for (int64_t id : dbm->SearchAssetIds(Roblox::AssetType::Image, keyword, limit, 0)) {
            if (static_cast<int>(ids.size()) >= limit) break;
            ids.push_back(id);
        }
    }
    return ids;
}

void ToolboxServiceHandler::HandleSearch(evhttp_request *req) {
    Roblox::AssetType type = ToolboxCategoryToAssetType(mHttpServer->GetRouteParam("category"));

    std::string keyword;
    int limit = 30;
    const char* uri = evhttp_request_get_uri(req);
    evkeyvalq query;
    if (uri != nullptr && evhttp_parse_query(uri, &query) == 0) {
        if (const char* kw = evhttp_find_header(&query, "keyword"))
            keyword = kw;
        if (const char* lim = evhttp_find_header(&query, "limit")) {
            int v = atoi(lim);
            if (v > 0) limit = v;
        }
        evhttp_clear_headers(&query);
    }
    if (limit > 100) limit = 100; // guard against an enormous page

    std::vector<int64_t> ids = SearchToolboxAssets(mEmuDbManager, type, keyword, limit);

    nlohmann::json data = nlohmann::json::array();
    for (int64_t id : ids) {
        nlohmann::json item;
        item["id"] = id;
        item["searchResultSource"] = "LexicalWithSort";
        data.push_back(std::move(item));
    }

    // Shape matches the real toolbox-service search response.
    nlohmann::json j;
    j["totalResults"] = static_cast<int>(ids.size());
    j["filteredKeyword"] = keyword;
    j["spellCheckerResult"] = { {"correctionState", 0} };
    j["queryFacets"] = { {"appliedFacets", nlohmann::json::array()}, {"availableFacets", nlohmann::json::array()} };
    j["imageSearchStatus"] = nullptr;
    j["data"] = std::move(data);
    j["nextPageCursor"] = "";
    SendJson(req, j);
}

void ToolboxServiceHandler::HandleSection(evhttp_request *req) {
    // Populates a home-page row, e.g. /toolbox-service/v1/home/10/section/trending/assets. The typeId
    // segment is the numeric Roblox asset type (10 = Model, 3 = Audio, ...). We surface the local
    // assets of that type in every section so the user's items appear on the toolbox home page.
    Roblox::AssetType type = static_cast<Roblox::AssetType>(atoi(mHttpServer->GetRouteParam("typeId").c_str()));

    int limit = 20;
    const char* uri = evhttp_request_get_uri(req);
    evkeyvalq query;
    if (uri != nullptr && evhttp_parse_query(uri, &query) == 0) {
        if (const char* lim = evhttp_find_header(&query, "limit")) {
            int v = atoi(lim);
            if (v > 0) limit = v;
        }
        evhttp_clear_headers(&query);
    }
    if (limit > 100) limit = 100;

    std::vector<int64_t> ids = SearchToolboxAssets(mEmuDbManager, type, "", limit);

    nlohmann::json data = nlohmann::json::array();
    for (int64_t id : ids) {
        nlohmann::json item;
        item["id"] = id;
        data.push_back(std::move(item));
    }

    nlohmann::json j;
    j["totalResults"] = static_cast<int>(ids.size());
    j["nextPageCursor"] = nullptr;
    j["data"] = std::move(data);
    SendJson(req, j);
}

void ToolboxServiceHandler::HandleItemDetails(evhttp_request *req) {
    std::vector<int64_t> ids = ParseIdParams(evhttp_request_get_uri(req), "assetIds");

    // The toolbox wants an entry for every requested id, and a batch is single-type (the Audio tab
    // asks about Studio's built-in default sounds we don't have). Pre-scan to learn the batch's type
    // so the ids we lack get type-shaped (e.g. audio) entries instead of breaking rendering.
    std::vector<std::optional<EmuDb::AssetSummary>> summaries;
    summaries.reserve(ids.size());
    int fallbackType = 0;
    for (int64_t id : ids) {
        summaries.push_back(mEmuDbManager->GetAssetSummary(id));
        if (fallbackType == 0 && summaries.back().has_value())
            fallbackType = summaries.back()->Type;
    }

    nlohmann::json data = nlohmann::json::array();
    for (size_t i = 0; i < ids.size(); i++) {
        const int64_t id = ids[i];
        const std::optional<EmuDb::AssetSummary>& summary = summaries[i];

        int typeId;
        std::string name;
        std::string description;
        int64_t created;
        int64_t updated;
        nlohmann::json creator;
        if (summary.has_value()) {
            typeId = summary->Type;
            name = summary->Name;
            description = summary->Description;
            created = summary->Created;
            updated = summary->Updated;
            creator = BuildCreator(mEmuDbManager, *summary);
        } else {
            // An id we don't have (e.g. a Studio default sound): shape it like the rest of the batch.
            typeId = fallbackType;
            name = std::to_string(id);
            description = "";
            created = 0;
            updated = 0;
            creator["id"] = 1;
            creator["name"] = "Player";
            creator["type"] = 1;
            creator["isVerifiedCreator"] = false;
        }

        // Field-for-field match of the real toolbox-service items/details asset object.
        nlohmann::json asset;
        asset["id"] = id;
        asset["name"] = name;
        asset["typeId"] = typeId;
        asset["assetSubTypes"] = nlohmann::json::array();
        asset["assetGenres"] = nlohmann::json::array({"All"});
        asset["isEndorsed"] = false;
        asset["description"] = description;
        asset["duration"] = 0;
        asset["hasScripts"] = false;
        asset["createdUtc"] = UnixToIso(created);
        asset["updatedUtc"] = UnixToIso(updated);
        asset["isAssetHashApproved"] = true;
        asset["visibilityStatus"] = 0;
        asset["socialLinks"] = nlohmann::json::array();
        asset["previewAssets"]["imagePreviewAssets"] = nlohmann::json::array();
        asset["previewAssets"]["videoPreviewAssets"] = nlohmann::json::array();
        if (typeId == static_cast<int>(Roblox::AssetType::Audio)) {
            // Superset of the old (2023M) and new audioDetails fields. The 2023M Studio audio parser
            // reads soundEffectCategory/soundEffectSubCategory/musicGenre (the modern API dropped them
            // for `tags`); omitting them makes its parser bail and blanks the whole Audio tab.
            nlohmann::json audioDetails;
            audioDetails["audioType"] = "SoundEffect";
            audioDetails["artist"] = "";
            audioDetails["title"] = name;
            audioDetails["musicAlbum"] = "";
            audioDetails["musicGenre"] = "";
            audioDetails["soundEffectCategory"] = "";
            audioDetails["soundEffectSubCategory"] = "";
            audioDetails["tags"] = nlohmann::json::array();
            asset["audioDetails"] = std::move(audioDetails);
            // We don't store real durations, but the audio list drops zero-length sounds, so give a
            // non-zero placeholder so the tiles actually appear.
            asset["duration"] = 1;
        } else if (typeId == static_cast<int>(Roblox::AssetType::Model)
                || typeId == static_cast<int>(Roblox::AssetType::MeshPart)) {
            // Model tiles carry technical details + capabilities; the toolbox reads capabilities to
            // decide insert sandboxing, so a model entry must include these (real assets do).
            asset["scriptCount"] = 0;
            asset["modelTechnicalDetails"]["objectMeshSummary"]["triangles"] = 0;
            asset["modelTechnicalDetails"]["objectMeshSummary"]["vertices"] = 0;
            asset["modelTechnicalDetails"]["instanceCounts"]["script"] = 0;
            asset["modelTechnicalDetails"]["instanceCounts"]["meshPart"] = 0;
            asset["modelTechnicalDetails"]["instanceCounts"]["animation"] = 0;
            asset["modelTechnicalDetails"]["instanceCounts"]["decal"] = 0;
            asset["modelTechnicalDetails"]["instanceCounts"]["audio"] = 0;
            asset["modelTechnicalDetails"]["instanceCounts"]["tool"] = 0;
            asset["categoryPath"] = "";
            asset["capabilities"]["shouldSandbox"] = false;
            asset["objectTypes"] = nlohmann::json::array();
        }

        nlohmann::json voting;
        voting["showVotes"] = true;
        voting["upVotes"] = 0;
        voting["downVotes"] = 0;
        voting["canVote"] = false;
        voting["hasVoted"] = false;
        voting["voteCount"] = 0;
        voting["upVotePercent"] = 0;

        // Free, published product so the toolbox treats the item as a free insert.
        nlohmann::json fiatProduct;
        fiatProduct["purchasePrice"]["currencyCode"] = "USD";
        fiatProduct["purchasePrice"]["quantity"]["significand"] = 0;
        fiatProduct["purchasePrice"]["quantity"]["exponent"] = 0;
        fiatProduct["published"] = true;
        fiatProduct["purchasable"] = true;
        fiatProduct["isFree"] = true;

        nlohmann::json entry;
        entry["asset"] = std::move(asset);
        entry["creator"] = std::move(creator);
        entry["voting"] = std::move(voting);
        entry["fiatProduct"] = std::move(fiatProduct);
        data.push_back(std::move(entry));
    }

    nlohmann::json j;
    j["data"] = std::move(data);
    SendJson(req, j);
}
