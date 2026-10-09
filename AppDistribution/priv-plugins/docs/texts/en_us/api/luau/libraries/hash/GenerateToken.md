```luau
hash.GenerateToken(): string
```

# Description
Returns 32 bytes from a cryptographically secure random source, as 64 hexadecimal characters. Use it for session cookies, password reset links and anything else that must not be guessable.

# Returns
The token.

# Errors
Raises an error if the system can't supply random bytes.

# Example
```luau
local token = hash.GenerateToken()
setcookie("MYSESSION", token)
```
