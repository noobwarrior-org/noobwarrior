```luau
core.GetPermissionRank(permission: string): number?
```

# Description
Returns the lowest rank that has a permission, read from `emu.permissions.<permission>` in the registry.

# Arguments
1. `permission: string`: the permission's name without the `emu.permissions.` prefix.

# Returns
The rank, or `nil` if the permission isn't defined.

# Example
```luau
print("Uploading games needs rank " .. tostring(core.GetPermissionRank("ugc.dev.games")))
```
