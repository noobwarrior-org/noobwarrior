// === noobWarrior ===
// File: Auth.cpp
// Started by: Hattozo
// Started on: 3/6/2025
// Description: Manages authentication of Roblox accounts for use with the noobWarrior library
#include <NoobWarrior/Auth.h>
#include <NoobWarrior/Log.h>

#include <curl/curl.h>
#include <curl/easy.h>
#include <nlohmann/json.hpp>
#include <keychain/keychain.h>

#include <format>

using namespace NoobWarrior;
using namespace nlohmann;

static size_t CurlWriteToBuf(void *contents, size_t size, size_t nmemb, std::string *buffer) {
    size_t totalSize = size * nmemb;
    buffer->insert(buffer->end(), (char*)contents, (char*)contents + totalSize);
    return totalSize;
}

bool Auth::HasAccountExpired(Account &account) {
    if (account.ExpireTimestamp > -1 && time(nullptr) > account.ExpireTimestamp) return true; // Automatic fail if it is past the expiration date. No way that will work.
    
    json userInfo = Auth::GetJsonFromToken(account.Token);
    if (!userInfo.empty() && userInfo.contains("id") && userInfo.contains("name"))
        return false;

    return true;
}

json Auth::GetJsonFromToken(const std::string &token) {
    std::string jsonStr;

    CURL *handle = curl_easy_init();
    std::string cock = std::format(".ROBLOSECURITY={};", token);
    curl_easy_setopt(handle, CURLOPT_URL, "https://users.roblox.com/v1/users/authenticated");
    curl_easy_setopt(handle, CURLOPT_COOKIE, cock.c_str());
    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, CurlWriteToBuf);
    curl_easy_setopt(handle, CURLOPT_WRITEDATA, &jsonStr);
    CURLcode ret = curl_easy_perform(handle);
    if (ret == CURLE_OK) {
        json jsonRes = json::parse(jsonStr);
        return jsonRes;
    }
    return json {};
}

Auth::Auth(Config *config) :
    ActiveAccount(nullptr),
    mConfig(config)
{}

AuthResponse Auth::ReadFromKeychain() {
    keychain::Error err;
    std::string jsonStr = keychain::getPassword("org.noobwarrior", "rbx-acc", "all", err);
    if (err.type == keychain::ErrorType::NotFound)
        return AuthResponse::Success;
    if (err.type != keychain::ErrorType::NoError)
        return AuthResponse::KeychainFailed;
    try {
        json accountsJson = json::parse(jsonStr);
        for (auto &accJson : accountsJson) {
            Account acc {};
            acc.Id = accJson["id"].get<int64_t>();
            acc.Name = accJson["name"].get<std::string>();
            acc.Token = accJson["token"].get<std::string>();
            acc.ExpireTimestamp = accJson["expire_timestamp"].get<long>();
            Accounts.push_back(acc);

            std::optional<std::string> active_account_thing = mConfig->GetKeyValue<std::string>("internet.roblox.active_account");
            if (active_account_thing.has_value() && active_account_thing.value().compare(acc.Name) == 0)
                ActiveAccount = &Accounts.back();
        }
    } catch (json::exception) {
        return AuthResponse::InvalidJson;
    }
    return AuthResponse::Success;
}

AuthResponse Auth::WriteToKeychain() {
    if (ActiveAccount != nullptr)
        mConfig->SetKeyValue<std::string>("internet.roblox.active_account", ActiveAccount->Name);

    json accountsJson {};
    for (auto &acc : Accounts) {
        json accJson {};
        accJson["id"] = acc.Id;
        accJson["name"] = acc.Name;
        accJson["token"] = acc.Token;
        accJson["expire_timestamp"] = acc.ExpireTimestamp;
        accountsJson.push_back(accJson);
    }
    
    keychain::Error err;
    keychain::setPassword("org.noobwarrior", "rbx-acc", "all", accountsJson.dump(), err);
    if (err.type != keychain::ErrorType::NoError)
        return AuthResponse::KeychainFailed;
    return AuthResponse::Success;
}

bool Auth::IsLoggedIn() {
    return ActiveAccount != nullptr;
}

AuthResponse Auth::TryAuthAccount(std::string& name, std::string& pass) {
    Out("Auth", "Attempting to log into account {}", name);
    CURL *handle = curl_easy_init();
    if (handle == nullptr) return AuthResponse::Failed;
    curl_easy_setopt(handle, CURLOPT_URL, "http://example.com/");
    CURLcode ret = curl_easy_perform(handle);
    curl_easy_cleanup(handle);
    return AuthResponse::Failed;
}

AuthResponse Auth::TryMultiAuth(int code) {
    return AuthResponse::Failed;
}

AuthResponse Auth::AddAccountFromToken(const std::string &token, Account *acc) {
    acc = nullptr;

    Account accStack {};
    accStack.Token = token;

    json userInfo = Auth::GetJsonFromToken(token);
    Out("Auth", "{}", userInfo.dump());
    if (userInfo.empty() || userInfo.contains("errors") || !userInfo.contains("id") || !userInfo.contains("name"))
        return AuthResponse::Failed;

    accStack.Id = userInfo["id"].get<long>();
    accStack.Name = userInfo["name"].get<std::string>();

    Accounts.push_back(accStack);
    acc = &accStack;
    return AuthResponse::Success;
}

void Auth::SetActiveAccount(Account *acc) {
    ActiveAccount = acc;
}

Account *Auth::GetActiveAccount() {
    return ActiveAccount;
}

std::vector<Account> Auth::GetAccounts() {
    return Accounts;
}
