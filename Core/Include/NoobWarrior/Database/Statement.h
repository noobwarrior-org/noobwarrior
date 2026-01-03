// === noobWarrior ===
// File: Statement.h
// Started by: Hattozo
// Started on: 8/8/2025
// Description:
#pragma once
#include <NoobWarrior/Database/Common.h>

#include <sqlite3.h>

#include <string>
#include <vector>
#include <map>

namespace NoobWarrior {
class Database;
class Statement {
public:
    Statement(Database *db, const std::string &str);
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
    bool Failed();
    bool IsColumnIndexNull(int columnIndex);
    SqlValue GetValueFromColumnIndex(int columnIndex);

    SqlRow GetColumns();
    std::map<std::string, SqlValue> GetColumnMap();
protected:
    Database *mDatabase;
    sqlite3_stmt *mStmt;
    bool mFailed;

    SqlRow mRow;
};
}
