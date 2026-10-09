# Registry
```cpp
#include <NoobWarrior/Registry.h>
```

noobWarrior's settings, saved to `registry.lua` in the user's data directory. Get it from `Core::GetRegistry()`. The generic methods come from `BaseRegistry`.

Keys are dotted paths into nested tables, such as `"emu.auth.enabled"`. A key cannot be empty or start or end with a period. Plugins reach the same settings through the Luau [reg](/api/luau/libraries/reg/reg) library.

New settings belong in `Registry::Open()` in `Core/Source/Registry.cpp`, declared with `SetKeyValueIfNotSet` and described with `SetKeyComment`.

# Reading
## GetKeyValue
```cpp
template <typename T>
std::optional<T> GetKeyValue(const std::string &key);
```

Returns the key's value as `T`. Returns `std::nullopt` if the key isn't set or holds a different type.

Numbers are stored by Luau, so a value written as an `int` may not read back as `int64_t`. If a key could hold either, try both.

## HasKey
```cpp
bool HasKey(const std::string &key);
```

Returns `true` if the key has a value.

# Writing
## SetKeyValue
```cpp
template <typename T>
void SetKeyValue(const std::string &key, T value);
```

Stores `value` at `key`, creating any tables along the path. Errors are written to the log, not returned.

## SetKeyValueIfNotSet
```cpp
template <typename T>
void SetKeyValueIfNotSet(const std::string &key, T value);
```

Stores `value` unless the key already holds a value of type `T`. A key holding a value of the wrong type is overwritten, so a broken hand edit fixes itself. The Luau version only checks whether the key is set.

## SetKeyComment
```cpp
void SetKeyComment(const char *key, const char *comment);
```

Attaches a comment, written above the key in `registry.lua`.

# Saving
## Save
```cpp
RegistryResponse Save();
```

Writes the registry to disk. Returns `RegistryResponse::Success`, or a value saying what failed. `GetLuaError()` gives the details.

# Example
```cpp
Registry *reg = core.GetRegistry();

reg->SetKeyValueIfNotSet("myfeature.enabled", true);
reg->SetKeyComment("myfeature.enabled", "Turns my feature on.");

if (reg->GetKeyValue<bool>("myfeature.enabled").value_or(false))
    core.Out("MyFeature", "Enabled");
```
