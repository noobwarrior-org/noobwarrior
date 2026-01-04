static const char *global_env_metatable_lua = R"***(
local protected = {
    rawget_path = _G.rawget_path
}

local mt = {
    __index = function()

    end,
    __metatable = "This metatable is locked!"
}
setmetatable(_G, mt)
)***";