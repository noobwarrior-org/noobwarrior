```luau
hash.VerifyPassword(password: string, hash: string, salt: string): boolean
```

# Description
Checks a password against a hash and salt from [HashPassword](/api/luau/libraries/hash/HashPassword). The comparison takes the same time whether or not the password matches, so timing can't reveal how close a guess was.

# Arguments
1. `password: string`: the password someone typed.
2. `hash: string`: the stored hash.
3. `salt: string`: the stored salt.

# Returns
`true` if the password matches.

# Errors
Raises an error if `salt` isn't 32 hexadecimal characters.

# Example
```luau
if hash.VerifyPassword(_POST.password, row.Hash, row.Salt) then
    echo("Welcome back!")
end
```
