```luau
crypto.Verify(publicKey: string, message: string, signature: string): boolean
```

# Description
Checks that `signature` was made for `message` by the private key matching `publicKey`.

# Arguments
1. `publicKey: string`: the signer's public key, in hexadecimal.
2. `message: string`: the message that was signed.
3. `signature: string`: the signature, in hexadecimal.

# Returns
`true` if the signature is valid.
