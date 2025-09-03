# noobWarrior
noobWarrior is a preservation toolkit and server emulator for Roblox; it lets you play and host entirely offline Roblox games on your computer that do not rely on the official Roblox services by leveraging your backed up content. It's invaluable for both developers and players.

## Technicals
Content for a piece of media on Roblox is organized into databases, which can be mounted and layered on top of each other.
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
