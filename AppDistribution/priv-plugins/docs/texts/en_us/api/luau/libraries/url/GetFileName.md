```luau
url.GetFileName(url: string): string
```

# Description
Returns everything after the last `/` in the URL.

# Arguments
1. `url: string`: the URL to read.

# Returns
The file name, such as `"main.lua"`.

# Example
```luau
print(url.GetFileName("user://databases/master.nwdb")) -- master.nwdb
```
