// === noobWarrior ===
// File: Auth.h
// Started by: Hattozo
// Started on: 3/6/2025
// Description:
#pragma once

#include <string>

namespace NoobWarrior::Auth {
    enum class AuthResponse {
        Success, // You don't need to do anything else, you are logged in.
        Failed, // Could not log in. Maybe a bad username/password?
        TwoFactorAuthentication, // 2FA required, use TryMultiAuth() to continue
        EmailVerification // Email code required, use TryMultiAuth() to continue
        // Hardware pass keys are currently not planned to be supported because I have no idea how they work.
    };
    typedef struct {
        char* Name;
        char* Token; // .ROBLOSECURITY token
        long ExpireTimestamp;
    } Account;
    extern std::vector<Account> gAccounts;
    extern Account* gActiveAccount;

    int SetFile(std::string_view dir);
    int Deserialize();
    bool IsLoggedIn();
    bool IsTokenValid(Account&);
    void AddAccount(Account&);
    int SetActiveAccount(Account&);
    Account* GetActiveAccount();

    AuthResponse TryAuthAccount(std::string& name, std::string& pass);
    AuthResponse TryMultiAuth(int code);
}