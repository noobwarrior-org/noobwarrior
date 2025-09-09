// === noobWarrior ===
// File: NetClient.cpp
// Started by: Hattozo
// Started on: 3/15/2025
// Description: A thin layer over libcurl that makes it less annoying to use
#include <NoobWarrior/NetClient.h>

using namespace NoobWarrior;

static size_t WriteToBuf(void *contents, size_t size, size_t nmemb, std::string *buffer) {
    size_t totalSize = size * nmemb;
    buffer->insert(buffer->end(), (char*)contents, (char*)contents + totalSize);
    return totalSize;
}

static size_t WriteToDisk(void *contents, size_t size, size_t nmemb, void *userp) {
    static_cast<std::ofstream*>(userp)->write(static_cast<char*>(contents), size * nmemb);
    return size * nmemb;
}

NetClient::NetClient(Account *account, const std::filesystem::path &outputDir) :
    mFailed(true),
    mOutputDir(outputDir),
    mAccount(account)
{
    mHandle = curl_easy_init();
    if (mHandle == nullptr)
        return;

    std::string cookies;
    if (mAccount != nullptr) {
        cookies.append(std::format(".ROBLOSECURITY={};", mAccount->Token));
        curl_easy_setopt(mHandle, CURLOPT_USERAGENT, "Roblox/WinINet");
    }

    curl_easy_setopt(mHandle, CURLOPT_COOKIE, cookies.c_str());
    curl_easy_setopt(mHandle, CURLOPT_WRITEFUNCTION, WriteToBuf);
    curl_easy_setopt(mHandle, CURLOPT_WRITEDATA, &mData);
    
    mFailed = false;
}

NetClient::~NetClient() {
    curl_easy_cleanup(mHandle);
}

bool NetClient::Failed() {
    return mFailed;
}

CURLcode NetClient::Request(const std::string &url) {
    curl_easy_setopt(mHandle, CURLOPT_URL, url.c_str());
    
    return curl_easy_perform(mHandle);
}

void NetClient::OnWriteToMemoryFinished(std::function<void(std::vector<unsigned char>&)> callback) {
    callback(mData);
}