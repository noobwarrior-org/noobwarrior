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
// File: UserProfilesHandler.cpp
// Started by: Hattozo
// Started on: 8/10/2026
// Description: Modern in-experience batch user-profile endpoint.
#include <NoobWarrior/HttpServer/Emulator/UserProfilesHandler.h>

#include <NoobWarrior/HttpServer/Emulator/ServerEmulator.h>
#include <NoobWarrior/NoobWarrior.h>
#include <NoobWarrior/Registry.h>

#include <event2/buffer.h>
#include <event2/http.h>
#include <nlohmann/json.hpp>

#include <utility>

using namespace NoobWarrior;

UserProfilesHandler::UserProfilesHandler(ServerEmulator* emu) : mEmu(emu) {
}

void UserProfilesHandler::OnRequest(evhttp_request* req, void* userdata) {
    if (evhttp_request_get_command(req) != EVHTTP_REQ_POST) {
        evhttp_send_error(req, HTTP_BADMETHOD, "POST required");
        return;
    }

    evbuffer* input = evhttp_request_get_input_buffer(req);
    const std::size_t inputSize = evbuffer_get_length(input);
    const unsigned char* inputData = evbuffer_pullup(input, -1);
    if (inputData == nullptr || inputSize == 0) {
        evhttp_send_error(req, HTTP_BADREQUEST, "Invalid request body");
        return;
    }

    const nlohmann::json request = nlohmann::json::parse(
        inputData, inputData + inputSize, nullptr, false);
    if (request.is_discarded() || !request.contains("userIds") ||
        !request["userIds"].is_array()) {
        evhttp_send_error(req, HTTP_BADREQUEST, "Invalid request body");
        return;
    }

    Registry* registry = mEmu->GetCore()->GetRegistry();
    std::string username = registry->GetKeyValue<std::string>("user.name").value_or("Player");
    std::string displayName = registry->GetKeyValue<std::string>("user.display_name").value_or(username);

    if (registry->GetKeyValue<bool>("emu.auth.enabled").value_or(false)) {
        if (auto user = mEmu->ResolveJoiningUser(req)) {
            username = user->name;
            displayName = user->displayName.empty() ? user->name : user->displayName;
        }
    }

    const std::string combinedName = displayName == username
        ? username
        : displayName + " (@" + username + ")";

    nlohmann::json response;
    response["profileDetails"] = nlohmann::json::array();
    response["errors"] = nlohmann::json::array();
    for (const nlohmann::json& userId : request["userIds"]) {
        if (!userId.is_string() && !userId.is_number_integer() &&
            !userId.is_number_unsigned()) {
            continue;
        }

        nlohmann::json detail;
        detail["userId"] = userId;
        detail["names"] = {
            {"username", username},
            {"displayName", displayName},
            {"combinedName", combinedName},
            {"inExperienceCombinedName", combinedName},
            {"contactName", displayName},
            {"alias", ""},
            {"platformName", username},
        };
        detail["platformProfileId"] = "";
        detail["isVerified"] = false;
        detail["hasRobloxSubscription"] = false;
        response["profileDetails"].push_back(std::move(detail));
    }

    const std::string body = response.dump();
    evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type", "application/json");
    evbuffer* output = evbuffer_new();
    evbuffer_add(output, body.data(), body.size());
    evhttp_send_reply(req, HTTP_OK, nullptr, output);
    evbuffer_free(output);
}
