```luau
lang.GetAvailableLanguages(): { string }
```

# Description
Lists the codes of every language that has strings available.

# Returns
An array of language codes.

# Example
```luau
for _, code in ipairs(lang.GetAvailableLanguages()) do
    print(code)
end
```
