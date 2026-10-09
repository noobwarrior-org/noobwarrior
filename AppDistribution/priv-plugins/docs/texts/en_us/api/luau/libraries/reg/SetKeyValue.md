```luau
reg.SetKeyValue(key: string, value: any)
```

# Description
Stores `value` at `key`, replacing whatever was there. Tables along the path are created as needed. The change is not written to disk until the registry is [saved](/api/luau/libraries/reg/Save).

To give a setting a default without overwriting what the user chose, use [SetKeyValueIfNotSet](/api/luau/libraries/reg/SetKeyValueIfNotSet) instead.

# Arguments
1. `key: string`: the dotted path of the key.
2. `value: any`: the value to store. Strings, numbers, booleans and tables are saved; functions and userdata are not.

# Errors
Raises an error if `key` is empty or starts or ends with a period.

# Example
```luau
reg.SetKeyValue("myplugin.last_run", os.time())
reg.Save()
```
