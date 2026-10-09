```luau
url.ResolveWithoutProtocol(url: string): string
```

# Description
Returns the full URL with the scheme and `://` removed.

# Arguments
1. `url: string`: the URL to resolve.

# Returns
The host and path.

# Example
```luau
print(url.ResolveWithoutProtocol("https://example.com/page")) -- example.com/page
```
