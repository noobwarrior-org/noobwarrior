return {
    identifier = "plugin@example.com",
    title = "Example Plugin",
    version = "v1.0",
    description = "An example plugin for noobWarrior",
    authors = { "hattozo@noobwarrior.org" },

    -- Runs each file in sequential order, since Lua automatically uses incremental integer-based indexing for its arrays.
    autorun = { "lua/main.lua" },
    engine_autorun = {
        -- Same goes for here
        client = {
            "engine_scripts/client.lua"
        },
        server = {
            "engine_scripts/server.lua"
        },
        shared = {
            "engine_scripts/shared.lua"
        }
    },
    datamodel = {
        { -- Inserting instances into the client side is currently unsupported for now.
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
