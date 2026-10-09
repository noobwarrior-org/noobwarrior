# EmuDb
A noobWarrior database (`.nwdb` file) of Roblox items: assets, games, users, badges and so on. It is an SQLite database underneath, so it also has every [SqlDb](/api/luau/classes/SqlDb) method for running your own queries.

To work with the databases the user has mounted, get them from [core.GetEmuDbManager](/api/luau/libraries/core/GetEmuDbManager) or [core.GetMasterDatabase](/api/luau/libraries/core/GetMasterDatabase). Use `EmuDb.new` only to open a file of your own.

Methods that change data return a [SqlResponse](/api/luau/enums/SqlResponse). A database that is read-only, or whose owner turned off runtime changes, refuses them.

# Constructors
## EmuDb.new
```luau
EmuDb.new(url: string, autocommit: boolean?): EmuDb
```

Opens the database at `url`, creating it if it doesn't exist, and upgrades it to the current format.

`autocommit` is `false` unless you pass `true`. With it off, changes are held until you call [WriteChangesToDisk](#writechangestodisk), which suits a batch of edits. With it on, every change is written as it happens.

# Static functions
## EmuDb.IsZstdCompressed
```luau
EmuDb.IsZstdCompressed(data: string): boolean
```

Returns `true` if `data` is compressed with Zstandard.

## EmuDb.RetrieveAssetTypeImageData
```luau
EmuDb.RetrieveAssetTypeImageData(assetType: AssetType): string
```

Returns the placeholder image noobWarrior shows for an [AssetType](/api/luau/enums/AssetType) that has no thumbnail.

# Saving
## WriteChangesToDisk
```luau
EmuDb:WriteChangesToDisk(): SqlResponse
```

Commits pending changes to the file.

## SaveAs
```luau
EmuDb:SaveAs(url: string): SqlResponse
```

Writes a copy of the database to `url`.

## IsDirty
```luau
EmuDb:IsDirty(): boolean
```

Returns `true` if there are changes that haven't been written yet. `MarkDirty()` and `UnmarkDirty()` set this flag by hand.

## GetMigrationVersion
```luau
EmuDb:GetMigrationVersion(): number
```

Reads SQLite's `user_version` field. noobWarrior doesn't set it, so it doesn't tell you the database's format version.

## GetMigrationFailMsg
```luau
EmuDb:GetMigrationFailMsg(): string
```

Returns the error from the last failed format upgrade, if there was one.

# Database details
These describe the database itself. They appear in noobWarrior's database list.

| Read | Write | Description |
| --- | --- | --- |
| `GetTitle(): string` | `SetTitle(title: string)` | The database's name. |
| `GetDescription(): string` | `SetDescription(text: string)` | A description of what it holds. |
| `GetVersion(): string` | `SetVersion(version: string)` | The database's own version, chosen by its author. |
| `GetAuthor(): string` | `SetAuthor(author: string)` | Who made it. |
| `GetIcon(): string` | `SetIcon(image: string)` | The icon image's bytes. |
| `GetMetaKeyValue(key: string): string` | `SetMetaKeyValue(key: string, value: string)` | Any other detail, by name. |
| `GetCompressionType(): CompressionType` | | How asset data is compressed. See [CompressionType](/api/luau/enums/CompressionType). |

# Items
Every item has a type and an id. The type is an [ItemType](/api/luau/enums/ItemType) and picks the table the item lives in.

## AddItem
```luau
EmuDb:AddItem(itemType: ItemType, row: { [string]: any }): SqlResponse
```

Adds an item. The keys of `row` are column names in the item's table, such as `Id`, `Name` and `Description`. Keys the table doesn't have are skipped.

## UpdateItem
```luau
EmuDb:UpdateItem(itemType: ItemType, id: number, row: { [string]: any }): SqlResponse
```

Changes the columns listed in `row` and leaves the rest alone.

## DeleteItem
```luau
EmuDb:DeleteItem(itemType: ItemType, id: number): SqlResponse
```

Deletes an item, along with the rows that depend on it. Stored data that nothing uses anymore is removed too.

## DoesItemExist
```luau
EmuDb:DoesItemExist(itemType: ItemType, id: number): boolean
```

Returns `true` if this database has the item.

## GetItemName
```luau
EmuDb:GetItemName(itemType: ItemType, id: number): string?
```

Returns the item's name, or `nil` if it isn't here.

## GetCreatorUserId
```luau
EmuDb:GetCreatorUserId(itemType: ItemType, id: number): number?
```

Returns the id of the user who made the item, or `nil` if it isn't here or a group made it.

## RetrieveImageData
```luau
EmuDb:RetrieveImageData(itemType: ItemType, id: number): string
```

Returns the item's thumbnail or icon as image bytes.

# Assets
## GetAssetSummary
```luau
EmuDb:GetAssetSummary(id: number): AssetSummary?
```

Returns an asset's main details, or `nil` if it isn't here. The table has these fields:

| Field | Type | Description |
| --- | --- | --- |
| `Id` | `number` | The asset's id. |
| `Name` | `string` | Its name. |
| `Description` | `string` | Its description. |
| `Type` | [AssetType](/api/luau/enums/AssetType) | What kind of asset it is. |
| `UserId` | `number?` | The creator, if a user made it. |
| `GroupId` | `number?` | The creator, if a group made it. |
| `Created` | `number` | When it was created, as a Unix time. |
| `Updated` | `number` | When it last changed, as a Unix time. |

## SearchAssetIds
```luau
EmuDb:SearchAssetIds(assetType: AssetType, keyword: string, limit: number, offset: number): { number }
```

Finds assets of a type whose names contain `keyword`, newest first. Pass `AssetType.None` to search every type and `""` to match every name. `limit` is how many to return, and `offset` skips that many for paging.

The result is a list you can index and measure with `#`. Loop over it with a numeric `for` loop, because `ipairs` doesn't accept it.

## RetrieveAssetData
```luau
EmuDb:RetrieveAssetData(id: number, version: number): (SqlResponse, string, string)
```

Reads an asset's content, such as a place file or an image. Pass `0` as `version` for the latest one. Returns the result, the content, and the content's SHA-256 hash.

## AttachDataToAsset
```luau
EmuDb:AttachDataToAsset(id: number, version: number, data: string): SqlResponse
```

Stores content as a version of an asset. Identical content is stored only once, however many assets use it.

## DetachDataFromAsset
```luau
EmuDb:DetachDataFromAsset(id: number, version: number): SqlResponse
```

Removes a version's content from an asset.

## AddBlob
```luau
EmuDb:AddBlob(data: string): (SqlResponse, string)
```

Stores content without attaching it to anything. Returns the result and the content's hash, which [AttachBlobHashToAsset](#attachblobhashtoasset) can use later.

## AttachBlobHashToAsset
```luau
EmuDb:AttachBlobHashToAsset(id: number, version: number, hash: string): SqlResponse
```

Makes already-stored content a version of an asset. `DetachBlobHashFromAsset(id, version, hash)` undoes it.

## AttachThumbnailDataToAsset
```luau
EmuDb:AttachThumbnailDataToAsset(id: number, data: string): SqlResponse
```

Sets an asset's thumbnail from image bytes.

## RenderThumbnailForAsset
```luau
EmuDb:RenderThumbnailForAsset(id: number, version: number?): SqlResponse
```

Renders a thumbnail for an asset from its content. `version` defaults to the latest.

## AttachHistoricalDataToAsset
```luau
EmuDb:AttachHistoricalDataToAsset(id: number, row: { [string]: any }): SqlResponse
```

Stores a snapshot of an asset's details from a point in time. `DetachHistoricalDataFromAsset(id, row)` removes it.

## AttachMicrotransactionDataToAsset
```luau
EmuDb:AttachMicrotransactionDataToAsset(id: number, row: { [string]: any }): SqlResponse
```

Stores an asset's sale details, such as its price. `DetachMicrotransactionDataFromAsset(id, row)` removes them.

# Games
## GetUniverseIdForPlace
```luau
EmuDb:GetUniverseIdForPlace(placeId: number): number?
```

Returns the game a place belongs to.

## GetStartPlaceIdForUniverse
```luau
EmuDb:GetStartPlaceIdForUniverse(universeId: number): number?
```

Returns the place players join first in a game.

## AddThumbnailToPlace
```luau
EmuDb:AddThumbnailToPlace(placeId: number, imageId: number): SqlResponse
```

Adds an image asset to a place's thumbnails. `RemoveThumbnailFromPlace(placeId, imageId)` removes it.

# Bundles, outfits and avatars
Each of these adds an asset to a group of assets. A matching `Remove` method takes the same arguments.

| Add | Remove | Description |
| --- | --- | --- |
| `AddAssetToBundle(bundleId, assetId)` | `RemoveAssetFromBundle` | Adds an asset to a bundle. |
| `AddAssetToOutfit(outfitId, assetId)` | `RemoveAssetFromOutfit` | Adds an asset to a saved outfit. |
| `AddAssetToUserCharacter(userId, assetId)` | `RemoveAssetFromUserCharacter` | Puts an asset on a user's avatar. |

# Example
```luau
local db = EmuDb.new("data://myplugin/collection.nwdb")
db:SetTitle("My Collection")

db:AddItem(ItemType.Asset, {
    Id = 1,
    Name = "My Sword",
    Type = AssetType.Gear,
})

db:WriteChangesToDisk()
```
