# lhp
Renders LHP pages. LHP works like PHP with Luau inside: a page is ordinary HTML with `<?lua ... ?>` blocks, and anything the code passes to `echo` goes into the output where the block was.

```html
<ul>
<?lua for i = 1, 3 do ?>
    <li>Item <?lua echo(i) ?></li>
<?lua end ?>
</ul>
```

Most plugins never call these functions directly. The `http-base` plugin renders the page for each route in a plugin's sitemap.

# Functions
| Function | Description |
| --- | --- |
| [Render](/api/luau/libraries/lhp/Render) | Renders LHP source held in a string. |
| [RenderFile](/api/luau/libraries/lhp/RenderFile) | Renders an LHP file. |

# Page globals
Code inside a page can use everything a normal script can, plus these:

| Global | Description |
| --- | --- |
| `echo(text)` | Adds `text` to the page output. |
| `include(path)` | Renders another LHP file into this page. A relative path is resolved against the current file's folder. The included file shares this page's globals, so values set in one are visible in the other. |
| `exit(text)` | Adds `text` to the output and stops rendering. |
| `die(text)` | The same as `exit`. |
| `plugin` | The [Plugin](/api/luau/classes/Plugin) the page file belongs to. |

Pages served through `http-base` also get request globals such as `_GET`, `_POST`, `_COOKIE` and `_SESSION`. Those come from the plugin, not from LHP itself.
