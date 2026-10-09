```luau
url.GetHostName(url: string): string
```

# Description
Returns the part of the URL between `://` and the first `/`. For a plugin URL, this is the plugin's identifier. A `file://` URL has no host.

# Arguments
1. `url: string`: the URL to read.

# Returns
The host name, or `""` for a `file://` URL.

# Example
```luau
print(url.GetHostName("plugin://emu-frontend@noobwarrior.org/lua/main.lua"))
-- emu-frontend@noobwarrior.org
```
