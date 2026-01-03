// === noobWarrior ===
// File: BaseAuth.h
// Started by: Hattozo
// Started on: 3/6/2025
// Description: Base class for a list of accounts that can authenticate with a specific service
#pragma once
#include <NoobWarrior/Config.h>

#include <string>
#include <vector>
#include <cstdint>

#include <nlohmann/json.hpp>
#include <keychain/keychain.h>
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

template<typename AccountClass>
class BaseAuth {
public:
    bool HasAccountExpired(AccountClass &acc) {
        if (acc.ExpireTimestamp > -1 && time(nullptr) > acc.ExpireTimestamp) return true; // Automatic fail if it is past the expiration date. No way that will work.
        
        nlohmann::json userInfo = GetJsonFromToken(acc.Token);
        if (!userInfo.empty() && userInfo.contains("id") && userInfo.contains("name"))
            return false;

        return true;
    }

    BaseAuth(Config *config) :
        ActiveAccount(nullptr),
        mConfig(config)
    {}

    virtual std::string GetName() = 0;

    virtual nlohmann::json AccStructToJson(AccountClass &acc) {
        nlohmann::json accJson {};
        accJson["id"] = acc.Id;
        accJson["name"] = acc.Name;
        accJson["token"] = acc.Token;
        accJson["expire_timestamp"] = acc.ExpireTimestamp;
        return accJson;
    }

    virtual AccountClass AccJsonToStruct(nlohmann::json &json) {
        AccountClass acc {};
        acc.Id = json["id"].get<int64_t>();
        acc.Name = json["name"].get<std::string>();
        acc.Token = json["token"].get<std::string>();
        acc.ExpireTimestamp = json["expire_timestamp"].get<long>();
        return acc;
    }

    AuthResponse ReadFromKeychain() {
        keychain::Error err;
        std::string jsonStr = keychain::getPassword("org.noobwarrior", GetName(), "accounts", err);
        if (err.type == keychain::ErrorType::NotFound)
            return AuthResponse::Success;
        if (err.type != keychain::ErrorType::NoError)
            return AuthResponse::KeychainFailed;
        try {
            nlohmann::json accountsJson = nlohmann::json::parse(jsonStr);
            for (auto &accJson : accountsJson) {
                AccountClass acc = AccJsonToStruct(accJson);
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

    AuthResponse WriteToKeychain() {
        if (ActiveAccount != nullptr)
            mConfig->SetKeyValue<std::string>(std::format("internet.{}.active_account", GetName()), ActiveAccount->Name);

        nlohmann::json accountsJson {};
        for (auto &acc : Accounts) {
            nlohmann::json accJson = AccStructToJson(acc);
            accountsJson.push_back(accJson);
        }

        keychain::Error err;
        keychain::setPassword("org.noobwarrior", GetName(), "accounts", accountsJson.dump(), err);
        if (err.type != keychain::ErrorType::NoError)
            return AuthResponse::KeychainFailed;
        return AuthResponse::Success;
    }

    bool IsLoggedIn() {
        return ActiveAccount != nullptr;
    }

    AuthResponse AddAccountFromToken(const std::string &token, AccountClass **acc = nullptr) {
        if (acc != nullptr) *acc = nullptr;

        AccountClass accStack {};
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

    void AddAccount(AccountClass&);

    void SetActiveAccount(AccountClass *acc) {
        ActiveAccount = acc;
    }

    AccountClass* GetActiveAccount() {
        return ActiveAccount;
    }
    
    std::vector<AccountClass>& GetAccounts() {
        return Accounts;
    }

    AuthResponse TryAuthAccount(std::string& name, std::string& pass) {
        Out("Auth", "Attempting to log into account {}", name);
        CURL *handle = curl_easy_init();
        if (handle == nullptr) return AuthResponse::Failed;
        curl_easy_setopt(handle, CURLOPT_URL, "http://example.com/");
        CURLcode ret = curl_easy_perform(handle);
        curl_easy_cleanup(handle);
        return AuthResponse::Failed;
    }

    AuthResponse TryMultiAuth(int code) {
        return AuthResponse::Failed;
    }
protected:
    virtual nlohmann::json GetJsonFromToken(const std::string &token) = 0;

    std::vector<AccountClass>    Accounts;
    AccountClass*                ActiveAccount;
    Config*                 mConfig;
};
}