static const char *plugin_env_metatable_lua = R"***(
local mt = {
    __index = function(self, key)
        if key == "plugin" then
            
        end
        return _G
    end,
    __newindex = function(t, key, value)
    end,
    __metatable = "This metatable is locked!"
}

return mt
)***";