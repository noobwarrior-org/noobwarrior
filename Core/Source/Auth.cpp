// === noobWarrior ===
// File: Auth.cpp
// Started by: Hattozo
// Started on: 3/6/2025
// Description: Manages authentication of Roblox accounts for use with the noobWarrior library
#include <NoobWarrior/Auth.h>
#include <NoobWarrior/Log.h>

#include <curl/curl.h>
#include <curl/easy.h>

#include <format>

using namespace NoobWarrior;
Auth::Account* Auth::gActiveAccount = nullptr;

bool Auth::IsLoggedIn() {
    return gActiveAccount != nullptr;
}

bool Auth::IsTokenValid(Account &account) {
    if (time(nullptr) > account.ExpireTimestamp) return false; // Automatic fail if it is past the expiration date. No way that will work.
    bool isValid;
    long code;
    std::string cock = std::format(".ROBLOSECURITY={};", account.Token);
    CURL *handle = curl_easy_init();
    struct curl_slist *list = curl_slist_append(list, "");
    curl_easy_setopt(handle, CURLOPT_URL, "https://users.roblox.com/v1/users/authenticated");
    curl_easy_setopt(handle, CURLOPT_HTTPHEADER, list);
    curl_easy_setopt(handle, CURLOPT_COOKIE, cock.c_str());
    CURLcode ret = curl_easy_perform(handle);
    if (ret == CURLE_OK) {
        curl_easy_getinfo(handle, CURLINFO_RESPONSE_CODE, &code);
        if (code == 200) isValid = true;
    }
    curl_slist_free_all(list);
    curl_easy_cleanup(handle);
    return isValid;
}

Auth::AuthResponse Auth::TryAuthAccount(std::string& name, std::string& pass) {
    Out("Auth", "Attempting to log into account {}", name);
    CURL *handle = curl_easy_init();
    if (handle == nullptr) return AuthResponse::Failed;
    curl_easy_setopt(handle, CURLOPT_URL, "http://example.com/");
    CURLcode ret = curl_easy_perform(handle);
    curl_easy_cleanup(handle);
    return AuthResponse::Failed;
}

Auth::AuthResponse Auth::TryMultiAuth(int code) {

}