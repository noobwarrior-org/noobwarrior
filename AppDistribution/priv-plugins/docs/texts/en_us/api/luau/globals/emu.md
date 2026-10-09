```luau
emu: ServerEmulator
```

# Description
The [ServerEmulator](/api/luau/classes/ServerEmulator) that serves Roblox clients, servers and Studio. Plugins use it to add web pages and react to requests the native handlers don't cover.

# Example
```luau
emu.OnRequest:Connect(function(req)
    if req.Uri == "/hello" then
        req:SendReplyString(200, nil, "Hello!")
    end
end)
```
