```luau
lhp.Render(source: string, globals: { [string]: any }?): string
```

# Description
Renders LHP source held in a string. The page runs in a new environment built on top of the caller's, so it can read the caller's globals, but variables it creates don't leak back out.

The source has no file of its own, so `include` inside it can't resolve relative paths. Use [RenderFile](/api/luau/libraries/lhp/RenderFile) for pages that include others.

# Arguments
1. `source: string`: the LHP source.
2. `globals: { [string]: any }?`: extra globals for the page to see.

# Returns
The rendered output.

# Errors
Raises an error if the page has a syntax error or its code fails. Calling `exit` or `die` inside the page is not an error.

# Example
```luau
local html = lhp.Render("<p>Hello, <?lua echo(name) ?>!</p>", { name = "Builderman" })
print(html) -- <p>Hello, Builderman!</p>
```
