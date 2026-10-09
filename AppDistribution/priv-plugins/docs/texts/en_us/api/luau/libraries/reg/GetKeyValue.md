```luau
reg.GetKeyValue(key: string): any
```

# Description
Returns the value stored at `key`. Reading a key never creates it, so asking for a missing key, or one whose parent table is missing, gives `nil`.

# Arguments
1. `key: string`: the dotted path of the key, such as `"emu.http_port"`.

# Returns
The key's value, or `nil` if it isn't set. A key that names a table returns the table.

# Errors
Raises an error if `key` is empty or starts or ends with a period.

# Example
```luau
local port = reg.GetKeyValue("emu.http_port")
print("The emulator listens on port " .. port)
```
