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
// File: OmniRecHandler.cpp
// Started by: Hattozo
// Started on: 6/6/2026
// Description:
#include <NoobWarrior/HttpServer/Emulator/OmniRecHandler.h>
#include <nlohmann/json.hpp>

using namespace NoobWarrior;

OmniRecHandler::OmniRecHandler() {}

void OmniRecHandler::OnRequest(evhttp_request *req, void *userdata) {
    nlohmann::json rec;
    rec["contentType"] = "Game";
    rec["contentId"] = 1818;
    rec["contentStringId"] = "1818";

    nlohmann::json sort;
    sort["sortId"] = "recommended";
    sort["topic"] = "Recommended For You";
    sort["topicId"] = 1;
    sort["treatmentType"] = "Carousel";
    sort["recommendationList"] = nlohmann::json::array({rec});
    sort["nextSortToken"] = "";
    sort["contentTypeFiltersToExclude"] = nlohmann::json::array();

    nlohmann::json gameMeta;
    gameMeta["universeId"] = 1;
    gameMeta["rootPlaceId"] = 1818;
    gameMeta["name"] = "Crossroads";
    gameMeta["playerCount"] = 1;
    gameMeta["totalUpVotes"] = 1;
    gameMeta["totalDownVotes"] = 0;
    gameMeta["creatorName"] = "Player";
    gameMeta["creatorId"] = 1;
    gameMeta["creatorType"] = "User";
    gameMeta["creatorHasVerifiedBadge"] = false;
    gameMeta["isSponsored"] = false;
    gameMeta["nativeAdData"] = "";
    gameMeta["isShowSponsoredLabel"] = false;
    gameMeta["price"] = nullptr;
    gameMeta["analyticsIdentifier"] = nullptr;
    gameMeta["gameDescription"] = "noobWarrior team test";
    gameMeta["genre"] = "All";

    nlohmann::json j;
    j["sorts"] = nlohmann::json::array({sort});
    j["contentMetadata"]["Game"]["1818"] = gameMeta;
    j["nextPageToken"] = "";

    const std::string body = j.dump();
    evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type", "application/json");
    evbuffer* buf = evbuffer_new();
    evbuffer_add_printf(buf, "%s", body.c_str());
    evhttp_send_reply(req, HTTP_OK, nullptr, buf);
    evbuffer_free(buf);
}
