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
// File: AvatarFetchHandler.cpp
// Started by: Hattozo
// Started on: 6/5/2026
// Description:
#include <NoobWarrior/HttpServer/Emulator/AvatarFetchHandler.h>
#include <nlohmann/json.hpp>

using namespace NoobWarrior;

AvatarFetchHandler::AvatarFetchHandler(ServerEmulator* emu) : mEmu(emu) {

}

void AvatarFetchHandler::OnRequest(evhttp_request *req, void *userdata) {
    nlohmann::json j;
    j["resolvedAvatarType"] = "R6";
    j["equippedGearVersionIds"] = nlohmann::json::array();
    j["backpackGearVersionIds"] = nlohmann::json::array();
    j["assetAndAssetTypeIds"] = nlohmann::json::array();
    j["animationAssetIds"] = nlohmann::json::object();
    j["bodyColors"]["headColorId"] = 194;
    j["bodyColors"]["torsoColorId"] = 23;
    j["bodyColors"]["rightArmColorId"] = 194;
    j["bodyColors"]["leftArmColorId"] = 194;
    j["bodyColors"]["rightLegColorId"] = 102;
    j["bodyColors"]["leftLegColorId"] = 102;
    j["scales"]["height"] = 1.0;
    j["scales"]["width"] = 1.0;
    j["scales"]["head"] = 1.0;
    j["scales"]["depth"] = 1.0;
    j["scales"]["proportion"] = 0.0;
    j["scales"]["bodyType"] = 0.0;
    j["emotes"] = nlohmann::json::array();

    const std::string body = j.dump();
    evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type", "application/json");
    evbuffer* buf = evbuffer_new();
    evbuffer_add_printf(buf, "%s", body.c_str());
    evhttp_send_reply(req, HTTP_OK, nullptr, buf);
    evbuffer_free(buf);
}
