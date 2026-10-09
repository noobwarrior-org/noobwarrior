```luau
reg.SetKeyComment(key: string, comment: string)
```

# Description
Attaches a comment to `key`. When the registry is saved, the comment is written above the key in `registry.lua`, so someone editing the file by hand knows what the setting does.

# Arguments
1. `key: string`: the dotted path of the key. It can name a table, which comments the whole group.
2. `comment: string`: the comment text.

# Example
```luau
reg.SetKeyValueIfNotSet("myplugin.max_players", 12)
reg.SetKeyComment("myplugin.max_players", "How many players can join at once.")
```
