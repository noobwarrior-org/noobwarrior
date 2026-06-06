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
// File: AvatarHandler.cpp
// Started by: Hattozo
// Started on: 6/6/2026
// Description:
#include <NoobWarrior/HttpServer/Emulator/AvatarHandler.h>
#include <nlohmann/json.hpp>

using namespace NoobWarrior;

AvatarHandler::AvatarHandler() {}

void AvatarHandler::OnRequest(evhttp_request *req, void *userdata) {
    nlohmann::json j;
    j["scales"]["height"] = 1.0;
    j["scales"]["width"] = 1.0;
    j["scales"]["head"] = 1.0;
    j["scales"]["depth"] = 1.0;
    j["scales"]["proportion"] = 0.0;
    j["scales"]["bodyType"] = 0.0;
    j["playerAvatarType"] = "R6";
    j["bodyColors"]["headColorId"] = 1001;
    j["bodyColors"]["torsoColorId"] = 1001;
    j["bodyColors"]["rightArmColorId"] = 1001;
    j["bodyColors"]["leftArmColorId"] = 1001;
    j["bodyColors"]["rightLegColorId"] = 1001;
    j["bodyColors"]["leftLegColorId"] = 1001;
    j["assets"] = nlohmann::json::array();
    j["defaultShirtApplied"] = true;
    j["defaultPantsApplied"] = true;

    nlohmann::json emote1, emote2, emote3, emote4;
    emote1["assetId"] = 3576686446;
    emote1["assetName"] = "Hello";
    emote1["position"] = 1;
    emote2["assetId"] = 3360686498;
    emote2["assetName"] = "Stadium";
    emote2["position"] = 2;
    emote3["assetId"] = 3576823880;
    emote3["assetName"] = "Point2";
    emote3["position"] = 3;
    emote4["assetId"] = 3576968026;
    emote4["assetName"] = "Shrug";
    emote4["position"] = 4;
    j["emotes"] = nlohmann::json::array({emote1, emote2, emote3, emote4});

    const std::string body = j.dump();
    evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type", "application/json");
    evbuffer* buf = evbuffer_new();
    evbuffer_add_printf(buf, "%s", body.c_str());
    evhttp_send_reply(req, HTTP_OK, nullptr, buf);
    evbuffer_free(buf);
}
