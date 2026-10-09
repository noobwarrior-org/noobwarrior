```luau
core.GetEmuDbManager(): EmuDbManager
```

# Description
Returns the [EmuDbManager](/api/luau/classes/EmuDbManager), which holds every mounted database and looks items up across all of them. The global [emu_db_mgr](/api/luau/globals/emu_db_mgr) is the same object.

# Example
```luau
for _, db in ipairs(core.GetEmuDbManager():GetMountedDatabases()) do
    print(db:GetTitle())
end
```
