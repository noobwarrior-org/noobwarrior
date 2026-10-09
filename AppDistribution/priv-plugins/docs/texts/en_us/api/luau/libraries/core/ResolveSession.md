```luau
core.ResolveSession(token: string): Session?
```

# Description
Looks up the account a login session belongs to. When someone logs in, noobWarrior sets a `.LOGINSESSION` cookie holding a session token. Pass that token here to find out who is making a request.

Sessions expire after the number of days in `emu.auth.session_ttl_days`, which is 30 by default.

# Arguments
1. `token: string`: the value of the `.LOGINSESSION` cookie.

# Returns
`nil` if the token doesn't match a live session. Otherwise a table with these fields:

| Field | Type | Description |
| --- | --- | --- |
| `Id` | `number` | The user's id. |
| `Name` | `string` | The user's username. |
| `DisplayName` | `string` | The user's display name. |
| `Rank` | `number` | The user's rank on this server, from 0 to 255. |
| `IsGuest` | `boolean` | Whether the user is a guest without an account. |
| `IsFederated` | `boolean` | Whether the account comes from another master server. |

# Example
```luau
local user = core.ResolveSession(_COOKIE[".LOGINSESSION"] or "")
if user then
    echo("Hello, " .. user.DisplayName)
else
    echo("You are not logged in.")
end
```
