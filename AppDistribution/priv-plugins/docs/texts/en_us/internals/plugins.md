# Plugins
A plugin is a folder or `.zip` with a manifest at its root. The loading code is `Plugin` (one plugin) and `PluginManager` (all of them), in `Core/Source/Plugin.cpp` and `PluginManager.cpp`.

# Where plugins come from
Built-in plugins sit in `priv-plugins/` in the install folder. They load in the order listed by `priv-plugins/loadlist.lua`, which returns an array of folder names: `http-base`, `emu-frontend`, `master-server`, `docs` and `overlay`. A plugin counts as privileged when its parent folder is named `priv-plugins`. That only changes how it's listed: nothing else in the code treats privileged plugins differently yet.

User plugins load after the built-in ones, in the order of the `plugins.selected` registry key. Each entry is a file name, looked up first in `<userdata>/plugins/` and then in the install folder's `plugins/`. The user's copy wins, which makes it easy to override a shipped plugin. Order matters because later plugins overwrite what earlier ones put into a place, so enabling a plugin adds it to the end of the list and disabling one leaves the others where they were. Read the list by index: iterating a Luau table with `pairs` loses the order.

# The manifest
`Plugin`'s constructor opens the plugin through `VirtualFileSystem::New()`, which handles folders and zips alike, then runs `manifest.luau`, or `manifest.lua` if there is no `.luau` file. The manifest must return a table. These fields are read:

| Field | Required | Read by |
| --- | --- | --- |
| `identifier` | Yes | Everything. It is also the host in the plugin's `plugin://` URLs. |
| `title` | Yes | The plugin list. |
| `version`, `description`, `icon`, `authors` | No | The plugin list. |
| `autorun` | No | `Plugin::Execute()`. |
| `databases` | No | Database mounting. |
| `datamodel` | No | Studio server place preparation. |
| `engine_autorun` | No | Studio server place preparation. |

Unknown fields are ignored, and there is no schema or manifest version. `permissions` appears in every shipped manifest, but nothing reads it, and the `Permission` enum has no checks behind it. Adding real permissions means writing both the parsing and the enforcement.

The manifest runs before the `emu` global exists, because the server emulator is created later in startup. Manifests are plain tables, so they don't need it.

# Startup
`Core`'s constructor splits plugin loading in two, with databases in between:

1. `PluginManager::MountPlugins()` reads every manifest. It doesn't run any plugin code.
2. The user's databases are mounted. A database entry in `databases.mounted` can point at a plugin with a `plugin://` URL, which only resolves once the manifests have been read.
3. `MountRequiredDatabases()` mounts the databases plugins mark as `required`.
4. The server emulator, keychains and certificate are set up.
5. `PluginManager::ExecutePlugins()` runs each plugin's `autorun` scripts.

Plugin code runs last so it finds its databases mounted and `emu` available. `MountPlugins()` must not call `ExecutePlugins()`.

# The script environment
Each plugin gets one `sol::environment` whose fallback is the Luau globals, with `plugin` set to the plugin. Every script that plugin runs gets its own environment on top of that one, holding `script`, `plugin`, `print` and `require` (see `LuaScript::Execute()`). Writing a global from a script stays in that script's environment, while `_G` is shared by everything.

`require()` caches modules by resolved URL in `LuaState`, so a module runs once however many plugins require it. A module that requires itself, directly or through others, raises an error.

What plugins can call from C++ is whatever `LuaState::Open()` binds, in `Core/Source/Lua/LuaState.cpp`. New C++ features have to be bound there before Luau can reach them. The API Reference documents the current bindings.

# Plugin databases
Each entry in `databases` has a `url`, `required` and `writable`. A relative URL means a file inside the plugin.

SQLite can't open a file inside a zip, and a plugin shouldn't change its own files, so the manager turns each entry into a path it can open. A read-only database inside a folder plugin is opened in place. Anything else is copied to `<userdata>/data/<identifier>/db/` first.

A writable copy is made once and never refreshed, because it holds what players created, such as DataStore values and published places, and a plugin update must not wipe that. Read-only copies are replaced whenever the plugin file is newer. If a database's `Mutable` flag is off, it opens read-only even when the manifest asks for `writable`.

Databases marked `required` mount whenever the plugin is enabled, and the user can't unmount them. Others are offered to the user, and an accepted one is stored in `databases.mounted` as its `plugin://` URL.

A plugin database that is out of date and opened read-only fails to mount, and the log suggests setting `writable = true` or opening it once so it can be upgraded.

# Language strings
`PluginManager::Mount()` registers each plugin with the `Language` system, which then looks for strings in the plugin's `lang/` folder. Sources registered later win when two define the same key, so a plugin can override noobWarrior's own strings.

# Content for game servers
`datamodel` and `engine_autorun` put a plugin's content into the place a Studio team test runs. Each `datamodel` entry names a `dir` laid out like a Rojo project, a `side`, and an optional `predicate` function that decides per place whether to include it. Only `server` and `shared` entries are used; client entries are skipped.

`engine_autorun` lists Luau scripts by side (`shared`, `server`, `client`). Each one is wrapped so it runs in its own sandbox inside the engine, without access to `script`, `getfenv` or `setfenv`. See [Studio server places](/internals/roblox-files#studio-server-places) for how both are added to the place.
