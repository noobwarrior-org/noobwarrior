# NetClient
Makes web requests to other servers. HTTPS certificates are checked against the operating system's trusted certificates.

Requests block until they finish or time out, and noobWarrior waits while one is running. Keep them short and avoid making them in response to every page view.

# Constructors
## NetClient.new
```luau
NetClient.new(): NetClient
```

Creates a client with a 30 second timeout and no extra headers.

# Settings
## SetTimeout
```luau
NetClient:SetTimeout(seconds: number)
```

Sets how long a request may take before it fails.

## SetHeader
```luau
NetClient:SetHeader(name: string, value: string)
```

Adds a header to every request this client makes. Setting the same name twice sends it twice.

# Requests
Each request returns a response table:

| Field | Type | Description |
| --- | --- | --- |
| `Ok` | `boolean` | `true` if a response came back. A `404` or `500` still counts. Check `Status` for those. |
| `Status` | `number` | The HTTP status code. |
| `Body` | `string` | The response body. |
| `Error` | `string?` | What went wrong, when `Ok` is `false`. |

## Get
```luau
NetClient:Get(url: string): Response
```

Sends a `GET` request.

## Post
```luau
NetClient:Post(url: string, body: string, contentType: string): Response
```

Sends a `POST` request with `body`.

## PostJson
```luau
NetClient:PostJson(url: string, data: { [string]: any }): Response
```

Sends `data` as a JSON `POST`, converted with [json.stringify](/api/luau/libraries/json/json) and sent with `Content-Type: application/json`.

## Request
```luau
NetClient:Request(options: RequestOptions): Response
```

Sends a request described by a table:

| Field | Type | Description |
| --- | --- | --- |
| `Url` | `string` | Where to send it. |
| `Method` | `string?` | `GET`, `POST`, `PUT`, `PATCH`, `DELETE`, `HEAD` or `OPTIONS`, in any case. Defaults to `GET`, which is also used for anything unrecognized. |
| `Body` | `string?` | The request body. |
| `ContentType` | `string?` | The body's `Content-Type`. |
| `Headers` | `{ [string]: string }?` | Headers for this request only, added to the client's own. |

# Example
```luau
local client = NetClient.new()
client:SetTimeout(10)

local res = client:Get("https://example.com/status.json")
if res.Ok and res.Status == 200 then
    local status = json.parse(res.Body)
    print(status.message)
else
    print("Request failed: " .. (res.Error or res.Status))
end
```
