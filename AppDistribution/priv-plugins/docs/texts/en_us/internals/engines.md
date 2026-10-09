# Engines
noobWarrior calls a copy of Roblox an engine. Each engine is a Player (`RobloxPlayerBeta.exe`), a Studio (`RobloxStudioBeta.exe`) or a game server (`RCCService.exe`), and the code for finding and starting them lives in `Core/Source/Engine.cpp`.

# Finding installed engines
Engines live in `<userdata>/engines/<folder>/`. `Core::GetInstalledEngines()` looks at every folder there, oldest first by modification time, and passes each to `InspectEngineDirectory()`. That function looks for one of the three executable names to decide the engine's side, then reads the rest from the executable itself:

| Field | Where it comes from |
| --- | --- |
| `Side` | Which of the three executable names is in the folder. |
| `Hash` | The folder's name. Users name it after the Roblox version hash, such as `version-acc4b74f79e743b9`. |
| `Architecture` | The PE header's machine type (`Pe::ReadMachine`). |
| `Version` | The PE version resource (`Pe::ReadProductVersion`), such as `0.574.0.5740446`. |

A folder without a recognized executable is skipped and logged.

# Picking an engine
Callers describe the engine they want with an `Engine` struct, and `PickBestMatch()` chooses among the installed ones. If the request has a hash, only engines in a folder with that name are considered. Among engines on the same side, it takes the first match it finds in this order:

1. An exact version match.
2. The closest era. The era is the second number of the version, so `0.574.0.5740446` is era 574. `ParseEraVersion()` extracts it.
3. Any engine on the same side.

Because of step 3, asking for a version that isn't installed can start a different one. `ResolveInstalledEngine()` exposes the same matcher so callers can find out which build they will get.

# Launching
`Core::LaunchEngine()` takes `EngineStartParameters` and does the preparation each case needs before handing off to the injector:

| Case | Preparation |
| --- | --- |
| Joining someone else's server | Pushes a proxy layer onto the server emulator, so requests the local emulator can't answer go to the host. Posts the local user's avatar to the host's `/emu/v1/avatar-override` on a background thread, so the host's game server builds the player with their own look. |
| Game server (RCCService) | Writes `gameserver.json` into the engine folder with the place id, universe id and port, plus fixed values RCCService expects. RCCService reads it at startup through `-localtest`. |
| Studio hosting a team test | Builds `server.rbxl` from the place and every plugin's content. See [Studio server places](/internals/roblox-files#studio-server-places). |
| Any Studio | Records the Studio version and hash on the emulator, which answers Studio's version checks with them. |
| Player, with `emu.auth.enabled` on | A client launched from the desktop app has no login cookie, so this mints a one-time join ticket for the logged-in account, or encodes a guest ticket when nobody is logged in. The ticket goes to the injector as `--authticket`. |

`LaunchProcessThroughInjector()` then starts `noobhook_x86_injector.exe` or `noobhook_x86-64_injector.exe` from the install folder, picked by the engine's architecture. A 64-bit process can't inject into a 32-bit one, which is why there are two.

On Linux and macOS the injector and Roblox run under Wine. `GetWinePath()` converts the executable's path to the form Wine expects, and the `wine.*` registry keys control the Wine install.

## Injector arguments
| Argument | Meaning |
| --- | --- |
| `--file` | The engine executable. |
| `--ip`, `--port`, `--placeid` | Where to join, when joining a game. |
| `--side` | `client`, `server` or `studio`. |
| `--emuhttp`, `--emuhttps` | The emulator's ports, from `emu.http_port` and `emu.https_port`. |
| `--authticket` | The join ticket. The injector uses `1` when it is missing. |
| `--scheme` | Which command line the engine expects. See below. |

The scheme exists because each era launches differently. It is picked from the era of the executable that will run, which may differ from the one requested:

| Scheme | Used for | Command line |
| --- | --- | --- |
| `old` | Era 463 | `-a <Negotiate.ashx> -j <PlaceLauncher.ashx> -t <ticket>` |
| `new` | Era 574 | `--play` with a `roblox://` deep link |
| `app` | Era 719, joining a game | `--app` with the ticket and `PlaceLauncher.ashx` URL |
| `home` | Era 719, no join target | `--app`, which opens the normal app screen |

RCCService always gets `-localtest gameserver.json`, and Studio hosting a team test gets `-task StartServer`.

## Certificates
Era 574 and later Players and Studios read `ssl/cacert.pem` beside their executable. Before launching one, `MergeEmulatorCertIntoEngineCaBundle()` backs up the original bundle once and writes a merged bundle that also trusts the emulator's certificate (`<userdata>/ssl/cert.pem`). Running it again produces the same file.

The hook has an older mechanism too: when `NOOBHOOK_EMU_CERT` is set, it redirects the engine's attempts to open `cacert.pem` to a merged temporary copy. The core doesn't pass `--emucert` to the injector at the moment, so this only runs when the injector is started by hand.

# The injector
`Injector/Injector.cpp` is a small Windows program. It:

1. Builds the engine's command line from the scheme.
2. Sets environment variables the hook reads: `NOOBHOOK_HTTP_PORT`, `NOOBHOOK_HTTPS_PORT`, `NOOBHOOK_SIDE`, `NOOBHOOK_PORT`, `NOOBHOOK_PLACEID`, and `NOOBHOOK_EMU_CERT` when it was given `--emucert`.
3. Starts the engine suspended with `CreateProcessW`, so the environment is inherited.
4. Loads the hook DLL into it with `CreateRemoteThread` calling `LoadLibraryW`.
5. Resumes the engine.

Era 719 Players need more work before the hook can load. That code is in `Injector/Bypass.cpp` and `Injector/ManualMap.cpp`, and it loads `noobhook_x86-64_hyperion.dll` instead of the normal hook.

The injector writes its log to `noobhook_injector.log` next to itself.

# The hook
`Hook/Hook.cpp` builds `noobhook.dll`, which runs inside the engine. It is Windows only and builds only with MSVC: under any other compiler, `Hook/CMakeLists.txt` returns without defining the target. Packaging renames the build outputs to the arch-suffixed names the injector looks for.

## Redirecting traffic
The hook uses MinHook to replace networking functions, including `connect`, `getaddrinfo`, `InternetConnectW`, `WinHttpConnect` and `WinHttpSendRequest`. Connections to Roblox's hosts on port 80 or 443 are sent to `127.0.0.1` on the emulator's HTTP or HTTPS port.

## Patches
`Hook/Patch/` holds patches that find code in the engine by byte pattern and change it. They turn off checks that would stop the engine talking to an emulator, such as TLS verification, signature checks and a trust check, and guard against a few crashes. Patterns differ between versions, so a patch that finds nothing logs it and does nothing.

Two rules have caused hard-to-find bugs:

- `MH_Initialize()` must run before any `MH_CreateHook()`. A hook created earlier silently fails.
- In patterns for Hooking.Patterns, a wildcard byte is a single `?`. Writing `??` counts as two wildcards and shifts the rest of the pattern.

Byte patches must run in `DllMain`, while the engine is still suspended. If they run later from the hook's own thread, Studio has already fetched its settings over TLS and failed.

## Reporting in
The hook tells the emulator about its process by posting to `/emu/v1/process-ping` (see `Hook/Ping.cpp`). It sends Hello when it loads, a heartbeat every 5 seconds, and Goodbye from hooked `ExitProcess` and `RtlExitUserProcess`. The emulator drops processes it hasn't heard from in 30 seconds. This is what `ServerEmulator:GetRunningInstances()` reports.

## Studio team tests
When the hook finds it is inside Studio, it loads `noobhook_x86-64_localrcc.dll` from its own folder, which lets Studio's team test start a local game server. That DLL is a loader (`Hook/LocalRcc/LocalRccLoader.cpp`). It identifies the Studio build from its PE header's timestamp and image size, then loads `noobhook_x86-64_localrcc_0574.dll` or `noobhook_x86-64_localrcc_0719.dll`. Each of those is a separate build of `NoobWarrior.LocalRcc`, chosen with the `NOOBWARRIOR_LOCALRCC_LUAU_VERSION` CMake option (`0.574` or `0.719`). LocalRcc exists only for x64.
