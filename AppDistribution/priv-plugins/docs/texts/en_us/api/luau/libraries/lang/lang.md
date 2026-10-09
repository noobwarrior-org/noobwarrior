# lang
Translates text into the user's language. noobWarrior picks the language from the `language` registry key, which is `en_US` by default. Codes are case-insensitive and reported in lowercase, such as `en_us`.

Strings live in `.luau` files that return a table of keys and translations. noobWarrior has its own in `lang/`, and a plugin can add more from its own `lang/` folder. When several sources define the same key, the one loaded last wins.

# Functions
| Function | Description |
| --- | --- |
| [Translate](/api/luau/libraries/lang/Translate) | Looks up a string and fills in its placeholders. |
| [Has](/api/luau/libraries/lang/Has) | Checks whether a key has a translation. |
| [GetCode](/api/luau/libraries/lang/GetCode) | Returns the current language code. |
| [GetAvailableLanguages](/api/luau/libraries/lang/GetAvailableLanguages) | Lists every language that has strings. |
