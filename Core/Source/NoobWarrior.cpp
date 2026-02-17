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
// File: NoobWarrior.cpp
// Started by: Hattozo
// Started on: 6/17/2025
// Description: Contains code for the main class used to utilize the noobWarrior library
#include <NoobWarrior/NoobWarrior.h>
#include <NoobWarrior/NetClient.h>
#include <NoobWarrior/PluginManager.h>
#include <NoobWarrior/FileSystem/VirtualFileSystem.h>
#include <NoobWarrior/EmuDb/EmuDb.h>
#include <NoobWarrior/Url.h>
#include <NoobWarrior/Paths.h>

#include <event.h>
#include <sqlite3.h>

#if defined(_WIN32)
#include <windows.h>
#include <winsock2.h>
#include <shlobj.h>
#endif

#include <utility>
#include <istream>

using namespace NoobWarrior;

Core::Core(Init init) :
    mInitResponse(Response::Failed),
    mInit(std::move(init)),
    mLuaState(this),
    mEmuDbManager(this),
    mServerEmulator(nullptr),
    mPluginManager(this),
    mIndexDirty(true)
{
#if defined(_WIN32)
    // https://learn.microsoft.com/en-us/windows/win32/api/winsock/nf-winsock-wsastartup
    WORD wVersionRequested;
    WSADATA wsaData;
    int err;

    wVersionRequested = MAKEWORD(2, 2);

    err = WSAStartup(wVersionRequested, &wsaData);
    if (err != 0) {
        Out("Winsock", "WSAStartup failed with error: {}", err);
    }

    if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2) {
        Out("Winsock", "Could not find a usable version of Winsock.dll");
        WSACleanup();
    }
#endif

    Out("Core", std::format("noobWarrior is{}in portable mode", mInit.Portable ? " " : " not "));

    if (mInit.AutocreateStandardUserDataDirectories)
        CreateStandardUserDataDirectories();

    mEventBase = event_base_new();
    mLuaState.Open();
    mConfig = new Config(GetUserDataDir() / "config.lua", &mLuaState);
    mRbxKeychain = new RbxKeychain(mConfig);
    ConfigReturnCode = mConfig->Open();
    curl_global_init(CURL_GLOBAL_ALL);
    sqlite3_initialize();

    GetEmuDbManager()->MountDatabases();
    GetEmuDbManager()->CreateMasterDatabaseIfDoesntExist();

    if (mInit.EnableKeychain)
        GetRbxKeychain()->ReadFromKeychain();

    if (mInit.LoadPlugins)
        GetPluginManager()->LoadPlugins();

    mInitResponse = Response::Success;
}

Core::~Core() {
    GetPluginManager()->UnloadPlugins();
    
    if (mInit.EnableKeychain)
        GetRbxKeychain()->WriteToKeychain();

    GetEmuDbManager()->UnmountDatabases();
    StopServerEmulator();
    sqlite3_shutdown();
    curl_global_cleanup();
    ConfigReturnCode = mConfig->Close();
    NOOBWARRIOR_FREE_PTR(mConfig)
    mLuaState.Close();
    event_base_free(mEventBase);
#if defined(_WIN32)
    WSACleanup();
#endif
}

bool Core::Fail() {
    return mInitResponse != Response::Success;
}

int Core::ProcessEvents(bool block) {
    return event_base_loop(mEventBase, block ? (EVLOOP_ONCE) : (EVLOOP_ONCE | EVLOOP_NONBLOCK));
}

event_base *Core::GetEventBase() {
    return mEventBase;
}

LuaState *Core::GetLuaState() {
    return &mLuaState;
}

Config *Core::GetConfig() {
    return mConfig;
}

EmuDbManager *Core::GetEmuDbManager() {
    return &mEmuDbManager;
}

PluginManager *Core::GetPluginManager() {
    return &mPluginManager;
}

MasterKeychain *Core::GetMasterKeychain() {
    return mMasterKeychain;
}

EmuKeychain *Core::GetEmuKeychain() {
    return mEmuKeychain;
}

RbxKeychain *Core::GetRbxKeychain() {
    return mRbxKeychain;
}

const Init& Core::GetInit() {
    return mInit;
}

std::filesystem::path Core::GetInstallationDir() const {
    assert(mInit.ArgCount > 0 && "You must pass in your argc to ArgCount in order to use GetInstallationDir()");

    auto exePath = std::filesystem::path(mInit.ArgVec[0]);
    auto path = exePath.parent_path();
    if (!mInit.InstallDataRelativePath.empty()) {
        std::filesystem::create_directories(path / mInit.InstallDataRelativePath);
        path /= mInit.InstallDataRelativePath;
    }
    
#if defined(__APPLE__)
    // Are we part of an app bundle?
    if (path.filename().compare("MacOS") == 0)
        return std::filesystem::path(path / ".." / "Resources");
#endif
    return path;
}

std::filesystem::path Core::GetUserDataDir() {
    if (!mInit.Portable) {
#if defined(_WIN32)
        // Our user data directory is in the Documents folder instead of the %LocalAppData% directory, which is where most developers will store their user data.
        // We do this because Windows wipes away your AppData folder when you reinstall the OS, even when you choose to keep your data.
        // Typically it won't actually do this and it will instead move it away to a Windows.old folder
        // But, most people do not realize this and they end up losing everything when the OS decides to delete the directory after a grace period.
        WCHAR *path;
        SHGetKnownFolderPath(FOLDERID_Documents, KF_FLAG_DEFAULT, NULL, &path);
        std::filesystem::path documentsDir(path);
        std::filesystem::create_directory(documentsDir / NOOBWARRIOR_USERDATA_DIRNAME);
        CoTaskMemFree(path);
        return documentsDir / NOOBWARRIOR_USERDATA_DIRNAME;
#elif defined(__unix__) || defined(__APPLE__)
        std::filesystem::path home_path(getenv("HOME"));
        #if defined(__APPLE__)
            std::filesystem::path user_data_path(home_path / "Library" / "Application Support" / NOOBWARRIOR_USERDATA_DIRNAME);
        #else
            std::filesystem::path user_data_path(home_path / ".local" / "share" / NOOBWARRIOR_USERDATA_DIRNAME);
        #endif
        std::filesystem::create_directory(user_data_path);
        return user_data_path;
#endif
    }
    return GetInstallationDir();
}

void Core::CreateStandardUserDataDirectories() {
#define NW_CREATE(path) std::filesystem::create_directories(GetUserDataDir() / path);
    NW_CREATE(NW_PATH_DATABASES)
    NW_CREATE(NW_PATH_PLUGINS)
    NW_CREATE(NW_PATH_REGISTRY)
    NW_CREATE(NW_PATH_ENGINES)
    NW_CREATE(NW_PATH_ENGINES_ROBLOX)
    NW_CREATE(NW_PATH_ENGINES_ROBLOX_CLIENT)
    NW_CREATE(NW_PATH_ENGINES_ROBLOX_SERVER)
    NW_CREATE(NW_PATH_ENGINES_ROBLOX_STUDIO)
    NW_CREATE(NW_PATH_TEMP)
    NW_CREATE(NW_PATH_TEMP_DOWNLOADS)
    NW_CREATE(NW_PATH_TEMP_DOWNLOADS_ENGINES)
#if defined(__unix__) || defined(__APPLE__)
    NW_CREATE(NW_PATH_WINE)
    NW_CREATE(NW_PATH_WINE_ROOT)
    NW_CREATE(NW_PATH_WINE_PREFIX)
#endif
#undef NW_CREATE
}

int Core::StartServerEmulator(uint16_t port) {
    if (mServerEmulator != nullptr && !StopServerEmulator()) { // try stopping the HTTP server if it's already on
        return -2;
    }

    mServerEmulator = new ServerEmulator(this);
    return mServerEmulator->Start(port);
}

int Core::StopServerEmulator() {
    if (mServerEmulator != nullptr && !mServerEmulator->Stop()) {
        return 0;
    }
    NOOBWARRIOR_FREE_PTR(mServerEmulator)
    return 1;
}

bool Core::IsServerEmulatorRunning() {
    if (mServerEmulator == nullptr)
        return false;
    return mServerEmulator->IsRunning();
}

static size_t WriteToString(void *contents, size_t size, size_t nmemb, void *userp) {
    static_cast<std::string*>(userp)->append(static_cast<char*>(contents), size * nmemb);
    return size * nmemb;
}

int Core::RetrieveIndex(nlohmann::json &index, bool forceRefresh) {
    if (!mIndexDirty && !forceRefresh) {
        index = mIndexJson;
        return CURLE_OK;
    }

    auto url = mConfig->GetKeyValue<const char*>("internet.index");
    if (!url.has_value())
        return -1;

    CURL *curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_URL, url.value());

    std::string jsonStr;
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteToString);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &jsonStr);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK)
        return res;

    index = nlohmann::json::parse(jsonStr);
    mIndexJson = index;
    mIndexDirty = false;
    return res;
}

std::string Core::GetIndexMessage() {
    nlohmann::json index;
    int res = RetrieveIndex(index);
    if (res != CURLE_OK)
        return "";
    if (index.contains("Message"))
        return index["Message"].get<std::string>();
    return "";
}

std::string NoobWarrior::WideCharToUTF8(wchar_t* wc) {
#if defined(_WIN32)
    std::vector<char> buf;
    while (*wc != '\0') {
        int utf8_len = WideCharToMultiByte(CP_UTF8, 0, wc, 1, NULL, 0, NULL, NULL);
        if (utf8_len == 0) {
            wc++;
            continue;
        }
        std::vector<char> utf8_buffer(utf8_len);
        WideCharToMultiByte(CP_UTF8, 0, wc, 1, utf8_buffer.data(), utf8_len, NULL, NULL);
        buf.insert(buf.end(), utf8_buffer.begin(), utf8_buffer.end());
        wc++;
    }
    return std::string(buf.begin(), buf.end());
#else
    return "";
#endif
}