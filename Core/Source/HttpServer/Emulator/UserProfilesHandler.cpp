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

#include <charconv>
#include <limits>
#include <optional>
#include <string>
#include <utility>

using namespace NoobWarrior;

namespace {
struct ProfileIdentity {
    std::string Username;
    std::string DisplayName;
};

std::optional<int64_t> ParseUserId(const nlohmann::json& value) {
    if (value.is_number_integer())
        return value.get<int64_t>();
    if (value.is_number_unsigned()) {
        const uint64_t unsignedValue = value.get<uint64_t>();
        if (unsignedValue <= static_cast<uint64_t>(std::numeric_limits<int64_t>::max()))
            return static_cast<int64_t>(unsignedValue);
        return std::nullopt;
    }
    if (!value.is_string())
        return std::nullopt;

    const std::string& text = value.get_ref<const std::string&>();
    int64_t result = 0;
    const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), result);
    if (error != std::errc() || end != text.data() + text.size())
        return std::nullopt;
    return result;
}

nlohmann::json MakeProfileDetail(const nlohmann::json& requestedUserId,
                                 const std::optional<ProfileIdentity>& identity) {
    std::string username;
    std::string displayName;
    std::string combinedName;
    if (identity) {
        username = identity->Username;
        displayName = identity->DisplayName.empty() ? username : identity->DisplayName;
        combinedName = displayName == username
            ? username
            : displayName + " (@" + username + ")";
    }

    nlohmann::json detail;
    detail["userId"] = requestedUserId;
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
    return detail;
}
}

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
    const int64_t localUserId = registry->GetKeyValue<int64_t>("user.id").value_or(1000);
    const std::string localUsername =
        registry->GetKeyValue<std::string>("user.name").value_or("Player");
    const std::string localDisplayName =
        registry->GetKeyValue<std::string>("user.display_name").value_or(localUsername);

    std::optional<AuthUtil::SessionUser> requestUser;
    if (registry->GetKeyValue<bool>("emu.auth.enabled").value_or(false))
        requestUser = mEmu->ResolveJoiningUser(req);

    nlohmann::json response;
    response["profileDetails"] = nlohmann::json::array();
    response["errors"] = nlohmann::json::array();
    for (const nlohmann::json& userId : request["userIds"]) {
        const std::optional<int64_t> parsedUserId = ParseUserId(userId);
        if (!parsedUserId)
            continue;

        // Only label the identity owned by this emulator/request. The old implementation copied
        // that identity onto every requested id, producing duplicate names in PlayerList.
        std::optional<ProfileIdentity> identity;
        if (requestUser && requestUser->id == *parsedUserId) {
            identity = ProfileIdentity{
                requestUser->name,
                requestUser->displayName.empty() ? requestUser->name : requestUser->displayName,
            };
        } else if (*parsedUserId == localUserId) {
            identity = ProfileIdentity{localUsername, localDisplayName};
        }
        response["profileDetails"].push_back(MakeProfileDetail(userId, identity));
    }

    const std::string body = response.dump();
    evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type", "application/json");
    evbuffer* output = evbuffer_new();
    evbuffer_add(output, body.data(), body.size());
    evhttp_send_reply(req, HTTP_OK, nullptr, output);
    evbuffer_free(output);
}
