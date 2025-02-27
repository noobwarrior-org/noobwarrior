# noobWarrior
An all-in-one toolkit to help aid the preservation and archival of Roblox content, so that you can edit and play Roblox games in an untouched state; even if they entirely disappear off the Roblox website. It's invaluable for both developers and players (although it benefits mostly developers as of now)

List of features:
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

## Current Issues
- I don't have access to Roblox's OAuth2 thing and I am not giving my government ID to them
    - This unfortunately means that you will have to login to your Roblox account using your email and password. If you don't trust this program, then don't log in. You are free to audit the source code and compile it yourself if you wish.

## Tools
### Archive Editor
What all of your games 

### Import Game as Archive

### Import File as Archive

### Preserve Game
Downloads all places in the experience and their corresponding thumbnails. Also finds and downloads all of the assets used in each place, and compiles all of it to a folder on your disk. You can only use this tool for uncopylocked games and games where you have edit access.

### Save Place Assets
Finds and downloads all of the assets in a local place file (.rbxl) and saves it onto your disk. It will only be able to download assets that your account is able to access.

### Download Asset(s)
Downloads an asset, or a list of assets, and saves it to disk. You can only download assets that your account is able to access.

### Explorer
Previews a model or a place file 

### Edit Archive

### Play Archive

### Host Server

### Join Server

## Is this safe?
If it wasn't, my reputation would be ruined by now.

If you really don't trust it, then just don't use it.

## Disclaimer
This was made to help prevent media from becoming lost; I do not condone the use of this project for nefarious activites, and would actively discourage you from doing so. Remember, it is a computer software that is indifferent to both good and bad actors.