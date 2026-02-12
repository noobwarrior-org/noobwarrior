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

using namespace NoobWarrior;

nlohmann::json Keychain::AccStructToJson(Account &acc) {
    nlohmann::json accJson {};
    accJson["id"] = acc.Id;
    accJson["name"] = acc.Name;
    accJson["token"] = acc.Token;
    accJson["expire_timestamp"] = acc.ExpireTimestamp;
    return accJson;
}

Account Keychain::AccJsonToStruct(nlohmann::json &json) {
    Account acc {};
    acc.Id = json["id"].get<int64_t>();
    acc.Name = json["name"].get<std::string>();
    acc.Token = json["token"].get<std::string>();
    acc.ExpireTimestamp = json["expire_timestamp"].get<long>();
    return acc;
}

Keychain::Keychain(Config* config)  :
    ActiveAccount(nullptr),
    mConfig(config)
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
            Account acc = AccJsonToStruct(accJson);
            Accounts.push_back(acc);

            std::optional<std::string> active_account_thing = mConfig->GetKeyValue<std::string>(std::format("internet.{}.active_account", GetName()));
            if (active_account_thing.has_value() && active_account_thing.value().compare(acc.Name) == 0)
                ActiveAccount = &Accounts.back();
        }
    } catch (nlohmann::json::exception) {
        return AuthResponse::InvalidJson;
    }
    return AuthResponse::Success;
}

AuthResponse Keychain::WriteToKeychain() {
    if (ActiveAccount != nullptr)
        mConfig->SetKeyValue<std::string>(std::format("internet.{}.active_account", GetName()), ActiveAccount->Name);

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
    Out("Auth", "{}", userInfo.dump());
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
    Out("Auth", "Attempting to log into account {}", name);
    CURL *handle = curl_easy_init();
    if (handle == nullptr) return AuthResponse::Failed;
    curl_easy_setopt(handle, CURLOPT_URL, "http://example.com/");
    CURLcode ret = curl_easy_perform(handle);
    curl_easy_cleanup(handle);
    return AuthResponse::Failed;
}

AuthResponse Keychain::TryMultiAuth(int code) {
    return AuthResponse::Failed;
}