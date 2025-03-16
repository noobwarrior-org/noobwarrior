# noobWarrior
An all-in-one toolkit to help aid the preservation and archival of Roblox content, so that you can edit and play Roblox games in an untouched state; even if they entirely disappear off the Roblox website. It's invaluable for both developers and players (although it benefits mostly developers as of now)

## Tools
### Archive Utility
The main part of the program. All noobWarrior archive files are edited and viewed using this tool, and it allows you to backup EVERYTHING. Entire Roblox games (thumbnails, places, visits, badges, etc), assets (models, clothing, gear, ...), user profiles (outfits, inventories), whatever the hell you can name. Of course, you must have the appropriate permissions for your account in order to back up any private data.

If you are backing up a game and you do not have edit access, it will not back up the place files and the assets within them, but it will still try to save all other publically available content.

All of this data is compiled into an archive database where it can be easily retrieved using the built-in archive utility.

There is also a built-in feature to convert your archives into human-readable folders, and then upload them to the Internet Archive.

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
All source code is viewable and compiled binaries have not been tampered with. I do not expect everyone to know how to read code, so if you do not trust this program then don't use it.

Also, if I ever added any malware to this program, then my reputation would be ruined by now.

## Disclaimer
This was made to help prevent media from becoming lost; I do not condone the use of this project for nefarious activites, and would actively discourage you from doing so. Remember, it is a computer software that is indifferent to both good and bad actors.