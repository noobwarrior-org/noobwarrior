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
#include "Lua/LuaState.h"
#include "EmuDb/EmuDb.h"
#include "Config.h"
#include "NoobWarrior/Keychain/RbxKeychain.h"
#include "PluginManager.h"
#include "EmuDb/EmuDb.h"
#include "RccServiceManager.h"
#include "Engine.h"
#include "HttpServer/Emulator/ServerEmulator.h"
#include "Roblox/FileFormat/RobloxFile.h"
#include "Roblox/Api/Asset.h"
#include "Engine.h"
#include "NetClient.h"
#include "Keychain/MasterKeychain.h"
#include "Keychain/EmuKeychain.h"
#include "Keychain/RbxKeychain.h"
#include "Url.h"

#include <lua.hpp>
#include <curl/curl.h>

#include <vector>

namespace NoobWarrior {
struct Init {
    int         ArgCount        {};
    char**      ArgVec          {};
    bool        Portable        { true };
    bool        EnableKeychain  { true };
    bool        LoadPlugins     { true };
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

std::string WideCharToUTF8(wchar_t* wc);

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

    ConfigResponse ConfigReturnCode;

    event_base *GetEventBase();
    LuaState *GetLuaState();
    Config *GetConfig();
    EmuDbManager *GetEmuDbManager();
    PluginManager *GetPluginManager();

    MasterKeychain* GetMasterKeychain();
    EmuKeychain* GetEmuKeychain();
    RbxKeychain* GetRbxKeychain();

    std::filesystem::path GetInstallationDir() const;

    /**
     * @brief Warning: Any call to this function will automatically create a directory if it does not exist.
    */
    std::filesystem::path GetUserDataDir();

    void CreateStandardUserDataDirectories();

    int StartServerEmulator(uint16_t port = 8080);
    int StopServerEmulator();
    bool IsServerEmulatorRunning();
    
    /**
     * @brief Lets you download a batch of Roblox assets to a directory.
     */
    int DownloadAssets(DownloadAssetArgs args);
    // std::future<char*> DownloadAssetAsync(DownloadAssetArgs);

    int GetAssetDetails(int64_t id, Roblox::AssetDetails *details);
    
    //////////////// Index Related Functions ////////////////
    int RetrieveIndex(nlohmann::json &index, bool forceRefresh = false);
    std::string GetIndexMessage();

    //////////////// Client Related Functions ////////////////
    std::vector<Engine> GetInstalledEngines();
    std::vector<Engine> GetEnginesFromIndex();
    std::vector<Engine> GetAllEngines();
    std::filesystem::path GetEngineDirectory(const Engine &client);

    /* This searches your engines directory and compiles a list of usable engines */
    void DiscoverEngines();

    bool IsEngineInstalled(const Engine &client);
    void DownloadAndInstallEngine(const Engine &client, std::shared_ptr<std::vector<std::shared_ptr<Transfer>>> &transfers, std::shared_ptr<std::function<void(EngineInstallState, CURLcode, size_t, size_t)>> callback);
    EngineLaunchResponse LaunchEngine(const Engine &client);
private:
    EngineLaunchResponse LaunchProcessThroughInjector(const std::filesystem::path &filePath);

    Response                        mInitResponse;

    event_base*                     mEventBase;

    Init                            mInit;
    LuaState                        mLuaState;
    Config*                         mConfig;
    EmuDbManager                    mEmuDbManager;
    PluginManager                   mPluginManager;

    ServerEmulator*                 mServerEmulator;

    MasterKeychain*                 mMasterKeychain;
    EmuKeychain*                    mEmuKeychain;
    RbxKeychain*                    mRbxKeychain;
    std::vector<RccServiceManager*> mRccServiceManagers;
    bool                            mPortable;

    nlohmann::json                  mIndexJson;
    bool                            mIndexDirty;
};
}
