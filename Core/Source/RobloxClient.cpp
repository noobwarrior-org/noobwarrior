// === noobWarrior ===
// File: RobloxClient.cpp
// Started by: Hattozo
// Started on: 8/8/2025
// Description: Implementation for all methods related to handling Roblox clients
#include <NoobWarrior/Log.h>
#include <NoobWarrior/RobloxClient.h>
#include <NoobWarrior/NoobWarrior.h>
#include <NoobWarrior/NetClient.h>

#include <curl/curl.h>
#include <curl/multi.h>

#include <thread>

using namespace NoobWarrior;

std::vector<RobloxClient> Core::GetInstalledClients() {
    return {};
}

std::vector<RobloxClient> Core::GetClientsFromIndex() {
    nlohmann::json index;
    int res = RetrieveIndex(index);
    if (res != CURLE_OK)
        return {};
}

std::vector<RobloxClient> Core::GetAllClients() {
    return {};
}

std::filesystem::path Core::GetClientDirectory(const RobloxClient &client) {
    std::string dirName;
    switch (client.Type) {
    case ClientType::Client: dirName = "client"; break;
    case ClientType::Server: dirName = "server"; break;
    case ClientType::Studio: dirName = "studio"; break;
    }
    const std::filesystem::path dir = GetUserDataDir() / std::format("roblox/{}/version-{}", dirName, client.Hash);
    return dir;
}

bool Core::IsClientInstalled(const RobloxClient &client) {
    return std::filesystem::exists(GetClientDirectory(client));
}

static size_t WriteToDisk(void *contents, size_t size, size_t nmemb, void *userp) {
    static_cast<std::ofstream*>(userp)->write(static_cast<char*>(contents), size * nmemb);
    return size * nmemb;
}

// We can pass only one thing at a time to a curl callback, so group it all together in a struct
struct data_to_pass_to_progress_callback {
    int transfer_id;
    std::vector<std::shared_ptr<Transfer>> *transfers_vector;
    std::mutex *transfers_vector_mutex; // Our mutex that locks down access to the std::vector<> object that contains the data for the download total and stuff.
    std::function<void(ClientInstallState, CURLcode, size_t, size_t)> *callback;
    std::mutex *callback_mutex;
    std::atomic_bool *cancelled;
};

static int ProgressCallback(void *clientp,
                      curl_off_t dltotal,
                      curl_off_t dlnow,
                      curl_off_t ultotal,
                      curl_off_t ulnow) {
    auto data = static_cast<data_to_pass_to_progress_callback*>(clientp);
    if (data->cancelled && data->cancelled->load()) {
        return 1; // Aborts the transfer
    }

    data->transfers_vector_mutex->lock();
        std::shared_ptr<Transfer> myTransfer = data->transfers_vector->at(data->transfer_id);
        myTransfer->DownloadTotal = dltotal;
        myTransfer->DownloadNow = dlnow;
        myTransfer->UploadTotal = ultotal;
        myTransfer->UploadNow = ulnow;

        size_t combinedDownloadTotal = 0;
        size_t combinedDownloadNow = 0;
        size_t combinedUploadTotal = 0;
        size_t combinedUploadNow;
        for (int i = 0; i < data->transfers_vector->size(); i++) {
            std::shared_ptr<Transfer> transfer = data->transfers_vector->at(i);
            combinedDownloadTotal += transfer->DownloadTotal;
            combinedDownloadNow += transfer->DownloadNow;
            combinedUploadTotal += transfer->UploadTotal;
            combinedUploadNow += transfer->UploadNow;
        }
    data->transfers_vector_mutex->unlock();

    data->callback_mutex->lock();
        (*data->callback)(ClientInstallState::DownloadingFiles, CURLE_OK, combinedDownloadNow, combinedDownloadTotal);
    data->callback_mutex->unlock();
    return 0;
}

void Core::DownloadAndInstallClient(const RobloxClient &client, std::shared_ptr<std::vector<std::shared_ptr<Transfer>>> &transfers, std::shared_ptr<std::function<void(ClientInstallState, CURLcode, size_t, size_t)>> callback) {
    nlohmann::json index;
    int res = RetrieveIndex(index);
    if (res != CURLE_OK) {
        Out("Download", "Failed to retrieve index");
        (*callback)(ClientInstallState::Failed, static_cast<CURLcode>(res), 0, 0);
        return;
    }

    bool foundClient = false;

    auto callback_mutex = std::make_shared<std::mutex>();
    auto access_transfers_mutex = std::make_shared<std::mutex>();
    
    for (nlohmann::json &clientInfo : index["Roblox"][ClientTypeAsTranslatableString(client.Type)]) {
        if (clientInfo.contains("Hash") && clientInfo["Hash"].get<std::string>().compare(client.Hash) == 0) {
            // we have found the client we want to install!
            Out("Download", "Found client {}", clientInfo["Hash"].get<std::string>());
            foundClient = true;
            std::function addFile = [&](nlohmann::json &fileInfo) {
                std::string url;
                std::string customFileName;

                if (fileInfo.is_string()) {
                    url = fileInfo.get<std::string>();
                } else if (fileInfo.is_array()) {
                    url = fileInfo["Url"].get<std::string>();
                    customFileName = fileInfo["FileName"].get<std::string>();
                }
                
                std::string fileName;
                auto pos = url.find_last_of('/');
                if (pos == std::string::npos || pos + 1 >= url.size()) {
                    // fallback if URL ends with '/' or no slash found
                    fileName = "unknown";
                } else {
                    fileName = url.substr(pos + 1);
                }
                if (customFileName.empty()) customFileName = fileName;

                Out("Download", "Downloading file {} from URL \"{}\"", customFileName, url);

                std::filesystem::path tmp_download_dir = GetUserDataDir() / "temp" / "downloads" / "clients" / client.Hash;
                std::filesystem::create_directories(tmp_download_dir);

                std::filesystem::path tmp_download_file = tmp_download_dir / customFileName;
                std::shared_ptr<std::ofstream> file = std::make_shared<std::ofstream>();
                file->open(tmp_download_file.string(), std::ios::out | std::ios::binary | std::ios::trunc);

                if (!file->is_open()) {
                    Out("Download", "Failed to open file \"{}\" for writing", tmp_download_file.string());
                    (*callback)(ClientInstallState::Failed, CURLE_OK, 0, 0);
                    return;
                }
                
                auto transfer = std::make_shared<Transfer>();
                size_t transfer_id = transfers->size();
                transfers->push_back(transfer);
                std::thread([transfer, transfer_id, callback_mutex, access_transfers_mutex, callback, transfers, url, file]() -> void {
                    CURL *handle = curl_easy_init();
                    if (!handle) {
                        (*callback)(ClientInstallState::Failed, CURLE_FAILED_INIT, 0, 0);
                        return;
                    }
                    transfer->Handle = handle;
                    
                    auto *my_data = new data_to_pass_to_progress_callback;
                    my_data->transfer_id = transfer_id;
                    my_data->transfers_vector = transfers.get();
                    my_data->transfers_vector_mutex = access_transfers_mutex.get();
                    my_data->callback = callback.get();
                    my_data->callback_mutex = callback_mutex.get();
                    my_data->cancelled = transfer->Cancelled.get();

                    curl_easy_setopt(handle, CURLOPT_URL, url.c_str());
                    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, WriteToDisk);
                    curl_easy_setopt(handle, CURLOPT_WRITEDATA, file.get());
                    curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 1L);
                    curl_easy_setopt(handle, CURLOPT_NOPROGRESS, 0L);
                    curl_easy_setopt(handle, CURLOPT_XFERINFOFUNCTION, ProgressCallback);
                    curl_easy_setopt(handle, CURLOPT_XFERINFODATA, my_data);

                    CURLcode res = curl_easy_perform(handle);
                    if (res != CURLE_OK) {
                        callback_mutex->lock();
                            (*callback)(ClientInstallState::Failed, res, 0, 0);
                        callback_mutex->unlock();
                    }

                    access_transfers_mutex->lock();
                        transfers->erase(transfers->begin() + transfer_id);
                    access_transfers_mutex->unlock();
                    
                    curl_easy_cleanup(handle);
                    delete my_data;
                }).detach();
            };

            // TODO: make this not shit and make sure my future employers wont see this
            if (clientInfo["Files"].contains("Win32"))
                for (nlohmann::json &fileInfo : clientInfo["Files"]["Win32"]) { addFile(fileInfo); }
            if (clientInfo["Files"].contains("Win64"))
                for (nlohmann::json &fileInfo : clientInfo["Files"]["Win64"]) { addFile(fileInfo); }
            if (clientInfo["Files"].contains("Shared"))
                for (nlohmann::json &fileInfo : clientInfo["Files"]["Shared"]) { addFile(fileInfo); }
        }
    }

    if (!foundClient) {
        Out("Download", "Failed to find a client");
        (*callback)(ClientInstallState::Failed, CURLE_OK, 0, 0);
    }
}

int Core::LaunchClient(const RobloxClient &client) {
    const std::filesystem::path dir = GetClientDirectory(client);
    if (!std::filesystem::exists(dir)) return -3;
    std::filesystem::path exe;
    for (const std::filesystem::directory_entry &entry : std::filesystem::directory_iterator(dir)) {
        if (entry.path().extension().compare(".exe") == 0) {
            exe = entry.path();
            break;
        }
    }
    if (!exe.empty()) return LaunchInjectProcess(exe); else return -4;
}