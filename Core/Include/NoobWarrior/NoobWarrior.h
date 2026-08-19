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
// File: NoobWarrior.h
// Started by: Hattozo
// Started on: 12/15/2024
// Description: Core of the library
#pragma once
#include "Macros.h"
#include "Log.h"
#include "Paths.h"
#include "Lua/LuaState.h"
#include "EmuDb/EmuDb.h"
#include "Registry.h"
#include "NoobWarrior/Keychain/RbxKeychain.h"
#include "PluginManager.h"
#include "EmuDb/EmuDb.h"
#include "RccServiceManager.h"
#include "Engine.h"
#include "HttpServer/Emulator/ServerEmulator.h"
#include "Roblox/FileFormat/RobloxFile.h"
#include "Roblox/FileFormat/BinaryFormat/BinaryRobloxFile.h"
#include "Roblox/Api/Asset.h"
#include "Roblox/DataType/Color3.h"
#include "Roblox/DataType/BrickColor.h"
#include "Engine.h"
#include "Keychain/MasterKeychain.h"
#include "Keychain/EmuKeychain.h"
#include "Keychain/RbxKeychain.h"
#include "Url.h"
#include "FileSystem/VirtualFileSystem.h"
#include "Backup.h"
#include "Console/Console.h"
#include "Console/Command/Command.h"
#include "Lua/LuaSignal.h"

#include <event.h>
#include <lua.hpp>
#include <curl/curl.h>

#include <string_view>

#include <functional>
#include <vector>
#include <map>
#include <memory>

namespace NoobWarrior {
struct Init {
    int         ArgCount        {};
    char**      ArgVec          {};
    bool        AutocreateStandardUserDataDirectories { true };
    bool        Portable                              { true };
    bool        EnableKeychain                        { true };
    bool        AutocreateCert                        { true };
    bool        LoadPlugins                           { true };
    bool        AutoStartServerEmulator               { true };
    /* noobWarrior will try to find its installation files in this directory name, relative to the executable location
       If you are not using noobWarrior as a simple library, you should probably just set this to an empty string
       so that it uses the root directory of the executable. */
    std::string InstallDataRelativePath               { "noobwarrior" };
    /* Absolute path to the user-data directory. If non-empty, GetUserDataDir() returns this verbatim
       instead of deriving a path from the OS. Required on platforms where argv[0] isn't meaningful
       and there's no $HOME (e.g. Android, where Java must hand us the app's filesDir). */
    std::string UserDataDir                           {};
    /* Absolute path to the install-data directory. If non-empty, GetInstallDataDir() returns this
       verbatim and skips the argv[0] derivation. Required on platforms with no argv[0] (Android). */
    std::string InstallDataDir                        {};
};

enum class AssetFileNameStyle {
    Raw, // File name is retrieved from the server that is hosting the file. In this case you will get a MD5 hash, since that is how Roblox indexes files.
    AssetId,
    AssetName
};

enum AssetFlags {
    DA_PRESERVE_AUTHORS = 1 << 0, // Sets the Authors metadata to be the name of the Asset's creator
    DA_PRESERVE_DATECREATED = 1 << 1, // Sets the Date Created metadata to be the Asset's time of creation.
    DA_PRESERVE_DATEMODIFIED = 1 << 2, // Sets the Date Modified metadata to be the time the Asset was last updated.
    DA_FIND_ASSET_IDS_IN_MODEL = 1 << 3, // If the Asset is a Model or a Place, it parses the file and checks for any asset URLs/IDs located within scripts and any properties.
    DA_DISABLE_TEMP_DIR = 1 << 4 // Prevents noobWarrior from temporarily storing the downloaded assets in a temp directory, instead it will just always store them in the output directory from the get-go.
};

struct DownloadAssetArgs {
    int                     Flags {};
    AssetFileNameStyle      FileNameStyle {};
    std::ostream*           OutStream {};
    std::string             OutDir {};
    std::vector<int64_t>    Id {};
    std::vector<uint64_t>   Version {};
};

struct BackupArgs {
    int                                         Flags {};
    AssetFileNameStyle                          FileNameStyle {};
    std::ostream*                               OutStream {};
    std::vector<std::pair<int64_t, int64_t>>    IdAndVersion {};
};

enum class ServerEmulatorConnectFailReason {
    None,
    Unknown,
    TimedOut,
    EndpointNotFound,
    JsonFailed
};

std::string WideCharToUTF8(wchar_t* wc);
bool IsLoopbackOrEmpty(const std::string &ip);
int ParseEraVersion(const std::string &version);
class Core {
public:
    enum class Response {
        Failed,
        Success,
        RegistryFailed
    };

    Core(Init init = {});
    ~Core();

    bool Fail();

    /**
     * @brief Must be called in order to poll async I/O events, like for HTTP requests.
     * The HTTP server will not work without this.
     */
    int ProcessEvents(bool block = false);

    RegistryResponse RegistryReturnCode;

    event_base *GetEventBase();

    /**
     * @brief Schedules fn to run on the event-loop thread (the thread that calls ProcessEvents).
     * Safe to call from any thread. Used by background workers to hand results back to the
     * thread that owns libevent/SQLite state (e.g. to send an HTTP reply after an off-thread fetch).
     */
    void RunOnEventLoop(std::function<void()> fn);

    LuaState *GetLuaState();
    Registry *GetRegistry();
    EmuDbManager *GetEmuDbManager();
    PluginManager *GetPluginManager();

    ServerEmulator *GetServerEmulator();

    MasterKeychain* GetMasterKeychain();
    EmuKeychain* GetEmuKeychain();
    RbxKeychain* GetRbxKeychain();

    const Init& GetInit();

    std::filesystem::path GetInstallDataDir() const;

    /**
     * @brief Warning: Any call to this function will automatically create a directory if it does not exist.
     */
    std::filesystem::path GetUserDataDir();

    void CreateStandardUserDataDirectories();

    VirtualFileSystem* GetFileVfs();
    VirtualFileSystem* GetInstallDataVfs();
    VirtualFileSystem* GetUserDataVfs();
    VirtualFileSystem* GetPluginDataVfs();

    int StartServerEmulator();
    int StopServerEmulator();
    void RestartServerEmulator();
    bool IsServerEmulatorRunning();
    
    /**
     * @brief Lets you download a batch of Roblox assets to a directory.
     */
    int DownloadAssets(DownloadAssetArgs args);
    // std::future<char*> DownloadAssetAsync(DownloadAssetArgs);

    int GetAssetDetails(int64_t id, Roblox::AssetDetails *details);

    //////////////// Engine Related Functions ////////////////
    std::vector<Engine> GetInstalledEngines();
    std::vector<Engine> GetAllEngines();
    std::filesystem::path GetEngineDirectory(const Engine &client);
    // Resolve an engine specification (side + version, hash optional) to the best-matching installed engine,
    // using the same matcher as GetEngineDirectory. Lets callers adopt the real installed version
    // instead of a requested one that may not exist as an installed build. std::nullopt if none match.
    std::optional<Engine> ResolveInstalledEngine(const Engine &want);

    /* This searches your engine manifest file, finds engines from master servers you have added, and compiles a list of usable engines */
    void DiscoverEngines();

    bool IsEngineInManifest(const Engine &client);

    /* Updates local Wine installation in user data directory and also pulls in DXVK and WebView2
     * The callback needs to return a boolean. If it returns true, the update function continues. If it
     * returns false, any downloads are aborted and the function is cancelled.
     * The 3 parameters represent the current state of the function as it is progressing. 
     * WineUpdateState is the stage (downloading, installing), the 2nd double parameter is a
     * decimal percentage describing how much of the current stage is complete. */
    void UpdateWine(std::function<bool(WineUpdateState, double)> callback);
    void DownloadAndInstallEngine(const Engine &client, std::function<void()> callback);
    EngineLaunchResponse LaunchEngine(
        EngineStartParameters params,
        const EngineLaunchProgressCallback& progressCallback = {});

    /* This is a two-part flow.
     * Whenever callback is fired, the first parameter indicates if the request succeeded or not, and the second parameter is the response if successful.
     * The vector part contains start parameters for each server containing their respective IP addresses, ports, and what version of Roblox they use.
     * This response should be passed to LaunchEngine, which will promptly start the game.
     */
    void ConnectToServerEmulator(const std::string &ip, uint16_t port, std::function<void(ServerEmulatorConnectFailReason, std::vector<EngineStartParameters>)> callback, const std::string &sessionToken = "");

    /* Logs into a remote host emulator's website (POST /v1/login) and returns the .LOGINSESSION token
     * from the Set-Cookie response, or std::nullopt on failure. Used before ConnectToServerEmulator
     * when the host has master authentication enabled, so the resulting token can be forwarded on join.
     * On success the session is cached in the Emu keychain (see CacheRemoteHostLogin). */
    std::optional<std::string> LoginToRemoteHost(const std::string &ip, uint16_t port,
                                                 const std::string &username, const std::string &password);

    /* Cached master-auth logins for server emulators, so a repeat connect can skip the login prompt.
     * Cached per host (ip:port) in the Emu keychain. GetCachedRemoteHostToken returns "" when none;
     * ValidateRemoteHostSession confirms a cached token is still live (drop it and re-prompt if not). */
    void CacheRemoteHostLogin(const std::string &ip, uint16_t port, const std::string &username,
                              const std::string &token);
    std::string GetCachedRemoteHostToken(const std::string &ip, uint16_t port);
    void ForgetRemoteHostLogin(const std::string &ip, uint16_t port);
    bool ValidateRemoteHostSession(const std::string &ip, uint16_t port, const std::string &token);

    /* Signs in to a home master server (POST <masterUrl>/v1/login), resolves the online identity via
     * <masterUrl>/fed/v1/users/<name>, and stores it as the active account in the Master keychain.
     * Returns true on success. This active identity is reused to join federated slave servers. */
    bool LoginToMaster(const std::string &masterUrl, const std::string &username, const std::string &password);

    /* Signs out of the active master identity (Master keychain), keeping it cached for re-activation. */
    void LogoutFromMaster();

    /* Mints a one-time join voucher on the home master (POST <masterUrl>/v1/join/mint-voucher, authed
     * by sessionToken) bound to the target slave's master (targetMasterUrl). Returns a compact
     * "fedvoucher." credential string to forward on join, or std::nullopt on failure. */
    std::optional<std::string> MintJoinVoucher(const std::string &masterUrl, const std::string &sessionToken,
                                               const std::string &targetMasterUrl, std::string *outError = nullptr);

    Backup::Process* CreateBackupProcess(const Backup::ProcessOptions options);

    LuaSignal* GetConsoleAddedSignal();
protected:
    std::string GetWinePath(const std::filesystem::path &path);
    bool WriteGameServerConfig(const std::filesystem::path &engineDir, const EngineStartParameters &params);
    bool WriteServerRbxl(int64_t placeId, int version = 0,
                         const StudioServerBootstrap& serverBootstrap = {},
                         const EngineLaunchProgressCallback& progressCallback = {},
                         std::string_view studioFingerprint = {});
    std::filesystem::path FindEngineExecutable(const std::filesystem::path &engineDir);
    EngineLaunchResponse LaunchProcessThroughInjector(EngineArchitecture arch, const std::filesystem::path &filePath, EngineStartParameters params);
    void AutocreateCert();
private:
    Response                        mInitResponse;

    event_base*                     mEventBase;

    Init                            mInit;
    LuaState*                       mLuaState;
    Registry*                       mRegistry;
    EmuDbManager                    mEmuDbManager;
    PluginManager                   mPluginManager;

    VirtualFileSystem*              mFileVfs;
    VirtualFileSystem*              mInstallDataVfs;
    VirtualFileSystem*              mUserDataVfs;
    VirtualFileSystem*              mPluginDataVfs;

    ServerEmulator*                 mServerEmulator;

    MasterKeychain*                 mMasterKeychain;
    EmuKeychain*                    mEmuKeychain;
    RbxKeychain*                    mRbxKeychain;
    std::vector<RccServiceManager*> mRccServiceManagers;

    LuaSignal                       mConsoleAddedSignal;
};
}
