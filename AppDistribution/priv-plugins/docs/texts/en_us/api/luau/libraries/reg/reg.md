# reg
Reads and writes noobWarrior's registry, the settings store saved to `registry.lua` in the user's data directory.

Keys are dotted paths into nested tables, so `emu.auth.enabled` is the `enabled` field of the `auth` table inside `emu`. A key cannot be empty or start or end with a period. Setting a key creates any tables along its path that don't exist yet.

Changes are kept in memory until [reg.Save](/api/luau/libraries/reg/Save) runs or noobWarrior saves the registry itself.

# Functions
| Function | Description |
| --- | --- |
| [GetKeyValue](/api/luau/libraries/reg/GetKeyValue) | Reads a key's value. |
| [SetKeyValue](/api/luau/libraries/reg/SetKeyValue) | Sets a key's value. |
| [SetKeyValueIfNotSet](/api/luau/libraries/reg/SetKeyValueIfNotSet) | Sets a key's value only if it has none yet. |
| [SetKeyComment](/api/luau/libraries/reg/SetKeyComment) | Attaches a comment that is written above the key in `registry.lua`. |
| [Save](/api/luau/libraries/reg/Save) | Writes the registry to disk. |

# Example
A plugin usually declares its settings when it starts, the same way noobWarrior declares its own:

```luau
reg.SetKeyValueIfNotSet("myplugin.greeting", "Hello")
reg.SetKeyComment("myplugin.greeting", "What the plugin says when the server starts.")

print(reg.GetKeyValue("myplugin.greeting"))
```
