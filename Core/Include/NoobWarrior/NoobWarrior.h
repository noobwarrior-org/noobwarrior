// === noobWarrior ===
// File: NoobWarrior.h
// Started by: Hattozo
// Started on: 12/15/2024
// Description: Core of the library
#pragma once

#include "Macros.h"
#include "Log.h"
#include "Database/Database.h"
#include "Config.h"
#include "Database/DatabaseManager.h"
#include "RccServiceManager.h"
#include "RobloxClient.h"
#include "HttpServer/Emulator/ServerEmulator.h"
#include "Roblox/FileFormat/RobloxFile.h"
#include "Roblox/Api/Asset.h"
#include "RobloxClient.h"
#include "NetClient.h"
#include "Auth/RobloxAuth.h"
#include "Auth/MasterServerAuth.h"

#include <lua.hpp>
#include <curl/curl.h>

#include <vector>

namespace NoobWarrior {
struct Init {
    int         ArgCount        {};
    char**      ArgVec          {};
    bool        Portable        { true };
    bool        EnableKeychain  { true };
};

enum class AssetFileNameStyle {
    Raw, // File name is retrieved from the server that is hosting the file. In this case you will get a MD5 hash, since that is how Roblox indexes files.
    AssetId,
    AssetName
};

enum class BackupResponse {
    Failed,
    Ok, // Not a success yet, we are just getting started.
    UrlNotSet,
    AccountRequired,
    UnsupportedContentType
};

enum class BackupState {
    Failed,
    Success,
    Finalizing,
    DownloadingFile,
    ParsingFile,
    CompressingFile,
    ScrapingMetadata,
    AddingToDatabase,
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
    Core(Init init = {});
    ~Core();

    ConfigResponse ConfigReturnCode;

    lua_State *GetLuaState();
    Config *GetConfig();
    DatabaseManager *GetDatabaseManager();
    RobloxAuth *GetRobloxAuth();

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

    template<typename T>
    BackupResponse Backup(BackupArgs args, int64_t id, Database *db, std::function<void(BackupState, std::string, size_t, size_t)> &callback) {

    }

    template<typename T>
    BackupResponse Backup(int64_t id, const std::filesystem::path &outputDir, std::function<void(BackupState, std::string, size_t, size_t)> &callback) {

    }

    /**
     * @brief Parses the given input file as a Roblox model/place, and automatically searches all asset IDs and downloads them. Downloaded files are stored in outputDir
     * 
     * @param inputFile The path to the .rbxm/.rbxl file
     * @param outputDir The path to where all files should be installed
     * @param callback A std::function object that gets called everytime the status of the backup process is updated
     * @return BackupResponse 
     */
    BackupResponse BackupFromFile(const std::filesystem::path &inputFile, const std::filesystem::path &outputDir, std::function<void(BackupState, std::string, size_t, size_t)> &callback);
    
    /**
     * @brief Parses the given input file as a Roblox model/place, and automatically searches all asset IDs and downloads them. Downloaded files are stored in the given database.
     * 
     * @param inputFile The path to the .rbxm/.rbxl file
     * @param db The database where all found content should be stored.
     * @param callback 
     * @return BackupResponse 
     */
    BackupResponse BackupFromFile(const std::filesystem::path &inputFile, Database *db, std::function<void(BackupState, std::string, size_t, size_t)> &callback);

    BackupResponse BackupAsset(int64_t id, Database *db, std::function<void(BackupState, std::string, size_t, size_t)> &callback);
    BackupResponse BackupAsset(int64_t id, const std::filesystem::path &outputDir, std::function<void(BackupState, std::string, size_t, size_t)> &callback);

    BackupResponse BackupGame(int64_t id, Database *db, std::function<void(BackupState, std::string, size_t, size_t)> &callback);
    BackupResponse BackupGame(int64_t id, const std::filesystem::path &outputDir, std::function<void(BackupState, std::string, size_t, size_t)> &callback);

    //////////////// Index Related Functions ////////////////
    int RetrieveIndex(nlohmann::json &index, bool forceRefresh = false);
    std::string GetIndexMessage();

    //////////////// Client Related Functions ////////////////
    std::vector<RobloxClient> GetInstalledClients();
    std::vector<RobloxClient> GetClientsFromIndex();
    std::vector<RobloxClient> GetAllClients();
    std::filesystem::path GetClientDirectory(const RobloxClient &client);

    bool IsClientInstalled(const RobloxClient &client);
    void DownloadAndInstallClient(const RobloxClient &client, std::shared_ptr<std::vector<std::shared_ptr<Transfer>>> &transfers, std::shared_ptr<std::function<void(ClientInstallState, CURLcode, size_t, size_t)>> callback);
    ClientLaunchResponse LaunchClient(const RobloxClient &client);
private:
    int InitLuaState();
    ClientLaunchResponse LaunchProcessThroughInjector(const std::filesystem::path &filePath);

    Init                            mInit;
    lua_State*                      mLuaState;
    Config*                         mConfig;
    DatabaseManager                 mDatabaseManager;

    HttpServer::ServerEmulator*     mServerEmulator;

    RobloxAuth*                     mRobloxAuth;
    std::vector<RccServiceManager*> mRccServiceManagers;
    bool                            mPortable;

    nlohmann::json                  mIndexJson;
    bool                            mIndexDirty;
};
}
