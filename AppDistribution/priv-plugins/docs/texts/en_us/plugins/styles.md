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
| `web` | Stylesheets for the websites. See [Websites](#websites). |

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
| `controlHover` | Those controls when hovered |
| `controlPressed` | Those controls when pressed |
| `controlDisabled` | Those controls when disabled |
| `controlBorder` | Button outline |
| `controlStroke` | Bottom line of text fields and spin boxes |
| `indicatorFill`, `indicatorBorder` | Unchecked checkboxes and radio buttons |
| `border` | Panel and separator lines |
| `subtleBorder` | Frames around lists and other boxed widgets |
| `bevelLight` | The lightest bevel some Qt widgets draw themselves |
| `bevelMidlight` | The middle bevel |
| `bevelDark` | The darkest bevel |
| `rowHover` | Hovered list row |
| `selection` | Selected list row |
| `tabHover` | Hovered tab |
| `tabSelected` | Selected side tab |
| `accent` | Default buttons, checked boxes, focus rings and links |
| `accentHover`, `accentPressed` | Accent-colored controls when hovered or pressed |
| `onAccent` | Text and check marks drawn on the accent |
| `textSelection`, `textOnSelection` | Selected text and its color |
| `toolBar` | Toolbar background |
| `toolBarEdge` | Toolbar outline |
| `toolBarGrip` | Toolbar drag handle |
| `captionHover`, `captionPressed` | Minimize and maximize buttons in the title bar |
| `captionGlyphHover` | The minimize and maximize symbols when hovered |
| `closeHover`, `closePressed` | The close button when hovered or pressed |
| `closeGlyphHover` | The close symbol when hovered |
| `text` | Main text |
| `textSecondary` | Descriptions and title bar symbols |
| `textPlaceholder` | Placeholder text |
| `textDisabled` | Disabled text |
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
| `light`, `midlight` | Light bevels and edges Qt draws |
| `mid`, `dark` | Darker bevels and edges |
| `shadow` | Shadows Qt draws |
| `tabPane` | Inside of tab pages |
| `listBase` | Background of list views |

# Websites
A style can also restyle the server emulator website and the master server website. The desktop colors you set already carry over, and you can add your own CSS on top.

The host picks each site's default style in Settings > General > Websites. That stores `emu.web_style` and `master.web_style` in the registry: `""` for the plain look, `"app"` to use whatever `gui.theme` is, or a style id such as `"solarized/classic"`. Visitors can override it with the Style menu at the bottom of every page. Their choice is saved in a `nw_style` cookie in their browser, and "Plain" turns styling off for them. A cookie naming a style that no enabled plugin offers is ignored.

Both choices load through `/css/theme.css`, which every page links after the site's main stylesheet.

## Adding CSS
Point `web.emu` and `web.master` at stylesheets in your plugin:

```luau
return {
    colors = { accent = "#2aa198" },
    web = {
        emu = "web/emu.css",
        master = "web/master.css",
    },
}
```

The file is sent as-is after the generated colors, so it can override any rule. A few pages load their own stylesheet after `theme.css` (the forums, Develop and the admin panels), so their selectors can still win over yours there. Setting variables avoids that problem.

A stylesheet can make visitors' browsers fetch from other servers through `url()` or `@import`. Those requests come from the host's visitors, so avoid loading anything from outside the site.

## Light and dark
On the web, each visitor's browser decides between light and dark through `prefers-color-scheme`. Your `dark` colors apply to dark visitors and your `light` colors to light ones. A dark-only style still shows its shared `colors` to light visitors, on top of the default light look, so it doesn't force dark mode on anyone.

## Colors that carry over
These desktop colors set website variables. Colors you don't set leave the website's own defaults alone.

| Fluent color | Website variables |
| --- | --- |
| `shell` | `--nw-backdrop-outer`, `--nw-backdrop-inner`, `--nw-backdrop-center` |
| `pane` | `--nw-content` |
| `toolBar` | `--nw-navbar` |
| `control` | `--nw-surface`, `--nw-surface-raised`, `--nw-surface-strong` |
| `controlHover` | `--nw-hover` |
| `controlBorder` | `--nw-border` |
| `border`, `subtleBorder` | `--nw-divider-strong`, `--nw-divider` |
| `text` | `--nw-text` |
| `textSecondary` | `--nw-text-soft`, `--nw-text-muted` |
| `textPlaceholder`, `textDisabled` | `--nw-text-faint`, `--nw-text-footer` |
| `accent` | `--nw-accent` |
| `accentHover` | `--nw-link-hover` |
| `accentPressed` | `--nw-accent-pressed` |
| `onAccent` | `--nw-on-accent` |

| Darcula color | Website variables |
| --- | --- |
| `window` | `--nw-backdrop-outer`, `--nw-backdrop-inner`, `--nw-backdrop-center` |
| `base` | `--nw-content` |
| `button` | `--nw-surface`, `--nw-surface-raised`, `--nw-surface-strong` |
| `dark`, `mid` | `--nw-border`, `--nw-divider-strong` |
| `text` | `--nw-text`, `--nw-text-soft` |
| `placeholderText` | `--nw-text-muted`, `--nw-text-faint`, `--nw-text-footer` |
| `highlight`, `link` | `--nw-accent`, `--nw-link` |

If you set Fluent's `control` or `accent` but not their shades, `controlHover`, `accentHover` and `accentPressed` are filled in the same way as on the desktop.

## Website variables
Every website color is one of these variables, defined in http-base's `main.css`. Your stylesheet can set any of them on `:root`, inside `@media (prefers-color-scheme: dark)` or `light` blocks if they should differ by mode.

| Variable | Used for |
| --- | --- |
| `--nw-backdrop-outer` | The edges of the page background gradient |
| `--nw-backdrop-inner` | Between the edges and the center of the gradient |
| `--nw-backdrop-center` | The center of the gradient |
| `--nw-navbar` | The navigation bar |
| `--nw-content` | The main content panel |
| `--nw-surface` | Cards, sidebars and tabs |
| `--nw-surface-raised` | Raised panels and buttons |
| `--nw-surface-strong` | Tables, badges and secondary buttons |
| `--nw-surface-sunken` | Inputs and image wells |
| `--nw-inset` | Comments, messages and hovered server rows |
| `--nw-hover` | Hovered sidebar and tab items, and the asset preview card |
| `--nw-selected` | The selected sidebar or tab item |
| `--nw-selected-text` | Text on the selected item |
| `--nw-border` | Outlines |
| `--nw-divider` | Light dividers |
| `--nw-divider-strong` | Stronger lines and input outlines |
| `--nw-input-border` | Login inputs |
| `--nw-shadow` | The content panel's shadow |
| `--nw-shadow-soft` | Toast shadows |
| `--nw-text` | Main text |
| `--nw-text-soft` | Descriptions and badge text |
| `--nw-text-muted` | Less prominent text, such as sidebar links |
| `--nw-text-faint` | Dates, counts and empty-list messages |
| `--nw-text-footer` | The page footer |
| `--nw-accent` | Accent buttons |
| `--nw-accent-pressed` | Accent buttons when hovered |
| `--nw-on-accent` | Text on accent, join and delete buttons |
| `--nw-link` | Links. Defaults to `--nw-accent`. |
| `--nw-link-hover` | Hovered links |
| `--nw-highlight` | Highlighted names, such as comment authors |
| `--nw-info` | Blue buttons and the current page in the item pager |
| `--nw-info-hover` | Blue buttons when hovered |
| `--nw-info-text` | Item pager links |
| `--nw-on-info` | Text drawn on blue buttons |
| `--nw-primary` | The forum admin's main buttons |
| `--nw-primary-hover` | Those buttons when hovered |
| `--nw-primary-border` | Those buttons' outline |
| `--nw-primary-text` | The forum admin's links |
| `--nw-primary-text-hover` | Those links when hovered |
| `--nw-focus` | Focused inputs in the forum admin |
| `--nw-success` | Join buttons |
| `--nw-success-hover` | Join buttons when hovered |
| `--nw-success-surface` | Success message background |
| `--nw-success-border` | Success message outline |
| `--nw-success-text` | Success message text |
| `--nw-danger` | Delete buttons |
| `--nw-danger-hover` | Delete buttons when hovered |
| `--nw-danger-border` | Delete buttons' outline |
| `--nw-danger-surface` | Error message background |
| `--nw-danger-surface-border` | Error message outline |
| `--nw-danger-text` | Error message text |
| `--nw-danger-link` | Delete links |
| `--nw-danger-link-hover` | Delete links when hovered |
| `--nw-error` | Error text in the control panel |

# Mistakes
Problems with a style go to the log, and the plugin still loads. What happens to the style depends on the mistake.

- The whole style is skipped if its file is missing, doesn't compile or raises an error, its `id` is missing, repeated or contains `/`, its `version` is newer than noobWarrior understands, or `base`, `dark`, `light` or `web` has the wrong type.
- Only that color is skipped if the color isn't one of the forms above or isn't a color the base style has. The log names it, for example `"menuEdge" is not a color of base "darcula"`.
- Only that stylesheet is skipped if a `web` entry isn't a string or names a site other than `emu` or `master`.
- The default style is used instead if the chosen style's plugin is no longer enabled, or its `base` isn't `"fluent"` or `"darcula"`.

A style file has no time limit, so a loop that never ends hangs the app at startup. Plugin manifests have no time limit either.
