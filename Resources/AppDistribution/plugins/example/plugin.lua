return {
    title = "Example Plugin",
    version = "v1.0",
    description = "An example plugin for noobWarrior",
    authors = { "Hattozo" },

    -- Runs each file in sequential order, since Lua automatically uses incremental integer-based indexing for its arrays.
    autorun = { "main.lua" },
    hook_autorun = { "main.lua" },
    permissions = { PERMISSION_OS_LIBRARY }
}