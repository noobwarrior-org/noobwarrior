```luau
plugin: Plugin?
```

# Description
The [Plugin](/api/luau/classes/Plugin) that the running script belongs to. It is `nil` for scripts that don't come from a plugin, such as code built with [Script.new](/api/luau/classes/Script).

LHP pages also get `plugin`, set to the plugin the page file belongs to.

# Example
```luau
print("Running inside " .. plugin:GetIdentifier())
```
