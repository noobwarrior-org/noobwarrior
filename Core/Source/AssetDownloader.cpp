// === noobWarrior ===
// File: AssetDownloader.cpp
// Started by: Hattozo
// Started on: 3/5/2025
// Description: Downloads an asset from Roblox servers and returns the data to you
#include <NoobWarrior/AssetDownloader.h>
#include <NoobWarrior/Config.h>
#include <NoobWarrior/NoobWarrior.h>

#include <curl/curl.h>

#include <iostream>
#include <filesystem>

using namespace NoobWarrior;

static int CountDigits(int c) {
    if (c == 0)
        return 1;
    return (int)floor(log10(abs(c))) + 1;
}

static size_t WriteCallback(char* ptr, size_t size, size_t nmemb, void* userdata) {
    size_t written;
    written = fwrite(ptr, size, nmemb, (FILE*)userdata);
    return written;
}

static size_t HeaderCallback(char* buffer, size_t size, size_t nitems, void* userdata) {
    size_t totalSize = nitems * size;
    char* contentDepositionHeader = (char*)"Content-Disposition: ";
    size_t headerLength = strlen(contentDepositionHeader);

    if (totalSize > headerLength && strncmp(buffer, contentDepositionHeader, headerLength) == 0) {
        // our content deposition header.
        char* val = buffer + headerLength;
        userdata = val; // our userdata in this case is the file name, since the content deposition header contains that stuff.
    }

    return totalSize;
}

int NoobWarrior::DownloadAssets(DownloadAssetArgs args) {
    curl_version_info_data *vinfo = curl_version_info(CURLVERSION_NOW);
    if (!(vinfo->features & CURL_VERSION_SSL))
        Out("AssetDownloader", "WARNING! SSL in curl library is not enabled. HTTPS links will be unsupported!", args.OutDir);
    if (!std::filesystem::is_directory(args.OutDir)) {
        Out("AssetDownloader", "Failed to download assets: Directory \"{}\" doesn't exist", args.OutDir);
        return -1;
    }
    CURL *handle = curl_easy_init();
    if (!handle) {
        Out("AssetDownloader", "Failed to download assets: Failed to create curl handle");
        return 0;
    }
    curl_easy_setopt(handle, CURLOPT_USERAGENT, "Roblox/WinINet"); // use the same user agent that the Roblox client uses.
    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(handle, CURLOPT_HEADERFUNCTION, HeaderCallback);
    for (int i = 0; i < args.Id.size(); i++) {
        Out("AssetDownloader", "Downloading ID %i", args.Id.at(i));

        // by default we create a placeholder file with the ID as its name.
        int64_t id = args.Id.at(i);
        size_t idDigits = CountDigits(id);
        char* fileName = (char*)malloc(idDigits + 1);
        snprintf(fileName, idDigits + 1, "%i", (int)id);

        std::string fmtApiCall = std::vformat(gConfig.Api_AssetDownload, std::make_format_args(id));
        std::string fileDir = args.OutDir + "/" + fileName;

        FILE* filePointer = fopen(fileDir.c_str(), "wb");
        if (filePointer == NULL) {
            Out("AssetDownloader", "Failed to download ID %i: Failed to create file");
            continue;
        }

        // we had this pointing to memory that was allocated to the number of digits in our asset id, so we could have the file name be the asset ID.
        // now we're done with that part, and we might soon rename it to something else anyways if the user decides to not like ID's as the file name.
        free(fileName);

        curl_easy_setopt(handle, CURLOPT_URL, fmtApiCall.c_str());
        curl_easy_setopt(handle, CURLOPT_WRITEDATA, filePointer);
        if (args.FileNameStyle == AssetFileNameStyle::Raw) // header callback makes the file name pointer point to content deposition's header value.
            curl_easy_setopt(handle, CURLOPT_HEADERDATA, fileName);

        CURLcode res = curl_easy_perform(handle);
        Out("AssetDownloader", "Code {}, {}", (int)res, curl_easy_strerror(res));
        if (res != CURLE_OK)
            Out("AssetDownloader", "Failed to download ID {}: {}", (int)res, curl_easy_strerror(res));
        else if (args.FileNameStyle != AssetFileNameStyle::AssetId) { // because the file is already named with its corresponding asset id, so it's pointless to rename it to the same thing.
            std::string newFileDir = args.OutDir + "/" + fileName;
            rename(fileDir.c_str(), newFileDir.c_str());
        }
        
        fclose(filePointer);
    }
    curl_easy_cleanup(handle);
    return 1;
}