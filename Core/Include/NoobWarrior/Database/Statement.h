// === noobWarrior ===
// File: Statement.h
// Started by: Hattozo
// Started on: 8/8/2025
// Description:
#pragma once
#include <sqlite3.h>
#include <string>

#include "Database.h"

namespace NoobWarrior {
class Statement {
public:
    Statement(Database *db, const std::string &str);
    ~Statement();
protected:
    Database *mDatabase;
    sqlite3_stmt *mStmt;
};
}
