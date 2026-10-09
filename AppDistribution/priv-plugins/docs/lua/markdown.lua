-- ////////////////////////////////////////////////////////////////////////////////
-- noobWarrior
-- Plugin: Documentation
-- File: markdown.lua
-- Description: Converts Markdown documents into HTML for the documentation pages
-- Started by: Hattozo
-- Started on: 10/9/2026
-- ////////////////////////////////////////////////////////////////////////////////
local md = {}

local renderBlocks: (lines: { string }, tight: boolean, ids: { [string]: number }) -> string

local function escape(s: string): string
    return (s:gsub("&", "&amp;"):gsub("<", "&lt;"):gsub(">", "&gt;"):gsub("\"", "&quot;"))
end

local function isBlank(line: string?): boolean
    return line == nil or line:match("^%s*$") ~= nil
end

local function indentOf(line: string): number
    return #line:match("^( *)")
end

local function trim(s: string): string
    return s:match("^%s*(.-)%s*$")
end

local function slugify(text: string): string
    local slug = text:gsub("<[^>]+>", ""):lower():gsub("[^%w%s%-_]", ""):gsub("%s+", "-")
    return slug
end

local function uniqueId(ids: { [string]: number }, text: string): string
    local id = slugify(text)
    if ids[id] then
        ids[id] += 1
        return `{id}-{ids[id]}`
    end
    ids[id] = 0
    return id
end

local function set(words: string): { [string]: boolean }
    local s = {}
    for word in words:gmatch("%S+") do
        s[word] = true
    end
    return s
end

local LUAU_KEYWORDS = set([[
    and break continue do else elseif end export for function if in local not or repeat return then
    type typeof until while
]])
local LUAU_LITERALS = set("true false nil")
local LUAU_BUILTINS = set([[
    assert error getmetatable ipairs next pairs pcall print rawequal rawget rawlen rawset require
    select setmetatable tonumber tostring unpack xpcall loadstring gcinfo newproxy
    bit32 buffer coroutine debug math os string table utf8 vector _G
    core reg url hash crypto lang lhp json serpent emu emu_db_mgr script plugin
    echo include exit die _GET _POST _COOKIE _SESSION _PARAMS _FILES header http_response_code setcookie
]])
local LUAU_TYPES = set("any boolean buffer never nil number string thread unknown userdata vector")

local function highlightLuau(src: string): string
    local out = {}
    local pos = 1
    local len = #src
    local prevSignificant = ""

    local function emit(class: string?, text: string)
        out[#out + 1] = if class then `<span class="tok-{class}">{escape(text)}</span>` else escape(text)
    end

    while pos <= len do
        local c = src:sub(pos, pos)
        local token, class

        local longOpen = src:match("^%-%-%[(=*)%[", pos)
        if longOpen then
            local _, stop = src:find(`]{longOpen}]`, pos, true)
            token, class = src:sub(pos, stop or len), "com"
        elseif src:sub(pos, pos + 1) == "--" then
            token, class = src:match("^[^\n]*", pos), "com"
        elseif src:match("^%[=*%[", pos) then
            local level = src:match("^%[(=*)%[", pos)
            local _, stop = src:find(`]{level}]`, pos, true)
            token, class = src:sub(pos, stop or len), "str"
        elseif c == "\"" or c == "'" or c == "`" then
            local i = pos + 1
            while i <= len do
                local ch = src:sub(i, i)
                if ch == "\\" then
                    i += 2
                elseif ch == c or ch == "\n" then
                    break
                else
                    i += 1
                end
            end
            if src:sub(i, i) == "\n" then
                i -= 1
            end
            token, class = src:sub(pos, math.min(i, len)), "str"
        elseif c:match("%d") or (c == "." and src:sub(pos + 1, pos + 1):match("%d")) then
            token = src:match("^0[xX][%x_]+", pos) or src:match("^0[bB][01_]+", pos)
                or src:match("^[%d_]*%.?[%d_]*[eE][%+%-]?%d+", pos) or src:match("^[%d_]*%.?[%d_]+", pos)
                or c
            class = "num"
        elseif c:match("[%a_]") then
            token = src:match("^[%a_][%w_]*", pos)
            local after = src:match("^%s*(.)", pos + #token) or ""
            if LUAU_KEYWORDS[token] then
                class = "kw"
            elseif LUAU_LITERALS[token] then
                class = "lit"
            elseif (prevSignificant == ":" or prevSignificant == "->" or prevSignificant == "|" or prevSignificant == "&")
                and after ~= "(" and after ~= "." and (LUAU_TYPES[token] or token:match("^%u")) then
                class = "type"
            elseif after == "(" or after == "\"" or after == "{" then
                class = if prevSignificant == "." or prevSignificant == ":" or not LUAU_BUILTINS[token] then "fn" else "builtin"
            elseif LUAU_BUILTINS[token] and prevSignificant ~= "." and prevSignificant ~= ":" then
                class = "builtin"
            elseif token:match("^%u") and prevSignificant ~= "." and prevSignificant ~= ":" then
                class = "type"
            end
        elseif c:match("%s") then
            token = src:match("^%s+", pos)
        else
            token = src:match("^%->", pos) or src:match("^%.%.%.?", pos) or c
        end

        emit(class, token)
        if not token:match("^%s+$") then
            prevSignificant = if class == "com" then prevSignificant else token
        end
        pos += #token
    end

    return table.concat(out)
end

local HIGHLIGHTERS: { [string]: (string) -> string } = {
    luau = highlightLuau,
    lua = highlightLuau,
}

local function inline(text: string): string
    local slots = {}
    local function stash(html)
        slots[#slots + 1] = html
        return `\1{#slots}\2`
    end

    text = text:gsub("\\([%p])", function(c)
        return stash(escape(c))
    end)
    text = text:gsub("(`+)(.-)%1", function(_, code)
        return stash(`<code>{escape(code:match("^ ?(.-) ?$"))}</code>`)
    end)
    text = text:gsub("<(https?://[^>%s]+)>", function(url)
        return stash(`<a href="{escape(url)}">{escape(url)}</a>`)
    end)
    text = escape(text)
    text = text:gsub("!%[([^%]]*)%]%(([^%)%s]+)%)", function(alt, src)
        return stash(`<img src="{src}" alt="{alt}">`)
    end)
    text = text:gsub("%[([^%]]+)%]%(([^%)%s]+)%)", function(label, href)
        return stash(`<a href="{href}">`) .. `{label}</a>`
    end)
    text = text:gsub("%*%*(.-)%*%*", "<strong>%1</strong>")
    text = text:gsub("%f[%w_]__(.-)__%f[^%w_]", "<strong>%1</strong>")
    text = text:gsub("%*([^%*%s][^%*]-)%*", "<em>%1</em>")
    text = text:gsub("%f[%w_]_([^_%s][^_]-)_%f[^%w_]", "<em>%1</em>")
    text = text:gsub("~~(.-)~~", "<del>%1</del>")

    local restored
    repeat
        text, restored = text:gsub("\1(%d+)\2", function(i)
            return slots[tonumber(i)]
        end)
    until restored == 0
    return text
end

local function headingOf(line: string): (number?, string?)
    local hashes, content = line:match("^ ? ? ?(#+)%s+(.-)%s*$")
    if not hashes then
        hashes = line:match("^ ? ? ?(#+)%s*$")
        content = ""
    end
    if hashes and #hashes <= 6 then
        return #hashes, (content:gsub("%s+#+$", ""))
    end
end

local function isRule(line: string): boolean
    local stripped = line:gsub("%s", "")
    return #stripped >= 3 and (stripped:match("^%-+$") or stripped:match("^%*+$") or stripped:match("^_+$")) ~= nil
end

local function fenceOf(line: string): (string?, string?, number?)
    local indent, fence, info = line:match("^( ? ? ?)(```+)%s*(.-)%s*$")
    if not fence then
        indent, fence, info = line:match("^( ? ? ?)(~~~+)%s*(.-)%s*$")
    end
    if fence then
        return fence, info, #indent
    end
end

local function listMarkerOf(line)
    local indent, marker, space, rest = line:match("^( *)([%*%-%+])( +)(.*)$")
    if marker then
        return #indent, marker, rest, #indent + 1 + #space, false
    end
    local number, delim
    indent, number, delim, space, rest = line:match("^( *)(%d+)([%.%)])( +)(.*)$")
    if number then
        return #indent, delim, rest, #indent + #number + 1 + #space, true, tonumber(number)
    end
end

local function isTableDelimiter(line: string?): boolean
    if not line or not line:find("-", 1, true) then
        return false
    end
    local row = trim(line):gsub("^|", ""):gsub("|$", "")
    for cell in (row .. "|"):gmatch("([^|]*)|") do
        if not trim(cell):match("^:?%-+:?$") then
            return false
        end
    end
    return true
end

local function splitRow(line: string): { string }
    local row = trim(line):gsub("^|", ""):gsub("|$", "")
    local cells = {}
    for cell in (row .. "|"):gmatch("([^|]*)|") do
        cells[#cells + 1] = trim(cell)
    end
    return cells
end

local function startsBlock(line: string): boolean
    return headingOf(line) ~= nil
        or fenceOf(line) ~= nil
        or isRule(line)
        or line:match("^ ? ? ?>") ~= nil
        or line:match("^ ? ? ?</?%a[%w%-]*[%s/>]") ~= nil
        or line:match("^ ? ? ?[%*%-%+] +%S") ~= nil
        or line:match("^ ? ? ?1[%.%)] +%S") ~= nil
end

local function parseList(lines: { string }, i: number, out: { string }, ids: { [string]: number }): number
    local baseIndent, baseMarker, _, _, ordered, start = listMarkerOf(lines[i])
    local items = {}
    local loose = false
    local current, contentIndent

    while i <= #lines do
        local line = lines[i]
        local indent, marker, rest, ci = listMarkerOf(line)
        if marker and indent <= baseIndent + 1 and marker == baseMarker then
            current = { rest }
            items[#items + 1] = current
            contentIndent = ci
            i += 1
        elseif isBlank(line) then
            local nextLine = lines[i + 1]
            if nextLine == nil or isBlank(nextLine) then
                break
            end
            local nIndent, nMarker = listMarkerOf(nextLine)
            if nMarker == baseMarker and nIndent <= baseIndent + 1 then
                loose = true
                i += 1
            elseif indentOf(nextLine) >= contentIndent then
                current[#current + 1] = ""
                i += 1
            else
                break
            end
        elseif indentOf(line) >= contentIndent then
            current[#current + 1] = line:sub(contentIndent + 1)
            i += 1
        elseif not isBlank(current[#current]) and not startsBlock(line) then
            current[#current + 1] = trim(line)
            i += 1
        else
            break
        end
    end

    local tag = if ordered then "ol" else "ul"
    if ordered and start ~= 1 then
        out[#out + 1] = `<ol start="{start}">`
    else
        out[#out + 1] = `<{tag}>`
    end
    for _, item in ipairs(items) do
        out[#out + 1] = `<li>{renderBlocks(item, not loose, ids)}</li>`
    end
    out[#out + 1] = `</{tag}>`
    return i
end

renderBlocks = function(lines, tight, ids)
    local out = {}
    local i = 1

    while i <= #lines do
        local line = lines[i]
        local fence, info, fenceIndent = fenceOf(line)
        local level, headingText = headingOf(line)

        if isBlank(line) then
            i += 1

        elseif fence then
            local code = {}
            i += 1
            while i <= #lines do
                local closing = fenceOf(lines[i])
                if closing and closing:sub(1, 1) == fence:sub(1, 1) and #closing >= #fence and lines[i]:match("^%s*[`~]+%s*$") then
                    i += 1
                    break
                end
                local l = lines[i]
                local strip = math.min(fenceIndent, indentOf(l))
                code[#code + 1] = l:sub(strip + 1)
                i += 1
            end
            local lang = info:match("^(%S+)")
            local class = if lang then ` class="language-{escape(lang)}"` else ""
            local highlighter = lang and HIGHLIGHTERS[lang:lower()]
            local source = table.concat(code, "\n")
            local body = if highlighter then highlighter(source) else escape(source)
            out[#out + 1] = `<pre><code{class}>{body}</code></pre>`

        elseif level then
            local id = uniqueId(ids, headingText)
            out[#out + 1] = `<h{level} id="{id}">{inline(headingText)}</h{level}>`
            i += 1

        elseif isRule(line) then
            out[#out + 1] = "<hr>"
            i += 1

        elseif line:match("^ ? ? ?>") then
            local quoted = {}
            while i <= #lines and not isBlank(lines[i]) do
                quoted[#quoted + 1] = lines[i]:gsub("^ ? ? ?> ?", "")
                i += 1
            end
            out[#out + 1] = `<blockquote>{renderBlocks(quoted, false, ids)}</blockquote>`

        elseif listMarkerOf(line) then
            i = parseList(lines, i, out, ids)

        elseif indentOf(line) >= 4 then
            local code = {}
            while i <= #lines and (indentOf(lines[i]) >= 4 or (isBlank(lines[i]) and lines[i + 1] and indentOf(lines[i + 1]) >= 4)) do
                code[#code + 1] = escape(lines[i]:sub(5))
                i += 1
            end
            out[#out + 1] = `<pre><code>{table.concat(code, "\n")}</code></pre>`

        elseif line:match("^ ? ? ?</?%a[%w%-]*[%s/>]") or line:match("^ ? ? ?</?%a[%w%-]*$") then
            local html = {}
            while i <= #lines and not isBlank(lines[i]) do
                html[#html + 1] = lines[i]
                i += 1
            end
            out[#out + 1] = table.concat(html, "\n")

        elseif line:find("|", 1, true) and isTableDelimiter(lines[i + 1]) then
            local headers = splitRow(line)
            local aligns = {}
            for n, cell in ipairs(splitRow(lines[i + 1])) do
                if cell:match("^:.*:$") then
                    aligns[n] = " style=\"text-align: center\""
                elseif cell:match(":$") then
                    aligns[n] = " style=\"text-align: right\""
                elseif cell:match("^:") then
                    aligns[n] = " style=\"text-align: left\""
                else
                    aligns[n] = ""
                end
            end
            local html = { "<div class=\"table-wrap\"><table><thead><tr>" }
            for n, cell in ipairs(headers) do
                html[#html + 1] = `<th{aligns[n] or ""}>{inline(cell)}</th>`
            end
            html[#html + 1] = "</tr></thead><tbody>"
            i = i + 2
            while i <= #lines and not isBlank(lines[i]) and lines[i]:find("|", 1, true) do
                html[#html + 1] = "<tr>"
                local cells = splitRow(lines[i])
                for n = 1, #headers do
                    html[#html + 1] = `<td{aligns[n] or ""}>{inline(cells[n] or "")}</td>`
                end
                html[#html + 1] = "</tr>"
                i += 1
            end
            html[#html + 1] = "</tbody></table></div>"
            out[#out + 1] = table.concat(html)

        else
            local para = {}
            local setext
            while i <= #lines and not isBlank(lines[i]) do
                local l = lines[i]
                if #para > 0 and l:match("^ ? ? ?=+%s*$") then
                    setext = 1
                    i += 1
                    break
                elseif #para > 0 and l:match("^ ? ? ?%-+%s*$") then
                    setext = 2
                    i += 1
                    break
                elseif #para > 0 and startsBlock(l) then
                    break
                end
                para[#para + 1] = l
                i += 1
            end
            local parts = {}
            for n, l in ipairs(para) do
                local hardBreak = n < #para and (l:match("  +$") or l:match("\\$"))
                l = trim(l)
                if hardBreak and l:sub(-1) == "\\" then
                    l = l:sub(1, -2)
                end
                parts[#parts + 1] = inline(l) .. (if hardBreak then "<br>" else "")
            end
            local content = table.concat(parts, "\n")
            if setext then
                local id = uniqueId(ids, content)
                out[#out + 1] = `<h{setext} id="{id}">{content}</h{setext}>`
            elseif tight then
                out[#out + 1] = content
            else
                out[#out + 1] = `<p>{content}</p>`
            end
        end
    end

    return table.concat(out, "\n")
end

function md.ToHtml(text: string): string
    text = text:gsub("\r\n?", "\n"):gsub("\t", "    ")
    local lines = {}
    for line in (text .. "\n"):gmatch("([^\n]*)\n") do
        lines[#lines + 1] = line
    end
    return renderBlocks(lines, false, {})
end

return md
