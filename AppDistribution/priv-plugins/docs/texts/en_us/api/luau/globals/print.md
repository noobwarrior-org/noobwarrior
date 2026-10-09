```luau
print(...: any)
```

# Description
Converts each argument with `tostring`, joins them with spaces, and writes the line to noobWarrior's log. Inside a plugin script, the line is prefixed with the script's URL so you can tell where it came from.

# Arguments
1. `...: any`: the values to print.

# Example
```luau
print("Loaded", 3, "pages")
```
