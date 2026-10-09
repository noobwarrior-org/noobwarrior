```luau
crypto.Sign(privateKey: string, message: string): string
```

# Description
Signs `message` with a private key from [GenerateEd25519](/api/luau/libraries/crypto/GenerateEd25519).

# Arguments
1. `privateKey: string`: the private key, in hexadecimal.
2. `message: string`: the message to sign.

# Returns
The signature, in hexadecimal.
