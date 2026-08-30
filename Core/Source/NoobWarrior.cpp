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
// Description: I Don't know why this doesn't have a description but it has one now
// This implements the Core object for noobWarrior, the main portion of the library.
// You should be initializing it if you want to use noobWarrior in your program.
// See the library version in Macros.h
#include <cpr/cpr.h>

#include <NoobWarrior/NoobWarrior.h>
#include <NoobWarrior/HttpServer/Emulator/AuthUtil.h>
#include <NoobWarrior/PluginManager.h>
#include <NoobWarrior/FileSystem/VirtualFileSystem.h>
#include <NoobWarrior/FileSystem/StdFileSystem.h>
#include <NoobWarrior/EmuDb/EmuDb.h>
#include <NoobWarrior/Url.h>
#include <NoobWarrior/Paths.h>

#include <event.h>
#include <event2/thread.h>
#include <filesystem>
#include <sqlite3.h>

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/rsa.h>
#include <openssl/x509v3.h>

#if defined(_WIN32)
#include <windows.h>
#include <winsock2.h>
#include <shlobj.h>
#endif

#if defined(__unix__) || defined(__APPLE__) || defined(__ANDROID__)
#include <spawn.h>
#include <sys/wait.h>
extern char** environ;
#endif

#include <utility>
#include <istream>
#include <thread>

using namespace NoobWarrior;

Core::Core(Init init) :
    mInitResponse(Response::Failed),
    mInit(std::move(init)),
    mLuaState(nullptr),
    mEmuDbManager(this),
    mServerEmulator(nullptr),
    mPluginManager(this)
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

#if OPENSSL_VERSION_NUMBER < 0x30000000L
    // These explicit init calls were removed in OpenSSL 3; initialization is now automatic
    SSL_library_init();
    ERR_load_crypto_strings();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();
#endif

    Out("Core", std::format("noobWarrior is{}in portable mode", mInit.Portable ? " " : " not "));

    if (mInit.AutocreateStandardUserDataDirectories)
        CreateStandardUserDataDirectories();
    
#if defined(_WIN32)
    evthread_use_windows_threads();
#elif defined(__unix__) || defined(__APPLE__) || defined(__ANDROID__)
    evthread_use_pthreads();
#endif

    mEventBase = event_base_new();

    mLuaState = new LuaState(this);
    mLuaState->Open();

    mRegistry = new Registry(GetUserDataDir() / "registry.lua", mLuaState);
    mRbxKeychain = new RbxKeychain(mRegistry);
    mMasterKeychain = new MasterKeychain(mRegistry);
    mEmuKeychain = new EmuKeychain(mRegistry);
    RegistryReturnCode = mRegistry->Open();
    curl_global_init(CURL_GLOBAL_ALL);
    sqlite3_initialize();

    VirtualFileSystem::Response res1 = VirtualFileSystem::New(&mFileVfs, std::filesystem::current_path().root_path());
    VirtualFileSystem::Response res2 = VirtualFileSystem::New(&mInstallDataVfs, GetInstallDataDir());
    VirtualFileSystem::Response res3 = VirtualFileSystem::New(&mUserDataVfs, GetUserDataDir());
    VirtualFileSystem::Response res4 = VirtualFileSystem::New(&mPluginDataVfs, GetUserDataDir() / NW_PATH_PLUGINDATA);
    if (res1 != VirtualFileSystem::Response::Success ||
        res2 != VirtualFileSystem::Response::Success ||
        res3 != VirtualFileSystem::Response::Success ||
        res4 != VirtualFileSystem::Response::Success)
    {
        Out("Core", "One of the virtual file systems failed to initialize. Continuing...");
    }

    // Manifests are parsed first so a plugin:// entry in databases.mounted can resolve, but plugin
    // databases mount after the user's so none can take index 0 from the master database.
    if (mInit.LoadPlugins)
        GetPluginManager()->MountPlugins();

    GetEmuDbManager()->MountDatabases();
    GetEmuDbManager()->MountMasterDbIfNotAlreadyMounted();

    if (mInit.LoadPlugins)
        GetPluginManager()->MountRequiredDatabases();

    mServerEmulator = new ServerEmulator(this);
    mLuaState->set("emu", mServerEmulator);
    mLuaState->set("emu_db_mgr", mEmuDbManager);

    if (mInit.EnableKeychain) {
        GetRbxKeychain()->ReadFromKeychain();
        GetMasterKeychain()->ReadFromKeychain();
        GetEmuKeychain()->ReadFromKeychain();
    }

    if (mInit.AutocreateCert)
        AutocreateCert();

    if (mInit.LoadPlugins)
        GetPluginManager()->ExecutePlugins();

    if (mInit.AutoStartServerEmulator &&
        (GetRegistry() == nullptr || GetRegistry()->GetKeyValue<bool>("emu.autostart").value_or(true)))
        StartServerEmulator();

    mInitResponse = Response::Success;
}

Core::~Core() {
    StopServerEmulator();
    NOOBWARRIOR_FREE_PTR(mServerEmulator)

    GetPluginManager()->UnmountPlugins();
    
    if (mInit.EnableKeychain) {
        AuthResponse res = GetRbxKeychain()->WriteToKeychain();
        if (res != AuthResponse::Success) {
            Out("Core", "GetRbxKeychain()->WriteToKeychain() failed", (int)res);
        }
        if (GetMasterKeychain()->WriteToKeychain() != AuthResponse::Success)
            Out("Core", "GetMasterKeychain()->WriteToKeychain() failed");
        if (GetEmuKeychain()->WriteToKeychain() != AuthResponse::Success)
            Out("Core", "GetEmuKeychain()->WriteToKeychain() failed");
    }

    GetEmuDbManager()->UnmountDatabases();
    sqlite3_shutdown();
    curl_global_cleanup();

    NOOBWARRIOR_FREE_PTR(mRbxKeychain)
    NOOBWARRIOR_FREE_PTR(mMasterKeychain)
    NOOBWARRIOR_FREE_PTR(mEmuKeychain)

    RegistryReturnCode = mRegistry->Close();
    NOOBWARRIOR_FREE_PTR(mRegistry)

    // release any Lua refs held by signals owned directly by Core before the Lua state is torn down.
    // without this, ~LuaSignal runs during member destruction (after mLuaState is gone) and
    // sol::protected_function's destructor calls lua_unref on a dead state.
    mConsoleAddedSignal.DisconnectAll();

    NOOBWARRIOR_FREE_PTR(mLuaState)

    Out("Core", "Freeing event base");
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

static void RunEventLoopTaskCb(evutil_socket_t, short, void *arg) {
    // WTf is this bro im crine
    auto *fn = static_cast<std::function<void()>*>(arg);
    (*fn)();
    delete fn;
}

void Core::RunOnEventLoop(std::function<void()> fn) {
    auto *heapFn = new std::function<void()>(std::move(fn));
    struct timeval tv { 0, 0 };
    if (event_base_once(mEventBase, -1, EV_TIMEOUT, RunEventLoopTaskCb, heapFn, &tv) != 0) {
        (*heapFn)();
        delete heapFn;
    }
}

LuaState *Core::GetLuaState() {
    return mLuaState;
}

Registry *Core::GetRegistry() {
    return mRegistry;
}

EmuDbManager *Core::GetEmuDbManager() {
    return &mEmuDbManager;
}

PluginManager *Core::GetPluginManager() {
    return &mPluginManager;
}

ServerEmulator *Core::GetServerEmulator() {
    return mServerEmulator;
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

std::filesystem::path Core::GetInstallDataDir() const {
    if (!mInit.InstallDataDir.empty()) {
        std::filesystem::path path(mInit.InstallDataDir);
        std::filesystem::create_directories(path);
        return path;
    }

    assert(mInit.ArgCount > 0 && "You must pass in your argc to ArgCount in order to use GetInstallDataDir()");

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
    if (!mInit.UserDataDir.empty()) {
        std::filesystem::path path(mInit.UserDataDir);
        std::filesystem::create_directories(path);
        return path;
    }
    if (!mInit.Portable) {
#if defined(_WIN32)
        WCHAR *path;
        SHGetKnownFolderPath(FOLDERID_LocalAppData, KF_FLAG_DEFAULT, NULL, &path);
        std::filesystem::path baseDir(path);
        std::filesystem::create_directory(baseDir / NOOBWARRIOR_USERDATA_DIRNAME);
        CoTaskMemFree(path);
        return baseDir / NOOBWARRIOR_USERDATA_DIRNAME;
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
    return GetInstallDataDir();
}

void Core::CreateStandardUserDataDirectories() {
#define NW_CREATE(path) std::filesystem::create_directories(GetUserDataDir() / path);
    NW_CREATE(NW_PATH_DATABASES)
    NW_CREATE(NW_PATH_PLUGINS)
    NW_CREATE(NW_PATH_PLUGINDATA)
    NW_CREATE(NW_PATH_ENGINES)
    NW_CREATE(NW_PATH_TEMP)
    NW_CREATE(NW_PATH_TEMP_DOWNLOADS)
    NW_CREATE(NW_PATH_TEMP_DOWNLOADS_ENGINES)
    NW_CREATE(NW_PATH_SSL)
#if (defined(__unix__) || defined(__APPLE__)) && !defined(__ANDROID__)
    NW_CREATE(NW_PATH_WINE)
    NW_CREATE(NW_PATH_WINE_ROOT)
    NW_CREATE(NW_PATH_WINE_PREFIX)
#endif
#undef NW_CREATE
}

VirtualFileSystem* Core::GetFileVfs() {
    return mFileVfs;
}

VirtualFileSystem* Core::GetInstallDataVfs() {
    return mInstallDataVfs;
}

VirtualFileSystem* Core::GetUserDataVfs() {
    return mUserDataVfs;
}

VirtualFileSystem* Core::GetPluginDataVfs() {
    return mPluginDataVfs;
}

int Core::StartServerEmulator() {
    mServerEmulator->Start(mRegistry->GetKeyValue<uint16_t>("emu.http_port").value_or(8080));
    return mServerEmulator->StartSecure(mRegistry->GetKeyValue<uint16_t>("emu.https_port").value_or(53640));
}

int Core::StopServerEmulator() {
    mServerEmulator->Stop();
    return mServerEmulator->StopSecure();
}

void Core::RestartServerEmulator() {
    Out("Core", "Restarting server emulator!");
    /* Can't actually do a hard-reset by deleting it from memory and allocating
       a new one because that would ruin the state set by the Lua plugins */
    StopServerEmulator();
    // NOOBWARRIOR_FREE_PTR(mServerEmulator)
    // mServerEmulator = new ServerEmulator(this);
    // mLuaState->set("emu", mServerEmulator);
    StartServerEmulator();
}

bool Core::IsServerEmulatorRunning() {
    return mServerEmulator->IsRunning();
}

void Core::ConnectToServerEmulator(const std::string &ip, uint16_t port, std::function<void(ServerEmulatorConnectFailReason, std::vector<EngineStartParameters>)> callback, const std::string &sessionToken) {
    std::thread([=, this]() {
        cpr::Response response = cpr::Get(
            cpr::Url{"https://" + ip + ":" + std::to_string(port) + "/emu/v1/running-game-servers"},
            cpr::Timeout{std::chrono::seconds(10)},
            cpr::VerifySsl{false}); // Because players will be logging into server emulators with self-signed certificates most of the time.

        if (response.error.code != cpr::ErrorCode::OK) {
            callback(ServerEmulatorConnectFailReason::TimedOut, {});
            return;
        }

        if (response.status_code == 404) {
            callback(ServerEmulatorConnectFailReason::EndpointNotFound, {});
            return;
        }

        std::string remoteEmulatorId;
        if (auto it = response.header.find(ServerEmulator::kIdentityHeader);
            it != response.header.end()) {
            remoteEmulatorId = it->second;
        }
        const bool connectedToSelf = mServerEmulator != nullptr &&
            !remoteEmulatorId.empty() &&
            remoteEmulatorId == mServerEmulator->GetInstanceId();
        if (connectedToSelf) {
            Out("ConnectToServerEmulator",
                "Recognized this emulator through {}:{}; using local request handling",
                ip, port);
        }

        std::vector<EngineStartParameters> paramsList;
        nlohmann::json json;
        try {
            json = nlohmann::json::parse(response.text);
        } catch (nlohmann::json::exception &e) {
            callback(ServerEmulatorConnectFailReason::JsonFailed, {});
            return;
        }
        Out("ConnectToServerEmulator", "{}", json.dump());

        for (auto &obj : json.items()) {
            nlohmann::json gameServerArray = obj.value();
            if (!gameServerArray.contains("Port")) {
                Out("ConnectToServerEmulator", "WARNING! Port not found in JSON object from endpoint /emu/v1/running-game-servers. Skipping...");
                continue;
            }

            EngineStartParameters params {};

            if (gameServerArray.contains("Ip") && gameServerArray["Ip"].is_string()) {
                params.Ip = gameServerArray["Ip"].get<std::string>();
            } else {
                Out("ConnectToServerEmulator", "WARNING! Invalid IP address found in JSON object from endpoint /emu/v1/running-game-servers. Skipping...");
                continue;
            }

            try {
                params.Port = gameServerArray["Port"].get<uint16_t>();
            } catch (nlohmann::json::exception &e) {
                Out("ConnectToServerEmulator", "WARNING! Invalid port found in JSON object from endpoint /emu/v1/running-game-servers. Skipping...");
                continue;
            }

            if (gameServerArray.contains("EngineVersion") && gameServerArray["EngineVersion"].is_string()) {
                params.Engine.Version = gameServerArray["EngineVersion"].get<std::string>();
            } else {
                Out("ConnectToServerEmulator", "WARNING! Invalid engine version found in JSON object from endpoint /emu/v1/running-game-servers. Skipping...");
                continue;
            }

            if (gameServerArray.contains("EngineType") && gameServerArray["EngineType"].is_string()) {
                params.Engine.Type = gameServerArray["EngineType"].get<std::string>().compare("Roblox") == 0 ? EngineType::Roblox : EngineType::Roblox; // yea kind of useless
            } else {
                Out("ConnectToServerEmulator", "WARNING! Invalid engine type found in JSON object from endpoint /emu/v1/running-game-servers. Skipping...");
                continue;
            }
            
            if (gameServerArray.contains("PlaceId") && gameServerArray["PlaceId"].is_number_integer()) {
                params.PlaceId = gameServerArray["PlaceId"].get<int64_t>();
            }

            params.Engine.Side = EngineSide::Client;
            
            if (auto resolved = ResolveInstalledEngine(params.Engine)) {
                resolved->Side = EngineSide::Client;
                params.Engine = *resolved;
            }

            if (!sessionToken.empty())
                params.SessionToken = sessionToken; // the account we logged in as (local launch ticket)
            if (!IsLoopbackOrEmpty(ip) && !connectedToSelf) {
                params.RemoteEmulatorHost = ip;
                params.RemoteEmulatorPort = port;
                if (!sessionToken.empty())
                    params.RemoteEmulatorSessionToken = sessionToken; // forwarded to the host (remote auth)
            }
            paramsList.push_back(params);
        }

        callback(ServerEmulatorConnectFailReason::None, paramsList);
    }).detach();
}

std::optional<std::string> Core::LoginToRemoteHost(const std::string &ip, uint16_t port,
                                                   const std::string &username, const std::string &password) {
    // The host's /v1/login sets a .LOGINSESSION cookie and 302-redirects on success. Don't follow the
    // redirect; we only want the Set-Cookie token to forward on join.
    cpr::Response response = cpr::Post(
        cpr::Url{"https://" + ip + ":" + std::to_string(port) + "/v1/login"},
        cpr::Payload{{"username", username}, {"password", password}},
        cpr::Redirect{false},
        cpr::Timeout{std::chrono::seconds(10)},
        cpr::VerifySsl{false}); // hosts typically present self-signed certs

    if (response.error.code != cpr::ErrorCode::OK) {
        Out("LoginToRemoteHost", "Login to {}:{} failed: {}", ip, port, response.error.message);
        return std::nullopt;
    }
    if (response.status_code >= 400) {
        Out("LoginToRemoteHost", "Login to {}:{} rejected (HTTP {})", ip, port, static_cast<long>(response.status_code));
        return std::nullopt;
    }

    // cpr lower-cases header names in its case-insensitive map; Set-Cookie may appear once per cookie.
    auto range = response.header.equal_range("Set-Cookie");
    for (auto it = range.first; it != range.second; ++it) {
        std::string token = AuthUtil::ExtractCookieValue(it->second.c_str(), ".LOGINSESSION");
        if (!token.empty() && token != "deleted") {
            // Cache this master-auth login so a repeat connect to this host can skip the prompt.
            CacheRemoteHostLogin(ip, port, username, token);
            return token;
        }
    }
    Out("LoginToRemoteHost", "Login to {}:{} returned no .LOGINSESSION cookie", ip, port);
    return std::nullopt;
}

static std::string HostKey(const std::string &ip, uint16_t port) {
    return ip + ":" + std::to_string(port);
}

void Core::CacheRemoteHostLogin(const std::string &ip, uint16_t port, const std::string &username,
                                const std::string &token) {
    if (token.empty())
        return;
    // Keyed by host (ip:port) so one cached login is kept per emulator; the username is display-only.
    EmuKeychain *kc = GetEmuKeychain();
    Account acc {};
    acc.Id = -1;
    acc.Name = HostKey(ip, port);
    acc.DisplayName = username;
    acc.Token = token;
    acc.Url = HostKey(ip, port);
    kc->AddOrUpdateAccount(acc);
    if (mInit.EnableKeychain)
        kc->WriteToKeychain();
}

std::string Core::GetCachedRemoteHostToken(const std::string &ip, uint16_t port) {
    std::string key = HostKey(ip, port);
    for (Account &a : GetEmuKeychain()->GetAccounts())
        if (a.Name == key)
            return a.Token;
    return "";
}

void Core::ForgetRemoteHostLogin(const std::string &ip, uint16_t port) {
    std::string key = HostKey(ip, port);
    EmuKeychain *kc = GetEmuKeychain();
    std::vector<Account> &accounts = kc->GetAccounts();
    for (int i = 0; i < static_cast<int>(accounts.size()); i++) {
        if (accounts[i].Name == key) {
            kc->RemoveAccount(i);
            if (mInit.EnableKeychain)
                kc->WriteToKeychain();
            return;
        }
    }
}

bool Core::ValidateRemoteHostSession(const std::string &ip, uint16_t port, const std::string &token) {
    if (token.empty())
        return false;
    cpr::Response res = cpr::Get(
        cpr::Url{"https://" + ip + ":" + std::to_string(port) + "/emu/v1/session-check"},
        cpr::Header{{"Cookie", ".LOGINSESSION=" + token}},
        cpr::Timeout{std::chrono::milliseconds(5000)},
        cpr::VerifySsl{false});
    return res.error.code == cpr::ErrorCode::OK && res.status_code == 200;
}

static std::string StripTrailingSlash(std::string url) {
    while (!url.empty() && url.back() == '/')
        url.pop_back();
    return url;
}

bool Core::LoginToMaster(const std::string &masterUrl, const std::string &username, const std::string &password) {
    std::string base = StripTrailingSlash(masterUrl);
    if (base.empty())
        return false;

    cpr::Response login = cpr::Post(
        cpr::Url{base + "/v1/login"},
        cpr::Payload{{"username", username}, {"password", password}},
        cpr::Redirect{false},
        cpr::Timeout{std::chrono::seconds(10)},
        cpr::VerifySsl{false});
    if (login.error.code != cpr::ErrorCode::OK || login.status_code >= 400) {
        Out("LoginToMaster", "Login to {} failed (HTTP {})", base, static_cast<long>(login.status_code));
        return false;
    }

    std::string token;
    auto range = login.header.equal_range("Set-Cookie");
    for (auto it = range.first; it != range.second; ++it) {
        std::string t = AuthUtil::ExtractCookieValue(it->second.c_str(), ".LOGINSESSION");
        if (!t.empty() && t != "deleted") {
            token = t;
            break;
        }
    }
    if (token.empty()) {
        Out("LoginToMaster", "Login to {} returned no .LOGINSESSION cookie", base);
        return false;
    }

    // Resolve the online identity (username@domain, OnlineUserId) from the master's federation profile.
    cpr::Response profile = cpr::Get(
        cpr::Url{base + "/fed/v1/users/" + cpr::util::urlEncode(username)},
        cpr::Timeout{std::chrono::seconds(10)},
        cpr::VerifySsl{false});
    std::string identity, displayName;
    int64_t userId = 0;
    if (profile.error.code == cpr::ErrorCode::OK && profile.status_code < 400) {
        try {
            nlohmann::json p = nlohmann::json::parse(profile.text);
            identity = p.value("Identity", "");
            displayName = p.value("DisplayName", "");
            userId = p.value("UserId", static_cast<int64_t>(0));
        } catch (nlohmann::json::exception &) {}
    }
    if (identity.empty())
        identity = username; // fall back to the bare name if the master has no federation profile

    // store this in the master keychain and make it the active one
    MasterKeychain *kc = GetMasterKeychain();
    Account acc {};
    acc.Id = userId;
    acc.Name = identity;
    acc.DisplayName = displayName.empty() ? username : displayName;
    acc.Token = token;
    acc.Url = base;
    Account *stored = kc->AddOrUpdateAccount(acc);
    kc->SetActiveAccount(stored);
    if (mInit.EnableKeychain)
        kc->WriteToKeychain();
    Out("LoginToMaster", "Signed in as {} on {}", identity, base);
    return true;
}

void Core::LogoutFromMaster() {
    MasterKeychain *kc = GetMasterKeychain();
    kc->SetActiveAccount(nullptr);
    if (mInit.EnableKeychain)
        kc->WriteToKeychain();
}

std::optional<std::string> Core::MintJoinVoucher(const std::string &masterUrl, const std::string &sessionToken,
                                                 const std::string &targetMasterUrl, std::string *outError) {
    auto fail = [&](const std::string &msg) -> std::optional<std::string> {
        if (outError) *outError = msg;
        Out("MintJoinVoucher", "{}", msg);
        return std::nullopt;
    };

    std::string base = StripTrailingSlash(masterUrl);
    if (base.empty())        return fail("You aren't signed in to a master server.");
    if (sessionToken.empty()) return fail("Your master session is empty; sign in again.");
    if (targetMasterUrl.empty()) return fail("This server didn't advertise a master URL (authMasterUrl).");

    cpr::Response res = cpr::Post(
        cpr::Url{base + "/v1/join/mint-voucher"},
        cpr::Header{{"Cookie", ".LOGINSESSION=" + sessionToken}},
        cpr::Payload{{"target_url", targetMasterUrl}},
        cpr::Timeout{std::chrono::seconds(10)},
        cpr::VerifySsl{false});
    if (res.error.code != cpr::ErrorCode::OK)
        return fail("Couldn't reach your master server " + base + ": " + res.error.message);
    if (res.status_code >= 400) {
        std::string serverMsg;
        try { serverMsg = nlohmann::json::parse(res.text).value("Error", ""); } catch (...) {}
        return fail("Master " + base + " refused (HTTP " + std::to_string(res.status_code) + ", target=" +
                    targetMasterUrl + ")" + (serverMsg.empty() ? "" : ": " + serverMsg));
    }

    std::string actionId, identity, body, signature;
    try {
        nlohmann::json j = nlohmann::json::parse(res.text);
        actionId = j.value("actionId", "");
        identity = j.value("identity", "");
        body = j.value("body", "");
        signature = j.value("signature", "");
    } catch (nlohmann::json::exception &) {
        return fail("Master returned an unreadable voucher response.");
    }
    if (actionId.empty() || identity.empty() || body.empty())
        return fail("Master returned an incomplete voucher.");
    if (signature.empty())
        return fail("Master returned an unsigned voucher (update your master server)");

    // fedvoucher.<b64url identity>.<actionId>.<b64url body>.<signature hex>
    return "fedvoucher." + AuthUtil::Base64UrlEncode(identity) + "." + actionId + "." +
           AuthUtil::Base64UrlEncode(body) + "." + signature;
}

std::string Core::GetWinePath(const std::filesystem::path &path) {
#if (defined(__unix__) || defined(__APPLE__)) && !defined(__ANDROID__)
    std::filesystem::path absPath = std::filesystem::absolute(path);
    std::string str = absPath.generic_string();
    std::replace(str.begin(), str.end(), '/', '\\');
    return "Z:" + str;
#endif
    return std::filesystem::absolute(path).string();
}

void Core::AutocreateCert() {
    std::filesystem::path certPath = GetUserDataDir() / NW_PATH_SSL / "cert.pem";
    std::filesystem::path keyPath = GetUserDataDir() / NW_PATH_SSL / "key.pem";

    bool needsRegen = true;
    if (std::filesystem::exists(certPath) && std::filesystem::exists(keyPath)) {
#if defined(_WIN32)
        FILE *existing = _wfopen(certPath.c_str(), L"rb");
#else
        FILE *existing = fopen(certPath.c_str(), "rb");
#endif
        if (existing) {
            if (X509 *cert = PEM_read_X509(existing, nullptr, nullptr, nullptr)) {
                needsRegen = X509_get_ext_by_NID(cert, NID_subject_alt_name, -1) < 0;
                X509_free(cert);
            }
            fclose(existing);
        }
    }
    if (!needsRegen) {
        Out("AutocreateCert", "Existing certificate has SANs, keeping it.");
        return;
    }

    EVP_PKEY *pkey = NULL;
    X509 *x509 = NULL;
    FILE *fcrt = NULL, *fkey = NULL;

    pkey = EVP_RSA_gen(2048);

    x509 = X509_new();
    X509_set_version(x509, 2);
    ASN1_INTEGER_set(X509_get_serialNumber(x509), 1);
    X509_gmtime_adj(X509_getm_notBefore(x509), 0);
    X509_gmtime_adj(X509_getm_notAfter(x509), 315576000L); // 10 years
    X509_set_pubkey(x509, pkey);

    X509_NAME *name = X509_get_subject_name(x509);
    X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC, (unsigned char*)"localhost", -1, -1, 0);
    X509_set_issuer_name(x509, name);
    
    {
        X509V3_CTX ctx;
        X509V3_set_ctx_nodb(&ctx);
        X509V3_set_ctx(&ctx, x509, x509, nullptr, nullptr, 0);
        const char *san = "DNS:localhost,DNS:roblox.com,DNS:*.roblox.com,DNS:*.api.roblox.com,"
                          "DNS:rbxcdn.com,DNS:*.rbxcdn.com,DNS:robloxlabs.com,DNS:*.robloxlabs.com,"
                          "IP:127.0.0.1";
        if (X509_EXTENSION *ext = X509V3_EXT_conf_nid(nullptr, &ctx, NID_subject_alt_name, san)) {
            X509_add_ext(x509, ext, -1);
            X509_EXTENSION_free(ext);
        }
        if (X509_EXTENSION *bc = X509V3_EXT_conf_nid(nullptr, &ctx, NID_basic_constraints, "critical,CA:TRUE")) {
            X509_add_ext(x509, bc, -1);
            X509_EXTENSION_free(bc);
        }
    }

    X509_sign(x509, pkey, EVP_sha256());

#if defined(_WIN32)
    fcrt = _wfopen(certPath.c_str(), L"wb");
#else
    fcrt = fopen(certPath.c_str(), "wb");
#endif
    PEM_write_X509(fcrt, x509);
    fclose(fcrt);

#if defined(_WIN32)
    fkey = _wfopen(keyPath.c_str(), L"wb");
#else
    fkey = fopen(keyPath.c_str(), "wb");
#endif
    PEM_write_PrivateKey(fkey, pkey, NULL, NULL, 0, NULL, NULL);
    fclose(fkey);

    X509_free(x509);
    EVP_PKEY_free(pkey);
    Out("AutocreateCert", "Generated key.pem and cert.pem");
}

Backup::Process* Core::CreateBackupProcess(const Backup::ProcessOptions options) {
    return new Backup::Process(this, options);
}

LuaSignal* Core::GetConsoleAddedSignal() {
    return &mConsoleAddedSignal;
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

bool NoobWarrior::IsLoopbackOrEmpty(const std::string &ip) {
    return ip.empty()
        || ip == "localhost"
        || ip == "::1"
        || ip == "::ffff:127.0.0.1"
        || ip.rfind("127.", 0) == 0;
}
