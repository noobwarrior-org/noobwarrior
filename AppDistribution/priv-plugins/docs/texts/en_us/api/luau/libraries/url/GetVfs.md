```luau
url.GetVfs(url: string): VirtualFileSystem?
```

# Description
Returns the [VirtualFileSystem](/api/luau/classes/VirtualFileSystem) a URL points into. Pass the result of [ResolveAsVfsPath](/api/luau/libraries/url/ResolveAsVfsPath) to its methods to reach the file.

# Arguments
1. `url: string`: the URL to look up.

# Returns
The file system, or `nil` if there is none for that URL.

# Example
```luau
local target = "data://myplugin/save.json"
local vfs = url.GetVfs(target)
if vfs and vfs:EntryExists(url.ResolveAsVfsPath(target)) then
    print("Found it")
end
```
