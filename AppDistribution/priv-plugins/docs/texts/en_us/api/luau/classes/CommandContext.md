# CommandContext
Passed to a command registered with [Console:RegisterCommand](/api/luau/classes/Console). It holds what the user typed and lets the command answer.

# Properties
| Property | Type | Description |
| --- | --- | --- |
| `Args` | `{ string }` | The words typed after the command name, split on spaces. |

# Methods
## Reply
```luau
CommandContext:Reply(message: string)
```

Prints `message` in the shell the command was typed into.
