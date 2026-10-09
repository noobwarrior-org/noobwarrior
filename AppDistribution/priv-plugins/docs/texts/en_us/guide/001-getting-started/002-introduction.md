# Introduction
There are a lot of ways into noobWarrior. Where do you even begin?

It depends on what you want to do, but almost everything needs a copy of Roblox first, so start there.

# What you need
noobWarrior doesn't come with any Roblox binaries or assets, and we can't give them to you. To play, host or open anything in Studio, you have to supply your own copy of one of these versions:

| Version | Era | Player | Studio | RCCService |
| --- | --- | --- | --- | --- |
| 0.719.0.7191339 | 2026M | `version-acc4b74f79e743b9` | `version-547feaebd3b44131` | |
| 0.574.0.5740446 | 2023M | `version-08c4cfa3d43c47ef` | `version-c2e4d104afaf449c` | |
| 0.463.0.417004 | 2021E | `version-5a54208fe8e24e87` | `version-ef266da340bc4058` | `version-07b64feec0bd47c1` |

Anything not on this list will probably crash when you try to load it.

Each copy goes in its own folder inside the `engines` folder of your noobWarrior data directory, named after its version hash (for example `engines/version-acc4b74f79e743b9`). noobWarrior looks inside each folder for a Roblox executable it recognizes, so it works out by itself whether a copy is a Player, Studio or RCCService.

The data directory is `%LOCALAPPDATA%\noobWarrior` on Windows, `~/.local/share/noobWarrior` on Linux and `~/Library/Application Support/noobWarrior` on macOS. If you put an empty file named `NW_PORTABLE` next to the noobWarrior executable, it keeps its data in its own folder instead, which is handy for testing without touching your real data.

On Linux and macOS, Roblox runs under Wine.

# The launcher
Opening noobWarrior gives you a small window with three groups of buttons.

Under Play, Online opens the server browser and Start Game Server hosts a game from one of your databases. The first time you open Online, you'll see a disclaimer: servers are run by other people, possibly strangers, and we have no way to moderate them.

Developer Tools has Launch SDK, where you build and edit databases, and Launch Studio, which asks which Studio version to open. The last group, Application, holds Databases, Plugins, Player (your name and avatar), Settings and About.

# Where to start
## Playing
Open Online and add a master server. Master servers list the game servers people are running, and they also host a workshop where you can download databases and plugins. You can browse one without an account. If you already know a server's IP address, Direct Connect skips the list.

## Hosting
Open Start Game Server, pick a database and a place from it, and choose which engine runs it. To let strangers find your server, list it on a master server. People outside your network can only join once you open the ports the dialog shows on your router.

## Archiving and making content
Start with the SDK. Everything noobWarrior serves to the engine, from places and models to users and badges, lives in databases (`.nwdb` files). You can mount several at once, and when two of them have the same item, the higher-priority one wins.

## Changing noobWarrior itself
Read the Plugins section. The website your server shows to players, this documentation and the master server are all Luau plugins, and you can write your own.
