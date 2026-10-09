# EmuDbManager
Holds every mounted [EmuDb](/api/luau/classes/EmuDb) and looks items up across all of them. Get it from [core.GetEmuDbManager](/api/luau/libraries/core/GetEmuDbManager) or the [emu_db_mgr](/api/luau/globals/emu_db_mgr) global.

Databases are searched in the order they were mounted, and the first one that has an item wins. That lets one database override an item that also exists in another.

# Finding databases
## GetMasterDatabase
```luau
EmuDbManager:GetMasterDatabase(): EmuDb
```

Returns the master database, which holds users, login sessions and forum posts.

## GetMountedDatabases
```luau
EmuDbManager:GetMountedDatabases(): { EmuDb }
```

Returns every mounted database, in the order they are searched. The master database is first.

## GetFirstDbWhereItemExists
```luau
EmuDbManager:GetFirstDbWhereItemExists(itemType: ItemType, id: number): EmuDb?
```

Returns the first database in search order that has the item, or `nil` if none does.

## GetDbFromFileName
```luau
EmuDbManager:GetDbFromFileName(name: string): EmuDb?
```

Returns the mounted database with this file name, such as `"master.nwdb"`.

# Looking up items
These work like the [EmuDb](/api/luau/classes/EmuDb) methods of the same name, but search every mounted database and use the first answer found.

| Method | Returns |
| --- | --- |
| `GetItemName(itemType, id)` | `string?` |
| `GetCreatorUserId(itemType, id)` | `number?` |
| `GetUniverseIdForPlace(placeId)` | `number?` |
| `GetStartPlaceIdForUniverse(universeId)` | `number?` |
| `GetAssetSummary(id)` | `AssetSummary?` |
| `SearchAssetIds(assetType, keyword, limit, offset)` | `{ number }` |
| `RetrieveImageData(itemType, id)` | `string` |
| `RetrieveAssetData(id, version)` | `(SqlResponse, string, string)` |

# Example
```luau
local db = emu_db_mgr:GetFirstDbWhereItemExists(ItemType.Universe, 1)
if db then
    print("Game 1 comes from " .. db:GetTitle())
end
```
