```luau
url.ResolveAsPath(url: string): string
```

# Description
Returns the path part of the URL, starting from the first `/` after the host. [VirtualFileSystem](/api/luau/classes/VirtualFileSystem) methods expect paths in this form.

# Arguments
1. `url: string`: the URL to resolve.

# Returns
The path, such as `"/lua/main.lua"`.

# Example
```luau
print(url.ResolveAsPath("https://example.com/watch?v=1")) -- /watch?v=1
```
