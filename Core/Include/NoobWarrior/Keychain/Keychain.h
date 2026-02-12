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
// File: Keychain.h
// Started by: Hattozo
// Started on: 3/6/2025
// Description: A list of accounts that can authenticate with a specific service
// These keys are securely stored using the appropriate API's for your operating system
#pragma once
#include <NoobWarrior/Config.h>
#include <NoobWarrior/Keychain/OsKeychain.h>

#include <string>
#include <vector>
#include <cstdint>

#include <nlohmann/json.hpp>
#include <curl/curl.h>

namespace NoobWarrior {
enum class AuthResponse {
    Success, // You don't need to do anything else, you are logged in.
    Failed, // Could not log in. Maybe a bad username/password?
    TwoFactorAuthentication, // 2FA required, use TryMultiAuth() to continue
    EmailVerification, // Email code required, use TryMultiAuth() to continue
    // Hardware pass keys are currently not planned to be supported because I have no idea how they work.
    InvalidToken,
    InvalidJson,
    KeychainFailed

};

struct Account {
    int64_t Id;
    std::string Name;
    std::string Token;
    std::string Url;
    long ExpireTimestamp { -1 }; // Defaults to -1 if the expiration date is unknown.
};

class Keychain {
public:
    static nlohmann::json AccStructToJson(Account &acc);
    static Account AccJsonToStruct(nlohmann::json &json);

    Keychain(Config* config);

    bool HasAccountExpired(Account &acc);

    virtual std::string GetName() = 0;

    AuthResponse ReadFromKeychain();
    AuthResponse WriteToKeychain();

    bool IsLoggedIn();

    AuthResponse AddAccountFromToken(const std::string &token, Account **acc = nullptr);

    void AddAccount(Account&);

    void SetActiveAccount(Account *acc);

    Account* GetActiveAccount();
    
    std::vector<Account>& GetAccounts();

    AuthResponse TryAuthAccount(std::string& name, std::string& pass);

    AuthResponse TryMultiAuth(int code);
protected:
    virtual nlohmann::json GetJsonFromToken(const std::string &token) = 0;

    std::vector<Account>    Accounts;
    Account*                ActiveAccount;
    Config*                 mConfig;
};
}