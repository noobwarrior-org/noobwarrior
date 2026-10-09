# ServerEmulator
The server that stands in for Roblox's website, so Roblox clients, game servers and Studio can run against noobWarrior. There is one, available as the [emu](/api/luau/globals/emu) global.

It is an [HttpServer](/api/luau/classes/HttpServer), so it has every method and event listed there. Its built-in handlers answer the requests Roblox makes. Anything they don't answer goes to `OnRequest`, which is how plugins add web pages.

# Methods
## GetRunningInstances
```luau
ServerEmulator:GetRunningInstances(): { Instance }
```

Lists every Roblox process connected to this emulator: clients, game servers and Studio. Each process reports in every few seconds, and one that stays silent for 30 seconds is dropped from the list.

Each entry has these fields:

| Field | Type | Description |
| --- | --- | --- |
| `Pid` | `number` | The process id. |
| `Side` | `string` | `"Client"`, `"Server"` or `"Studio"`. |
| `Version` | `string` | The Roblox version, such as `"0.463.0.417004"`. |
| `Ip` | `string` | The address it connected from. |
| `Port` | `number?` | The game port, for a game server. |
| `PlaceId` | `number?` | The place it is running. |
| `FirstSeen` | `number` | When it first reported in, as a Unix time. |
| `LastSeen` | `number` | When it last reported in, as a Unix time. |

## GetRunningGameServers
```luau
ServerEmulator:GetRunningGameServers(): { Instance }
```

The same as [GetRunningInstances](#getrunninginstances), but only game servers, and without the `Side` field.

# Example
```luau
for _, server in ipairs(emu:GetRunningGameServers()) do
    print(`Place {server.PlaceId} on port {server.Port}`)
end
```
