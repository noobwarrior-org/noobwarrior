# json
Converts between Luau values and JSON text. This is a small bundled library, and its functions are named `stringify` and `parse`. There is no `json.encode` or `json.decode`.

# Functions
| Function | Description |
| --- | --- |
| `json.stringify(value: any): string` | Turns a value into JSON text. |
| `json.parse(text: string): any` | Turns JSON text into a value. |

# Values
| Value | Description |
| --- | --- |
| `json.null` | Stands in for JSON's `null`. A Luau table can't hold `nil`, so `parse` puts this in its place. Check for it with `value == json.null`. |

# Arrays and objects
A table is written as a JSON array only if its keys are exactly 1, 2, 3 and so on with no gaps. Any other table becomes an object. Tables can't refer back to themselves, and object keys must be strings or numbers.

# Example
```luau
local text = json.stringify({ name = "Builderman", friends = { 1, 2, 3 } })
local data = json.parse(text)
print(data.name, #data.friends) -- Builderman 3
```
