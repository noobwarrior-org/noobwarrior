// === noobWarrior ===
// File: Auth.h
// Started by: Hattozo
// Started on: 3/6/2025
// Description:
#pragma once

#include <string>
#include <vector>
#include <cstdint>

#include <nlohmann/json_fwd.hpp>
#include <keychain/keychain.h>

#include "Config.h"

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
        std::string Token; // .ROBLOSECURITY token
        long ExpireTimestamp { -1 }; // Defaults to -1 if the expiration date is unknown.
    };

    class Auth {
    public:
        static bool             HasAccountExpired(Account&);

        Auth(Config *config);

        AuthResponse            ReadFromKeychain();
        AuthResponse            WriteToKeychain();

        bool                    IsLoggedIn();
        AuthResponse            AddAccountFromToken(const std::string &token, Account *acc = nullptr);
        void                    AddAccount(Account&);
        void                    SetActiveAccount(Account *acc);
        Account*                GetActiveAccount();
        std::vector<Account>    GetAccounts();

        AuthResponse            TryAuthAccount(std::string& name, std::string& pass);
        AuthResponse            TryMultiAuth(int code);
    private:
        static nlohmann::json   GetJsonFromToken(const std::string &token);

        std::vector<Account>    Accounts;
        Account*                ActiveAccount;
        Config*                 mConfig;
    };
}