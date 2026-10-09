```luau
reg.SetKeyValueIfNotSet(key: string, value: any)
```

# Description
Stores `value` at `key` only if the key has no value yet. Use it to declare a setting's default: the first run creates it, and later runs leave the user's choice alone.

# Arguments
1. `key: string`: the dotted path of the key.
2. `value: any`: the default value.

# Errors
Raises an error if `key` is empty or starts or ends with a period.

# Example
```luau
reg.SetKeyValueIfNotSet("myplugin.max_players", 12)
```
