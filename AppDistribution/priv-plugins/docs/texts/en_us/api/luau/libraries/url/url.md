# url
Takes apart and resolves noobWarrior URLs. Every function takes the URL as a string and returns a new value. None of them change anything.

Most file paths in noobWarrior are URLs. The scheme says where the file lives, and [ProtocolType](/api/luau/enums/ProtocolType) lists them all. For a plugin URL, the part between `://` and the first `/` is the plugin's identifier:

```
plugin://emu-frontend@noobwarrior.org/lua/main.lua
\____/   \__________________________/\___________/
scheme             host                  path
```

# Functions
| Function | Returns for `plugin://docs@noobwarrior.org/lua/main.lua` |
| --- | --- |
| [GetProtocol](/api/luau/libraries/url/GetProtocol) | `ProtocolType.Plugin` |
| [GetProtocolString](/api/luau/libraries/url/GetProtocolString) | `"plugin"` |
| [GetHostName](/api/luau/libraries/url/GetHostName) | `"docs@noobwarrior.org"` |
| [GetDirectory](/api/luau/libraries/url/GetDirectory) | `"/lua"` |
| [GetFileName](/api/luau/libraries/url/GetFileName) | `"main.lua"` |
| [Resolve](/api/luau/libraries/url/Resolve) | `"plugin://docs@noobwarrior.org/lua/main.lua"` |
| [ResolveWithoutProtocol](/api/luau/libraries/url/ResolveWithoutProtocol) | `"docs@noobwarrior.org/lua/main.lua"` |
| [ResolveAsPath](/api/luau/libraries/url/ResolveAsPath) | `"/lua/main.lua"` |
| [ResolveAsVfsPath](/api/luau/libraries/url/ResolveAsVfsPath) | `"/lua/main.lua"`, the path inside the URL's file system |
| [ResolveAsLocalPath](/api/luau/libraries/url/ResolveAsLocalPath) | The file's path on disk. |
| [ResolveFromCaller](/api/luau/libraries/url/ResolveFromCaller) | A relative path resolved against the plugin that called yours. |
| [GetVfs](/api/luau/libraries/url/GetVfs) | The [VirtualFileSystem](/api/luau/classes/VirtualFileSystem) the URL points into. |
| [GetCwd](/api/luau/libraries/url/GetCwd) | Always `""` from Luau. |
