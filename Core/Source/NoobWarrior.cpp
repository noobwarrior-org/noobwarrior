// === noobWarrior ===
// File: NoobWarrior.cpp
// Started by: Hattozo
// Started on: 6/17/2025
// Description: Contains code for the main class used to utilize the noobWarrior library
#include <NoobWarrior/NoobWarrior.h>
#include <NoobWarrior/NetClient.h>
#include <NoobWarrior/PluginManager.h>
#include <NoobWarrior/FileSystem/VirtualFileSystem.h>
#include <NoobWarrior/Auth/MasterServerAuth.h>
#include <NoobWarrior/Auth/ServerEmulatorAuth.h>

#include <event.h>
#include <sqlite3.h>

#if defined(_WIN32)
#include <windows.h>
#include <winsock2.h>
#include <shlobj.h>
#endif

#include <utility>

using namespace NoobWarrior;

Core::Core(Init init) :
    mInit(std::move(init)),
    mServerEmulator(nullptr),
    mPortable(mInit.Portable),
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
    mEventBase = event_base_new();
    mLuaState.Open();
    mConfig = new Config(GetUserDataDir() / "config.lua", &mLuaState);
    mRobloxAuth = new RobloxAuth(mConfig);
    ConfigReturnCode = mConfig->Open();
    curl_global_init(CURL_GLOBAL_ALL);
    sqlite3_initialize();

    mDatabaseManager.AutocreateMasterDatabase();

    if (mInit.EnableKeychain)
        GetRobloxAuth()->ReadFromKeychain();

    if (mInit.LoadPlugins)
        GetPluginManager()->LoadPlugins();
}

Core::~Core() {
    if (mInit.EnableKeychain)
        GetRobloxAuth()->WriteToKeychain();

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

DatabaseManager *Core::GetDatabaseManager() {
    return &mDatabaseManager;
}

PluginManager *Core::GetPluginManager() {
    return &mPluginManager;
}

MasterServerAuth *Core::GetMasterServerAuth() {
    return mMasterServerAuth;
}

ServerEmulatorAuth *Core::GetServerEmulatorAuth() {
    return mServerEmulatorAuth;
}

RobloxAuth *Core::GetRobloxAuth() {
    return mRobloxAuth;
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
    if (!mPortable) {
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
    // portable versions aren't allowed. cause, reasons.
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
#define NW_CREATE(path) std::filesystem::create_directories(GetUserDataDir() / path)
    NW_CREATE("databases");
    NW_CREATE("roblox");
    NW_CREATE("roblox" / "client");
    NW_CREATE("roblox" / "server");
    NW_CREATE("roblox" / "studio");
    NW_CREATE("temp");
    NW_CREATE("temp" / "downloads");
    NW_CREATE("temp" / "downloads" / "clients");
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