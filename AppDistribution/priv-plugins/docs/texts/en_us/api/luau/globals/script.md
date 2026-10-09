```luau
script: Script
```

# Description
The [Script](/api/luau/classes/Script) object for the script that is currently running. Every script gets its own.

Functions that take a relative path, such as [lhp.RenderFile](/api/luau/libraries/lhp/RenderFile) and [HttpServer:MountVolume](/api/luau/classes/HttpServer), use `script` to work out what the path is relative to.

# Example
```luau
local html = lhp.RenderFile("/src/page.lhp") -- resolved against this script's plugin
```
