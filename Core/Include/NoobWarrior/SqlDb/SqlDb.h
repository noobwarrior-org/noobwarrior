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
        StatementConstraintViolation,
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
        MigrationFailed
    };

    SqlDb(const std::string &path = ":memory:");
    virtual ~SqlDb();

    inline sqlite3 *Get() { return mDb; }

    bool Fail();
    bool ExecStatement(const std::string &stmtStr);
    
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
    std::string GetMetaKeyValue(const std::string &key);

    FailReason GetFailReason();

    Statement PrepareStatement(const std::string &stmtStr);
protected:
    sqlite3 *mDb;
    FailReason mFailReason;
    std::string mPath;
};
}
