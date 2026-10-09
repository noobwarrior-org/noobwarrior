```luau
hash.HashPassword(password: string): { hash: string, salt: string }
```

# Description
Hashes a password with Argon2id and a new random salt. Store both values and check logins later with [VerifyPassword](/api/luau/libraries/hash/VerifyPassword). Never store the password itself.

It uses the same Argon2id settings as noobWarrior's own login handler.

# Arguments
1. `password: string`: the password to hash.

# Returns
A table with two hexadecimal strings: `hash` (32 bytes) and `salt` (16 bytes).

# Errors
Raises an error if the salt can't be generated or hashing fails.

# Example
```luau
local result = hash.HashPassword(_POST.password)
db:QueryTyped("UPDATE MyAccounts SET Hash = ?, Salt = ? WHERE Id = ?",
    result.hash, result.salt, accountId)
```
