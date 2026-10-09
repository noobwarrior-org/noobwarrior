```luau
lhp.RenderFile(path: string, globals: { [string]: any }?): string
```

# Description
Renders an LHP file. A relative path is resolved against the calling script's plugin, so `"/src/index.lhp"` means that file in your own plugin.

The page runs in a new environment built on top of the caller's, the same way [Render](/api/luau/libraries/lhp/Render) does.

# Arguments
1. `path: string`: the URL or path of the file.
2. `globals: { [string]: any }?`: extra globals for the page to see.

# Returns
The rendered output.

# Errors
Raises an error if the file can't be read, has a syntax error, or its code fails.

# Example
```luau
local html = lhp.RenderFile("/src/profile.lhp", { userId = 1 })
```
