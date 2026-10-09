# Core
```cpp
#include <NoobWarrior/NoobWarrior.h>
```

The center of noobWarrior. One `Core` owns every other system, and everything else is reached through it.

# Constructor
```cpp
Core(Init init = {});
```

Sets up every system in order: the Luau state, the registry, the file systems, plugins, databases, the server emulator, keychains and the HTTPS certificate. Plugin code runs last. Check [Fail](#fail) afterwards.

## Init
Options for the constructor. Every field has a default.

| Field | Type | Default | Description |
| --- | --- | --- | --- |
| `ArgCount` | `int` | `0` | `argc` from `main`. |
| `ArgVec` | `char**` | `nullptr` | `argv` from `main`. Used to find the install folder. |
| `AutocreateStandardUserDataDirectories` | `bool` | `true` | Creates the folders noobWarrior expects in the user's data directory. |
| `Portable` | `bool` | `true` | Keeps user data in the install folder instead of the operating system's per-user folder. The desktop app turns this on only when a file named `NW_PORTABLE` sits next to it. |
| `EnableKeychain` | `bool` | `true` | Reads saved logins from the operating system's credential store. |
| `AutocreateCert` | `bool` | `true` | Creates an HTTPS certificate if there isn't one. |
| `LoadPlugins` | `bool` | `true` | Loads and runs plugins. |
| `AutoStartServerEmulator` | `bool` | `true` | Starts the server emulator once setup finishes. |
| `InstallDataRelativePath` | `std::string` | `"noobwarrior"` | The install folder's name, relative to the program. Set it to `""` to use the program's own folder. |
| `UserDataDir` | `std::string` | `""` | Uses this path as the user's data directory instead of the operating system's default. |
| `InstallDataDir` | `std::string` | `""` | Uses this path as the install folder instead of working it out from `ArgVec`. |

On platforms without a useful `argv[0]` or home folder, such as Android, set both `UserDataDir` and `InstallDataDir`.

# Lifecycle
## Fail
```cpp
bool Fail();
```

Returns `true` if setup failed. Don't use a `Core` that failed.

## ProcessEvents
```cpp
int ProcessEvents(bool block = false);
```

Handles pending network events. Call it repeatedly from the thread that created the `Core`. With `block` set, it waits for at least one event before returning.

## RunOnEventLoop
```cpp
void RunOnEventLoop(std::function<void()> fn);
```

Schedules `fn` to run on the thread that calls `ProcessEvents`. It is safe to call from any thread. Use it to hand a background thread's result back, since the server and databases must only be used from that one thread.

## GetSingleton
```cpp
static Core *GetSingleton();
```

Returns the running `Core`.

# Systems
| Method | Returns |
| --- | --- |
| `GetRegistry()` | The [Registry](/api/cpp/classes/Registry), noobWarrior's settings. |
| `GetEmuDbManager()` | The `EmuDbManager` holding every mounted database. |
| `GetServerEmulator()` | The `ServerEmulator`. |
| `GetPluginManager()` | The `PluginManager`. |
| `GetLuaState()` | The `LuaState` plugins run in. |
| `GetLanguage()` | The `Language` used for translations. |
| `GetRbxKeychain()` | Saved Roblox accounts. |
| `GetEmuKeychain()` | Saved logins to other people's servers. |
| `GetMasterKeychain()` | Saved master server accounts, including the active one. |
| `GetInit()` | The `Init` the `Core` was built with. |

# Folders
| Method | Returns |
| --- | --- |
| `GetUserDataDir()` | The user's data directory, as a path. Creates it if it doesn't exist. |
| `GetInstallDataDir()` | The install folder, as a path. |
| `GetUserDataVfs()` | The user's data directory, as a `VirtualFileSystem`. |
| `GetInstallDataVfs()` | The install folder, as a `VirtualFileSystem`. |
| `GetPluginDataVfs()` | The plugin data folder, as a `VirtualFileSystem`. |
| `GetLogPath()` | The log file's path. |

# Server emulator
| Method | Description |
| --- | --- |
| `int StartServerEmulator()` | Starts it. |
| `int StopServerEmulator()` | Stops it. |
| `void RestartServerEmulator()` | Stops it and starts it again. |
| `bool IsServerEmulatorRunning()` | Returns whether it is running. |

# Engines
## GetInstalledEngines
```cpp
std::vector<Engine> GetInstalledEngines();
```

Lists the Roblox copies found in the `engines` folder of the user's data directory.

## ResolveInstalledEngine
```cpp
std::optional<Engine> ResolveInstalledEngine(const Engine &want);
```

Finds the installed copy that best matches `want`. An exact match wins, then one from the same era, then any copy of the same kind (Player, Studio or server).

## GetEngineDirectory
```cpp
std::filesystem::path GetEngineDirectory(const Engine &engine);
```

Returns the folder of the best match for `engine`, or an empty path if nothing matches.

## LaunchEngine
```cpp
EngineLaunchResponse LaunchEngine(EngineStartParameters params, const EngineLaunchProgressCallback &progressCallback = {});
```

Starts a Roblox Player, Studio or game server with noobWarrior's hook injected, so it talks to the server emulator instead of Roblox.

# Joining other servers
## ConnectToServerEmulator
```cpp
void ConnectToServerEmulator(const std::string &ip, uint16_t port,
    std::function<void(ServerEmulatorConnectFailReason, std::vector<EngineStartParameters>)> callback,
    const std::string &sessionToken = "");
```

Asks another person's server emulator how to join it. `callback` receives `ServerEmulatorConnectFailReason::None` and the start parameters for each of its game servers on success. Pass those to [LaunchEngine](#launchengine).

## LoginToRemoteHost
```cpp
std::optional<std::string> LoginToRemoteHost(const std::string &ip, uint16_t port,
    const std::string &username, const std::string &password);
```

Logs in to another server's website and returns the session token, which is also saved for next time. Pass the token to `ConnectToServerEmulator` when that server requires an account.

`GetCachedRemoteHostToken`, `ValidateRemoteHostSession` and `ForgetRemoteHostLogin` read, check and remove the saved token.

## LoginToMaster
```cpp
bool LoginToMaster(const std::string &masterUrl, const std::string &username, const std::string &password);
```

Signs in to a master server and makes that account the identity used when joining servers that trust it. `LogoutFromMaster()` signs out but keeps the account saved.
