# core
Functions for reaching noobWarrior's own systems: its folders, its databases and the accounts on this server.

# Functions
| Function | Description |
| --- | --- |
| [GetVersion](/api/luau/libraries/core/GetVersion) | Returns noobWarrior's version string. |
| [GetInstallDataDir](/api/luau/libraries/core/GetInstallDataDir) | Returns the file system for the install folder. |
| [GetUserDataDir](/api/luau/libraries/core/GetUserDataDir) | Returns the file system for the user's data directory. |
| [GetPluginDataDir](/api/luau/libraries/core/GetPluginDataDir) | Returns the file system for plugin data. |
| [GetEmuDbManager](/api/luau/libraries/core/GetEmuDbManager) | Returns the object that holds every mounted database. |
| [GetMasterDatabase](/api/luau/libraries/core/GetMasterDatabase) | Returns the master database. |
| [ResolveSession](/api/luau/libraries/core/ResolveSession) | Looks up the account behind a login session token. |
| [HasPermission](/api/luau/libraries/core/HasPermission) | Checks whether an account's rank allows something. |
| [GetPermissionRank](/api/luau/libraries/core/GetPermissionRank) | Returns the lowest rank that has a permission. |
| [BuildAvatarFetchJson](/api/luau/libraries/core/BuildAvatarFetchJson) | Builds a user's avatar description in the format Roblox clients expect. |

# Events
| Event | Description |
| --- | --- |
| [ConsoleAdded](/api/luau/libraries/core/ConsoleAdded) | Fires when a console is created, so a plugin can add commands to it. |
