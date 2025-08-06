static const char *rawget_path_lua = R"***(
rawget_path = function(tbl, path)
    local current = tbl
    for key in path:gmatch("[^%.]+") do
        if type(current) ~= "table" then
            return nil
        end
        current = rawget(current, key)
        if current == nil then
            return nil
        end
    end
    return current
end
)***";