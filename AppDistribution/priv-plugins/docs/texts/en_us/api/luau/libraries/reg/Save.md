```luau
reg.Save(): boolean
```

# Description
Writes the registry to `registry.lua`. Empty tables are left out.

# Returns
`true` if the file was written, `false` if it wasn't.

# Example
```luau
reg.SetKeyValue("myplugin.enabled", true)
if not reg.Save() then
    print("Could not save the registry")
end
```
