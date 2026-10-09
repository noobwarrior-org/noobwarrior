# SqlDb
A connection to an SQLite database. Plugins use it for databases of their own, such as the master server's list of game servers. For noobWarrior's item databases, use [EmuDb](/api/luau/classes/EmuDb), which has every method here as well.

# Constructors
## SqlDb.new
```luau
SqlDb.new(url: string, logName: string): SqlDb
```

Opens the database at `url`, creating the file if it doesn't exist. Use a `data://` URL so the file lands in a writable folder. `logName` labels the database's messages in the log. Pass `""` to use `"SqlDb"`.

# Methods
## Query
```luau
SqlDb:Query(sql: string): { Row }? | false
```

Runs a statement and returns every row it produced. Each row is a table keyed by column name.

Returns `nil` if the statement produced no rows, and `false` if it failed. Check for both before reading the result.

Don't build `sql` from user input. Use [QueryTyped](#querytyped), which keeps values separate from the statement.

## QueryTyped
```luau
SqlDb:QueryTyped(sql: string, ...: any): { Row }? | false
```

Like [Query](#query), but `?` placeholders in `sql` are filled from the extra arguments, in order. Values are bound safely, so this is the right way to put user input into a query. Whole numbers are bound as 64-bit integers.

Columns that are `NULL` are left out of the row table, so reading one gives `nil`.

## ExecStatement
```luau
SqlDb:ExecStatement(sql: string): boolean
```

Runs one or more statements that don't return rows, such as `CREATE TABLE`. Returns `true` on success.

## SetPragma
```luau
SqlDb:SetPragma(key: string, value: string): boolean
```

Sets an SQLite pragma, such as `journal_mode`. Returns `true` on success.

## GetFileName
```luau
SqlDb:GetFileName(): string
```

Returns the database file's name.

## GetFilePath
```luau
SqlDb:GetFilePath(): string
```

Returns the database file's full path on disk.

# Example
```luau
local db = SqlDb.new("data://myplugin/scores.db", "Scores")
db:ExecStatement("CREATE TABLE IF NOT EXISTS Score (UserId INTEGER PRIMARY KEY, Points INTEGER)")

db:QueryTyped("INSERT OR REPLACE INTO Score (UserId, Points) VALUES (?, ?)", 1, 500)

local rows = db:QueryTyped("SELECT Points FROM Score WHERE UserId = ?", 1)
if rows then
    print(rows[1].Points) -- 500
end
```
