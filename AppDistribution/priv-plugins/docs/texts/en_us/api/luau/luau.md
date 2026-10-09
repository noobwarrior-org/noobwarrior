# Luau
Plugins are written in [Luau](https://luau.org), the Lua dialect Roblox uses. It differs from standard Lua 5.x: `buffer`, string interpolation and type annotations are available, and `io` is not.

These standard libraries are loaded: `coroutine`, `string`, `os`, `math`, `table`, `debug`, `bit32`, `utf8`, `buffer` and `vector`, plus the base functions.

# The script environment
Every script runs in its own sandboxed environment. A global you assign in one script is not visible from another. To share a value between scripts, put it in `_G`, which every script sees.

Each script also gets a few globals of its own:

| Global | What it is |
| --- | --- |
| [script](/api/luau/globals/script) | The [Script](/api/luau/classes/Script) that is running. |
| [plugin](/api/luau/globals/plugin) | The [Plugin](/api/luau/classes/Plugin) the script belongs to. |
| [print](/api/luau/globals/print) | Writes to the noobWarrior log, tagged with the script's URL. |
| [require](/api/luau/globals/require) | Loads another script by URL and returns its result. |

# Numbers and ids
Luau numbers are 64-bit floats. Roblox ids above 2^53 cannot be represented exactly, though no real id is that large yet. When you pass a whole number to a database function, noobWarrior binds it as a 64-bit integer, so ids above 2^31 are not truncated.
