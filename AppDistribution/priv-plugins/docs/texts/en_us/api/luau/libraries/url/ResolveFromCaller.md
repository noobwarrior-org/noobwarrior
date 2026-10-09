```luau
url.ResolveFromCaller(path: string): string
```

# Description
Resolves a relative path against the plugin that called into yours, instead of against your own plugin.

This is for library plugins. When `emu-frontend` passes `http-base` a sitemap containing `"/src/index.lhp"`, that path means a file in `emu-frontend`, not in `http-base`. `http-base` calls `ResolveFromCaller` and gets `plugin://emu-frontend@noobwarrior.org/src/index.lhp`.

noobWarrior walks up the call stack until it finds a function from a different plugin than the one calling `ResolveFromCaller`, and resolves the path against that plugin's script. If there is no such function, the path is resolved against your own plugin. A path that already has a scheme comes back unchanged.

# Arguments
1. `path: string`: the path to resolve.

# Returns
The full URL.

# Example
```luau
-- Inside a library plugin
function MyLibrary.Render(page)
    return lhp.RenderFile(url.ResolveFromCaller(page))
end
```
