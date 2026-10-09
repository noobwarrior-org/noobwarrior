# Files and URLs
Anything a plugin can reach goes through `VirtualFileSystem` and is named by a `Url`, not a raw `std::filesystem::path`. The same code then reads a folder, a zip or a layered set of both. The code is in `Core/Source/FileSystem/` and `Core/Source/Url.cpp`.

# File systems
`VirtualFileSystem` is an abstract class with handle-based reading (`OpenHandle`, `ReadHandleChunk`, `ReadHandleLine`, `CloseHandle`) plus `GetEntriesInDirectory`, `EntryExists`, `WriteFile`, `CreateDirectories` and `DeleteEntry`. Paths start with `/` at the file system's root.

| Class | Backed by |
| --- | --- |
| `StdFileSystem` | A folder on disk. |
| `ZipFileSystem` | A zip file. Read-only. |
| `ArchiveDbFileSystem` | An SQLite archive (`.sqlar` or `.nwp`). |
| `OverlayFileSystem` | Other file systems mounted at path prefixes. |
| `EmuDbFileSystem` | The `FsNode` tree inside a `.nwdb`. Nothing uses it yet. |

`VirtualFileSystem::New(&vfs, path)` picks the class from the path: a folder gets `StdFileSystem`, `.sqlar` and `.nwp` get `ArchiveDbFileSystem`, and any other existing file is opened as a zip. Plugins are opened this way, which is why a plugin can be a folder or a zip.

## Overlays
An `OverlayFileSystem` holds file systems mounted at prefixes such as `/`. A lookup collects every mount whose prefix matches and strips the prefix. Newer mounts are placed in front, so when two have the same file, the most recently mounted one wins. `HttpServer::MountVolume` uses this: `http-base`, `emu-frontend` and `docs` all mount their `static` folders at `/` on the same server.

# URLs
`Url` parses strings like `plugin://emu-frontend@noobwarrior.org/lua/main.lua` into a scheme, a host and a path.

| Scheme | Points at | File system |
| --- | --- | --- |
| `plugin://<identifier>/...` | A plugin's files | The plugin's own |
| `user://...` | The user data folder | `Core::GetUserDataVfs()` |
| `data://...` | `<userdata>/data` | `Core::GetPluginDataVfs()` |
| `install://...` | The install folder | `Core::GetInstallDataVfs()` |
| `file://...` | The local disk | `Core::GetFileVfs()`, rooted at the disk root |
| `db://...` | A file inside a database | Not implemented |
| `http://`, `https://`, `rbxassetid://`, `rbxthumb://` | Network resources | None. `OpenHandle` refuses them. |

## Relative URLs
A string without a scheme is resolved against a `UrlContext`, which supplies a working directory (`Cwd`), a default scheme and a default host. A plugin's autorun scripts get a context with the `plugin` scheme and the plugin's identifier, so `"lua/main.lua"` in a manifest means `plugin://<identifier>/lua/main.lua`. The default context uses `file://` with no host. The `Url.*` unit tests pin down the exact rules.

## Methods
| Method | Returns for `plugin://docs@noobwarrior.org/lua/main.lua` |
| --- | --- |
| `Resolve()` | The full URL. |
| `ResolveWithoutProtocol()` | `docs@noobwarrior.org/lua/main.lua` |
| `GetHostName()` | `docs@noobwarrior.org` |
| `ResolveAsPath()` | `/lua/main.lua` |
| `ResolveAsVfsPath()` | `/lua/main.lua`, the path inside `GetVfs`'s file system |
| `GetDirectory()` | `/lua` |
| `GetVfs(core)` | The `docs` plugin's file system. |
| `OpenHandle(core, ...)` | A handle to the file, opened through `GetVfs` and `ResolveAsVfsPath`. |
| `ResolveAsLocalPath(core)` | A path on disk. See the traps below. |

## Folder URLs
In `user://`, `data://` and `install://` URLs, the first folder is parsed as the host, so `data://master/master.nwdb` has the host `master` and the path `/master.nwdb`. For `data://` the host is meant to be a plugin identifier, which is how `PluginManager::GetPluginFromUrl()` finds the plugin that owns a data folder.

`GetVfs` returns the file system for the whole folder, though, so the host has to go back into the path. `ResolveAsVfsPath()` does that (`/master/master.nwdb`), and so does `ResolveAsLocalPath` (`<userdata>/data/master/master.nwdb`). Pair `GetVfs` with `ResolveAsVfsPath`, never with `ResolveAsPath`.

# Traps
## ResolveAsLocalPath and zips
For a plugin URL, `ResolveAsLocalPath` joins the plugin's path and the inner path, so a zipped plugin gives something like `foo.zip/lua/main.lua`, which doesn't exist. Read plugin files through `Plugin::ReadFile()` or `Url::OpenHandle()`. When a real file is needed, for example because SQLite has to open it, copy it out with `Plugin::ExtractFile()`.

## db:// URLs
`db://` parses but does nothing. `ResolveAsLocalPath` returns an empty path for it, and code that trusts the result opens a database at `""`.
