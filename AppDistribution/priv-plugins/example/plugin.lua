return {
    identifier = "plugin@example.com",
    title = "Example Plugin",
    version = "v1.0",
    description = "An example plugin for noobWarrior",
    authors = { "hattozo@noobwarrior.org" },

    -- Runs each file in sequential order, since Lua automatically uses incremental integer-based indexing for its arrays.
    autorun = { "lua/main.lua" },
    hook_autorun = {
        -- Same goes for here
        client = {
            "hook_scripts/client.lua"
        },
        server = {
            "hook_scripts/server.lua"
        },
        shared = {
            "hook_scripts/shared.lua"
        }
    },
    datamodel = {
        {
            dir = "datamodel_client",
            side = "Client",
            predicate = function(data)
                -- example: data.UniverseId, data.PlaceId
                return true
            end
        },
        {
            dir = "datamodel_server",
            side = "Server",
            predicate = function(data)
                -- example: data.UniverseId, data.PlaceId
                return true
            end
        }
    },
    permissions = { PERMISSION_OS_LIBRARY }
}