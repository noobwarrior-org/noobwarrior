```luau
crypto.GenerateEd25519(): { priv: string, pub: string }?
```

# Description
Makes a new Ed25519 key pair. Keep `priv` secret and give `pub` to anyone who needs to check your signatures.

# Returns
A table with the private key in `priv` and the public key in `pub`, or `nil` if key generation failed.
