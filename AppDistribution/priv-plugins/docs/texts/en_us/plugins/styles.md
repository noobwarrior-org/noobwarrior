# Styles
A plugin can recolor the desktop app by declaring styles. A style is a Luau file that returns a table of colors, and noobWarrior paints the app with those colors.

Styles show up in Settings > General > Theme as "Style title (Plugin title)". Choosing one stores `"<plugin identifier>/<style id>"` in the `gui.theme` registry key.

# Declaring styles
List your styles in the manifest's `styles` array:

```luau
return {
    identifier = "solarized",
    title = "Solarized Pack",
    styles = {
        { id = "solarized", title = "Solarized", file = "styles/solarized.luau" },
    },
}
```

| Field | Required | Description |
| --- | --- | --- |
| `id` | Yes | Unique within your plugin. It can't contain `/`. |
| `file` | Yes | The style file, relative to your plugin's root. Works in zipped plugins too. |
| `title` | No | The name shown in Settings. Defaults to `id`. |

# The style file
```luau
return {
    version = 1,
    base = "fluent",
    colors = {
        accent = { 181, 137, 0 },
    },
    dark = {
        colors = {
            shell = { 0, 43, 54 },
            pane = { 7, 54, 66 },
            text = "#eee8d5",
        },
    },
    light = {
        colors = {
            shell = { 238, 232, 213 },
            pane = { 253, 246, 227 },
            text = "#073642",
        },
    },
}
```

| Field | Description |
| --- | --- |
| `version` | The style format version. Only `1` exists. A newer version is refused. Defaults to `1`. |
| `base` | Which built-in style to recolor: `"fluent"` (the default) or `"darcula"`. |
| `colors` | Colors used in both light and dark mode. |
| `dark`, `light` | Tables with their own `colors`, used only in that mode. |

The file runs in an empty environment, so globals such as `string`, `math` and `core` don't exist there. Write the colors out as plain values.

# Colors
A color is one of:

- `{ r, g, b }` or `{ r, g, b, a }`, with whole numbers from 0 to 255
- `"#rrggbb"` or `"#rrggbbaa"`

Anything you leave out keeps the base style's own color for that mode, so a style can change as little as one color. This recolors only the accent and keeps everything else:

```luau
return { colors = { accent = "#2aa198" } }
```

# Light and dark
The user picks a color scheme in Settings > General > Color Scheme (`gui.color_scheme`: `"dark"`, `"light"` or `"system"`).

For each color, noobWarrior uses the first of these that sets it:

1. The `dark` or `light` table that matches the scheme in use
2. The shared `colors` table
3. The base style's own color for that scheme

A style with neither `dark` nor `light` works in both modes. A style with only one of them supports only that mode: a dark-only style stays dark even when the user asks for light, and "Match System" doesn't switch it.

# Base styles
## Fluent
For a few colors, Fluent fills in the hover and pressed shades if you set the main color but not the shades. Set the shade yourself to override it.

| Set this | And these are worked out for you |
| --- | --- |
| `control` | `controlHover`, `controlPressed`, `controlDisabled` |
| `accent` | `accentHover`, `accentPressed` |
| `menu` | `menuHighlight` |
| `pane` | `rowHover` |
| `shell` | `menuBarHover`, `captionHover`, `captionPressed` |
| `scrollThumb` | `scrollThumbHover` |

In dark mode the worked-out shades are lighter than the main color. In light mode they're darker.

| Color | Used for |
| --- | --- |
| `shell` | Window background |
| `pane` | Panels, lists, docks and text field backgrounds |
| `alternateBase` | Alternating rows in lists |
| `menu` | Menus, drop-down lists and tooltips |
| `menuEdge` | Menu outline |
| `menuHighlight` | Hovered menu item |
| `menuBarHover` | Hovered menu bar item |
| `control` | Buttons, combo boxes and text fields |
| `controlHover`, `controlPressed`, `controlDisabled` | Those controls when hovered, pressed or disabled |
| `controlBorder` | Button outline |
| `controlStroke` | Bottom line of text fields and spin boxes |
| `indicatorFill`, `indicatorBorder` | Unchecked checkboxes and radio buttons |
| `border` | Panel and separator lines |
| `subtleBorder` | Frames around lists and other boxed widgets |
| `bevelLight`, `bevelMidlight`, `bevelDark` | Bevels that some Qt widgets draw themselves |
| `rowHover` | Hovered list row |
| `selection` | Selected list row |
| `tabHover` | Hovered tab |
| `tabSelected` | Selected side tab |
| `accent` | Default buttons, checked boxes, focus rings and links |
| `accentHover`, `accentPressed` | Accent-colored controls when hovered or pressed |
| `onAccent` | Text and check marks drawn on the accent |
| `textSelection`, `textOnSelection` | Selected text and its color |
| `toolBar`, `toolBarEdge`, `toolBarGrip` | Toolbar background, outline and drag handle |
| `captionHover`, `captionPressed` | Minimize and maximize buttons in the title bar |
| `captionGlyphHover` | The minimize and maximize symbols when hovered |
| `closeHover`, `closePressed`, `closeGlyphHover` | The close button and its symbol |
| `text`, `textSecondary`, `textPlaceholder`, `textDisabled` | Main text, descriptions, placeholder text and disabled text |
| `scrollThumb`, `scrollThumbHover` | Scroll bar handle |

## Darcula
Darcula sets Qt's palette and lets Qt draw the widgets. It has no hover colors to fill in.

| Color | Used for |
| --- | --- |
| `window` | Window background |
| `base` | Text fields and lists |
| `alternateBase` | Alternating rows in lists |
| `button` | Buttons |
| `text` | All text |
| `placeholderText` | Placeholder text |
| `highlight` | Selected items |
| `link` | Links |
| `toolTipBase` | Tooltip background |
| `brightText` | Text that must stand out on dark colors |
| `light`, `midlight`, `mid`, `dark`, `shadow` | Bevels and edges Qt draws |
| `tabPane` | Inside of tab pages |
| `listBase` | Background of list views |

# Mistakes
Problems with a style go to the log, and the plugin still loads. What happens to the style depends on the mistake.

- The whole style is skipped if its file is missing, doesn't compile or raises an error, its `id` is missing, repeated or contains `/`, its `version` is newer than noobWarrior understands, or `base`, `dark` or `light` has the wrong type.
- Only that color is skipped if the color isn't one of the forms above or isn't a color the base style has. The log names it, for example `"menuEdge" is not a color of base "darcula"`.
- The default style is used instead if the chosen style's plugin is no longer enabled, or its `base` isn't `"fluent"` or `"darcula"`.

A style file has no time limit, so a loop that never ends hangs the app at startup. Plugin manifests have no time limit either.
