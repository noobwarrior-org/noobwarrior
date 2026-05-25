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
// File: ServerEmulator.cpp
// Started by: Hattozo
// Started on: 9/2/2025
// Description:
#include <NoobWarrior/HttpServer/Emulator/ServerEmulator.h>
#include <NoobWarrior/HttpServer/Emulator/ClientSettingsHandler.h>
#include <NoobWarrior/NoobWarrior.h>

#include "NoobWarrior/HttpServer/Emulator/RequestAuthHandler.h"

#include <algorithm>
#include <ctime>

using namespace NoobWarrior;
using json = nlohmann::json;

ServerEmulator::ServerEmulator(Core *core) : HttpServer(core, "ServerEmulator"),
    mProcessPingHandler(this),
    mCreateAccountHandler(this),
    mLoginHandler(this),
    mLogoutHandler(this),
    mRunningGameServersHandler(this),
    mAssetHandler(this, mCore->GetEmuDbManager()),
    mAssetThumbnailJsonHandler(this, mCore->GetEmuDbManager()),
    mClientSettingsHandler(this),
    mClientSettingsV2Handler(this),
    mNegotiateHandler(),
    mPlaceLauncherHandler(),
    mJoinScriptJsonHandler(),
    mUniversalAppConfigStudioHandler(),
    mMySettingsJsonHandler(),
    mAuthenticatedUserHandler(),
    mCurrentUserHandler(),
    mRequestAuthHandler(),
    mStudioEditHandler(),
    mGameIconHandler(this, mCore->GetEmuDbManager()),
    mOAuthDiscoveryHandler(R"({"issuer":"https://apis.roblox.com","authorization_endpoint":"https://apis.roblox.com/oauth/v1/authorize","device_authorization_endpoint":"https://apis.roblox.com/oauth/v1/device/authorize","token_endpoint":"https://apis.roblox.com/oauth/v1/token","introspection_endpoint":"https://apis.roblox.com/oauth/v1/token/introspect","revocation_endpoint":"https://apis.roblox.com/oauth/v1/token/revoke","userinfo_endpoint":"https://apis.roblox.com/oauth/v1/userinfo","jwks_uri":"https://apis.roblox.com/oauth/v1/certs","registration_endpoint":"https://create.roblox.com/settings/api","service_documentation":"https://create.roblox.com/docs/cloud/auth/oauth2-overview","scopes_supported":["openid","profile","email","phone","address","offline_access"],"response_types_supported":["code"],"response_modes_supported":["query"],"token_endpoint_auth_methods_supported":["none","client_secret_post","client_secret_basic"],"grant_types_supported":["authorization_code","refresh_token","urn:ietf:params:oauth:grant-type:device_code"],"code_challenge_methods_supported":["S256"],"subject_types_supported":["public"],"id_token_signing_alg_values_supported":["ES256"],"claims_supported":["sub","name","nickname","preferred_username","created_at","profile","picture"],"claims_parameter_supported":false,"request_parameter_supported":false,"request_uri_parameter_supported":false})"),
    mOAuthAuthorizeHandler(),
    mOAuthTokenHandler(R"({"access_token":"eyJhbGciOiJFUzI1NiIsImtpZCI6Im5vb2J3YXJyaW9yIiwidHlwIjoiSldUIn0.eyJzdWIiOiI4NjEyMTg0MSIsInNjb3BlIjoib3BlbmlkIHByb2ZpbGUiLCJpc3MiOiJodHRwczovL2FwaXMucm9ibG94LmNvbS9vYXV0aC8iLCJhdWQiOiJub29id2FycmlvciJ9.","refresh_token":"noobwarrior_refresh","token_type":"Bearer","expires_in":2592000,"id_token":"eyJhbGciOiJFUzI1NiIsImtpZCI6Im5vb2J3YXJyaW9yIiwidHlwIjoiSldUIn0.eyJzdWIiOiI4NjEyMTg0MSIsIm5hbWUiOiJIYXR0b3pvIiwibmlja25hbWUiOiJIYXR0b3pvIiwicHJlZmVycmVkX3VzZXJuYW1lIjoiSGF0dG96byIsImNyZWF0ZWRfYXQiOjEsInByb2ZpbGUiOiJodHRwczovL3d3dy5yb2Jsb3guY29tL3VzZXJzLzg2MTIxODQxL3Byb2ZpbGUiLCJpc3MiOiJodHRwczovL2FwaXMucm9ibG94LmNvbS9vYXV0aC8iLCJhdWQiOiJub29id2FycmlvciJ9.","scope":"openid profile"})"),
    mOAuthUserinfoHandler(R"({"sub":"86121841","name":"Hattozo","nickname":"Hattozo","preferred_username":"Hattozo","created_at":1,"profile":"https://www.roblox.com/users/86121841/profile","picture":"http://localhost/headshots/default.png","age_bracket":"Age13OrOver","premium":false,"roles":[],"internal_user":false})")
{
}

ServerEmulator::~ServerEmulator() {}

void ServerEmulator::SetupHandlers() {
    HttpServer::SetupHandlers();

    SetRequestHandler("/v1/process-ping", &mProcessPingHandler);
    SetRequestHandler("/v1/create-account", &mCreateAccountHandler);
    SetRequestHandler("/v1/login", &mLoginHandler);
    SetRequestHandler("/v1/logout", &mLogoutHandler);
    SetRequestHandler("/v1/running-game-servers", &mRunningGameServersHandler);

    SetRequestHandler("/Asset", &mAssetHandler);
    SetRequestHandler("/asset", &mAssetHandler);
    SetRequestHandler("/asset/", &mAssetHandler);
    SetRequestHandler("/v1/asset", &mAssetHandler);
    SetRequestHandler("/v1/asset/", &mAssetHandler);

    SetRequestHandler("/asset-thumbnail/json", &mAssetThumbnailJsonHandler);

    SetRequestHandler("/v1/settings/application", &mClientSettingsHandler);
    SetRequestHandler("/v2/settings/application/PCStudioApp", &mClientSettingsV2Handler);

    SetRequestHandler("/Login/Negotiate.ashx", &mNegotiateHandler);
    SetRequestHandler("/login/negotiate.ashx", &mNegotiateHandler);

    SetRequestHandler("/Game/PlaceLauncher.ashx", &mPlaceLauncherHandler);
    SetRequestHandler("/game/placelauncher.ashx", &mPlaceLauncherHandler);

    SetRequestHandler("/Game/Join.ashx", &mJoinScriptJsonHandler);
    SetRequestHandler("/game/join.ashx", &mJoinScriptJsonHandler);

    SetRequestHandler("/universal-app-configuration/v1/behaviors/studio/content", &mUniversalAppConfigStudioHandler);

    SetRequestHandler("/my/settings/json", &mMySettingsJsonHandler);

    SetRequestHandler("/v1/users/authenticated", &mAuthenticatedUserHandler);

    SetRequestHandler("/game/GetCurrentUser.ashx", &mCurrentUserHandler);

    SetRequestHandler("/Login/RequestAuth.ashx", &mRequestAuthHandler);
    SetRequestHandler("/login/RequestAuth.ashx", &mRequestAuthHandler);

    SetRequestHandler("/Game/Edit.ashx", &mStudioEditHandler);
    SetRequestHandler("/game/edit.ashx", &mStudioEditHandler);

    SetRequestHandler("/Thumbs/GameIcon.ashx", &mGameIconHandler);
    SetRequestHandler("/Thumbs/gameicon.ashx", &mGameIconHandler);

    SetRequestHandler("/oauth/.well-known/openid-configuration", &mOAuthDiscoveryHandler);
    SetRequestHandler("/oauth/v1/authorize", &mOAuthAuthorizeHandler);
    SetRequestHandler("/oauth/v1/token", &mOAuthTokenHandler);
    SetRequestHandler("/oauth/v1/userinfo", &mOAuthUserinfoHandler);
}

int ServerEmulator::Start(uint16_t port) {
    return HttpServer::Start(port);
}

int ServerEmulator::Stop() {
    return HttpServer::Stop();
}

void ServerEmulator::SetMode(Mode mode) {
}

ServerEmulator::Mode ServerEmulator::GetMode() {
}

void ServerEmulator::SweepStaleInstancesLocked() {
    time_t now = std::time(nullptr);
    auto it = std::remove_if(mInstances.begin(), mInstances.end(),
        [&](const RunningInstance &inst) {
            bool stale = inst.LastSeen > 0 && (now - inst.LastSeen) > kStaleInstanceThresholdSecs;
            if (stale)
                Out(mLogName, "Reaping stale instance pid={} side={} (last seen {}s ago)",
                    inst.Pid, EngineSideAsString(inst.Side), static_cast<long long>(now - inst.LastSeen));
            return stale;
        });
    mInstances.erase(it, mInstances.end());
}

void ServerEmulator::RegisterInstance(const RunningInstance &instance) {
    std::lock_guard lock(mInstancesMutex);
    SweepStaleInstancesLocked();
    time_t now = std::time(nullptr);
    for (auto &existing : mInstances) {
        if (existing.Pid == instance.Pid) {
            existing = instance;
            existing.LastSeen = now;
            return;
        }
    }
    RunningInstance copy = instance;
    if (copy.FirstSeen == 0) copy.FirstSeen = now;
    copy.LastSeen = now;
    mInstances.push_back(copy);
    Out(mLogName, "Registered instance pid={} side={} version={}",
        copy.Pid, EngineSideAsString(copy.Side), copy.Version);
}

void ServerEmulator::UnregisterInstance(int pid) {
    std::lock_guard lock(mInstancesMutex);
    SweepStaleInstancesLocked();
    auto it = std::find_if(mInstances.begin(), mInstances.end(),
        [pid](const RunningInstance &i) { return i.Pid == pid; });
    if (it != mInstances.end()) {
        Out(mLogName, "Unregistered instance pid={} side={}", it->Pid, EngineSideAsString(it->Side));
        mInstances.erase(it);
    }
}

bool ServerEmulator::TouchInstance(int pid) {
    std::lock_guard lock(mInstancesMutex);
    SweepStaleInstancesLocked();
    auto it = std::find_if(mInstances.begin(), mInstances.end(),
        [pid](const RunningInstance &i) { return i.Pid == pid; });
    if (it == mInstances.end())
        return false;
    it->LastSeen = std::time(nullptr);
    return true;
}

std::vector<RunningInstance> ServerEmulator::GetRunningInstances() const {
    std::lock_guard lock(mInstancesMutex);
    const_cast<ServerEmulator*>(this)->SweepStaleInstancesLocked();
    return mInstances;
}

std::vector<RunningInstance> ServerEmulator::GetRunningGameServers() const {
    std::lock_guard lock(mInstancesMutex);
    const_cast<ServerEmulator*>(this)->SweepStaleInstancesLocked();
    std::vector<RunningInstance> servers;
    for (const auto &inst : mInstances) {
        if (inst.Side == EngineSide::Server)
            servers.push_back(inst);
    }
    return servers;
}

void ServerEmulator::SetCurrentEngine(const Engine &engine) {
    mCurrentEngine = engine;
}

std::optional<Engine> ServerEmulator::GetCurrentEngine() {
    return mCurrentEngine;
}
