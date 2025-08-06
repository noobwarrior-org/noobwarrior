static const char *config_metatable_lua = R"***(
local metatable
metatable = {
    __index = function(table, index)
        local value = rawget(table, index)
        if value == nil then
            local tbl = {}
            setmetatable(tbl, metatable) -- recursively do this :)
            rawset(table, index, tbl)
            return tbl
        else return value end
    end
}
setmetatable(%s, metatable)
)***";