# Script
A loaded Luau script. Each running script can reach its own through the [script](/api/luau/globals/script) global.

# Constructors
## Script.new
```luau
Script.new(source: string): Script
```

Creates a script from source code, using the caller's environment as its base. Luau can't run the result yet, because `Execute` isn't exposed. To compile and run code from a string, use [loadstring](/api/luau/globals/loadstring).

# Methods
## GetUrl
```luau
Script:GetUrl(): Url
```

Returns the URL the script was loaded from, as an object. The URL type has no methods in Luau yet, so the object can only be passed back to noobWarrior functions. noobWarrior itself uses it to resolve relative paths.
