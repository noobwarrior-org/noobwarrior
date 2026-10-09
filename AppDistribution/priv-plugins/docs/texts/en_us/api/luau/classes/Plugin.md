# Plugin
A loaded plugin. Scripts reach their own plugin through the [plugin](/api/luau/globals/plugin) global.

# Methods
## GetIdentifier
```luau
Plugin:GetIdentifier(): string
```

Returns the plugin's identifier from its manifest, such as `"emu-frontend@noobwarrior.org"`. This is also the host name in the plugin's `plugin://` URLs.

## GetVfs
```luau
Plugin:GetVfs(): VirtualFileSystem
```

Returns a [VirtualFileSystem](/api/luau/classes/VirtualFileSystem) for the plugin's files. It works the same whether the plugin is a folder or a `.zip`, which makes it the safe way to read your own files.

# Example
```luau
local vfs = plugin:GetVfs()
local data = vfs:ReadFile("/data/defaults.json")
if data then
    local defaults = json.parse(buffer.tostring(data))
end
```
