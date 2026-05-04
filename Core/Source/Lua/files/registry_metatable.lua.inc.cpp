/*
 * Copyright (C) 2026 Hattozo
 *
 * This file is part of noobWarrior.
 *
 * noobWarrior is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * noobWarrior is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with noobWarrior; if not, see
 * <https://www.gnu.org/licenses/>.
 */
static const char *registry_metatable_lua = R"***(
local metatable
metatable = {
    __index = function(table, index)
        local value = rawget(table, index)
        if value == nil then
            local tbl = {}
            setmetatable(tbl, metatable)
            rawset(table, index, tbl)
            return tbl
        else return value end
    end
}

local function attachMeta(tbl)
    setmetatable(tbl, metatable)
    for k, v in pairs(tbl) do
        if type(v) == "table" then
            attachMeta(v)
        end
    end
end

attachMeta(%s)
)***";