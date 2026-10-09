# Registry
The registry holds noobWarrior's settings in `<userdata>/registry.lua`. It is a Luau table, not the Windows registry. The generic code is `BaseRegistry` (`Core/Source/BaseRegistry.cpp`), and `Registry` (`Core/Source/Registry.cpp`) adds noobWarrior's own settings.

# How it's stored
The registry lives as a global table inside the program's Luau state. `Open()` runs `registry.lua` and stores the table it returns. The file has to `return` a table: a file that assigns globals instead produces an empty registry, and anything other than a table fails with `ReturningWrongType`. A missing file starts an empty registry.

Keys are dotted paths, so `emu.auth.enabled` is `registry.emu.auth.enabled`. After loading, `Open()` gives every table a metatable that creates a missing table the moment it's indexed. That is what lets `SetKeyValue("a.b.c", 1)` work when `a` doesn't exist yet. Reads go through `RawGetKeyObject()`, which uses `rawget` and so never creates anything.

`Save()` removes empty tables (the metatable leaves plenty behind), turns the table into source code with Serpent, inserts the comments from `SetKeyComment()` above their keys, and writes `return <table>`. `Close()` saves too.

# Reading and writing from C++
```cpp
std::optional<T> GetKeyValue<T>(const std::string &key);
void SetKeyValue<T>(const std::string &key, T value);
void SetKeyValueIfNotSet<T>(const std::string &key, T value);
void SetKeyComment(const char *key, const char *comment);
```

`GetKeyValue` returns `std::nullopt` when the key is missing or holds another type. Numbers come from Luau, so a value written as `int` may not read back as `int64_t`. Code that reads a number written elsewhere should try both widths, as `AuthUtil::PermissionFloor()` does.

`SetKeyValueIfNotSet` leaves a key alone only if it already holds a value of type `T`. A value of the wrong type is replaced with the default, so a broken hand edit repairs itself on the next start. The Luau version, `reg.SetKeyValueIfNotSet`, only checks whether the key is set.

# Adding a setting
Declare it in `Registry::Open()` with a default and a comment:

```cpp
SetKeyValueIfNotSet("emu.my_feature.enabled", false);
SetKeyComment("emu.my_feature.enabled", "Turns my feature on.");
```

The comment is the setting's only documentation for people who edit `registry.lua` by hand, so say what it does and what the values mean.

Plugins declare their own settings the same way through the [reg](/api/luau/libraries/reg/reg) library, usually at the top of their `main.lua`.
