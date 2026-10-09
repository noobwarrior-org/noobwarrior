```luau
core.BuildAvatarFetchJson(userId: number): string
```

# Description
Builds the avatar description for a user in the master database, as the JSON body Roblox clients expect from `/v1.1/avatar-fetch`. The master-server plugin uses it to share a user's avatar with other servers.

# Arguments
1. `userId: number`: the id of a user in the master database.

# Returns
The avatar description as a JSON string.
