# serpent
[Serpent](https://github.com/pkulchenko/serpent) turns Luau tables into Luau source code and back. noobWarrior uses it to write `registry.lua`. Use it to save data in a form a person can read and edit.

# Functions
| Function | Description |
| --- | --- |
| `serpent.line(value, options?)` | Writes the value on one line. |
| `serpent.block(value, options?)` | Writes the value across several indented lines. |
| `serpent.dump(value, options?)` | Writes the value as a chunk of code that rebuilds it. |
| `serpent.load(text, options?)` | Reads text written by the functions above. Returns `true` and the value, or `false` and an error message. |

The options are described in Serpent's own documentation.

# Example
```luau
local text = serpent.block({ port = 8080, admins = { "Builderman" } }, { comment = false })
print(text)

local ok, value = serpent.load(text)
if ok then
    print(value.port) -- 8080
end
```
