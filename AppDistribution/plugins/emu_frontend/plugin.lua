return {
    title = "Server Emulator Frontend",
    version = "v1.0",
    description = "The frontend for noobWarrior's server emulator",
    icon = "icon.png",
    authors = { "Hattozo" },
    autorun = { "main.lua" },
    httpserver = {
        shared = {
            mountpoints = {"static/shared"}
        },
        emulator = {
            mountpoints = {"static/emulator"}
        },
        master = {
            mountpoints = {"static/master"}
        }
    },
    permissions = {  },
    
    -- Prevents the user from being able to disable this plugin.
    -- Only allowed if the plugin is in the installation folder.
    critical = true
}