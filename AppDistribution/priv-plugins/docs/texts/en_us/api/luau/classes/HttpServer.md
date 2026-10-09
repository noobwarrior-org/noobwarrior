# HttpServer
A web server. Plugins create one to serve their own site, as the master server plugin does. The [emu](/api/luau/globals/emu) global is a [ServerEmulator](/api/luau/classes/ServerEmulator), which is an HttpServer, so everything here works on it too.

The server only runs while noobWarrior's event loop is running, which the desktop app takes care of.

# Constructors
## HttpServer.new
```luau
HttpServer.new(logName: string): HttpServer
```

Creates a stopped server. `logName` labels its messages in the log.

# Methods
## Start
```luau
HttpServer:Start(port: number): number
```

Starts serving plain HTTP on `port`, on every network interface. Returns 1 if it started and 0 if it was already running.

## StartSecure
```luau
HttpServer:StartSecure(port: number): number
```

Starts serving HTTPS on `port`, using the certificate and key in the `ssl` folder of the user's data directory (`cert.pem` and `key.pem`). Returns 1 if it started, 0 if it was already running, and a negative number if the certificate or key couldn't be loaded.

## Stop
```luau
HttpServer:Stop(): number
```

Stops serving plain HTTP. `StopSecure()` does the same for HTTPS.

## MountVolume
```luau
HttpServer:MountVolume(root: string, path: string)
```

Serves the files in `path` under the URL path `root`. A relative `path` is resolved against your plugin, so `"/static"` means the `static` folder in your plugin. Several plugins can mount at the same `root`: their files are layered together.

Raises an error if the folder can't be mounted.

## UnmountVolume
```luau
HttpServer:UnmountVolume(root: string, path: string)
```

Stops serving a folder mounted with [MountVolume](#mountvolume). Unlike `MountVolume`, it doesn't resolve a relative `path` against your plugin, so pass the full URL.

## GetVfs
```luau
HttpServer:GetVfs(): VirtualFileSystem
```

Returns the layered [VirtualFileSystem](/api/luau/classes/VirtualFileSystem) of everything mounted on the server.

# Events
Each of these is a [Signal](/api/luau/classes/Signal).

| Event | Parameters | Fires |
| --- | --- | --- |
| `OnRequest` | [HttpRequest](/api/luau/classes/HttpRequest) | For each request that no built-in handler answered. |
| `PreStart` | `secure: boolean` | Just before the server starts. |
| `PostStart` | `secure: boolean` | Just after the server starts. |
| `PreStop` | `secure: boolean` | Just before the server stops. |
| `PostStop` | `secure: boolean` | Just after the server stops. |

`secure` is `true` for the HTTPS side and `false` for plain HTTP.

# Example
```luau
local server = HttpServer.new("MyServer")
server:MountVolume("/", "/static")

server.OnRequest:Connect(function(req)
    if req.Uri == "/time" then
        req:AddHeader("Content-Type", "text/plain")
        req:SendReplyString(200, nil, os.date("%c"))
    end
end)

server:Start(8090)
```

Most plugins pass the server to the `http-base` plugin instead of handling `OnRequest` themselves. It adds routing, sessions, cookies and LHP pages.
