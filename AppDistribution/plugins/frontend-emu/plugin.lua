return {
    id = "frontend-emu@noobwarrior.org",
    title = "Server Emulator Frontend",
    version = "v1.0",
    description = "The frontend for noobWarrior's server emulator",
    icon = "icon.png",
    authors = { "hattozo@noobwarrior.org" },
    autorun = { "lua/main.lua" },
    permissions = {  },
    
    -- Prevents the user from being able to disable this plugin.
    -- Only allowed if the plugin is in the installation folder.
    critical = true
}