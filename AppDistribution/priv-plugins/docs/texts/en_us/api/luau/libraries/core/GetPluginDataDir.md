```luau
core.GetPluginDataDir(): VirtualFileSystem
```

# Description
Returns a [VirtualFileSystem](/api/luau/classes/VirtualFileSystem) rooted at the folder where plugins keep their writable files. The same folder is reachable by URL as `data://`.

# Example
```luau
local dir = core.GetPluginDataDir()
if dir:CreateDirectories("myplugin") then
    dir:WriteFile("myplugin/hello.txt", "Hello!")
end
```
