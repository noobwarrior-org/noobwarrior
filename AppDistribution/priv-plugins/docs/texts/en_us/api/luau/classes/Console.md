# Console
noobWarrior's command shell. Plugins get one from [core.ConsoleAdded](/api/luau/libraries/core/ConsoleAdded) and add their own commands to it.

# Methods
## RegisterCommand
```luau
Console:RegisterCommand(name: string, callback: (ctx: CommandContext) -> (), description: string)
```

Adds a command. When someone types `name` in the shell, `callback` runs with a [CommandContext](/api/luau/classes/CommandContext) holding the words typed after it. The description appears in the shell's help.

# Example
```luau
local startTime = os.time()

core.ConsoleAdded:Connect(function(console)
    console:RegisterCommand("greet", function(ctx)
        local name = ctx.Args[1] or "stranger"
        ctx:Reply("Hello, " .. name .. "! Running since " .. os.date("%c", startTime))
    end, "Greets someone by name.")
end)
```
