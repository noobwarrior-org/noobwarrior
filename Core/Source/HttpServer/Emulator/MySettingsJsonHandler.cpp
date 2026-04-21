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
#include <NoobWarrior/Log.h>

static constexpr const char* JSON = R"({"ChangeUsernameEnabled":true,"IsAdmin":false,"UserId":86121841,"Name":"Hattozo","DisplayName":"Hattozo","IsEmailOnFile":true,"IsEmailVerified":true,"IsPhoneFeatureEnabled":true,"RobuxRemainingForUsernameChange":573,"PreviousUserNames":"MohammadHasan2","UseSuperSafePrivacyMode":false,"IsAppChatSettingEnabled":true,"IsGameChatSettingEnabled":true,"IsParentalSpendControlsEnabled":true,"IsSetPasswordNotificationEnabled":false,"ChangePasswordRequiresTwoStepVerification":false,"ChangeEmailRequiresTwoStepVerification":false,"UserEmail":"t*********@gmail.com","UserEmailMasked":true,"UserEmailVerified":true,"CanHideInventory":true,"CanTrade":false,"MissingParentEmail":false,"IsUpdateEmailSectionShown":true,"IsUnder13UpdateEmailMessageSectionShown":false,"IsUserConnectedToFacebook":false,"IsTwoStepToggleEnabled":false,"AgeBracket":0,"UserAbove13":true,"ClientIpAddress":"108.17.63.2","AccountAgeInDays":3975,"IsPremium":false,"HasRobloxSubscription":false,"IsBcRenewalMembership":false,"PremiumFeatureId":null,"HasCurrencyOperationError":false,"CurrencyOperationErrorMessage":null,"Tab":null,"ChangePassword":false,"IsAccountPinEnabled":false,"IsAccountRestrictionsFeatureEnabled":true,"IsAccountSettingsSocialNetworksV2Enabled":false,"IsUiBootstrapModalV2Enabled":true,"IsDateTimeI18nPickerEnabled":true,"InApp":false,"MyAccountSecurityModel":{"IsEmailSet":true,"IsEmailVerified":true,"IsTwoStepEnabled":true,"ShowSignOutFromAllSessions":true,"TwoStepVerificationViewModel":{"UserId":86121841,"IsEnabled":true,"CodeLength":0,"ValidCodeCharacters":null}},"ApiProxyDomain":"https://api.roblox.com","AccountSettingsApiDomain":"https://accountsettings.roblox.com","AuthDomain":"https://auth.roblox.com","IsDisconnectFacebookEnabled":true,"IsDisconnectXboxEnabled":true,"NotificationSettingsDomain":"https://notifications.roblox.com","AllowedNotificationSourceTypes":["Test","FriendRequestReceived","FriendRequestAccepted","PartyInviteReceived","PartyMemberJoined","ChatNewMessage","PrivateMessageReceived","UserAddedToPrivateServerWhiteList","ConversationUniverseChanged","TeamCreateInvite","GameUpdate","DeveloperMetricsAvailable","GroupJoinRequestAccepted","Sendr","ExperienceInvitation"],"AllowedReceiverDestinationTypes":["NotificationStream"],"BlacklistedNotificationSourceTypesForMobilePush":[],"MinimumChromeVersionForPushNotifications":50,"PushNotificationsEnabledOnFirefox":false,"LocaleApiDomain":"https://locale.roblox.com","HasValidPasswordSet":true,"IsFastTrackAccessible":false,"IsAgeDownEnabled":true,"IsDisplayNamesEnabled":true,"IsBirthdateLocked":false})";

using namespace NoobWarrior;

MySettingsJsonHandler::MySettingsJsonHandler() {

}

void MySettingsJsonHandler::OnRequest(evhttp_request *req, void *userdata) {
    Out("MySettingsJsonHandler", "Sent!");
    evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type", "application/json");
    evbuffer* reply = evbuffer_new();
    evbuffer_add_printf(reply, "%s", JSON);
    evhttp_send_reply(req, 200, nullptr, reply);
    evbuffer_free(reply);
}
