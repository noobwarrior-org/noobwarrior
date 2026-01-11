static const char *lhp_env_metatable_lua = R"***(
return function(parentGlobal)
    local mt = {
        __index = function(self, _k)
            return parentGlobal
        end,
        __newindex = function(t, key, value)
        end,
        __metatable = "This metatable is locked!"
    }
    return mt
end
)***";