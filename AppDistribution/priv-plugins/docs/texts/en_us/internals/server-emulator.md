# Server emulator
The server emulator answers the web requests a Roblox engine makes, in place of Roblox's own servers. The hook sends every request on ports 80 and 443 to it (see [Engines](/internals/engines)). It lives in `Core/Source/HttpServer/`.

# HttpServer
`HttpServer` (`HttpServer/Base/`) wraps libevent's `evhttp`. One object can serve plain HTTP and HTTPS at once, on separate ports. HTTPS uses `<userdata>/ssl/cert.pem` and `key.pem`, which the core creates on first run, and the TLS setup only offers HTTP/1.1 because `evhttp` can't speak HTTP/2.

Every server in the program shares one libevent event base owned by `Core`. Nothing happens unless the host program keeps calling `Core::ProcessEvents()`: the desktop app does it from a `QTimer`, and the command-line app from its console loop.

## Threads
Handlers run on the thread that calls `ProcessEvents`, which also owns the SQLite connections and the Luau state. A handler that has to wait on the network hands that wait to a worker thread, and the worker hands its result back with `Core::RunOnEventLoop()` so the reply is sent from the right thread. Workers should do the network call and nothing else.

Blocking the event loop stalls every server in the process. It can also deadlock: if a handler waits on a request to a master server hosted by the same program, that master can't answer until the handler returns.

## Routing
`SetRequestHandler(nullptr, ...)` sets the root handler, which libevent calls for every request. `RootHandler` strips the query string, collapses repeated slashes and compares the path against each registered handler in registration order:

- A pattern without `:` must match the path exactly, including case.
- A pattern with `:name` segments matches paths with the same number of segments. Each `:name` segment captures a non-empty value, which the handler reads with `GetRouteParam("name")`.

The first match handles the request. When nothing matches, `RootHandler` builds the request table that Luau plugins see and fires `OnRequest`. If no listener replies, the server sends an empty `200 OK`.

Because matching is exact, legacy paths need every spelling the engine uses. `/Asset`, `/asset` and `/asset/` are three separate registrations, and so are both casings of `.ashx` paths.

# ServerEmulator
`ServerEmulator` (`HttpServer/Emulator/`) is the `HttpServer` that plays Roblox. Each endpoint is a `Handler` subclass with one method, `OnRequest(evhttp_request*, void*)`. Handlers are members of `ServerEmulator`, held by value, and `ServerEmulator::SetupHandlers()` binds them to paths. There are about 160 bindings.

Paths under `/emu/` are noobWarrior's own and not part of Roblox's API, such as `/emu/v1/process-ping` and `/emu/v1/running-game-servers`.

## Adding an endpoint
1. Create `MyThingHandler.h` and `.cpp` under `HttpServer/Emulator/`, with a class that inherits `Handler` and overrides `OnRequest`.
2. In `ServerEmulator.h`, include the header and add a `MyThingHandler mMyThingHandler;` member.
3. In `SetupHandlers()`, add `SetRequestHandler("/v1/my-thing", &mMyThingHandler);`, plus any other spelling the engine uses.
4. Add the `.cpp` to `Core/CMakeLists.txt`. Sources are listed by name, not found by globbing.

## What belongs in C++
Anything an engine calls has to be a native handler: login, asset delivery, game join, avatar fetch and so on. Those must keep working with no plugins loaded. Pages meant for people, such as the website, forums and control panel, belong in the `emu-frontend` Luau plugin, which receives them through `OnRequest`.

# Joining other servers
When a player joins a server hosted by someone else, their engine still talks to their own emulator, because the hook sends everything there. The local emulator forwards what it can't answer to the host.

That is the job of `EmulatorProxy`, a stack of remote emulators called layers. `Core::LaunchEngine()` pushes a layer for the host being joined. Handlers that may need the host call `ServerEmulator::TryProxyRequest()`:

- With no layers, it returns `false` and the handler answers locally.
- Otherwise it takes over the request and forwards it to the top layer from a worker thread. The first layer to answer with a 2xx status wins. A layer that answers 404 or can't be reached is skipped in favor of the next one down, and the handler's local fallback runs if every layer misses.
- A response transform can change the winning body before it is sent, for example to put the local player's identity into a join script fetched from the host.

Only `Accept` and `Content-Type` are forwarded, plus the joiner's `.LOGINSESSION` cookie for that host when the layer has one. Each emulator has a random instance id for the run, and a request whose proxy chain already includes this emulator is answered locally to stop routing loops.

## Avatars across servers
A joining player's appearance lives on their own machine or their home master server, not on the host. Two mechanisms carry it over:

- Before launching, the joiner posts their appearance to the host's `/emu/v1/avatar-override`. The host keeps it by user id, and `AvatarFetchHandler` serves it when the host's game server asks for that user.
- For federated players, the host records where their home master keeps their avatar. `GetFederatedAvatar()` fetches and caches it. It blocks, so code on the event loop calls `PeekFederatedAvatar()` first and passes a miss to a worker thread. Assets the player wears are fetched from the home master on demand and cached in the scratch database.

The block comments in `ServerEmulator.h` cover the details, and are worth reading before changing any of this.

# Tracking engines
The hook in each engine posts Hello, a heartbeat every 5 seconds and Goodbye to `/emu/v1/process-ping`, handled by `ProcessPingHandler`. The emulator keeps a `RunningInstance` per process id. A process can die without saying Goodbye, so `SweepStaleInstances()` drops any process not heard from in 30 seconds (`kStaleInstanceThresholdSecs`).
