// === noobWarrior ===
// File: Statement.cpp
// Started by: Hattozo
// Started on: 8/8/2025
// Description:
#include <NoobWarrior/Database/Statement.h>
#include <NoobWarrior/Database/Database.h>

using namespace NoobWarrior;

Statement::Statement(Database *db, const std::string &str): mDatabase(db), mFailed(false) {
    if (sqlite3_prepare_v2(db->mDatabase, str.c_str(), -1, &mStmt, nullptr) != SQLITE_OK) {
        mFailed = true;
    }
}

Statement::~Statement() {
    sqlite3_finalize(mStmt);
}

int Statement::Step() {
    return sqlite3_step(mStmt);
}

bool Statement::Failed() {
    return mFailed;
}

SqlValue Statement::GetValueFromColumnIndex(int columnIndex) {
    std::vector<unsigned char> data;
    unsigned char* buf;

    /* Originally the returned datatype depended on the value of the column, but I feel like that would
       create too many bugs and user errors so now it depends on the declared type of the field itself. */
    /*
    switch (sqlite3_column_type(mStmt, columnIndex)) {
    default: return std::monostate();
    case SQLITE_NULL: return std::monostate();
    case SQLITE_INTEGER: return sqlite3_column_int(mStmt, columnIndex);
    case SQLITE_FLOAT: return sqlite3_column_double(mStmt, columnIndex);
    case SQLITE_TEXT: return std::string(reinterpret_cast<const char*>(sqlite3_column_text(mStmt, columnIndex)));
    case SQLITE_BLOB:
        buf = static_cast<unsigned char*>(const_cast<void*>(sqlite3_column_blob(mStmt, columnIndex)));
        data.assign(buf, buf + sqlite3_column_bytes(mStmt, columnIndex));
        return data;
    }
    */
}

SqlRow Statement::GetColumns() {
    return mRow;
}

std::map<std::string, SqlValue> Statement::GetColumnMap() {
    std::map<std::string, SqlValue> columnMap;
    for (int i = 0; i < sqlite3_column_count(mStmt); i++) {
        std::string name = { sqlite3_column_name(mStmt, i) };
        columnMap.emplace(name, GetValueFromColumnIndex(i));
    }
    return columnMap;
}