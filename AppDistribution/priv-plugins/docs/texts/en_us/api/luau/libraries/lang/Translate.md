```luau
lang.Translate(key: string, fallback: string?, ...: any): string
```

# Description
Looks up `key` in the current language and fills in its placeholders. Placeholders are written `{0}`, `{1}` and so on, and the extra arguments replace them in order. Each argument is converted with `tostring`, and up to 16 are used.

If the current language has no string for `key`, noobWarrior uses the English (`en_us`) string instead, then `fallback`, and finally `key` itself. Passing an English fallback means the page still reads properly before anyone has translated it.

# Arguments
1. `key: string`: the string's key, such as `"docs.page_title"`.
2. `fallback: string?`: the text to use when no translation exists.
3. `...: any`: values for the placeholders.

# Returns
The translated text.

# Example
```luau
-- With "greeting" = "Hello, {0}! You have {1} messages."
print(lang.Translate("greeting", "Hello, {0}!", "Builderman", 3))
-- Hello, Builderman! You have 3 messages.
```
