# AssetType
The kind of an [Asset](/api/luau/enums/ItemType). The values match the ones Roblox uses on its website, which is why there are gaps between them.

| Name | Value |
| --- | --- |
| `None` | 0 |
| `Image` | 1 |
| `TShirt` | 2 |
| `Audio` | 3 |
| `Mesh` | 4 |
| `Lua` | 5 |
| `Hat` | 8 |
| `Place` | 9 |
| `Model` | 10 |
| `Shirt` | 11 |
| `Pants` | 12 |
| `Decal` | 13 |
| `Head` | 17 |
| `Face` | 18 |
| `Gear` | 19 |
| `Badge` | 21 |
| `Animation` | 24 |
| `Torso` | 27 |
| `RightArm` | 28 |
| `LeftArm` | 29 |
| `LeftLeg` | 30 |
| `RightLeg` | 31 |
| `Package` | 32 |
| `Gamepass` | 34 |
| `Plugin` | 38 |
| `MeshPart` | 40 |
| `HairAccessory` | 41 |
| `FaceAccessory` | 42 |
| `NeckAccessory` | 43 |
| `ShoulderAccessory` | 44 |
| `FrontAccessory` | 45 |
| `BackAccessory` | 46 |
| `WaistAccessory` | 47 |
| `ClimbAnimation` | 48 |
| `DeathAnimation` | 49 |
| `FallAnimation` | 50 |
| `IdleAnimation` | 51 |
| `JumpAnimation` | 52 |
| `RunAnimation` | 53 |
| `SwimAnimation` | 54 |
| `WalkAnimation` | 55 |
| `PoseAnimation` | 56 |
| `EarAccessory` | 57 |
| `EyeAccessory` | 58 |
| `EmoteAnimation` | 61 |
| `Video` | 62 |
| `TShirtAccessory` | 64 |
| `ShirtAccessory` | 65 |
| `PantsAccessory` | 66 |
| `JacketAccessory` | 67 |
| `SweaterAccessory` | 68 |
| `ShortsAccessory` | 69 |
| `LeftShoeAccessory` | 70 |
| `RightShoeAccessory` | 71 |
| `DressSkirtAccessory` | 72 |
| `FontFamily` | 73 |
| `EyebrowAccessory` | 76 |
| `EyelashAccessory` | 77 |
| `MoodAnimation` | 78 |
| `DynamicHead` | 79 |
| `FaceMakeup` | 88 |
| `LipMakeup` | 89 |
| `EyeMakeup` | 90 |

# Example
```luau
local hats = db:SearchAssetIds(AssetType.Hat, "", 50, 0)
for i = 1, #hats do
    print(hats[i])
end
print(#hats .. " hats found")
```
