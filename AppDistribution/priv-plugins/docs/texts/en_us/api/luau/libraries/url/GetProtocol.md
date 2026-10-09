```luau
url.GetProtocol(url: string): ProtocolType
```

# Description
Returns the URL's scheme as a [ProtocolType](/api/luau/enums/ProtocolType).

# Arguments
1. `url: string`: the URL to read.

# Returns
The scheme, or `ProtocolType.Unsupported` if noobWarrior doesn't recognize it.

# Example
```luau
print(url.GetProtocol("https://example.com") == ProtocolType.Https) -- true
```
