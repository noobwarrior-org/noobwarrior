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
// File: Keychain.cpp
// Started by: Hattozo
// Started on: 2/11/2026
// Description: A list of accounts that can authenticate with a specific service
// These keys are securely stored using the appropriate API's for your operating system
#include <NoobWarrior/Keychain/Keychain.h>

#include <cpr/cpr.h>

using namespace NoobWarrior;

nlohmann::json Keychain::AccStructToJson(Account &acc) {
    nlohmann::json accJson {};
    accJson["id"] = acc.Id;
    accJson["name"] = acc.Name;
    accJson["display_name"] = acc.DisplayName;
    accJson["token"] = acc.Token;
    accJson["url"] = acc.Url;
    accJson["expire_timestamp"] = acc.ExpireTimestamp;
    return accJson;
}

Account Keychain::AccJsonToStruct(nlohmann::json &json) {
    // .value() everywhere so one malformed entry (or an older record missing the newer url/display_name
    // fields) doesn't throw and lose every account.
    Account acc {};
    acc.Id = json.value("id", (int64_t)0);
    acc.Name = json.value("name", std::string());
    acc.DisplayName = json.value("display_name", std::string());
    acc.Token = json.value("token", std::string());
    acc.Url = json.value("url", std::string());
    acc.ExpireTimestamp = json.value("expire_timestamp", (long)-1);
    return acc;
}

Keychain::Keychain(Registry* registry)  :
    ActiveAccount(nullptr),
    mRegistry(registry)
{}

bool Keychain::HasAccountExpired(Account &acc) {
    if (acc.ExpireTimestamp > -1 && time(nullptr) > acc.ExpireTimestamp) return true; // Automatic fail if it is past the expiration date. No way that will work.
    
    nlohmann::json userInfo = GetJsonFromToken(acc.Token);
    if (!userInfo.empty() && userInfo.contains("id") && userInfo.contains("name"))
        return false;

    return true;
}

AuthResponse Keychain::ReadFromKeychain() {
    OsKeychain::Error err;
    std::string jsonStr = OsKeychain::GetPassword("org.noobwarrior", GetName(), "accounts", err);
    if (err.Type == OsKeychain::ErrorType::NotFound)
        return AuthResponse::Success;
    if (err.Type != OsKeychain::ErrorType::NoError)
        return AuthResponse::KeychainFailed;
    try {
        nlohmann::json accountsJson = nlohmann::json::parse(jsonStr);
        for (auto &accJson : accountsJson) {
            Accounts.push_back(AccJsonToStruct(accJson));
        }
    } catch (nlohmann::json::exception) {
        return AuthResponse::InvalidJson;
    }

    // Resolve the active account after all push_backs are done so the vector
    // is no longer growing and every &Accounts[i] is stable.
    auto activeName = mRegistry->GetKeyValue<std::string>(std::format("internet.{}.active_account", GetName()));
    if (activeName.has_value()) {
        for (auto &acc : Accounts) {
            if (acc.Name == activeName.value()) {
                ActiveAccount = &acc;
                break;
            }
        }
    }
    return AuthResponse::Success;
}

AuthResponse Keychain::WriteToKeychain() {
    // Persist which account is active (empty when signed out, so a removed/cleared active doesn't get
    // silently re-resolved on the next load).
    mRegistry->SetKeyValue<std::string>(std::format("internet.{}.active_account", GetName()),
                                        ActiveAccount != nullptr ? ActiveAccount->Name : std::string());

    nlohmann::json accountsJson {};
    for (auto &acc : Accounts) {
        nlohmann::json accJson = AccStructToJson(acc);
        accountsJson.push_back(accJson);
    }

    OsKeychain::Error err;
    OsKeychain::SetPassword("org.noobwarrior", GetName(), "accounts", accountsJson.dump(), err);
    if (err.Type != OsKeychain::ErrorType::NoError)
        return AuthResponse::KeychainFailed;
    return AuthResponse::Success;
}

bool Keychain::IsLoggedIn() {
    return ActiveAccount != nullptr;
}

AuthResponse Keychain::AddAccountFromToken(const std::string &token, Account **acc) {
    if (acc != nullptr) *acc = nullptr;

    Account accStack {};
    accStack.Token = token;

    nlohmann::json userInfo = GetJsonFromToken(token);
    Out("Keychain", "{}", userInfo.dump());
    if (userInfo.empty() || userInfo.contains("errors") || !userInfo.contains("id") || !userInfo.contains("name"))
        return AuthResponse::Failed;

    accStack.Id = userInfo["id"].get<long>();
    accStack.Name = userInfo["name"].get<std::string>();

    Accounts.push_back(accStack);
    *acc = &Accounts.back();
    return AuthResponse::Success;
}

void Keychain::AddAccount(Account &acc) {
    Accounts.push_back(acc);
}

Account* Keychain::AddOrUpdateAccount(const Account &acc) {
    // Overwrite in place if we already have this account (re-login refreshing the token) — the pointer,
    // and any ActiveAccount pointing at it, stay valid.
    for (Account &existing : Accounts) {
        if (existing.Name == acc.Name) {
            existing = acc;
            return &existing;
        }
    }
    // New account: push_back may reallocate, so re-resolve ActiveAccount by name afterwards.
    std::string activeName = ActiveAccount != nullptr ? ActiveAccount->Name : std::string();
    Accounts.push_back(acc);
    if (!activeName.empty()) {
        ActiveAccount = nullptr;
        for (Account &a : Accounts) {
            if (a.Name == activeName) {
                ActiveAccount = &a;
                break;
            }
        }
    }
    return &Accounts.back();
}

void Keychain::RemoveAccount(int index) {
    // Determine where the active account lives before the erase shifts indices.
    int activeIdx = -1;
    for (int i = 0; i < static_cast<int>(Accounts.size()); i++) {
        if (&Accounts[i] == ActiveAccount) {
            activeIdx = i;
            break;
        }
    }

    Accounts.erase(Accounts.begin() + index);

    if (activeIdx == index)
        ActiveAccount = nullptr;
    else if (activeIdx > index)
        ActiveAccount = &Accounts[activeIdx - 1];
    // activeIdx < index: pointer unaffected
}

void Keychain::SetActiveAccount(Account *acc) {
    ActiveAccount = acc;
}

Account* Keychain::GetActiveAccount() {
    return ActiveAccount;
}
    
std::vector<Account>& Keychain::GetAccounts() {
    return Accounts;
}

AuthResponse Keychain::TryAuthAccount(std::string& name, std::string& pass) {
    Out("Keychain", "Attempting to log into account {}", name);
    cpr::Get(cpr::Url{"http://example.com/"});
    return AuthResponse::Failed;
}

AuthResponse Keychain::TryMultiAuth(int code) {
    return AuthResponse::Failed;
}