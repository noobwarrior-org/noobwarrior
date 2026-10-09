# ItemType
The kinds of Roblox item a noobWarrior database can store. Database functions such as [EmuDb:AddItem](/api/luau/classes/EmuDb) and [EmuDb:DoesItemExist](/api/luau/classes/EmuDb) take one to say which table they work on.

| Name | Value | Description |
| --- | --- | --- |
| `Asset` | 0 | Anything in the catalog or library: models, places, clothing, audio, meshes and so on. [AssetType](/api/luau/enums/AssetType) says which kind. |
| `Badge` | 1 | A badge awarded in a game. |
| `Bundle` | 2 | A group of assets sold together, like a body package. |
| `DevProduct` | 3 | A developer product, bought inside a game and usable more than once. |
| `Group` | 4 | A Roblox group. |
| `Outfit` | 5 | A saved avatar outfit. |
| `Pass` | 6 | A game pass, bought once per game. |
| `Set` | 7 | A set of assets. Roblox removed sets long ago. |
| `Universe` | 8 | A game, also called an experience. It holds one or more places. |
| `User` | 9 | A user account. |

# Example
```luau
local db = core.GetMasterDatabase()
if db:DoesItemExist(ItemType.User, 1) then
    print("User 1 exists")
end
```
