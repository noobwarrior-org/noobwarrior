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

#include "FFlagJson/PCDesktopClientV2.json.inc.cpp"
#include "FFlagJson/AvatarRules.json.inc.cpp"
#include "FFlagJson/GamesSorts.json.inc.cpp"

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
    mClientSettingsV2DesktopHandler(PCDesktopClientV2_json),
    mNegotiateHandler(),
    mPlaceLauncherHandler(),
    mJoinScriptJsonHandler(),
    mGameJoinHandler(this),
    mAppBehaviorsHandler(R"({"results":[]})"),
    mAvatarRulesHandler(AvatarRules_json),
    mAvatarHandler(R"({"scales":{"height":1.0,"width":1.0,"head":1.0,"depth":1.0,"proportion":0.0,"bodyType":0.0},"playerAvatarType":"R6","bodyColors":{"headColorId":1001,"torsoColorId":1001,"rightArmColorId":1001,"leftArmColorId":1001,"rightLegColorId":1001,"leftLegColorId":1001},"assets":[],"defaultShirtApplied":true,"defaultPantsApplied":true,"emotes":[{"assetId":3576686446,"assetName":"Hello","position":1},{"assetId":3360686498,"assetName":"Stadium","position":2},{"assetId":3576823880,"assetName":"Point2","position":3},{"assetId":3576968026,"assetName":"Shrug","position":4}]})"),
    mAppLaunchInfoHandler(R"({"ageBracket":0,"countryCode":"US","isPremium":false,"id":1,"name":"ROBLOX","displayName":"ROBLOX"})"),
    mRolesHandler(R"({"roles":[]})"),
    mLocalesHandler(R"json({"signupAndLogin":{"id":1,"locale":"en_us","name":"English (United States)","nativeName":"English (United States)","language":{"id":41,"name":"English","nativeName":"English","languageCode":"en","isRightToLeft":false}},"generalExperience":{"id":1,"locale":"en_us","name":"English (United States)","nativeName":"English (United States)","language":{"id":41,"name":"English","nativeName":"English","languageCode":"en","isRightToLeft":false}},"ugc":{"id":1,"locale":"en_us","name":"English (United States)","nativeName":"English (United States)","language":{"id":41,"name":"English","nativeName":"English","languageCode":"en","isRightToLeft":false}},"showRobloxTranslations":false})json"),
    mGamesHandler(R"({"data":[{"id":1,"rootPlaceId":1818,"name":"noobWarrior Place","description":"","sourceName":"noobWarrior Place","sourceDescription":"","creator":{"id":1,"name":"Player","type":"User","isRNVAccount":false,"hasVerifiedBadge":false},"price":null,"allowedGearGenres":["All"],"allowedGearCategories":[],"isGenreEnforced":false,"copyingAllowed":false,"playing":1,"visits":1,"maxPlayers":50,"created":"2015-01-01T00:00:00Z","updated":"2015-01-01T00:00:00Z","studioAccessToApisAllowed":true,"createVipServersAllowed":false,"universeAvatarType":"MorphToR6","genre":"All","isAllGenre":true,"isFavoritedByUser":false,"favoritedCount":0}]})"),
    mUserChannelHandler(R"({"channelName":"LIVE","channelType":"Production","token":""})"),
    mPlaceDetailsHandler(R"([{"placeId":1818,"name":"noobWarrior Place","description":"","sourceName":"noobWarrior Place","sourceDescription":"","url":"","builder":"Player","builderId":1,"hasVerifiedBadge":false,"isPlayable":true,"reasonProhibited":"None","reasonProhibitedMessage":"","universeId":1,"universeRootPlaceId":1818,"price":0,"imageToken":""}])"),
    mOmniRecHandler(R"({"sorts":[{"sortId":"recommended","topic":"Recommended For You","topicId":1,"treatmentType":"Carousel","recommendationList":[{"contentType":"Game","contentId":1818,"contentStringId":"1818"}],"nextSortToken":"","contentTypeFiltersToExclude":[]}],"contentMetadata":{"Game":{"1818":{"universeId":1,"rootPlaceId":1818,"name":"Crossroads","playerCount":1,"totalUpVotes":1,"totalDownVotes":0,"creatorName":"Player","creatorId":1,"creatorType":"User","creatorHasVerifiedBadge":false,"isSponsored":false,"nativeAdData":"","isShowSponsoredLabel":false,"price":null,"analyticsIdentifier":null,"gameDescription":"noobWarrior team test","genre":"All"}}},"nextPageToken":""})"),
    mGamesSortsHandler(GamesSorts_json),
    mGamesListHandler(R"({"games":[{"creatorId":1,"creatorName":"Player","creatorType":"User","creatorHasVerifiedBadge":false,"totalUpVotes":1,"totalDownVotes":0,"universeId":1,"name":"Crossroads","placeId":1818,"playerCount":1,"imageToken":null,"isSponsored":false,"nativeAdData":"","isShowSponsoredLabel":false,"price":null,"analyticsIdentifier":null,"gameDescription":"noobWarrior team test","genre":"All"}],"suggestedKeyword":null,"correctedKeyword":null,"filteredKeyword":null,"hasMoreRows":false,"nextPageExclusiveStartId":null,"featuredSearchUniverseId":null,"emphasis":false,"cutOffIndex":null,"algorithm":null,"algorithmQueryType":null,"suggestionAlgorithm":null,"relatedGames":[],"esDebugInfo":null})"),
    mAvatarFetchHandler(R"({"resolvedAvatarType":"R6","equippedGearVersionIds":[],"backpackGearVersionIds":[],"assetAndAssetTypeIds":[{"assetId":855776103,"assetTypeId":11},{"assetId":855783877,"assetTypeId":12}],"animationAssetIds":{},"bodyColor3s":{"headColor3":"A3A2A5","torsoColor3":"4B974B","rightArmColor3":"A3A2A5","leftArmColor3":"A3A2A5","rightLegColor3":"6E99CA","leftLegColor3":"6E99CA"},"scales":{"height":1.0,"width":1.0,"head":1.0,"depth":1.0,"proportion":0.0,"bodyType":0.0},"emotes":[]})"),
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
    mOAuthUserinfoHandler(R"({"sub":"86121841","name":"Hattozo","nickname":"Hattozo","preferred_username":"Hattozo","created_at":1,"profile":"https://www.roblox.com/users/86121841/profile","picture":"http://localhost/headshots/default.png","age_bracket":"Age13OrOver","premium":false,"roles":[],"internal_user":false})"),
    mStudioLoginHandler(R"({"success":true,"user":{"UserId":86121841,"Username":"Hattozo","DisplayName":"Hattozo","AgeBracket":0,"Roles":[],"Email":{"value":"hattozo@noobwarrior.local","isVerified":true},"IsBanned":false,"isInternal":false}})"),
    mStudioOpenPlaceHandler(this),
    mAssetEnricher(mCore)
{
    mAssetEnricher.Start();
}

ServerEmulator::~ServerEmulator() {
    mAssetEnricher.Stop();
}

AssetEnricher* ServerEmulator::GetAssetEnricher() {
    return &mAssetEnricher;
}

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
    SetRequestHandler("/v2/settings/application/PCDesktopClient", &mClientSettingsV2DesktopHandler);
    SetRequestHandler("/v2/settings/application/PCStudioApp", &mClientSettingsV2Handler);

    SetRequestHandler("/Login/Negotiate.ashx", &mNegotiateHandler);
    SetRequestHandler("/login/negotiate.ashx", &mNegotiateHandler);

    SetRequestHandler("/Game/PlaceLauncher.ashx", &mPlaceLauncherHandler);
    SetRequestHandler("/game/placelauncher.ashx", &mPlaceLauncherHandler);

    SetRequestHandler("/Game/Join.ashx", &mJoinScriptJsonHandler);
    SetRequestHandler("/game/join.ashx", &mJoinScriptJsonHandler);

    SetRequestHandler("/gamejoin/v1/join-game", &mGameJoinHandler);
    SetRequestHandler("/v1/join-game", &mGameJoinHandler);
    SetRequestHandler("/universal-app-configuration/v1/behaviors/app-policy/content", &mAppBehaviorsHandler);
    SetRequestHandler("/universal-app-configuration/v1/behaviors/app-patch/content", &mAppBehaviorsHandler);
    SetRequestHandler("/v1/avatar-rules", &mAvatarRulesHandler);
    SetRequestHandler("/v1/avatar", &mAvatarHandler);
    SetRequestHandler("/v1/users/authenticated/app-launch-info", &mAppLaunchInfoHandler);
    SetRequestHandler("/v1/users/authenticated/roles", &mRolesHandler);
    SetRequestHandler("/v1/locales/user-localization-locus-supported-locales", &mLocalesHandler);
    SetRequestHandler("/v1/games", &mGamesHandler);
    SetRequestHandler("/v2/user-channel", &mUserChannelHandler);
    SetRequestHandler("/v1/games/multiget-place-details", &mPlaceDetailsHandler);
    SetRequestHandler("/v1/games/multiget-playability-status", &mPlaceDetailsHandler);
    SetRequestHandler("/discovery-api/omni-recommendation", &mOmniRecHandler);
    SetRequestHandler("/v1/games/sorts", &mGamesSortsHandler);
    SetRequestHandler("/v1/games/list", &mGamesListHandler);
    SetRequestHandler("/v2/avatar/avatar-fetch", &mAvatarFetchHandler);
    SetRequestHandler("/v1/avatar-fetch", &mAvatarFetchHandler);

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
    SetRequestHandler("/studio-login/v1/login", &mStudioLoginHandler);

    SetRequestHandler("/studio-open-place/v1/openplace", &mStudioOpenPlaceHandler);
}

int ServerEmulator::Start(uint16_t port) {
    mAssetHandler.ResumeProxy();
    return HttpServer::Start(port);
}

int ServerEmulator::Stop() {
    // Must pause the proxy pool before HttpServer::Stop frees the evhttp, so no in-flight proxy
    // fetch replies to a freed connection.
    mAssetHandler.PauseProxy();
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
