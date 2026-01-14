/*
 * Copyright (C) 2026 Hattozo
 *
 * This file is part of noobWarrior.
 *
 * noobWarrior is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * noobWarrior is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with noobWarrior; if not, see
 * <https://www.gnu.org/licenses/>.
 */
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
#include <filesystem>
#include <zip.h>

#include <thread>
#include <zipconf.h>

#include <set>

#if defined(_WIN32)
#include <windows.h>
#include <tlhelp32.h>
#endif

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
    const std::filesystem::path dir = GetUserDataDir() / "roblox" / dirName / ("version-" + client.Hash);
    return dir;
}

bool Core::IsClientInstalled(const RobloxClient &client) {
    if (!std::filesystem::exists(GetClientDirectory(client))) return false;
    bool foundExe = false;
    for (const auto &entry : std::filesystem::directory_iterator(GetClientDirectory(client))) {
        if (entry.path().extension() == ".exe")
            foundExe = true;
    }
    return foundExe;
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

    auto output_mutex = std::make_shared<std::mutex>(); // Don't ask me why I didn't just make this part of the Out function. Because I don't know either!
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

                output_mutex->lock();
                    Out("Download", "Downloading file {} from URL \"{}\"", customFileName, url);
                output_mutex->unlock();

                std::filesystem::path client_dir = GetClientDirectory(client);
                std::filesystem::path tmp_download_dir = GetUserDataDir() / "temp" / "downloads" / "clients" / client.Hash;
                std::filesystem::create_directories(client_dir);
                std::filesystem::create_directories(tmp_download_dir);

                std::filesystem::path tmp_download_file = tmp_download_dir / customFileName;
                auto ext = tmp_download_file.extension();

                // Do this because I don't want to bundle a bunch of decompression algorithms within my library.
                if (ext == ".7z" || ext == ".rar" || ext == ".gz" || ext == ".xz" || ext == ".bz2" || ext == ".zst") {
                    output_mutex->lock();
                        Out("Download", "Error downloading: Detected file \"{}\"; Only .zip files are allowed when it comes to archives!", tmp_download_file.string());
                    output_mutex->unlock();
                    (*callback)(ClientInstallState::Failed, CURLE_OK, 0, 0);
                    return;
                }

                std::shared_ptr<std::ofstream> file = std::make_shared<std::ofstream>();
                file->open(tmp_download_file.string(), std::ios::out | std::ios::binary | std::ios::trunc);

                if (!file->is_open()) {
                    output_mutex->lock();
                        Out("Download", "Failed to open file \"{}\" for writing", tmp_download_file.string());
                    output_mutex->unlock();
                    (*callback)(ClientInstallState::Failed, CURLE_OK, 0, 0);
                    return;
                }
                
                auto transfer = std::make_shared<Transfer>();
                size_t transfer_id = transfers->size();
                transfers->push_back(transfer);
                std::thread([=]() -> void {
                    // Downloading the file
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
                    callback_mutex->lock();
                        (*callback)(res != CURLE_OK ? ClientInstallState::Failed : ClientInstallState::ExtractingFiles, res, 0, 0);
                    callback_mutex->unlock();

                    access_transfers_mutex->lock();
                        transfers->erase(transfers->begin() + transfer_id);
                    access_transfers_mutex->unlock();
                    
                    curl_easy_cleanup(handle);
                    file->close();

                    if (res != CURLE_OK || (my_data->cancelled && my_data->cancelled->load())) {
                        output_mutex->lock();
                            Out("Download", res != CURLE_OK ? "Failed to download file {}" : "Cancelled download for file {}", customFileName);
                        output_mutex->unlock();
                        std::filesystem::remove(tmp_download_file.string());
                        delete my_data;
                        return;
                    }
                    
                    // Extracting the file
                    output_mutex->lock();
                    if (tmp_download_file.extension() != ".zip") {
                        // Not a zip, don't extract! Move it to its intended place.
                        Out("Download", "Successfully downloaded file {}! Moving...", customFileName);
                        output_mutex->unlock();
                        std::filesystem::rename(tmp_download_file, client_dir / customFileName);
                        delete my_data;
                        return;
                    } else Out("Download", "Successfully downloaded file {}! Extracting...", customFileName);
                    output_mutex->unlock();

                    int error;
                    zip_t *archive = zip_open(tmp_download_file.string().c_str(), 0, &error);
                    if (!archive) {
                        output_mutex->lock();
                            Out("Download", "Failed to open zip archive \"{}\"", tmp_download_file.string());
                        output_mutex->unlock();
                        callback_mutex->lock();
                            (*callback)(ClientInstallState::Failed, res, 0, 0);
                        callback_mutex->unlock();
                        std::filesystem::remove(tmp_download_file);
                        delete my_data;
                        return;
                    }
                    
                    // For some reason when I do C programming I fall back to snake case. Don't ask why
                    zip_int64_t num_entries = zip_get_num_entries(archive, 0);

                    // Some zip files were created in a way where the author decided to put a single directory containing all files in the root of the archive,
                    // instead of doing it correctly by having all of the contents in that single directory be within the root directory itself.
                    // This fucks up our current scheme where we make the directory ourselves and then extract all files within the root to it.
                    // So what we have to do is to first collect the number of entries we have in the root of the archive.
                    // And if it's only one and it's a directory, guess what, it's getting scrubbed away and replaced with MY directory.
                    std::set<std::string> root_entries;
                    for (zip_uint64_t i = 0; i < num_entries; ++i) {
                        zip_stat_t st;
                        if (zip_stat_index(archive, i, 0, &st) != 0) continue;
                        std::string name = st.name;
                        auto pos = name.find('/');
                        std::string top_level_name = (pos == std::string::npos) ? name : name.substr(0, pos + 1);
                        root_entries.insert(top_level_name);
                    }
                    
                    for (zip_int64_t i = 0; i < num_entries; ++i) {
                        zip_stat_t st;
                        if (zip_stat_index(archive, i, 0, &st) < 0) {
                            output_mutex->lock();
                                Out("Download", "Failed to get statistics for file inside of zip archive. The index of the file is {}", i);
                            output_mutex->unlock();
                            continue;
                        }

                        std::string name = st.name;

                        if (root_entries.size() == 1) {
                            // to get around the stupid "one directory at the root level with everything else inside of that directory" thing
                            // basically erases the prefix altogether
                            for (const auto &root_entry : root_entries) {
                                if (root_entry.ends_with('/') && name.rfind(root_entry, 0) == 0)
                                    name.erase(0, root_entry.size());
                            }
                        }

                        auto path = std::filesystem::path(client_dir / name);

                        if (path.string().ends_with('/')) {
                            // directory detected
                            std::filesystem::create_directories(path.string());
                            continue;
                        }

                        zip_file *zf = zip_fopen_index(archive, i, 0);
                        if (!zf) {
                            output_mutex->lock();
                                Out("Download", "Failed to open file inside of zip archive \"{}\"", name);
                            output_mutex->unlock();
                            continue;
                        }
                        
                        std::ofstream extracted_file(path.string(), std::ios::out | std::ios::binary | std::ios::trunc);
                        if (!extracted_file.is_open()) {
                            output_mutex->lock();
                                Out("Download", "Failed to open extracted file \"{}\"", path.string());
                            output_mutex->unlock();
                            zip_fclose(zf);
                            continue;
                        }

                        char buffer[4096];
                        zip_int64_t bytes_read = 0;
                        while ((bytes_read = zip_fread(zf, buffer, sizeof(buffer))) > 0) {
                            if (my_data->cancelled && my_data->cancelled->load()) {
                                // if the user decides to cancel mid-extraction
                                output_mutex->lock();
                                    Out("Download", "Cancelled extraction of file \"{}\"", path.string());
                                output_mutex->unlock();
                                std::filesystem::remove(tmp_download_file.string());
                                try {
                                    if (client_dir.parent_path().parent_path().filename() == "roblox") // make sure we are nuking the right thing here
                                        std::filesystem::remove_all(client_dir.string()); // just nuke the entire fucking client directory
                                } catch (std::exception &ex) {} 
                                break;
                            }
                            extracted_file.write(buffer, bytes_read);
                        }
                        if (bytes_read == -1) {
                            output_mutex->lock();
                                Out("Download", "Failed to extract file inside of zip archive \"{}\"", path.string());
                            output_mutex->unlock();
                        }

                        extracted_file.close();
                        zip_fclose(zf);
                    }
                    zip_close(archive);

                    if (!my_data->cancelled || !my_data->cancelled->load()) {
                        output_mutex->lock();
                            Out("Download", "Successfully extracted archive {}", customFileName);
                        output_mutex->unlock();
                        callback_mutex->lock();
                            (*callback)(ClientInstallState::Success, res, 0, 0);
                        callback_mutex->unlock();
                    }
                    std::filesystem::remove(tmp_download_file);
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

#if defined(_WIN32)
static std::string LastErrorStr(DWORD err = GetLastError()) {
    char buf[512] = {0};
    FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                   nullptr, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                   buf, (DWORD)sizeof(buf), nullptr);
    return std::string(buf);
}
#endif

ClientLaunchResponse Core::LaunchProcessThroughInjector(const std::filesystem::path &filePath) {
    const std::filesystem::path &injectorPath = GetInstallationDir() / "noobhook_x86_injector.exe";
    std::wstring wargs = std::format(L"{} --file {}", injectorPath.wstring(), filePath.wstring());
    std::vector<wchar_t> wargs_vec(wargs.begin(), wargs.end());
    wargs_vec.push_back(L'\0');
#if defined(_WIN32)
    PROCESS_INFORMATION pi {};
    STARTUPINFOW si = {};
    si.cb = sizeof(si);
    if (!CreateProcessW(nullptr, wargs_vec.data(), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi)) {
        DWORD err = GetLastError();
        Out("Inject", "Failed to create injector process: {} ({})", err, LastErrorStr(err));
        return ClientLaunchResponse::FailedToCreateProcess;
    }

    WaitForSingleObject(pi.hProcess, INFINITE);

    DWORD exitCode;
    if (!GetExitCodeProcess(pi.hProcess, &exitCode)) {
        DWORD err = GetLastError();
        Out("Inject", "Failed to get exit code for injector process: {} ({})", err, LastErrorStr(err));
        return ClientLaunchResponse::Failed;
    }

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return static_cast<ClientLaunchResponse>(exitCode);
#elif defined(__unix__) || defined(__APPLE__)
    // where wine comes in
    return ClientLaunchResponse::Failed;
#endif
}

ClientLaunchResponse Core::LaunchClient(const RobloxClient &client) {
    bool installed = IsClientInstalled(client);
    if (!installed) return ClientLaunchResponse::NotInstalled;
    const std::filesystem::path dir = GetClientDirectory(client);
    std::filesystem::path exe;
    for (const std::filesystem::directory_entry &entry : std::filesystem::directory_iterator(dir)) {
        std::string fn = entry.path().filename().string();
        if (fn == "RobloxPlayerBeta.exe" || fn == "RCCService.exe" || fn == "RobloxStudioBeta.exe") {
            exe = entry.path();
            break;
        }
    }
    if (!exe.empty()) return LaunchProcessThroughInjector(exe); else return ClientLaunchResponse::NoValidExecutable;
}