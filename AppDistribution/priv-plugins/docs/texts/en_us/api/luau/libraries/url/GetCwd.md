```luau
url.GetCwd(url: string): string
```

# Description
Returns the working directory a relative URL is resolved against. This function builds the URL from a plain string with no context, so from Luau the result is always an empty string. To resolve a path relative to another plugin, use [ResolveFromCaller](/api/luau/libraries/url/ResolveFromCaller).

# Arguments
1. `url: string`: the URL to read.

# Returns
Always `""`.
