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
// File: MySettingsJsonHandler.cpp
// Started by: Hattozo
// Started on: 4/21/2026
// Description:
#include <NoobWarrior/HttpServer/Emulator/MySettingsJsonHandler.h>
#include <NoobWarrior/HttpServer/Emulator/ServerEmulator.h>
#include <NoobWarrior/NoobWarrior.h>
#include <NoobWarrior/Log.h>
#include <nlohmann/json.hpp>

using namespace NoobWarrior;

MySettingsJsonHandler::MySettingsJsonHandler(ServerEmulator* emu) : mEmu(emu) {

}

void MySettingsJsonHandler::OnRequest(evhttp_request *req, void *userdata) {
    const char* uri = evhttp_request_get_uri(req);
    evhttp_connection* conn = evhttp_request_get_connection(req);

    const char* peer_address = "";
    uint16_t peer_port {};

    if (conn != NULL)
        evhttp_connection_get_peer(conn, &peer_address, &peer_port);
    Out("MySettingsJsonHandler", "{}:{} requested {}", peer_address, peer_port, uri);

    auto* registry = mEmu->GetCore()->GetRegistry();
    auto id = registry->GetKeyValue<int64_t>("user.id").value_or(1);
    auto name = registry->GetKeyValue<std::string>("user.name").value_or("Player");
    auto displayName = registry->GetKeyValue<std::string>("user.display_name").value_or("Player");

    nlohmann::json j = {
        {"ChangeUsernameEnabled", true},
        {"IsAdmin", false},
        {"UserId", id},
        {"Name", name},
        {"DisplayName", displayName},
        {"IsEmailOnFile", true},
        {"IsEmailVerified", true},
        {"IsPhoneFeatureEnabled", true},
        {"RobuxRemainingForUsernameChange", 573},
        {"PreviousUserNames", ""},
        {"UseSuperSafePrivacyMode", false},
        {"IsAppChatSettingEnabled", true},
        {"IsGameChatSettingEnabled", true},
        {"IsParentalSpendControlsEnabled", true},
        {"IsSetPasswordNotificationEnabled", false},
        {"ChangePasswordRequiresTwoStepVerification", false},
        {"ChangeEmailRequiresTwoStepVerification", false},
        {"UserEmail", "contact@noobwarrior.org"},
        {"UserEmailMasked", true},
        {"UserEmailVerified", true},
        {"CanHideInventory", true},
        {"CanTrade", false},
        {"MissingParentEmail", false},
        {"IsUpdateEmailSectionShown", true},
        {"IsUnder13UpdateEmailMessageSectionShown", false},
        {"IsUserConnectedToFacebook", false},
        {"IsTwoStepToggleEnabled", false},
        {"AgeBracket", 0},
        {"UserAbove13", true},
        {"ClientIpAddress", "localhost"},
        {"AccountAgeInDays", 3975},
        {"IsPremium", false},
        {"HasRobloxSubscription", false},
        {"IsBcRenewalMembership", false},
        {"PremiumFeatureId", nullptr},
        {"HasCurrencyOperationError", false},
        {"CurrencyOperationErrorMessage", nullptr},
        {"Tab", nullptr},
        {"ChangePassword", false},
        {"IsAccountPinEnabled", false},
        {"IsAccountRestrictionsFeatureEnabled", true},
        {"IsAccountSettingsSocialNetworksV2Enabled", false},
        {"IsUiBootstrapModalV2Enabled", true},
        {"IsDateTimeI18nPickerEnabled", true},
        {"InApp", false},
        {"MyAccountSecurityModel", {
            {"IsEmailSet", true},
            {"IsEmailVerified", true},
            {"IsTwoStepEnabled", true},
            {"ShowSignOutFromAllSessions", true},
            {"TwoStepVerificationViewModel", {
                {"UserId", id},
                {"IsEnabled", true},
                {"CodeLength", 0},
                {"ValidCodeCharacters", nullptr}
            }}
        }},
        {"ApiProxyDomain", "https://api.roblox.com"},
        {"AccountSettingsApiDomain", "https://accountsettings.roblox.com"},
        {"AuthDomain", "https://auth.roblox.com"},
        {"IsDisconnectFacebookEnabled", true},
        {"IsDisconnectXboxEnabled", true},
        {"NotificationSettingsDomain", "https://notifications.roblox.com"},
        {"AllowedNotificationSourceTypes", {
            "Test", "FriendRequestReceived", "FriendRequestAccepted", "PartyInviteReceived",
            "PartyMemberJoined", "ChatNewMessage", "PrivateMessageReceived",
            "UserAddedToPrivateServerWhiteList", "ConversationUniverseChanged", "TeamCreateInvite",
            "GameUpdate", "DeveloperMetricsAvailable", "GroupJoinRequestAccepted", "Sendr",
            "ExperienceInvitation"
        }},
        {"AllowedReceiverDestinationTypes", {"NotificationStream"}},
        {"BlacklistedNotificationSourceTypesForMobilePush", nlohmann::json::array()},
        {"MinimumChromeVersionForPushNotifications", 50},
        {"PushNotificationsEnabledOnFirefox", false},
        {"LocaleApiDomain", "https://locale.roblox.com"},
        {"HasValidPasswordSet", true},
        {"IsFastTrackAccessible", false},
        {"IsAgeDownEnabled", true},
        {"IsDisplayNamesEnabled", true},
        {"IsBirthdateLocked", false}
    };

    const std::string body = j.dump();
    evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type", "application/json");
    evbuffer* reply = evbuffer_new();
    evbuffer_add_printf(reply, "%s", body.c_str());
    evhttp_send_reply(req, 200, nullptr, reply);
    evbuffer_free(reply);
}
