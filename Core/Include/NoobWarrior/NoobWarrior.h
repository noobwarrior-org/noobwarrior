// === noobWarrior ===
// File: NoobWarrior.h
// Started by: Hattozo
// Started on: 12/15/2024
// Description: Core of the library
#pragma once

#include "Macros.h"
#include "Log.h"
#include "Database.h"
#include "Auth.h"
#include "Config.h"
#include "DatabaseManager.h"
#include "RccServiceManager.h"
#include "HttpServer/HttpServer.h"
#include "Roblox/DataModel/RobloxFile.h"
#include "Roblox/Api/Asset.h"
#include "Roblox/EngineType.h"

#include <lua.hpp>

#include <vector>

namespace NoobWarrior {
struct Init {
    bool Portable { true };
    std::string ConfigFileName = { "config.json" };
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

class Core {
public:
    Core(Init = {});
    ~Core();

    ConfigResponse ConfigReturnCode;

    lua_State *GetLuaState();
    Config *GetConfig();

    static std::filesystem::path GetInstallationDir();

    /**
     * @brief Warning: Any call to this function will automatically create a directory if it does not exist.
    */
    std::filesystem::path GetUserDataDir();

    int StartHttpServer(uint16_t port = 8080);
    int StopHttpServer();

    int LaunchRoblox(Roblox::EngineType type, std::string version);

    /**
     * @brief Lets you download a batch of Roblox assets to a directory.
     */
    int DownloadAssets(DownloadAssetArgs args);
    // std::future<char*> DownloadAssetAsync(DownloadAssetArgs);

    int GetAssetDetails(int64_t id, Roblox::AssetDetails *details);
private:
    int InitLuaState();
    int Inject(unsigned long pid, char *dllPath);
    int LaunchInjectProcess(const std::filesystem::path &filePath);

    lua_State*                      mLuaState;
    Config*                         mConfig;
    DatabaseManager                 mDatabaseManager;
    HttpServer::HttpServer*         mHttpServer;
    std::vector<RccServiceManager*> mRccServiceManagers;
    bool                            mPortable;
};
}
