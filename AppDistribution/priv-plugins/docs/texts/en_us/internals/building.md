# Building and testing
`BUILDING.md` at the root of the source tree covers setting up a compiler on each platform.

The project uses CMake 3.22 or newer and C++23. CPM downloads most dependencies from GitHub during the first configure and caches them in `.cache/cpm`, so the first configure needs a network connection and takes a while. OpenSSL comes from vcpkg or the system, and Qt 6 (Core, Widgets, Multimedia and MultimediaWidgets) has to be installed for the desktop app.

# Two builds on Windows
The main program and the hook need different compilers, and their build folders must never be mixed:

| Build | Compiler | Folder | Targets |
| --- | --- | --- | --- |
| Main | MinGW or Clang from MSYS2 | `build/` | Core, the desktop and command-line apps, tests |
| Hook | MSVC | `build-msvc-x64/`, `build-msvc-x86/` or `out/build/` | Hook, LocalRcc, injector |

`Hook/CMakeLists.txt` returns without doing anything under any compiler other than MSVC, so building the hook in the main folder silently produces nothing.

## Main build
```sh
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --target NoobWarrior.QtGui
```

Each target can be turned off with `NOOBWARRIOR_TARGET_CORE`, `_UNITTEST`, `_NOGUI`, `_QTGUI`, `_HOOK` and `_INJECTOR`, all on by default. Setting `NOOBWARRIOR_TARGET_QTGUI=OFF` gives a build without Qt. `NOOBWARRIOR_TARGET_TENFOOT` does nothing.

The build names differ from the target names:

| Target | Output |
| --- | --- |
| `NoobWarrior.QtGui` | `noobwarrior` |
| `NoobWarrior.NoGui` | `noobwarrior-cli` |
| `NoobWarrior.Hook` | `noobhook_x86-64` or `noobhook_x86` |
| `NoobWarrior.Injector` | `noobhook_x86-64_injector` or `noobhook_x86_injector` |
| `NoobWarrior.LocalRccLoader` | `noobhook_x86-64_localrcc` |
| `NoobWarrior.LocalRcc` | `noobhook_x86-64_localrcc_0574` or `_0719` |

## Hook build
Run these from a Visual Studio developer prompt (`vcvars64.bat`), with the main targets turned off:

```bat
cmake -B build-msvc-x64 -G Ninja -DCMAKE_C_COMPILER=cl -DCMAKE_CXX_COMPILER=cl -DCMAKE_BUILD_TYPE=Release ^
  -DNOOBWARRIOR_TARGET_CORE=OFF -DNOOBWARRIOR_TARGET_UNITTEST=OFF -DNOOBWARRIOR_TARGET_NOGUI=OFF ^
  -DNOOBWARRIOR_TARGET_QTGUI=OFF -DNOOBWARRIOR_TARGET_HOOK=ON -DNOOBWARRIOR_TARGET_INJECTOR=ON
cmake --build build-msvc-x64 --target NoobWarrior.Hook NoobWarrior.Injector NoobWarrior.LocalRccLoader NoobWarrior.LocalRcc
```

For 32-bit engines, do the same from `vcvars32.bat` into `build-msvc-x86`. LocalRcc is 64-bit only.

`NoobWarrior.LocalRcc` is built once per Studio version. The `NOOBWARRIOR_LOCALRCC_LUAU_VERSION` option picks which, `0.719` by default, so building the `0.574` copy needs a second build folder configured with `-DNOOBWARRIOR_LOCALRCC_LUAU_VERSION=0.574`.

Copy the DLLs and injectors next to `noobwarrior.exe` in `build/QtGui/`. The engine won't start from a build folder without them. The MSVC targets link the C runtime statically, so they don't need the Visual C++ redistributable.

## Android
`AndroidApp/` builds with Gradle, which drives the same root `CMakeLists.txt` with `ANDROID` set. Only Core is built, with a bundled OpenSSL, an older curl, and `ThirdParty/pkgconfig/fake-pkg-config.sh` instead of pkg-config.

# Tests
The tests are one Google Test program:

```sh
cmake --build build --target NoobWarrior.UnitTest
./build/UnitTest/NoobWarrior.UnitTest.exe
./build/UnitTest/NoobWarrior.UnitTest.exe --gtest_filter='Url.*'
```

New test files must be added to `UnitTest/CMakeLists.txt` by name.

Some tests depend on each other. `Core.Init` creates the `Core` that the `Lua.*` tests use, and the `Database.*` tests share one database and expect to run in order. Give a new test its own `EmuDb(":memory:")` instead of joining them. Name temporary files after `current_test_info()->name()`, or tests collide when run together. `Lua.CreateHttpServerFromScript` opens port 43000, which makes Windows Firewall ask for permission.

Six tests fail on a clean tree. They aren't caused by whatever you're changing:

- `Url.ResolveUsingContextForWebsiteWithoutHttpsSpecifier`
- `PluginDataModel.RbxmxIsEmbeddedInMaterializationPlan`
- `PROP.SequenceAndOptionalColumnsRoundTrip`
- `BinaryAppendSemantics.AppendedScriptsSurviveSaveAndReload`
- `BinaryAppendSemantics.RoutesServerScripts`
- `XmlAppendSemantics.RoutesServerScripts`

# Trying changes
Use the desktop app, not `noobwarrior-cli`, to work on anything served over HTTP. The command-line app doesn't run the event loop while it waits for input, so its servers never answer.

The desktop app's build copies `AppDistribution/` beside it. After editing a plugin, rebuild, or copy the changed files into `build/QtGui/priv-plugins/` by hand.

Only one copy of the app can run at a time unless `allow_multiple_instances` is set. Close the old one before starting a new build: a running copy keeps its ports, so newly added routes seem to be missing, and on Windows it also locks the program file so linking fails.

Put an empty `NW_PORTABLE` file next to the built program so testing doesn't touch your real data. See [Overview](/internals/overview).
