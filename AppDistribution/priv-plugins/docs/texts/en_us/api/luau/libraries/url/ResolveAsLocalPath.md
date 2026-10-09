```luau
url.ResolveAsLocalPath(url: string): string
```

# Description
Turns a URL into a real path on disk. Use it when something outside noobWarrior's file systems needs a file, such as opening a database by path.

This only works for files stored as ordinary files on disk. Two cases give a path that can't be opened:

- A file inside a zipped plugin. The result points inside the `.zip`, which isn't a folder. Read the file through [Plugin:GetVfs](/api/luau/classes/Plugin) instead.
- A `db://` URL. These aren't implemented yet, so the result is `""`.

# Arguments
1. `url: string`: the URL to resolve.

# Returns
The path on disk.

# Example
```luau
local path = url.ResolveAsLocalPath("data://myplugin/data.nwdb")
print("The database is at " .. path)
```
