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
// File: TeleportAuthorizeHandler.cpp
// Started by: Hattozo
// Started on: 8/24/2026
// Description: Authorizes LocalRcc teleports and starts missing destination servers.
#include <NoobWarrior/HttpServer/Emulator/TeleportAuthorizeHandler.h>

#include <NoobWarrior/EmuDb/EmuDbManager.h>
#include <NoobWarrior/EmuDb/ItemType.h>
#include <NoobWarrior/HttpServer/Emulator/AuthUtil.h>
#include <NoobWarrior/HttpServer/Emulator/ServerEmulator.h>
#include <NoobWarrior/Log.h>
#include <NoobWarrior/NoobWarrior.h>
#include <NoobWarrior/Registry.h>

#include <nlohmann/json.hpp>

#include <algorithm>
#include <chrono>
#include <limits>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

using namespace NoobWarrior;

namespace {
constexpr uint16_t kFirstAutomaticGamePort = 53640;
constexpr uint16_t kFirstEphemeralGamePort = 49152;
constexpr std::chrono::seconds kPendingLaunchTimeout {120};

void SendJson(evhttp_request *req, int status, const nlohmann::json &json) {
    evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type", "application/json");
    evbuffer *reply = evbuffer_new();
    const std::string body = json.dump();
    evbuffer_add(reply, body.data(), body.size());
    evhttp_send_reply(req, status, nullptr, reply);
    evbuffer_free(reply);
}

void SendJsonError(evhttp_request *req, int status, const std::string &message) {
    nlohmann::json errors = nlohmann::json::array();
    errors.push_back({{"message", message}});
    SendJson(req, status, {{"errors", std::move(errors)}});
}

bool CanHostGameServer(const Engine &engine) {
    return engine.Side == EngineSide::Server ||
        (engine.Side == EngineSide::Studio && ParseEraVersion(engine.Version) >= 574);
}

std::optional<Engine> SelectServerEngine(Core *core,
                                         const std::vector<RunningInstance> &servers,
                                         const std::pair<std::string, std::string> &launchedStudio) {
    if (core == nullptr)
        return std::nullopt;

    const std::vector<Engine> installed = core->GetInstalledEngines();
    auto findMatch = [&](const auto &predicate) -> std::optional<Engine> {
        auto found = std::find_if(installed.begin(), installed.end(),
            [&](const Engine &engine) { return CanHostGameServer(engine) && predicate(engine); });
        return found == installed.end() ? std::nullopt : std::optional<Engine>(*found);
    };

    for (auto server = servers.rbegin(); server != servers.rend(); ++server) {
        if (!server->Hash.empty()) {
            if (auto match = findMatch([&](const Engine &engine) {
                    return engine.Hash == server->Hash;
                })) {
                return match;
            }
        }
    }
    for (auto server = servers.rbegin(); server != servers.rend(); ++server) {
        if (!server->Version.empty()) {
            if (auto match = findMatch([&](const Engine &engine) {
                    return engine.Version == server->Version;
                })) {
                return match;
            }
        }
    }

    if (!launchedStudio.second.empty()) {
        if (auto match = findMatch([&](const Engine &engine) {
                return engine.Hash == launchedStudio.second;
            })) {
            return match;
        }
    }
    if (!launchedStudio.first.empty()) {
        if (auto match = findMatch([&](const Engine &engine) {
                return engine.Version == launchedStudio.first;
            })) {
            return match;
        }
    }

    if (auto server = findMatch([](const Engine &engine) {
            return engine.Side == EngineSide::Server;
        })) {
        return server;
    }
    return findMatch([](const Engine &engine) {
        return engine.Side == EngineSide::Studio;
    });
}
}

TeleportAuthorizeHandler::TeleportAuthorizeHandler(ServerEmulator *emu) : mEmu(emu) {

}

void TeleportAuthorizeHandler::OnRequest(evhttp_request *req, void *userdata) {
    (void)userdata;

    std::optional<int64_t> destinationPlaceId;
    if (evbuffer *input = evhttp_request_get_input_buffer(req)) {
        const size_t inputLength = evbuffer_get_length(input);
        std::string body(inputLength, '\0');
        if (inputLength > 0)
            evbuffer_copyout(input, body.data(), inputLength);

        const nlohmann::json request = nlohmann::json::parse(body, nullptr, false);
        if (!request.is_discarded() && request.contains("placeId") &&
            request["placeId"].is_number_integer()) {
            const int64_t placeId = request["placeId"].get<int64_t>();
            if (placeId > 0)
                destinationPlaceId = placeId;
        }
    }

    auto recordSuccessfulDestination =
        [this, destinationPlaceId](std::vector<unsigned char> responseBody) {
            if (mEmu != nullptr && destinationPlaceId.has_value())
                mEmu->SetJoinedPlaceId(destinationPlaceId);
            return responseBody;
        };

    if (mEmu != nullptr &&
        mEmu->TryProxyRequest(req, {}, std::move(recordSuccessfulDestination),
                              EmulatorProxy::LayerPolicy::TopOnly)) {
        Out("TeleportAuthorizeHandler",
            "Proxying teleport authorization to the joined remote emulator");
        return;
    }

    HandleLocally(req);
}

void TeleportAuthorizeHandler::HandleLocally(evhttp_request *req) {
    if (evhttp_request_get_command(req) != EVHTTP_REQ_POST) {
        SendJsonError(req, HTTP_BADMETHOD, "Request method needs to be POST");
        return;
    }

    evbuffer *input = evhttp_request_get_input_buffer(req);
    const size_t inputLength = input == nullptr ? 0 : evbuffer_get_length(input);
    std::string body(inputLength, '\0');
    if (inputLength > 0)
        evbuffer_copyout(input, body.data(), inputLength);

    nlohmann::json request;
    try {
        request = nlohmann::json::parse(body);
    } catch (const nlohmann::json::exception &error) {
        Out("TeleportAuthorizeHandler", "Malformed request body: {}", error.what());
        SendJsonError(req, HTTP_BADREQUEST, "Malformed JSON");
        return;
    }

    if (!request.contains("placeId") || !request["placeId"].is_number_integer()) {
        SendJsonError(req, HTTP_BADREQUEST, "Missing or invalid placeId");
        return;
    }
    const int64_t placeId = request["placeId"].get<int64_t>();
    if (placeId <= 0) {
        SendJsonError(req, HTTP_BADREQUEST, "Invalid placeId");
        return;
    }

    if (!request.contains("userIds") || !request["userIds"].is_array() ||
        request["userIds"].empty()) {
        SendJsonError(req, HTTP_BADREQUEST, "Missing or invalid userIds");
        return;
    }

    std::vector<int64_t> userIds;
    std::set<int64_t> uniqueUserIds;
    for (const nlohmann::json &value : request["userIds"]) {
        if (!value.is_number_integer()) {
            SendJsonError(req, HTTP_BADREQUEST, "Invalid userId");
            return;
        }
        const int64_t userId = value.get<int64_t>();
        if (userId == 0) {
            SendJsonError(req, HTTP_BADREQUEST, "Invalid userId");
            return;
        }
        if (uniqueUserIds.insert(userId).second)
            userIds.push_back(userId);
    }

    std::string launchError;
    if (!EnsureDestinationServer(placeId, &launchError)) {
        SendJsonError(req, HTTP_INTERNAL, launchError);
        return;
    }

    nlohmann::json teleportTokens = nlohmann::json::object();
    for (int64_t userId : userIds) {
        std::string token = CreateTeleportToken(userId, placeId);
        if (token.empty()) {
            SendJsonError(req, HTTP_INTERNAL,
                          "Could not create a teleport token for user " +
                              std::to_string(userId));
            return;
        }
        teleportTokens[std::to_string(userId)] = std::move(token);
    }

    Out("TeleportAuthorizeHandler", "Authorized {} player(s) for place {}",
        userIds.size(), placeId);
    SendJson(req, HTTP_OK, {{"teleportTokens", std::move(teleportTokens)}});
}

bool TeleportAuthorizeHandler::EnsureDestinationServer(int64_t placeId, std::string *error) {
    if (mEmu == nullptr || mEmu->GetCore() == nullptr) {
        if (error != nullptr)
            *error = "Server emulator is unavailable";
        return false;
    }

    Core *core = mEmu->GetCore();
    const std::vector<RunningInstance> servers = mEmu->GetRunningGameServers();
    const bool alreadyRunning = std::any_of(servers.begin(), servers.end(),
        [placeId](const RunningInstance &server) {
            return server.PlaceId == placeId && server.Port.has_value() && *server.Port != 0;
        });
    if (alreadyRunning) {
        std::lock_guard lock(mPendingLaunchesMutex);
        mPendingLaunches.erase(placeId);
        return true;
    }

    EmuDbManager *databases = core->GetEmuDbManager();
    if (databases == nullptr ||
        databases->GetFirstDbWhereItemExists(ItemType::Asset, placeId) == nullptr) {
        if (error != nullptr)
            *error = "Destination place is not installed";
        return false;
    }

    const auto now = std::chrono::steady_clock::now();
    uint16_t port = 0;
    {
        std::lock_guard lock(mPendingLaunchesMutex);
        for (auto pending = mPendingLaunches.begin(); pending != mPendingLaunches.end();) {
            if (now - pending->second.StartedAt >= kPendingLaunchTimeout)
                pending = mPendingLaunches.erase(pending);
            else
                ++pending;
        }

        if (mPendingLaunches.contains(placeId))
            return true;

        std::set<uint16_t> usedPorts;
        for (const RunningInstance &server : servers) {
            if (server.Port.has_value() && *server.Port != 0)
                usedPorts.insert(*server.Port);
        }
        for (const auto &[pendingPlaceId, pendingLaunch] : mPendingLaunches) {
            (void)pendingPlaceId;
            usedPorts.insert(pendingLaunch.Port);
        }

        auto selectPort = [&](uint32_t first, uint32_t last) {
            for (uint32_t candidate = first; candidate <= last; ++candidate) {
                const uint16_t candidatePort = static_cast<uint16_t>(candidate);
                if (!usedPorts.contains(candidatePort)) {
                    port = candidatePort;
                    break;
                }
            }
        };
        selectPort(kFirstAutomaticGamePort, std::numeric_limits<uint16_t>::max());
        if (port == 0)
            selectPort(kFirstEphemeralGamePort, kFirstAutomaticGamePort - 1);
        if (port == 0) {
            if (error != nullptr)
                *error = "No game server port is available";
            return false;
        }

        mPendingLaunches[placeId] = {port, now};
    }

    const std::optional<Engine> engine =
        SelectServerEngine(core, servers, mEmu->GetLaunchedStudioVersion());
    if (!engine.has_value()) {
        std::lock_guard lock(mPendingLaunchesMutex);
        mPendingLaunches.erase(placeId);
        if (error != nullptr)
            *error = "No installed engine can host the destination place";
        return false;
    }

    Out("TeleportAuthorizeHandler",
        "No server is running for place {}; starting {} {} on port {}",
        placeId, EngineSideAsString(engine->Side), engine->Version, port);
    const EngineLaunchResponse launch = core->LaunchEngine({
        .Engine = *engine,
        .Port = port,
        .PlaceId = placeId,
        .LaunchSide = EngineSide::Server,
    });
    if (launch != EngineLaunchResponse::Success) {
        std::lock_guard lock(mPendingLaunchesMutex);
        mPendingLaunches.erase(placeId);
        if (error != nullptr) {
            *error = "Could not start the destination game server (error " +
                std::to_string(static_cast<int>(launch)) + ")";
        }
        return false;
    }

    return true;
}

std::string TeleportAuthorizeHandler::CreateTeleportToken(int64_t userId, int64_t placeId) {
    Core *core = mEmu == nullptr ? nullptr : mEmu->GetCore();
    Registry *registry = core == nullptr ? nullptr : core->GetRegistry();
    const bool authEnabled = registry != nullptr &&
        registry->GetKeyValue<bool>("emu.auth.enabled").value_or(false);
    if (!authEnabled)
        return "1";

    if (std::optional<AuthUtil::SessionUser> user = mEmu->GetCurrentLaunchUser();
        user.has_value() && user->id == userId) {
        if (user->isGuest)
            return AuthUtil::EncodeGuestTicket(*user);
        if (user->isFederated)
            return AuthUtil::EncodeFederatedTicket(*user);
    }

    if (userId < 0) {
        AuthUtil::SessionUser guest;
        guest.id = userId;
        guest.isGuest = true;
        return AuthUtil::EncodeGuestTicket(guest);
    }

    EmuDbManager *databases = core->GetEmuDbManager();
    return AuthUtil::MintAuthTicket(
        databases == nullptr ? nullptr : databases->GetMasterDatabase(), userId, placeId);
}
