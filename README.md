# noobWarrior
noobWarrior is a self-contained toolkit & decentralized platform for everything Roblox. It aims to be an incredibly invaluable tool for anyone who has to frequently work with the Roblox platform. This includes the players, developers, content creators, archivists, the employees who work there, et cetera.

If you make games on Roblox, or even just play it without ever making anything on the site, I guarantee that you'll find something of value in this project.

You can:
- Back up and preserve your own games, accessories & models so that you have a frozen state of your game that can't be crippled by Roblox updates or moderation
    - It backs up everything. Place files (and all of their contained assets,) thumbnails, badges, gamepasses, game description, visit count, like/dislikes, groups, users, whatever you can name.
        - And by extension, you can use your preserved version as a template for how your game should look and feel when Roblox moderation decides to delete half the assets that your game uses. It can't all just disappear in noobWarrior, can it?
    - It compiles all of this data into a database where it can all be easily retrieved using the built in database editor.
        - noobWarrior databases are SQLite database files that contain records of Roblox items, like assets, games, groups, & users. They can be layered on top of one another when running a live server.
        - You can convert these databases into human-readable folders, so that you can view all data from your operating system's file manager.
        - You can upload these database files to other websites, so that other people can download them and seamlessly host servers containing your games with all the needed assets and full game functionality.
    - If you do not have edit access to the game you are trying to backup, it will not back up the place files and the assets within them, but it will still try to save all other publically available content.
    - You can also use local place files outside of the Roblox website and it'll try to download the assets and import it as a database.
    - And you can play it offline, or with friends!
    - And you'll be able to play them while Roblox is down, or when it ceases to exist.
- Visit games created by other people without having any fears that they will eventually be broken in one way or another because of a change made by Roblox
- Play almost any version of Roblox before 2021, in case a game you want to play is broken on newer versions, or if you just want nostalgia
- Freely upload your own items without an AI moderator deleting your entire account over a false positive
- Access Studio without an internet connection by mimicking the Roblox servers
- View the tree of models and place files and write to them without having to open Studio
- Download and save any asset(s) you want, guaranteed you have viewing permissions to the specified assets in question
- Run your own servers and participate in server lists, reminicisent of the now-forgotten decentralized nature of the Internet
- Create mods for existing games using the database & plugins system, and share them with others
- Administer servers at ease using a web-based interface
- Easily participate in communities that encourage hobbyist creativity instead of for-profit creations that exploit children
- PLAY ROBLOX ON LINUX!!! WAOWWW!
    - (correction: you can't actually play games on the main Roblox platform, but you can use our version of the Roblox client to play on noobWarrior servers using Linux)
- Re-enable previously removed features as they have been re-implemented for experimental purposes
    - Sets, tickets, group walls, comments on games, profile status, etc.
- Create your own "revivals" by tinkering the settings of the server emulator to function like a Roblox website clone

May I say more?

## Technicals
Items like games, images, audios, or accessories on Roblox are organized into databases, which can be mounted and layered on top of each other.
A server emulator is ran in the background, mimicking the Roblox API and serving files/metadata from these databases.

Users connect to these server emulators using the noobWarrior launcher, which is then set as a proxy (so that they can access asset files from the server hoster) and they are then coordinated to the right game servers.
The Roblox client is patched on the fly to have all asset and API requests redirect to the server emulator.

A master database is used to automatically store all user-generated actions/content that can occur from players interacting in your game servers, like friending each other.

## Features
### Database Editor
All noobWarrior database files are edited and viewed using this tool, and it allows you to backup almost any form of user-generated content on the Roblox website, granted you have the appropriate permissions.

This includes entire Roblox games (thumbnails, places, visits, badges, etc), assets (models, clothing, gear, ...), user profiles (outfits, inventories), whatever the hell you can name.

If you are backing up an off-sale model or a copylocked game that you do not have edit access to, it will not back up the data files and the assets within them, but it will still try to save all other publically available content.

All of this data is compiled into a database where it can be easily retrieved using the built-in database utility.

There is also a built-in feature to convert your databases into human-readable folders, and then upload them to the Internet Archive.

### Server Emulator
The server emulator plays several roles. It:
- Serves all the asset files from the database so that you can retrieve them from the Roblox client
- Mimics the Roblox API so that everything in-game can work as intended
- Lets you view and administer the current state of the program remotely (adding new assets, starting/stopping game servers, etc.)
- Lets your players be able to create user-generated content in your database from a web interface, if you allow them to.

### Plugins
Plugins extend the functionality of noobWarrior

WIP

### Download Asset(s)
Downloads an asset, or a list of assets, and saves it to disk. You can only download assets that your account is able to access.

One killer feature that this tool provides over other Roblox asset downloaders is that if your asset is an .rbxm/.rbxl file, it will attempt to read it and search for all asset IDs found within the model and then download them.

### Model/Place Explorer
Previews a Roblox model or a place file

### Scan Roblox Clients
Scans any selected drives for Roblox clients, and then gives you a list of what it found. You can choose to upload these versions to the Internet Archive.

### Scan Roblox Cache
Goes through all files in your Roblox cache directory and presents them in a readable format.

### Offline Studio
Allows you to use Roblox Studio entirely offline by mimicking the Roblox servers and proxying the program to connect to that instead.

### Host/Join Server & Play Solo
Allows you to play Roblox games entirely offline using the data that is provided by your saved archives.

Note that only certain builds of Roblox can be used for this feature, as the required server software for most versions is not publically available, plus the addition of the Hyperion anti-cheat in 2023-now.

## (Probably) Frequently Asked Questions
### Are there any current issues with this program?
- I don't have access to Roblox's OAuth2 thing and I am not giving my government ID to them
    - This unfortunately means that you will have to login to your Roblox account using your email and password. If you don't trust this program, then don't log in. You are free to audit the source code and compile it yourself if you wish.

### Is this safe?
All source code is viewable and pre-compiled binaries have not been tampered with. I do not expect everyone to know how to read code, so if you do not trust this program then don't use it.

Also, if I ever added any malware to this program, then my reputation would be ruined by now. I didn't spend multiple years working on this project just to infect people's computers.

### Why would I ever want to use this?
Did you not read the beginning of the text?

### What's preventing this program from being used by bad actors?
Good samitarians, private investigators & law enforcement. This is discussed in the disclaimer below. I made this program for a legitimate use case, and if there are people who abuse it in order to commit crimes, then maybe it makes more sense to put them behind bars instead of blaming the people who created the program.

### How can I contribute to the project?
Great question. Having knowledge of both C++ and the guts of Roblox would help a lot. Raising awareness for the project also works. Please make sure you're talking about it to people who would actually seem like they're interested, not someone who would be there just to annoy people.

If you meet the criteria for programming as I mentioned, well, I don't have a Discord server for this right now. Sorry

### Is there any documentation on how to actually use this thing?
I'm working on it. I know it's complicated. Sorry.

### Will you ever focus on working on a FOSS version of the Roblox engine?
This is way too ahead of the roadmap, but making it in a basic form is actually very plausible regardless of what some people say. You don't have to do anything crazy like reverse engineering binaries. Most of the concepts are high-level enough that there are only four big concerns: performance, physics accuracy, lighting, and terrain. More complex systems like layered clothing & facial expressions also exist, but I don't really care about those since they aren't as foundational to Roblox compared to the main concerns that I brought up.

Me and a buddy of mine have already been experimenting with this idea, but I've been allocating more of my time to working on this instead. If I think I've done enough for this project, maybe we can move ahead to working on that instead and make it compatible with noobWarrior.

## Really Important Disclaimer That Exists Because People Suck
This was made to help prevent media from becoming lost, and to provide an alternative to the official Roblox services for people who do not wish to use them; I do not condone the use of this project for nefarious activites, and I actively discourage you from doing so. Remember, it is computer software that is indifferent to both good and bad actors. If you see anyone using this program for any illegal activities, I recommend you report it to law enforcement immediately.

I also do not recommend children to connect to random servers using this program. DARPA didn't intend for the Internet to be used by children, and the controversy surrounding Roblox proves that. Can parents parent again?
