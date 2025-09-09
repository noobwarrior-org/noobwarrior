// === noobWarrior ===
// File: NetClient.h
// Started by: Hattozo
// Started on: 3/15/2025
// Description: A thin layer over libcurl that makes it less annoying to use
#pragma once
#include "Auth.h"

#include <curl/curl.h>

#include <functional>
#include <vector>
#include <memory>
#include <atomic>

namespace NoobWarrior {
struct Transfer {
    CURL* Handle = nullptr;
    std::shared_ptr<std::atomic_bool> Cancelled = std::make_shared<std::atomic_bool>(false);
    std::shared_ptr<std::ofstream> File;
    size_t DownloadTotal {};
    size_t DownloadNow {};
    size_t UploadTotal {};
    size_t UploadNow {};
};

class NetClient {
public:
    NetClient(Account *account = nullptr, const std::filesystem::path &outputDir = "");
    ~NetClient();

    bool Failed();

    CURLcode Request(const std::string &url);

    void OnDownloadProgress();
    void OnWriteToMemoryFinished(std::function<void(std::vector<unsigned char>&)> callback);
    void OnFileDownloaded(std::function<void()> callback);
private:
    bool mFailed;
    const std::filesystem::path &mOutputDir;
    std::vector<unsigned char> mData;
    Account *mAccount;
    CURL *mHandle;
};
}