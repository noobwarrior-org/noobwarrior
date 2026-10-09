```luau
loadstring(source: string, chunkName: string?): (((...any) -> ...any)?, string?)
```

# Description
Compiles a string of Luau source into a function without running it. The function runs in the environment of the script that called `loadstring`, so it sees the same globals.

# Arguments
1. `source: string`: the Luau source code to compile.
2. `chunkName: string?`: the name used in error messages. Defaults to `"=loadstring"`.

# Returns
The compiled function, or `nil` and an error message if the source does not compile.

# Example
```luau
local fn, err = loadstring("return 1 + 2")
if fn then
    print(fn()) -- 3
else
    print("Compile error: " .. err)
end
```
