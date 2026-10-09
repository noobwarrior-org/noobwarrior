```luau
core.HasPermission(session: Session?, permission: string): boolean
```

# Description
Checks whether a user's rank is high enough for a permission. Each permission is a registry key under `emu.permissions` holding the lowest rank allowed to use it. For example, `emu.permissions.ugc.dev.badges` is the rank needed to upload badges.

A few rules come first:

- If `session` is `nil`, the user counts as a guest with rank 0.
- The user whose id is in `emu.ranks.owner_user_id` always has rank 255.
- Rank 255 has every permission, so a broken permissions table can't lock the owner out.
- A permission with no registry key is denied.

# Arguments
1. `session: Session?`: a table from [ResolveSession](/api/luau/libraries/core/ResolveSession), or any table with `Id` and `Rank` fields.
2. `permission: string`: the permission's name without the `emu.permissions.` prefix, such as `"ugc.dev.badges"`.

# Returns
`true` if the user has the permission.

# Example
```luau
local user = core.ResolveSession(_COOKIE[".LOGINSESSION"] or "")
if not core.HasPermission(user, "ugc.dev.games") then
    http_response_code(403)
    die("You can't upload games.")
end
```
