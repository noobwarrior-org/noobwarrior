// === noobWarrior ===
// File: NetClient.h
// Started by: Hattozo
// Started on: 3/15/2025
// Description: A thin layer over libcurl that makes it less annoying to use
#pragma once

#include <curl/curl.h>

#include <functional>
#include <vector>

namespace NoobWarrior {
    class NetClient {
    public:
        NetClient();
        
        CURLcode Open();
        CURLcode Close();
        CURLcode Exec();

        void OnDownloadFinished(std::function<void(std::vector<char>)> callback);
        void OnFileDownloaded(std::function<void()> callback);
    private:
        CURL *mHandle;
    };
}