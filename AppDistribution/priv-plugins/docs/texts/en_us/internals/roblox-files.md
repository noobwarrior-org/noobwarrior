# Roblox files
noobWarrior reads and writes Roblox place and model files itself, mainly to prepare the place a Studio team test runs. The code is under `Core/Source/Roblox/FileFormat/` and `Core/Include/NoobWarrior/Roblox/FileFormat/`.

# The file format code
The code is a C++ port of MaximumADHD's [Roblox-File-Format](https://github.com/MaximumADHD/Roblox-File-Format), and it keeps the original's folders, files and class names on purpose: `BinaryFormat/`, `XmlFormat/`, `Tokens/`, `DataTypes/`, `Tree/`, `Interfaces/` and `Utility/`. That includes places where the names disagree. `BinaryFileReader.h` holds `BinaryRobloxFileReader`, for example, because the original does. Read the C# version before changing anything, and leave a comment wherever the port has to differ.

Known differences:

- `PhysicalProperties` keeps its raw flag byte and `AcousticAbsorption`, which the original's writer loses.
- `XmlFileReader` and `XmlFileWriter` are folded into `XmlRobloxFile`.
- `DataTypes/` has its own `NoobWarrior::Roblox::DataTypes` namespace, because the emulator already has a `NoobWarrior::Roblox::Color3` made of doubles.

`Tree/` is the document model. `RbxObject::props`, a `std::map<std::string, Property>`, is the only place property values are stored, and it is what gets saved. The generated classes' typed accessors read and write that map.

## Two rules for binary files
In a binary file, a `PROP` chunk stores one column per class and property, with a value for every instance of that class. The format can't say that one instance lacks a property. Breaking either of these rules has corrupted real places:

1. Never rename a property that files store. `Color3uint8`, `size`, `shape`, `MaterialVariantSerialized` and `Health_XML` are what files contain. `Color`, `Size`, `Shape`, `MaterialVariant` and `Health` are names scripts use, and no file contains them. A renamed property becomes a second column that every instance of the class must then carry.
2. When a column needs a value for an instance that didn't have one, use the engine's default, not the C++ type's zero. `CollisionGroup` defaults to `"Default"`, not `""`. `CanQuery`, `CanCollide`, `CanTouch` and `CastShadow` default to `true`. `Material` defaults to `Plastic`, which is 256; `0` isn't a valid material at all.

# Generated classes
`FileFormatGenerator/` turns a Roblox API dump into 906 class files plus `Classes.{h,cpp}`, `Registry.h`, `Enums.h` and `PropertyDescriptor.h`, written to `Core/Include/NoobWarrior/Roblox/FileFormat/Generated/`. The output is checked in, so a normal build doesn't run the generator. To regenerate:

```sh
cmake --build build --target NoobWarrior.FileFormatGenerator
./build/FileFormatGenerator/NoobWarrior.FileFormatGenerator.exe \
  build/FileFormatGenerator/API-Dump.json \
  Core/Include/NoobWarrior/Roblox/FileFormat/Generated
```

The API dump only describes what scripts can see. It marks script-only names with `CanSave=false`, which the generator skips, but it has no default values and no properties that only files use. Those come from `FileFormatGenerator/PropertyPatches.h`, a table extracted once from the original's generated C#.

`Instance` is written by hand in `Tree/`, so the generator skips the class but still writes a `ClassDescriptor` for it. Every superclass chain ends there.

Code that needs the generated classes checks `NOOBWARRIOR_HAVE_GENERATED_ROBLOX_API`, which `Core/CMakeLists.txt` defines only when `Generated/Classes.cpp` exists. Core still builds without it.

# Studio server places
A Studio team test runs a `server.rbxl` holding the user's place plus every enabled plugin's content. `Core::LaunchEngine()` builds it before starting Studio (see [Engines](/internals/engines)), in this order:

1. `PluginDataModel` walks each plugin's `datamodel` folders, laid out like a Rojo project, and writes a plan as tagged JSON.
2. `PluginTreeMaterializer` applies the plans to the loaded place.
3. `StudioServerPlace::InjectStudioServerBootstrap` adds the plans, splices in `.rbxm` models, appends each plugin's `engine_autorun` scripts and saves.
4. `StudioServerPlaceCache` keeps the result, so an unchanged place isn't prepared again.

A place is saved in the format it was loaded in: `.rbxlx` as XML and `.rbxl` as binary.

The plan is the only route a property takes from a plugin's XML into the place, so both ends must map each tag to the same `PropertyType`. Every bug found here so far has been the two ends disagreeing, in `PluginDataModel::XmlPropertyValue` and `PluginTreeMaterializer::ConvertPlanValue`.

Plugins are applied in load order, and later ones win. Folders and services merge. Anything else has its whole subtree replaced. A `.rbxm` is replaced as one unit, keyed on the names its root instances land under.

`BinaryRobloxFile::BuildTables` drops any column that no loaded instance of the class has, so plugin content can't add a property to a class the file already has instances of.

Change `kStudioServerPlaceCacheSchema` in `StudioServerPlaceCache.h` whenever the same inputs would produce different bytes. Otherwise users keep getting an old prepared place.

# Finding corruption
Three small tools build against Core. None build unless named:

```sh
cmake --build build --target NoobWarrior.RoundTripPlace NoobWarrior.DiffPlace NoobWarrior.PlacePollution
```

| Tool | Use |
| --- | --- |
| `RoundTripPlace <in.rbxl> <out.rbxl>` | Loads a place and saves it again. |
| `DiffPlace` | Compares a source place with a prepared one, matching instances by path, and lists properties that were added, changed or dropped per class. Preparing a place must never add a property to an instance from the original file. |
| `PlacePollution` | Looks at a prepared place alone for columns where a few instances have real values and the rest hold the type's zero. |

Before merging any change to the format code, run `RoundTripPlace` over every place you have, and check that the places Roblox ships come back byte for byte identical.

The format code also compiles without the rest of Core, which is the fastest way to try a change. The test program has to define `NoobWarrior::gLog_Mutex` and `gLog_PrintToStdOut` itself:

```sh
clang++ -std=c++23 -O1 -o harness.exe harness.cpp -ICore/Include \
  -I.cache/cpm/pugixml/*/src -I.cache/cpm/lz4/*/lib -I.cache/cpm/zstd/*/lib \
  -I.cache/cpm/zlib/* -Ibuild/_deps/zlib-build \
  Core/Source/Roblox/FileFormat/BinaryFormat/*.cpp \
  Core/Source/Roblox/FileFormat/BinaryFormat/Chunks/*.cpp \
  Core/Source/Roblox/FileFormat/RobloxFile.cpp \
  Core/Source/Roblox/FileFormat/XmlFormat/XmlRobloxFile.cpp \
  Core/Source/Roblox/FileFormat/Tree/*.cpp \
  build/_deps/lz4-build/liblz4.a build/_deps/zstd-build/build/cmake/lib/libzstd.a \
  build/_deps/zlib-build/libzlibstatic.a build/_deps/pugixml-build/libpugixml.a
```
