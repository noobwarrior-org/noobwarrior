# SignalListener
A connection between a [Signal](/api/luau/classes/Signal) and a function, returned by [Signal:Connect](/api/luau/classes/Signal).

The connection lasts until you call `Disconnect` or the signal goes away. Letting the listener go out of scope doesn't end it, so you only need to keep the listener if you plan to disconnect.

# Methods
## Disconnect
```luau
SignalListener:Disconnect()
```

Stops the function from being called when the signal fires. A function may disconnect its own listener while the signal is firing. Calling `Disconnect` again does nothing.

# Example
```luau
local listener
listener = emu.OnRequest:Connect(function(req)
    if req.Uri == "/once" then
        req:SendReplyString(200, nil, "You only see this the first time.")
        listener:Disconnect()
    end
end)
```
