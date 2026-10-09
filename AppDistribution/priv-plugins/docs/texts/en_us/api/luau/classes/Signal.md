# Signal
An event that functions can listen for. noobWarrior uses signals to tell plugins that something happened, such as [HttpServer.OnRequest](/api/luau/classes/HttpServer) or [core.ConsoleAdded](/api/luau/libraries/core/ConsoleAdded). You can also make your own.

# Constructors
## Signal.new
```luau
Signal.new(): Signal
```

Creates a signal with no listeners.

# Methods
## Connect
```luau
Signal:Connect(callback: (...any) -> ()): SignalListener
```

Adds `callback` to the signal's listeners. Every time the signal fires, `callback` is called with the values passed to [Fire](#fire). Listeners run in the order they connected.

Returns a [SignalListener](/api/luau/classes/SignalListener).

## Fire
```luau
Signal:Fire(...: any)
```

Calls every listener with the given values.

# Example
```luau
local playerJoined = Signal.new()

playerJoined:Connect(function(name)
    print(name .. " joined")
end)

playerJoined:Fire("Builderman") -- Builderman joined
```
