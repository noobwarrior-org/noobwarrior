```luau
url.ResolveAsVfsPath(url: string): string
```

# Description
Returns the path to pass to the [VirtualFileSystem](/api/luau/classes/VirtualFileSystem) from [GetVfs](/api/luau/libraries/url/GetVfs) to reach the URL's file.

For a `plugin://` URL this is the same as [ResolveAsPath](/api/luau/libraries/url/ResolveAsPath). For `data://`, `user://` and `install://` URLs, the first folder after `://` is read as the host, and `ResolveAsPath` leaves it out. `ResolveAsVfsPath` keeps it, because the file system covers the whole folder.

# Arguments
1. `url: string`: the URL to resolve.

# Returns
The path inside the URL's file system.

# Example
```luau
print(url.ResolveAsPath("data://myplugin/save.json"))    -- /save.json
print(url.ResolveAsVfsPath("data://myplugin/save.json")) -- /myplugin/save.json
```
