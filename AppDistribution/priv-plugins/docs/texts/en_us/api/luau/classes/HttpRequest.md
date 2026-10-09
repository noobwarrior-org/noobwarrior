# HttpRequest
One incoming web request, passed to [HttpServer.OnRequest](/api/luau/classes/HttpServer). It is a plain table with the request's details and functions for answering it.

Only the first reply counts. Later calls to `SendReply`, `SendReplyString` or `SendError` are ignored. If no listener replies, the server answers with an empty `200 OK`.

# Fields
| Field | Type | Description |
| --- | --- | --- |
| `Uri` | `string` | The requested path, including the query string, such as `"/search?q=sword"`. |
| `Method` | `string` | `"GET"`, `"POST"`, `"HEAD"`, `"PUT"`, `"DELETE"`, `"OPTIONS"` or `"PATCH"`. |
| `Headers` | `{ [string]: string? }` | The request's `Cookie`, `User-Agent`, `Host` and `Content-Type` headers. Other headers aren't included. |
| `PostBody` | `string?` | The request body. Only set for `POST` requests. |
| `PeerIp` | `string` | The IP address of whoever sent the request. |
| `PeerPort` | `number` | The port they sent it from. |

# Methods
## AddHeader
```luau
HttpRequest:AddHeader(name: string, value: string)
```

Adds a header to the reply. Call it before sending the reply.

## RemoveHeader
```luau
HttpRequest:RemoveHeader(name: string)
```

Removes every reply header with this name.

## SendReply
```luau
HttpRequest:SendReply(status: number, reason: string?, body: buffer)
```

Sends the reply with a `buffer` as its body. `reason` is the text after the status code. Pass `nil` to use the standard one.

## SendReplyString
```luau
HttpRequest:SendReplyString(status: number, reason: string?, body: string)
```

The same as [SendReply](#sendreply), with a string body.

## SendError
```luau
HttpRequest:SendError(status: number, reason: string?)
```

Sends an error reply with the server's standard error page.

# Example
```luau
emu.OnRequest:Connect(function(req)
    if req.Uri ~= "/api/echo" then
        return
    end
    if req.Method ~= "POST" then
        req:SendError(405, "Use POST")
        return
    end
    req:AddHeader("Content-Type", "text/plain")
    req:SendReplyString(200, nil, req.PostBody)
end)
```
