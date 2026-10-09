# CompressionType
How a database compresses the asset data it stores. [EmuDb:GetCompressionType](/api/luau/classes/EmuDb) returns one.

| Name | Value | Description |
| --- | --- | --- |
| `None` | 0 | Data is stored as is. |
| `ZStandard` | 1 | Data is compressed with Zstandard. |
