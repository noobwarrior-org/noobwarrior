# LHP
LHP is how noobWarrior's web pages are written: HTML with Luau inside `<?lua ... ?>` blocks, in the style of PHP. The renderer is `Lhp` in `Core/Source/Lua/Lhp.cpp`. Plugins reach it through the [lhp](/api/luau/libraries/lhp/lhp) library, and `http-base` uses it to serve every routed page.

# How a page becomes code
`Lhp::Render()` turns the whole page into one Luau chunk, then runs it. It walks the input once:

- Text outside a block is collected, and when a block opens or the input ends, it becomes `echo([[...]]);`. A long string keeps quotes and backslashes in the HTML as they are. `EchoStatement()` adds equals signs to the brackets (`[=[...]=]`, `[==[...]==]` and so on) until the closing bracket can't appear in the text, so any text is echoed exactly.
- Text inside a block is copied into the chunk unchanged, followed by a newline.

So this page:

```html
<p>Hello, <?lua echo(name) ?>!</p>
```

runs as:

```luau
echo([[<p>Hello, ]]);
 echo(name)
echo([[!</p>]]);
```

The chunk compiles and runs in one go. A syntax error anywhere in the page means nothing is sent.

## Closing tags in code
The walk is a plain character scan, so `?>` ends a block wherever it appears, including inside a Luau string. Write `"?" .. ">"` if code needs those characters. PHP behaves the same way.

A Luau long string drops a newline that comes straight after its opening bracket, so a newline right after `?>` doesn't reach the output. This also matches PHP.

# Environments
A top-level render gets a fresh environment whose fallback is the caller's. The page reads the caller's globals, and anything it sets stays inside the page. `lhp.Render` and `lhp.RenderFile` also copy the extra globals they're given into that environment.

`echo` is defined once per top-level render and appends to its output string.

## include
`include(path)` resolves `path` against the folder of the file being rendered, then renders that file recursively in the same environment instead of a new one. That is what lets `header.lhp` define helpers and set `_SESSION` and `pageTitle` for the page that included it, and lets `footer.lhp` read a start time set by the header.

The recursive render installs its own `exit` and `die`, which write to the included file's output. `include` saves the parent's versions first and puts them back afterwards. Without that, calling `die` in the parent after an include would write into an output string that no longer exists.

## plugin
While a file renders, `plugin` is set to the plugin that file belongs to, and restored when the render finishes. An included file from another plugin sees its own plugin.

# exit and die
Both append their argument to the output, then raise a Luau error with the message `__LHP_EXIT__`. `Render` recognizes that message and returns `RenderResponse::ExitCalled` instead of an error, and the output so far is kept. Inside an include, the include adds its output to the parent's and raises the same error again, so the exit reaches the top.

Any other error is logged with the file's URL and returns `RenderResponse::LuaError`. The `lhp` library turns both `LuaError` and `SyntaxError` into a Luau error for its caller.

# Pages served by http-base
`http-base` builds the request globals in the style of PHP and passes them to `lhp.RenderFile` as extra globals. The code is in `AppDistribution/priv-plugins/http-base/lua/base.lua`.

| Global | Holds |
| --- | --- |
| `_GET`, `_POST`, `_FILES` | The query string, form fields and uploaded files. |
| `_RAW_POST` | The request body as it arrived. |
| `_COOKIE` | The request's cookies. |
| `_PARAMS` | Values captured by `:name` segments in the sitemap route. |
| `_SERVER` | Request details such as `REQUEST_METHOD`, `HTTP_HOST`, `REMOTE_ADDR` and `QUERY_STRING`. |
| `_SESSION` | Session data tied to the `NWSESSID` cookie. The session starts the first time the page touches it. |
| `_USER` | The logged-in account, resolved from `.LOGINSESSION` with `core.ResolveSession`, or `nil`. |
| `has_permission(name)` | `core.HasPermission` for `_USER`. |
| `header()`, `http_response_code()`, `setcookie()`, `session_destroy()` | Change the response, in the style of their PHP namesakes. |

The page renders inside `pcall`. Its output is sent with the status code set by `header()` or `http_response_code()`, 200 by default.
