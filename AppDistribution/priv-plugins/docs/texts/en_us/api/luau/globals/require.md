```luau
require(url: string): any
```

# Description
Loads the script at `url`, runs it, and returns whatever it returned. The result is cached by resolved URL, so requiring the same module twice runs it only once and returns the same value both times.

The URL can point at any plugin's files, including your own, for example `plugin://http-base@noobwarrior.org/lua/base.lua`.

# Arguments
1. `url: string`: the URL of the script to load.

# Returns
The value the module returned.

# Errors
`require` raises an error if the URL's file system does not exist, the file is missing, the module fails to compile or run, or two modules require each other in a loop.

# Example
```luau
local http_base = require("plugin://http-base@noobwarrior.org/lua/base.lua")
```
