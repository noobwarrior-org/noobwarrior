/*
 * Copyright (C) 2026 Hattozo
 *
 * This file is part of noobWarrior.
 *
 * noobWarrior is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * noobWarrior is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with noobWarrior; if not, see
 * <https://www.gnu.org/licenses/>.
 */
// === noobWarrior ===
// File: SqlDb.h
// Started by: Hattozo
// Started on: 1/31/2026
// Description:
#pragma once
#include <NoobWarrior/SqlDb/Common.h>
#include <NoobWarrior/SqlDb/Statement.h>
#include <NoobWarrior/Log.h>

#include <sqlite3.h>

#include <string>
#include <vector>
#include <map>
#include <filesystem>

namespace NoobWarrior {
class SqlDb {
    friend class Statement;
public:
    enum class Response {
        Failed,
        Success,
        CantOpen,
        DidNothing,
        DatabaseFailed,
        ConstraintViolation,
        Busy,
        Misuse,
        DoesNotExist
    };

    enum class FailReason {
        None,
        Unknown,
        Uninitialized,
        CantOpen,
        TransactionFailed,
        MigrationFailed,
        FeatureUnavailable
    };

    SqlDb(const std::string &path = ":memory:", const std::string &logName = "SqlDb");
    virtual ~SqlDb();

    inline sqlite3 *Get() { return mDb; }

    bool Fail();
    bool ExecStatement(const std::string &stmtStr);
    bool SetPragma(const std::string &key, const std::string &val);
    
    /**
     * @brief Returns true if this database is not yet a tangible file and only exists within memory.
     */
    bool IsMemory();

    int GetLastError();

    /**
     * @return Returns the file name of the database's currently loaded file.
     If a file is currently not loaded or if the database is stored in memory only it returns a blank string.
     This does not return a file path, do not confuse this function with returning one.
     */
    std::string GetFileName();
    std::filesystem::path GetFilePath();
    std::string GetLastErrorMsg();

    FailReason GetFailReason();

    Statement PrepareStatement(const std::string &stmtStr);

    SqlRows GetPragma(const std::string &key);
    SqlRows Query(const std::string &stmtStr);

    /* Supports binding. Note that this only supports a single SQL statement since the bindings do not reset after the first statement.
       If you don't need binding and need to execute multiple statements in a single function call, use Query() instead */
    template <std::same_as<SqlValue>... Args>
    SqlRows QueryTyped(const std::string &stmtStr, Args&&... args) {
        SqlRows rows;
        Statement stmt = PrepareStatement(stmtStr);

        int idx = 0;
        for (const auto arg : {args...}) {
            idx++;
            stmt.Bind(idx, arg);
        }
        while (stmt.Step() == SQLITE_ROW) {
            rows.push_back(stmt.GetColumns());
        }
        return rows;
    }
protected:
    std::string mLogName;
    template <typename... Args>
    void Out(std::string_view fmt, Args...args) {
        NoobWarrior::Out(mLogName, std::format("[{}] {}", GetFileName(), fmt), std::forward<Args>(args)...);
    }

    sqlite3 *mDb;
    FailReason mFailReason;
    std::string mPath;
};
}
