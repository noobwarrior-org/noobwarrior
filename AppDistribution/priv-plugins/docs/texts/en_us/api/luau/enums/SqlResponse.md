# SqlResponse
The result of a database operation. Most [EmuDb](/api/luau/classes/EmuDb) methods that change data return one.

| Name | Value | Description |
| --- | --- | --- |
| `Failed` | 0 | The operation failed for a reason not listed below. |
| `Success` | 1 | The operation worked. |
| `CantOpen` | 2 | The database file could not be opened. |
| `DidNothing` | 3 | The statement ran but changed nothing, usually because no row matched. |
| `DatabaseFailed` | 4 | SQLite reported an error. |
| `ConstraintViolation` | 5 | The change broke a constraint, such as a duplicate id or a foreign key pointing at a missing row. |
| `Busy` | 6 | Another connection holds a lock on the database. |
| `Misuse` | 7 | The database was used incorrectly, for example after it was closed. |
| `NotFound` | 8 | The item being looked up does not exist. |
| `BlobTooLarge` | 9 | The data is too big to store. |
| `MissingBlob` | 10 | The item refers to stored data that is no longer there. |
| `BlobOpenFailed` | 11 | Stored data could not be opened for reading. |
| `BlobCompressionFailed` | 12 | The data could not be compressed. |
| `BlobDecompressionFailed` | 13 | Stored data could not be decompressed. |

# Example
```luau
local res = db:SetTitle("My Database")
if res ~= SqlResponse.Success then
    print("Could not rename the database")
end
```
