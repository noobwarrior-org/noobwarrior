```luau
core.GetUserDataDir(): VirtualFileSystem
```

# Description
Returns a [VirtualFileSystem](/api/luau/classes/VirtualFileSystem) rooted at the user's data directory, which holds databases, user plugins and `registry.lua`. In portable mode this is the install folder. The same folder is reachable by URL as `user://`.
