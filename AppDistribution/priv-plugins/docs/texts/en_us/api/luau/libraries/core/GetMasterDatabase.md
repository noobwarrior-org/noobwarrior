```luau
core.GetMasterDatabase(): EmuDb
```

# Description
Returns the master database, the first one mounted. It holds the data the whole server shares: user accounts, login sessions and forum posts.

# Example
```luau
local db = core.GetMasterDatabase()
local users = db:QueryTyped("SELECT Id, Name FROM User WHERE Id = ?", 1)
if users then
    print(users[1].Name)
end
```
