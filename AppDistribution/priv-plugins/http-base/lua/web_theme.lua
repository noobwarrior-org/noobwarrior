-- ////////////////////////////////////////////////////////////////////////////////
-- noobWarrior
-- Plugin: HTTP Server Base
-- File: web_theme.lua
-- Description: Builds /css/theme.css from the style a host chose for a website
-- Started by: Hattozo
-- Started on: 10/10/2026
-- ////////////////////////////////////////////////////////////////////////////////
local web_theme = {}

local BUILT_IN_STYLES = { fluent = true, darcula = true }

local WEB_VARIABLES = {
    fluent = {
        shell = { "--nw-backdrop-outer", "--nw-backdrop-inner", "--nw-backdrop-center" },
        pane = { "--nw-content" },
        toolBar = { "--nw-navbar" },
        control = { "--nw-surface", "--nw-surface-raised", "--nw-surface-strong" },
        controlHover = { "--nw-hover" },
        controlBorder = { "--nw-border" },
        border = { "--nw-divider-strong" },
        subtleBorder = { "--nw-divider" },
        text = { "--nw-text" },
        textSecondary = { "--nw-text-soft", "--nw-text-muted" },
        textPlaceholder = { "--nw-text-faint" },
        textDisabled = { "--nw-text-footer" },
        accent = { "--nw-accent" },
        accentHover = { "--nw-link-hover" },
        accentPressed = { "--nw-accent-pressed" },
        onAccent = { "--nw-on-accent" },
    },
    darcula = {
        window = { "--nw-backdrop-outer", "--nw-backdrop-inner", "--nw-backdrop-center" },
        base = { "--nw-content" },
        button = { "--nw-surface", "--nw-surface-raised", "--nw-surface-strong" },
        dark = { "--nw-border" },
        mid = { "--nw-divider-strong" },
        text = { "--nw-text", "--nw-text-soft" },
        placeholderText = { "--nw-text-muted", "--nw-text-faint", "--nw-text-footer" },
        highlight = { "--nw-accent" },
        link = { "--nw-link" },
    },
}

local DERIVATIONS = {
    fluent = {
        { target = "controlHover", source = "control", dark = 8, light = -7 },
        { target = "accentHover", source = "accent", dark = 14, light = 14 },
        { target = "accentPressed", source = "accent", dark = -22, light = -22 },
    },
    darcula = {},
}

local function shift(color, amount)
    local function channel(value)
        return math.clamp(value + amount, 0, 255)
    end
    return { R = channel(color.R), G = channel(color.G), B = channel(color.B), A = color.A }
end

local function css_color(color)
    if color.A == 255 then
        return string.format("rgb(%d, %d, %d)", color.R, color.G, color.B)
    end
    return string.format("rgba(%d, %d, %d, %.3f)", color.R, color.G, color.B, color.A / 255)
end

local function colors_for_scheme(style, scheme)
    local colors = {}
    for name, color in pairs(style.Colors) do
        colors[name] = color
    end
    local variant = style.DarkColors
    if scheme == "light" then
        variant = style.LightColors
    end
    if variant then
        for name, color in pairs(variant) do
            colors[name] = color
        end
    end
    for _, derivation in ipairs(DERIVATIONS[style.Base] or {}) do
        local source = colors[derivation.source]
        if source and not colors[derivation.target] then
            colors[derivation.target] = shift(source, derivation[scheme])
        end
    end
    return colors
end

local function variable_lines(style, scheme, indent)
    local mapping = WEB_VARIABLES[style.Base] or {}
    local lines = {}
    for name, color in pairs(colors_for_scheme(style, scheme)) do
        for _, variable in ipairs(mapping[name] or {}) do
            table.insert(lines, string.format("%s%s: %s;", indent, variable, css_color(color)))
        end
    end
    table.sort(lines)
    return lines
end

local function scheme_block(style, scheme)
    local lines = variable_lines(style, scheme, "        ")
    if #lines == 0 then
        return ""
    end
    return string.format("@media (prefers-color-scheme: %s) {\n    :root {\n%s\n    }\n}\n", scheme, table.concat(lines, "\n"))
end

local function forced_scheme_block(style, scheme)
    local lines = variable_lines(style, scheme, "    ")
    table.insert(lines, 1, string.format("    color-scheme: %s;", scheme))
    return string.format(":root {\n%s\n}\n", table.concat(lines, "\n"))
end

local function only_scheme(style)
    if style.DarkColors and not style.LightColors then
        return "dark"
    elseif style.LightColors and not style.DarkColors then
        return "light"
    end
    return nil
end

local function read_stylesheet(stylesheet_url)
    local vfs = url.GetVfs(stylesheet_url)
    if vfs == nil then
        return nil
    end
    local data = vfs:ReadFile(url.ResolveAsVfsPath(stylesheet_url))
    return data and buffer.tostring(data)
end

web_theme.VISITOR_COOKIE = "nw_style"
web_theme.PLAIN = "none"

function web_theme.GetDefaultStyleId(target)
    local style_id = reg.GetKeyValue(target .. ".web_style")
    if style_id == "app" then
        style_id = reg.GetKeyValue("gui.theme")
    end
    if type(style_id) ~= "string" or style_id == "" or BUILT_IN_STYLES[style_id] then
        return nil
    end
    return style_id
end

local function percent_decode(value)
    return (value:gsub("%%(%x%x)", function(hex)
        return string.char(tonumber(hex, 16))
    end))
end

function web_theme.GetVisitorChoice(cookies)
    local value = cookies and cookies[web_theme.VISITOR_COOKIE]
    if type(value) ~= "string" then
        return ""
    end
    return percent_decode(value)
end

function web_theme.ResolveStyle(target, visitor_choice)
    if visitor_choice == web_theme.PLAIN then
        return nil
    end
    if type(visitor_choice) == "string" and visitor_choice ~= "" then
        local chosen = core.GetDeclaredStyle(visitor_choice)
        if chosen then
            return chosen
        end
    end
    local default_id = web_theme.GetDefaultStyleId(target)
    if default_id == nil then
        return nil
    end
    local style = core.GetDeclaredStyle(default_id)
    if style == nil then
        print(string.format("[web_theme] No mounted plugin offers style \"%s\" for the %s website", default_id, target))
    end
    return style
end

function web_theme.BuildStylesheet(target, visitor_choice)
    local style = web_theme.ResolveStyle(target, visitor_choice)
    if style == nil then
        return ""
    end

    local parts = {}
    local scheme = only_scheme(style)
    if scheme then
        table.insert(parts, forced_scheme_block(style, scheme))
    else
        table.insert(parts, scheme_block(style, "dark"))
        table.insert(parts, scheme_block(style, "light"))
    end
    local stylesheet_url = style.Web[target]
    if stylesheet_url then
        local source = read_stylesheet(stylesheet_url)
        if source then
            table.insert(parts, source)
        else
            print(string.format("[web_theme] Style \"%s\" could not read its stylesheet %s", style.QualifiedId, stylesheet_url))
        end
    end
    return table.concat(parts, "\n")
end

return web_theme
