# VirtualFileSystem
A set of files and folders, stored in a folder on disk, a `.zip`, or several of those layered together. Every file system has the same methods, so code that reads a plugin's files works whether the plugin is zipped or not.

Paths start at the file system's root and use forward slashes, such as `"/lua/main.lua"`.

You get file systems from [core.GetUserDataDir](/api/luau/libraries/core/GetUserDataDir), [Plugin:GetVfs](/api/luau/classes/Plugin), [url.GetVfs](/api/luau/libraries/url/GetVfs) and [HttpServer:GetVfs](/api/luau/classes/HttpServer). You can also make one from one of these subtypes:

| Type | Constructor | Description |
| --- | --- | --- |
| `StdFileSystem` | `StdFileSystem.new(path: string)` | A folder on disk. |
| `ZipFileSystem` | `ZipFileSystem.new(path: string)` | The contents of a `.zip` file. |
| `OverlayFileSystem` | `OverlayFileSystem.new()` | Several file systems stacked on top of each other. Luau can't add layers to one yet. |

`VirtualFileSystem.new` exists but always raises an error.

# Reading files
## ReadFile
```luau
VirtualFileSystem:ReadFile(path: string): buffer?
```

Reads a whole file. Returns its contents as a `buffer`, or `nil` if the file doesn't exist. Use `buffer.tostring` to turn the result into a string.

## GetEntriesInDirectory
```luau
VirtualFileSystem:GetEntriesInDirectory(path: string): { Entry }
```

Lists the files and folders directly inside `path`. Each entry is a table with these fields:

| Field | Type | Description |
| --- | --- | --- |
| `Name` | `string` | The entry's name. |
| `Path` | `string` | The entry's full path. |
| `Type` | `string` | `"File"` or `"Directory"`. |
| `Size` | `number` | The size in bytes. |
| `Exists` | `boolean` | Whether the entry exists. |
| `Failed` | `boolean` | Whether reading the entry's details failed. |

## EntryExists
```luau
VirtualFileSystem:EntryExists(path: string): boolean
```

Returns `true` if a file or folder exists at `path`.

# Reading in pieces
For large files, open a handle and read a piece at a time.

## OpenHandle
```luau
VirtualFileSystem:OpenHandle(path: string): number
```

Opens a file for reading and returns a handle number. Pass the handle to the reading methods below, then to [CloseHandle](#closehandle) when you're done.

## ReadHandleChunk
```luau
VirtualFileSystem:ReadHandleChunk(handle: number, size: number): (boolean, buffer)
```

Reads up to `size` bytes. Returns whether there is more to read, and the bytes that were read as a `buffer`.

## ReadHandleChunkString
```luau
VirtualFileSystem:ReadHandleChunkString(handle: number, size: number): (boolean, string)
```

The same as [ReadHandleChunk](#readhandlechunk), but returns the bytes as a string.

## ReadHandleLine
```luau
VirtualFileSystem:ReadHandleLine(handle: number): (boolean, string)
```

Reads the next line, without its line ending. Returns whether a line was read, and the line.

## IsHandleEOF
```luau
VirtualFileSystem:IsHandleEOF(handle: number): boolean
```

Returns `true` once everything in the file has been read.

## CloseHandle
```luau
VirtualFileSystem:CloseHandle(handle: number): number
```

Closes a handle. Returns 1 on success.

# Writing
Not every file system can be written to. A `.zip` and the built-in plugins are read-only.

## WriteFile
```luau
VirtualFileSystem:WriteFile(path: string, data: string): boolean
```

Writes `data` to a file, replacing it if it exists. Returns `true` on success.

## CreateDirectories
```luau
VirtualFileSystem:CreateDirectories(path: string): boolean
```

Creates a folder, along with any missing folders above it. Returns `true` on success.

## DeleteEntry
```luau
VirtualFileSystem:DeleteEntry(path: string): number
```

Deletes a file or folder. Returns 1 on success.

# Example
```luau
local vfs = plugin:GetVfs()
for _, entry in ipairs(vfs:GetEntriesInDirectory("/lua")) do
    if entry.Type == "File" then
        print(entry.Name, entry.Size .. " bytes")
    end
end
```
