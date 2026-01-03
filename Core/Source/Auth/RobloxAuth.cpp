// === noobWarrior ===
// File: RobloxAuth.cpp
// Started by: Hattozo
// Started on: 11/7/2025
// Description: Manages authentication of Roblox accounts for use with the noobWarrior library
#include <NoobWarrior/Auth/RobloxAuth.h>

#include <curl/curl.h>

using namespace NoobWarrior;

static size_t CurlWriteToBuf(void *contents, size_t size, size_t nmemb, std::string *buffer) {
    size_t totalSize = size * nmemb;
    buffer->insert(buffer->end(), (char*)contents, (char*)contents + totalSize);
    return totalSize;
}

RobloxAuth::RobloxAuth(Config *config) : BaseAuth(config) {}

std::string RobloxAuth::GetName() {
    return "roblox";
}

nlohmann::json RobloxAuth::GetJsonFromToken(const std::string &token) {
    std::string jsonStr;

    CURL *handle = curl_easy_init();
    std::string cock = std::format(".ROBLOSECURITY={};", token);
    curl_easy_setopt(handle, CURLOPT_URL, "https://users.roblox.com/v1/users/authenticated");
    curl_easy_setopt(handle, CURLOPT_COOKIE, cock.c_str());
    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, CurlWriteToBuf);
    curl_easy_setopt(handle, CURLOPT_WRITEDATA, &jsonStr);
    CURLcode ret = curl_easy_perform(handle);
    if (ret == CURLE_OK) {
        nlohmann::json jsonRes = nlohmann::json::parse(jsonStr);
        return jsonRes;
    }
    return nlohmann::json {};
}