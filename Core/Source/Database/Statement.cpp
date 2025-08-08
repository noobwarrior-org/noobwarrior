// === noobWarrior ===
// File: Statement.cpp
// Started by: Hattozo
// Started on: 8/8/2025
// Description:
#include <NoobWarrior/Database/Statement.h>

using namespace NoobWarrior;

Statement::Statement(Database *db, const std::string &str): mDatabase(db) {
    if (sqlite3_prepare_v2(db->mDatabase, str.c_str(), -1, &mStmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error("Failed to prepare statement");
    }
}

Statement::~Statement() {
    sqlite3_finalize(mStmt);
}
