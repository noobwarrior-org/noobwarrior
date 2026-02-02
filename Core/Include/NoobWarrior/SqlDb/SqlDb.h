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

namespace NoobWarrior {
class SqlDb {
    friend class Statement;
public:
    enum class Response {
        Failed,
        Success,
        CouldNotOpen,
        CouldNotCreateTable,
        CouldNotSetKeyValues,
        DidNothing,
        NotInitialized,
        StatementConstraintViolation,
        Busy,
        Misuse,
        DoesNotExist,
        MigrationFailed
    };

    SqlDb(bool autocommit = true);
    ~SqlDb();

    inline sqlite3_stmt* Get() { return; }
protected:
};
}
