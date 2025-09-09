// === noobWarrior ===
// File: Backup.cpp
// Started by: Hattozo
// Started on: 3/5/2025
// Description: All functions that concern backing up data from Roblox servers to your computer belong here
#include <NoobWarrior/Config.h>
#include <NoobWarrior/NoobWarrior.h>
#include <NoobWarrior/Roblox/Api/Asset.h>

#include <curl/curl.h>
#include <nlohmann/json.hpp>

#include <cmath>
#include <iostream>
#include <filesystem>
#include <sstream>

using namespace NoobWarrior;
using json = nlohmann::json;

static const std::map<long, std::string> sHttpStatusMessages = {
    {100, "Continue"},
    {101, "Switching Protocols"},
    {200, "OK"},
    {201, "Created"},
    {202, "Accepted"},
    {204, "No Content"},
    {301, "Moved Permanently"},
    {302, "Found"},
    {304, "Not Modified"},
    {400, "Bad Request"},
    {401, "Unauthorized"},
    {403, "Forbidden"},
    {404, "Not Found"},
    {500, "Internal Server Error"},
    {502, "Bad Gateway"},
    {503, "Service Unavailable"}
};

static int CountDigits(int c) {
    if (c == 0)
        return 1;
    return (int)floor(log10(abs(c))) + 1;
}

static long ConvertISO8601ToTimestamp(const std::string& iso8601_string) {
    std::tm t{};
    std::istringstream ss(iso8601_string);
    ss >> std::get_time(&t, "%Y-%m-%dT%H:%M:%S%z");
    if (ss.fail())
        return -1;
    std::time_t time_since_epoch = mktime(&t);
    if (time_since_epoch == -1)
        return -2;
    return static_cast<long>(time_since_epoch);
}

static size_t CurlWriteToFile(char* ptr, size_t size, size_t nmemb, FILE *userdata) {
    size_t written;
    written = fwrite(ptr, size, nmemb, userdata);
    return written;
}

static size_t CurlWriteToBuf(void *contents, size_t size, size_t nmemb, std::vector<char> *buffer) {
    size_t totalSize = size * nmemb;
    buffer->insert(buffer->end(), (char*)contents, (char*)contents + totalSize);
    return totalSize;
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

int Core::DownloadAssets(DownloadAssetArgs args) {
    if (args.OutStream == nullptr) args.OutStream = &std::cout;
    curl_version_info_data *vinfo = curl_version_info(CURLVERSION_NOW);
    if (!(vinfo->features & CURL_VERSION_SSL))
        OutEx(args.OutStream, "AssetRequest", "WARNING! SSL in curl library is not enabled. HTTPS links will be unsupported!", args.OutDir);
    if (!std::filesystem::is_directory(args.OutDir)) {
        OutEx(args.OutStream, "AssetRequest", "Failed to download assets: Directory \"{}\" doesn't exist", args.OutDir);
        return -1;
    }
    CURL *handle = curl_easy_init();
    if (!handle) {
        OutEx(args.OutStream, "AssetRequest", "Failed to download assets: Failed to create curl handle");
        return 0;
    }
    curl_easy_setopt(handle, CURLOPT_USERAGENT, "Roblox/WinINet"); // use the same user agent that the Roblox client uses.
    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, &CurlWriteToFile);
    curl_easy_setopt(handle, CURLOPT_HEADERFUNCTION, &HeaderCallback);
    for (int i = 0; i < args.Id.size(); i++) {
        OutEx(args.OutStream, "AssetRequest", "Downloading ID {}", args.Id.at(i));

        // by default we create a placeholder file with the ID as its name.
        int64_t id = args.Id.at(i);
        size_t idDigits = CountDigits(id);
        char* fileName = (char*)malloc(idDigits + 1);
        snprintf(fileName, idDigits + 1, "%i", (int)id);

        std::optional<std::string> download_url = GetConfig()->GetKeyValue<std::string>("internet.roblox.asset_download");
        if (!download_url.has_value())
            return -2;

        std::string fmtApiCall = std::vformat(download_url.value(), std::make_format_args(id));
        std::string fileDir = args.OutDir + "/" + fileName;

        FILE* filePointer = fopen(fileDir.c_str(), "wb");
        if (filePointer == NULL) {
            OutEx(args.OutStream, "AssetRequest", "Failed to download ID %i: Failed to create file");
            continue;
        }

        // we had this pointing to memory that was allocated to the number of digits in our asset id, so we could have the file name be the asset ID.
        // now we're done with that part, and we might soon rename it to something else anyways if the user decides to not like ID's as the file name.
        free(fileName);

        curl_easy_setopt(handle, CURLOPT_URL, fmtApiCall.c_str());
        curl_easy_setopt(handle, CURLOPT_WRITEDATA, filePointer);
        if (args.FileNameStyle == AssetFileNameStyle::Raw) // header callback makes the file name pointer point to content deposition's header value.
            curl_easy_setopt(handle, CURLOPT_HEADERDATA, fileName);

        CURLcode ret = curl_easy_perform(handle);
        if (ret == CURLE_OK) {
            long res;
            curl_easy_getinfo(handle, CURLINFO_RESPONSE_CODE, &res);
            if (res != 200) { OutEx(args.OutStream, "AssetRequest", "Failed to download ID {}: {} {}", id, res, sHttpStatusMessages.count(res) ? sHttpStatusMessages.at(res) : "Unknown"); }
        }
        if (ret != CURLE_OK)
            OutEx(args.OutStream, "AssetRequest", "Failed to download ID {}: Curl error {}, {}", id, (int)ret, curl_easy_strerror(ret));
        else if (args.FileNameStyle != AssetFileNameStyle::AssetId) { // because the file is already named with its corresponding asset id, so it's pointless to rename it to the same thing.
            std::string newFileDir = args.OutDir + "/" + fileName;
            rename(fileDir.c_str(), newFileDir.c_str());
        }
        
        fclose(filePointer);
    }
    OutEx(args.OutStream, "AssetRequest", !args.Id.empty() ? "Finished iterating through all IDs." : "Stopping, nothing to download.");
    curl_easy_cleanup(handle);
    return 1;
}

int Core::GetAssetDetails(int64_t id, Roblox::AssetDetails *details) {
    int ret = 0;
    std::optional<std::string> details_url = GetConfig()->GetKeyValue<std::string>("internet.roblox.asset_details");
    if (!details_url.has_value())
        return -2;

    std::vector<char> buffer;
    CURL *handle = curl_easy_init();
    if (!handle)
        return ret;
    curl_easy_setopt(handle, CURLOPT_USERAGENT, "Roblox/WinINet");
    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, &CurlWriteToBuf);
    curl_easy_setopt(handle, CURLOPT_WRITEDATA, &buffer);
    curl_easy_setopt(handle, CURLOPT_URL, std::vformat(details_url.value(), std::make_format_args(id)).c_str());
    CURLcode res = curl_easy_perform(handle);
    if (res == CURLE_OK) {
        json data = json::parse(buffer, nullptr, false);

        if (!data["errors"].is_null()) {
            ret = -1;
            goto cleanup;
        }

        details->TargetId = data["TargetId"];
        details->ProductType = !data["ProductType"].is_null() ? (data["ProductType"] != "Collectible Item" ? Roblox::ProductType::UserProduct : Roblox::ProductType::CollectibleItem) : Roblox::ProductType::None;
        details->AssetId = data["AssetId"];
        details->ProductId = data["ProductId"];
        details->Name = data["Name"];
        details->Description = data["Description"];
        details->AssetType = data["AssetTypeId"];
        details->CreatorId = data["Creator"]["Id"];
        details->CreatorName = data["Creator"]["Name"];
        details->CreatorType = data["Creator"]["CreatorType"] != "Group" ? Roblox::CreatorType::User : Roblox::CreatorType::Group;
        details->CreatorTargetId = data["Creator"]["CreatorTargetId"];
        details->CreatorHasVerifiedBadge = data["Creator"]["HasVerifiedBadge"];
        details->IconImageAssetId = data["IconImageAssetId"];
        details->Created = ConvertISO8601ToTimestamp(data["Created"]);
        details->Updated = ConvertISO8601ToTimestamp(data["Updated"]);
        details->PriceInRobux = !data["PriceInRobux"].is_null() ? static_cast<int>(data["PriceInRobux"]) : -1;
        details->PriceInTickets = !data["PriceInRobux"].is_null() ? static_cast<int>(data["PriceInTickets"]) : -1;
        details->Sales = data["Sales"];
        details->IsNew = data["IsNew"];
        details->IsForSale = data["IsForSale"];
        details->IsPublicDomain = data["IsPublicDomain"];
        details->IsLimited = data["IsLimited"];
        details->IsLimitedUnique = data["IsLimitedUnique"];
        details->MinimumMembershipLevel = data["MinimumMembershipLevel"];
        ret = 1;
    }
cleanup:
    curl_easy_cleanup(handle);
    return ret;
}

BackupResponse Core::BackupFromFile(const std::filesystem::path &inputDir, const std::filesystem::path &outputDir, std::function<void(BackupState, std::string, size_t, size_t)> &callback) {

}

BackupResponse Core::BackupAsset(int64_t id, Database *db, std::function<void(BackupState, std::string, size_t, size_t)> &callback) {
    std::optional<std::string> asset_download_url = GetConfig()->GetKeyValue<std::string>("internet.roblox.asset_download");

    if (!asset_download_url.has_value())
        return BackupResponse::UrlNotSet;

    NetClient client(GetAuth()->GetActiveAccount());
    if (client.Failed())
        return BackupResponse::Failed;
    client.OnWriteToMemoryFinished([](std::vector<unsigned char> &data) {
        
    });
    client.Request(asset_download_url.value());
    return BackupResponse::Ok;
}

BackupResponse Core::BackupGame(int64_t id, Database *db, std::function<void(BackupState, std::string, size_t, size_t)> &callback) {

    return BackupResponse::Ok;
}