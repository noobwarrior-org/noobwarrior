# ProtocolType
The scheme at the start of a URL. [url.GetProtocol](/api/luau/libraries/url/GetProtocol) returns one.

| Name | Value | Scheme | Description |
| --- | --- | --- | --- |
| `Unsupported` | 0 | | A scheme noobWarrior doesn't recognize. |
| `File` | 1 | `file://` | A file on the local disk. |
| `Http` | 2 | `http://` | A web address over plain HTTP. |
| `Https` | 3 | `https://` | A web address over HTTPS. |
| `Install` | 4 | `install://` | The folder noobWarrior is installed in. |
| `User` | 5 | `user://` | The user's data directory, which holds databases, plugins and `registry.lua`. |
| `Database` | 6 | `db://` | A file inside a mounted database. Not implemented yet. |
| `Plugin` | 7 | `plugin://` | A file inside a plugin, such as `plugin://docs@noobwarrior.org/lua/main.lua`. |
| `Data` | 8 | `data://` | The folder where plugins keep their writable files, `<userdata>/data`. Each plugin uses a folder named after its identifier. |
| `RbxAssetId` | 9 | `rbxassetid://` | A Roblox asset, by id. |
| `RbxThumb` | 10 | `rbxthumb://` | A Roblox thumbnail. |

# Example
```luau
if url.GetProtocol(path) == ProtocolType.Plugin then
    print(path .. " is inside a plugin")
end
```
