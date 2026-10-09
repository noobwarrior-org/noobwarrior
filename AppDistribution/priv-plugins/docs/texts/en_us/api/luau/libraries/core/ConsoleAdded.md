```luau
core.ConsoleAdded: Signal
```

# Description
A [Signal](/api/luau/classes/Signal) that fires with a [Console](/api/luau/classes/Console) whenever one is created. Connect to it to add your own commands to noobWarrior's shell.

# Parameters
1. `console: Console`: the console that was created.

# Example
```luau
core.ConsoleAdded:Connect(function(console)
    console:RegisterCommand("hello", function(ctx)
        ctx:Reply("Hello from my plugin!")
    end, "Says hello.")
end)
```
