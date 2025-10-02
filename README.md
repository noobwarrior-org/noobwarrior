# noobWarrior
noobWarrior is a self-contained toolkit & decentralized platform for everything Roblox. It aims to be an incredibly invaluable tool for the Roblox community, serving players, developers, content creators, and archivists.

You can:
- Back up and preserve your own games, accessories & models so that they won't be crippled by Roblox updates
    - You can also upload your games (containing all assets & full game functionality) to other websites, so that other people can download your stuff and host it on their servers.
    - And you can play it offline, or with friends!
    - And you'll be able to play them even while Roblox is down, or when it ceases to exist.
    - The forced privatization of audios and removal of unrated experiences comes to mind...
- Run your own servers and participate in server lists, reminicisent of the now-forgotten decentralized nature of the Internet
- Create mods for existing games using the database & plugins system, and share them with others
- Freely upload your own assets without an AI moderator deleting your entire account over a false positive
- Play any version of Roblox, in case a game you want to play is broken on newer versions, or if you just want nostalgia
- Easily participate in communities that encourage hobbyist creativity instead of for-profit creations that exploit children

May I say more?

## Manifesto
Roblox has had a history of imposing certain features or updates on their website that have severely crippled the state of many games, most of them abandoned. The forced privatization of audios and the removal of unrated experiences comes to mind. A single company cannot just take away the only copy of a beloved game and say, "oh, you can't play this anymore." You'd say that's bullshit. Of course it is. So why not try to prevent that from happening?

I started realizing that no one in the community was actually taking measures to have any sort of failsafes in place for when these sorts of things happen. You complain and pray, but a solution is never thought of. So I took it upon myself to actually do something about it because it was frustrating me.

noobWarrior tries to address all of these problems by not relying on any sort of online Roblox infrastracture as much as possible. It aims to be decentralized by delegating many of its online capabilities to separated services that either you yourself or other volunteers are able to maintain, without putting all of those features to the power of a mega-corporation.

For example, a typical Roblox game has all of its places, images, and audios hosted on the Roblox website, with that being the only source of data. Once that is gone, it's very hard to get it back, especially because retrieving the IDs and files for these assets is getting harder than ever. No one was able to back it up because they couldn't; it was all streamed online and cached to random files on their disk, and later deleted in order to save space. With noobWarrior, it's being done the right way like God intended. You share copies of your games to your friends by distributing the files, others host servers using those copies, those other people find it interesting and share it to more people. Now you have thousands of copies in circulation that otherwise would've never seen the light of day had it been on Roblox. Sound familiar? Because that's literally how the Internet works. You might as well work with a system instead of going against it. noobWarrior tries to make Roblox work like this.

And then we get into the general problem of Big Tech. You can apply what's happening with Roblox to pretty much all of the big websites. YouTube, Reddit, Discord, Facebook, Twitter (not X,) whatever. No one likes getting their account deleted and deplatformed because a faulty algorithm decided that would be the correct punishment for doing something that it thought you did wrong. These websites are just simply way too large to be moderated by actual people, not robots. Roblox is no exception; do you seriously think there are human workers manually checking the hundreds of assets that are uploaded every single second? Exactly, no. Everything goes through a process of complicated algorithms that check how loud your audio is, if it can read the text on an image, if it somehow finds your audio in a content ID database, whatever. How about the chat logs? Human moderators cannot be doing all of that for trillions of messages, sent by hundreds of millions of active users, in hundreds of thousands of games. It's just not possible.

Well, once upon a time, you'd be banned from a small forum, or a community game server, because the human moderators thought you were being an ass. And then you would just move onto a different server that respected you. That's what decentralization and delegation of authority gives you. There wasn't a big website owning a million servers like it is now. noobWarrior aims to go back to the old ways and have people self-govern their own servers.

I hope you can see where I'm going by saying this.

## Technicals
Items like games, images, audios, or accessories on Roblox are organized into databases, which can be mounted and layered on top of each other.
A server emulator is ran in the background, mimicking the Roblox API and serving files/metadata from these databases.

Users connect to these server emulators using the noobWarrior launcher, which is then set as a proxy (so that they can access asset files from the server hoster) and they are then coordinated to the right game servers.
The Roblox client is patched on the fly to have all asset and API requests redirect to the server emulator.

A master database is used to automatically store all user-generated actions/content that can occur from players interacting in your game servers, like friending each other.

## Features
### Database Utility
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

## Examples
- Backup your own Roblox games and all of the assets they use so that you have a frozen state of your game
    - It backs up EVERYTHING. Place files (and all of their contained assets,) thumbnails, badges, gamepasses, game description, visit count, like/dislikes, whatever you can name.
    - It compiles all of this data into an archive database where it can be easily retrieved using the built in archive editor.
    - Provides a built-in feature to convert archives into human-readable folders.
    - Provides a built-in feature to upload it to the Internet Archive so that other people can enjoy it
    - If you do not have edit access to the game you are trying to backup, it will not back up the place files and the assets within them, but it will still try to save all other publically available content.
    - You can also use local place files outside of the Roblox website and it'll try to download the assets and import it as an archive.
- Lets you access Studio without an internet connection by mimicking the Roblox servers
- Lets you play an older offline build of Roblox without an internet connection by mimicking the Roblox servers
    - This lets you play your archives, as it will redirect all assets to your backed up local versions of them. Even gamepasses and badges will be functional, since all of the data has been backed up.
    - This uses an older version of Roblox from 2021 since newer versions have anti-cheat (Hyperion) and they cannot be modified as easily.
        - I have no idea how to approach this, but right now it's not really a problem.
- Lets you view the tree of models and place files and write to them without having to open Studio
- Lets you download and save any asset(s) you want, guaranteed you have viewing permissions to the specified assets in question (although you could easily just do this without having to install this program)

## Frequently Asked Questions
### Are there any current issues with this program?
- I don't have access to Roblox's OAuth2 thing and I am not giving my government ID to them
    - This unfortunately means that you will have to login to your Roblox account using your email and password. If you don't trust this program, then don't log in. You are free to audit the source code and compile it yourself if you wish.

### Is this safe?
All source code is viewable and pre-compiled binaries have not been tampered with. I do not expect everyone to know how to read code, so if you do not trust this program then don't use it.

Also, if I ever added any malware to this program, then my reputation would be ruined by now.

## Really Important Disclaimer That Exists Because People Suck
This was made to help prevent media from becoming lost, and to provide an alternative to the official Roblox services for people who do not wish to use them; I do not condone the use of this project for nefarious activites, and I actively discourage you from doing so. Remember, it is computer software that is indifferent to both good and bad actors. If you see anyone using this program for any illegal activities, I recommend you report it to law enforcement immediately.

I also do not recommend children to connect to random servers using this program. The internet was never intended to be used by children, and Roblox proves that. Can parents parent again?
