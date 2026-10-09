# Overview
These pages explain how noobWarrior works inside, for people changing its code. Building it is covered in `BUILDING.md` at the root of the source tree.

# Source tree
| Folder | What it is |
| --- | --- |
| `Core/` | The whole program as a library, `NoobWarrior.Core`. Everything below is a front end or helper around it. |
| `QtGui/` | The desktop app (`noobwarrior`). |
| `NoGui/` | The command-line app (`noobwarrior-cli`). |
| `AndroidApp/` | The Android app, built with Gradle around the same CMake project. Only Core is built for it. |
| `Hook/` | `noobhook.dll`, loaded into Roblox. Windows only, built with MSVC. |
| `Injector/` | Starts Roblox and loads the hook into it. Windows only. |
| `AppDistribution/` | Files shipped beside the app: built-in plugins, user plugins and language strings. |
| `UnitTest/` | Google Test suite. |
| `FileFormatGenerator/` | Generates the Roblox class definitions in `Core/Include/NoobWarrior/Roblox/FileFormat/Generated/`. |
| `RoundTripPlace/`, `DiffPlace/`, `PlacePollution/` | Tools for finding damage in prepared places. See [Roblox files](/internals/roblox-files). |

`Tenfoot/` is a Godot project and `QtBootstrapper/` has a `CMakeLists.txt`, but the root CMake project includes neither.

# Core
`NoobWarrior::Core` owns every subsystem, and everything else is reached through it:

| Subsystem | Class | Page |
| --- | --- | --- |
| Settings | `Registry` | [Registry](/internals/registry) |
| Luau | `LuaState`, `Lhp` | [Plugins](/internals/plugins), [LHP](/internals/lhp) |
| Plugins | `PluginManager`, `Plugin` | [Plugins](/internals/plugins) |
| Databases | `EmuDbManager`, `EmuDb` | [EmuDb](/internals/emudb) |
| Web server for Roblox | `ServerEmulator` | [Server emulator](/internals/server-emulator) |
| Roblox processes | `Engine.cpp` | [Engines](/internals/engines) |
| Saved logins | `RbxKeychain`, `EmuKeychain`, `MasterKeychain` | |
| Files | `VirtualFileSystem`, `Url` | [Files and URLs](/internals/files-and-urls) |

The keychains store credentials in the operating system's credential store, with a separate implementation per platform (`OsKeychainWin.cpp`, `OsKeychainMac.cpp`, `OsKeychainLinux.cpp` and a generic fallback). `Rbx` holds real Roblox accounts, `Emu` holds logins to other people's servers, and `Master` holds master server accounts, including the active one.

# Startup
`Core::Core(Init)` sets everything up in one constructor. The order is deliberate, and several steps depend on the ones before:

1. The libevent event base.
2. The Luau state, with its bindings.
3. The registry, read from `<userdata>/registry.lua`, and the keychain objects.
4. File systems for the disk root, install folder, user data folder and plugin data folder.
5. Language strings, in the language set by `language`.
6. Plugin manifests (`MountPlugins`). No plugin code runs yet.
7. The user's databases (`MountDatabases`), which can refer to plugins by URL now that their manifests are read.
8. `master.nwdb`, if nothing else was mounted.
9. Databases plugins require. They come after the user's so the master database stays first.
10. The server emulator, which also becomes the `emu` and `emu_db_mgr` Luau globals.
11. Saved logins from the keychains.
12. The HTTPS certificate, created if missing.
13. Plugin code (`ExecutePlugins`), now that its databases, `emu` and the certificate exist.
14. The server emulator starts, unless `Init::AutoStartServerEmulator` or `emu.autostart` is off.

The destructor stops the emulator, unloads plugins, writes the keychains back, unmounts databases and closes the registry.

# The event loop
`Core` has no thread of its own. All HTTP serving and async I/O runs inside `Core::ProcessEvents()`, which the host program has to keep calling: `QtGui/Application.cpp` calls it from a `QTimer`, and `NoGui` from its console loop. The command-line app doesn't call it while waiting for console input, so its web server accepts connections but never answers. Use the desktop app to work on the website.

Background threads send work back to the event loop with `Core::RunOnEventLoop()`. The event-loop thread owns the libevent servers, the Luau state and the SQLite connections, so anything touching those has to run there.

# Data folders
| URL | Folder |
| --- | --- |
| `install://` | Where noobWarrior is installed. Holds `priv-plugins/`, `lang/` and the hook and injector binaries. |
| `user://` | `%LOCALAPPDATA%\noobWarrior`, `~/.local/share/noobWarrior` or `~/Library/Application Support/noobWarrior`. In portable mode, the install folder. |
| `data://` | `<userdata>/data`, where plugins keep writable files. |

Inside `userdata` are `registry.lua`, `databases/`, `plugins/`, `data/`, `engines/`, `ssl/` and `logs/`.

The desktop app turns portable mode on when a file named `NW_PORTABLE` sits beside it. That also makes a safe test setup: a build folder with `NW_PORTABLE` in it gets its own registry, databases and plugins, which can be deleted afterwards. A test `registry.lua` has to `return` a table, because assigning globals in it is read as an empty registry.

# Conventions
- Each source file starts with the LGPL-3.0-or-later header, then a `// === noobWarrior ===` block with the file name, author, start date and description.
- Code is in the `NoobWarrior` namespace, with `PascalCase` methods, `mMember` fields and `kConstant` constants.
- Operations that can fail return a class-scoped `enum class Response` or `FailReason` and don't throw. Objects that can fail during construction have `Fail()`.
- Comments in the emulator usually say which Roblox version needs a behavior. They are often the only record of why the code is the way it is, so keep them.
- Line endings differ between files, and some files mix both. Match the lines around an edit, and check `git diff --numstat` to make sure an editor hasn't rewritten a whole file.
