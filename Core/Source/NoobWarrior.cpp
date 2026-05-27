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
#include <event2/thread.h>
#include <sqlite3.h>

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/x509v3.h>

#if defined(_WIN32)
#include <windows.h>
#include <winsock2.h>
#include <shlobj.h>
#endif

#if defined(__unix__) || defined(__APPLE__)
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

    SSL_library_init();
    ERR_load_crypto_strings();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();

    Out("Core", std::format("noobWarrior is{}in portable mode", mInit.Portable ? " " : " not "));

    if (mInit.AutocreateStandardUserDataDirectories)
        CreateStandardUserDataDirectories();
    
#if defined(_WIN32)
    evthread_use_windows_threads();
#elif defined(__unix__) || defined(__APPLE__)
    evthread_use_pthreads();
#endif

    mEventBase = event_base_new();

    mLuaState = new LuaState(this);
    mLuaState->Open();

    mRegistry = new Registry(GetUserDataDir() / "registry.lua", mLuaState);
    mRbxKeychain = new RbxKeychain(mRegistry);
    RegistryReturnCode = mRegistry->Open();
    curl_global_init(CURL_GLOBAL_ALL);
    sqlite3_initialize();

    GetEmuDbManager()->MountDatabases();
    GetEmuDbManager()->MountMasterDbIfNotAlreadyMounted();

    mServerEmulator = new ServerEmulator(this);
    mLuaState->set("emu", mServerEmulator);
    mLuaState->set("emu_db_mgr", mEmuDbManager);

    if (mInit.EnableKeychain)
        GetRbxKeychain()->ReadFromKeychain();

    if (mInit.AutocreateCert)
        AutocreateCert();

    if (mInit.LoadPlugins)
        GetPluginManager()->MountPlugins();

    if (mInit.AutoStartServerEmulator)
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
    }

    GetEmuDbManager()->UnmountDatabases();
    sqlite3_shutdown();
    curl_global_cleanup();

    NOOBWARRIOR_FREE_PTR(mRbxKeychain)

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
    return GetInstallationDir();
}

void Core::CreateStandardUserDataDirectories() {
#define NW_CREATE(path) std::filesystem::create_directories(GetUserDataDir() / path);
    NW_CREATE(NW_PATH_DATABASES)
    NW_CREATE(NW_PATH_PLUGINS)
    NW_CREATE(NW_PATH_ENGINES)
    NW_CREATE(NW_PATH_TEMP)
    NW_CREATE(NW_PATH_TEMP_DOWNLOADS)
    NW_CREATE(NW_PATH_TEMP_DOWNLOADS_ENGINES)
    NW_CREATE(NW_PATH_SSL)
#if defined(__unix__) || defined(__APPLE__)
    NW_CREATE(NW_PATH_WINE)
    NW_CREATE(NW_PATH_WINE_ROOT)
    NW_CREATE(NW_PATH_WINE_PREFIX)
#endif
#undef NW_CREATE
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

static size_t WriteToString(void *contents, size_t size, size_t nmemb, void *userp) {
    static_cast<std::string*>(userp)->append(static_cast<char*>(contents), size * nmemb);
    return size * nmemb;
}

void Core::ConnectToServerEmulator(const std::string &ip, uint16_t port, std::function<void(ServerEmulatorConnectFailReason, std::vector<EngineStartParameters>)> callback) {
    std::thread([=]() {
        NetClient client;
        HttpRequest req;
        req.Url = "https://" + ip + ":" + std::to_string(port) + "/v1/running-game-servers";
        req.TimeoutSeconds = 10;
        req.IgnoreTLSVerification = true; // Because players will be logging into server emulators with self-signed certificates most of the time.
        HttpResponse response = client.Fetch(req);

        if (response.Code != CURLE_OK) {
            callback(ServerEmulatorConnectFailReason::TimedOut, {});
            return;
        }

        if (response.HttpStatus == 404) {
            callback(ServerEmulatorConnectFailReason::EndpointNotFound, {});
            return;
        }

        std::vector<EngineStartParameters> paramsList;
        nlohmann::json json;
        try {
            json = nlohmann::json::parse(response.Body);
        } catch (nlohmann::json::exception &e) {
            callback(ServerEmulatorConnectFailReason::JsonFailed, {});
            return;
        }
        Out("ConnectToServerEmulator", "{}", json.dump());

        for (auto &obj : json.items()) {
            nlohmann::json gameServerArray = obj.value();
            if (!gameServerArray.contains("Port")) {
                Out("ConnectToServerEmulator", "WARNING! Port not found in JSON object from endpoint /v1/running-game-servers. Skipping...");
                continue;
            }

            EngineStartParameters params {};

            if (gameServerArray.contains("Ip") && gameServerArray["Ip"].is_string()) {
                params.Ip = gameServerArray["Ip"].get<std::string>();
            } else {
                Out("ConnectToServerEmulator", "WARNING! Invalid IP address found in JSON object from endpoint /v1/running-game-servers. Skipping...");
                continue;
            }

            try {
                params.Port = gameServerArray["Port"].get<uint16_t>();
            } catch (nlohmann::json::exception &e) {
                Out("ConnectToServerEmulator", "WARNING! Invalid port found in JSON object from endpoint /v1/running-game-servers. Skipping...");
                continue;
            }

            if (gameServerArray.contains("EngineVersion") && gameServerArray["EngineVersion"].is_string()) {
                params.Engine.Version = gameServerArray["EngineVersion"].get<std::string>();
            } else {
                Out("ConnectToServerEmulator", "WARNING! Invalid engine version found in JSON object from endpoint /v1/running-game-servers. Skipping...");
                continue;
            }

            if (gameServerArray.contains("EngineType") && gameServerArray["EngineType"].is_string()) {
                params.Engine.Type = gameServerArray["EngineType"].get<std::string>().compare("Roblox") == 0 ? EngineType::Roblox : EngineType::Roblox; // yea kind of useless
            } else {
                Out("ConnectToServerEmulator", "WARNING! Invalid engine type found in JSON object from endpoint /v1/running-game-servers. Skipping...");
                continue;
            }
            
            if (gameServerArray.contains("PlaceId") && gameServerArray["PlaceId"].is_number_integer()) {
                params.PlaceId = gameServerArray["PlaceId"].get<int64_t>();
            }

            params.Engine.Side = EngineSide::Client;
            paramsList.push_back(params);
        }

        callback(ServerEmulatorConnectFailReason::None, paramsList);
    }).detach();
}

std::string Core::GetWinePath(const std::filesystem::path &path) {
#if defined(__unix__) || defined(__APPLE__)
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
    RSA *rsa = NULL;
    X509 *x509 = NULL;
    FILE *fcrt = NULL, *fkey = NULL;

    pkey = EVP_PKEY_new();
    rsa = RSA_generate_key(2048, RSA_F4, NULL, NULL);
    EVP_PKEY_assign_RSA(pkey, rsa);

    x509 = X509_new();
    X509_set_version(x509, 2); // X.509 v3 — required for extensions below
    ASN1_INTEGER_set(X509_get_serialNumber(x509), 1);
    X509_gmtime_adj(X509_get_notBefore(x509), 0);
    X509_gmtime_adj(X509_get_notAfter(x509), 315576000L); // 10 years
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
