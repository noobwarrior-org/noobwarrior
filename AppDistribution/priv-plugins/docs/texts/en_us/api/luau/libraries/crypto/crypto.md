# crypto
Ed25519 signatures. Master servers use them to prove to each other that a message came from them. Keys and signatures are hexadecimal strings.

# Functions
| Function | Description |
| --- | --- |
| [GenerateEd25519](/api/luau/libraries/crypto/GenerateEd25519) | Makes a new key pair. |
| [Sign](/api/luau/libraries/crypto/Sign) | Signs a message with a private key. |
| [Verify](/api/luau/libraries/crypto/Verify) | Checks a signature with a public key. |

# Example
```luau
local keys = crypto.GenerateEd25519()
local signature = crypto.Sign(keys.priv, "hello")
print(crypto.Verify(keys.pub, "hello", signature)) -- true
print(crypto.Verify(keys.pub, "goodbye", signature)) -- false
```
