// === noobWarrior ===
// File: NoobWarrior.cpp
// Started by: Hattozo
// Started on: 6/17/2025
// Description: Contains code for the main class used to utilize the noobWarrior library
#include <NoobWarrior/NoobWarrior.h>

#include <civetweb.h>
#include <sqlite3.h>

#if defined(_WIN32)
#include <windows.h>
#include <shlobj.h>
#elif defined(__APPLE__)
#include <CoreServices/CoreServices.h>

#include <utility>
#endif

using namespace NoobWarrior;

Core::Core(Init init) :
    mInit(std::move(init)),
    mLuaState(nullptr),
    mHttpServer(nullptr),
    mPortable(mInit.Portable)
{
    InitLuaState();
    mConfig = new Config(GetUserDataDir() / "config.lua", mLuaState);
    ConfigReturnCode = mConfig->Open();
    sqlite3_initialize();
    mg_init_library(0);
}

Core::~Core() {
    StopHttpServer();
    sqlite3_shutdown();
    ConfigReturnCode = mConfig->Close();
    NOOBWARRIOR_FREE_PTR(mConfig)
    lua_close(mLuaState);
}

static int printBS(lua_State *L) {
    const char *str = luaL_checkstring(L, 1);
    Out("Lua", str);
    return 0;
}

int Core::InitLuaState() {
    mLuaState = luaL_newstate();
    luaL_openlibs(mLuaState);

    lua_pushcfunction(mLuaState, printBS);
    lua_setglobal(mLuaState, "print");

    // lua_pushcfunction(mLuaState, printBS);
    // lua_setglobal(mLuaState, "error");
    return 1;
}

lua_State *Core::GetLuaState() {
    return mLuaState;
}

Config *Core::GetConfig() {
    return mConfig;
}

DatabaseManager *Core::GetDatabaseManager() {
    return &mDatabaseManager;
}

std::filesystem::path Core::GetInstallationDir() const {
    assert(mInit.ArgCount > 0 && "You must pass in your argc to ArgCount in order to use GetInstallationDir()");
#if defined(__APPLE__)
    // Assuming everyone who calls this function wants to access non-executable files.
    auto path = std::filesystem::path(mInit.ArgVec[0]).parent_path();
    // Are we part of an app bundle?
    if (path.filename().compare("MacOS") == 0)
        return std::filesystem::path(path / ".." / "Resources");
    // No? okay just do it like how everyone else is doing it lol
    return path;
#else
    return std::filesystem::path(mInit.ArgVec[0]).parent_path();
#endif
}

std::filesystem::path Core::GetUserDataDir() {
#if defined(_WIN32)
    if (!mPortable) {
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
    }
#elif defined(__APPLE__)
    // portable versions aren't allowed. cause, reasons.
    char path[PATH_MAX];
    FSRef ref;
    OSType type = kApplicationSupportFolderType;
    OSStatus ret = FSFindFolder(kUserDomain, type, kCreateFolder, &ref); // Apparently this is deprecated, but it's the only way to do it without Objective-C so whatever.
    FSRefMakePath(&ref, (UInt8*)&path, PATH_MAX);

    std::filesystem::path appSupportDir(path);
    std::filesystem::create_directory(appSupportDir / NOOBWARRIOR_USERDATA_DIRNAME);
    return appSupportDir / NOOBWARRIOR_USERDATA_DIRNAME;
#endif
    return GetInstallationDir();
}

int Core::StartHttpServer(uint16_t port) {
    if (mHttpServer != nullptr && !StopHttpServer()) { // try stopping the HTTP server if it's already on
        return -2;
    }

    mHttpServer = new HttpServer::HttpServer(this);
    return mHttpServer->Start(port);
}

int Core::StopHttpServer() {
    if (mHttpServer != nullptr && !mHttpServer->Stop()) {
        return 0;
    }
    NOOBWARRIOR_FREE_PTR(mHttpServer)
    return 1;
}
