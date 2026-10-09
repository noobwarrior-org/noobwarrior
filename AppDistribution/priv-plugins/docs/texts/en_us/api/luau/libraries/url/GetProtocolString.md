```luau
url.GetProtocolString(url: string): string
```

# Description
Returns the URL's scheme as text, without the `://`.

# Arguments
1. `url: string`: the URL to read.

# Returns
The scheme, such as `"https"` or `"plugin"`.

# Example
```luau
print(url.GetProtocolString("https://example.com")) -- https
```
