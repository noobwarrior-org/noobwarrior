```luau
url.GetDirectory(url: string): string
```

# Description
Returns the folder part of the URL's path, without the host or the file name.

# Arguments
1. `url: string`: the URL to read.

# Returns
The folder path, such as `"/lua"`.

# Example
```luau
print(url.GetDirectory("plugin://docs@noobwarrior.org/lua/main.lua")) -- /lua
```
