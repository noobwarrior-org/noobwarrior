```luau
url.Resolve(url: string): string
```

# Description
Returns the URL in its full form. A URL that already has a scheme comes back unchanged.

# Arguments
1. `url: string`: the URL to resolve.

# Returns
The full URL.

# Example
```luau
print(url.Resolve("user://databases/master.nwdb")) -- user://databases/master.nwdb
```
