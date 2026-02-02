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
// File: Statement.h
// Started by: Hattozo
// Started on: 8/8/2025
// Description:
#pragma once
#include <NoobWarrior/SqlDb/Common.h>

#include <sqlite3.h>

#include <string>
#include <vector>
#include <map>

namespace NoobWarrior {
class SqlDb;
class Statement {
public:
    Statement(SqlDb *db, const std::string &str);
    ~Statement();

    inline sqlite3_stmt* Get() { return mStmt; }

    inline int Bind(int pos, const std::string &val) { return sqlite3_bind_text(mStmt, pos, val.c_str(), -1, nullptr); }
    inline int Bind(int pos, const char* val) { return sqlite3_bind_text(mStmt, pos, val, -1, nullptr); }
    inline int Bind(int pos, char* val) { return sqlite3_bind_text(mStmt, pos, val, -1, nullptr); }
    inline int Bind(int pos, int val) { return sqlite3_bind_int(mStmt, pos, val); }
    inline int Bind(int pos, bool val) { return sqlite3_bind_int(mStmt, pos, val); }
    inline int Bind(int pos, int64_t val) { return sqlite3_bind_int64(mStmt, pos, val); }
    inline int Bind(int pos, std::vector<unsigned char> val) { return sqlite3_bind_blob64(mStmt, pos, val.data(), val.size(), nullptr); }
    inline int Bind(int pos, double val) { return sqlite3_bind_double(mStmt, pos, val); }
    inline int Bind(int pos) { return sqlite3_bind_null(mStmt, pos); }

    int Step();
    int Reset();
    int ClearBindings();

    bool Fail();
    bool IsColumnIndexNull(int columnIndex);
    SqlValue GetValueFromColumnIndex(int columnIndex);

    SqlRow GetColumns();
    std::map<std::string, SqlValue> GetColumnMap();
protected:
    SqlDb *mDatabase;
    sqlite3_stmt *mStmt;
    bool mFailed;

    SqlRow mRow;
};
}
