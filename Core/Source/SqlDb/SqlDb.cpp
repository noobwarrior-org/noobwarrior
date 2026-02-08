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
// File: SqlDb.cpp
// Started by: Hattozo
// Started on: 1/31/2026
// Description:
#include <NoobWarrior/SqlDb/SqlDb.h>

#include <format>
#include <fstream>

using namespace NoobWarrior;

SqlDb::SqlDb(const std::string &path, const std::string &logName) :
    mLogName(logName),
    mDb(nullptr),
    mFailReason(FailReason::Uninitialized),
    mPath(path)
{
    if (!IsMemory() && !std::filesystem::exists(path)) {
        std::ofstream out(path, std::ios::app); // create dummy file
        if (out.is_open())
            out.close();
    }

    int val = sqlite3_open_v2(path.c_str(), &mDb, SQLITE_OPEN_READWRITE, nullptr);

    switch (val) {
    case SQLITE_OK: Out("Opened"); break;
    case SQLITE_CANTOPEN: Out("Failed to open: {}", GetLastErrorMsg()); mFailReason = FailReason::CantOpen; return;
    default: Out("Failed to open: {}", GetLastErrorMsg()); mFailReason = FailReason::Unknown; return;
    }

    mFailReason = FailReason::None;
}

SqlDb::~SqlDb() {
    if (mDb != nullptr)
        sqlite3_close_v2(mDb);
}

bool SqlDb::Fail() {
    return mFailReason != FailReason::None;
}

bool SqlDb::ExecStatement(const std::string &stmtStr) {
	int res = sqlite3_exec(mDb, stmtStr.c_str(), nullptr, nullptr, nullptr);
	return res == SQLITE_OK;
}

bool SqlDb::SetPragma(const std::string &key, const std::string &val) {
    return ExecStatement(std::format("PRAGMA {}={};", key, val));
}

bool SqlDb::IsMemory() {
	return mPath.compare(":memory:") == 0;
}

int SqlDb::GetLastError() {
    return sqlite3_errcode(mDb);
}

std::string SqlDb::GetFileName() {
	if (mPath.compare(":memory:") == 0)
		return "Unsaved Database";
    std::string::size_type last_slash = mPath.find_last_of("/");
	return last_slash != std::string::npos ? mPath.substr(last_slash + 1) : mPath;
}

std::filesystem::path SqlDb::GetFilePath() {
    if (mPath.compare(":memory:") == 0)
        return "";
    return mPath;
}

std::string SqlDb::GetLastErrorMsg() {
    return sqlite3_errmsg(mDb);
}

std::string SqlDb::GetPragma(const std::string &key) {
    Statement stmt = PrepareStatement("PRAGMA " + key + ";");
    if (stmt.Fail())
        return "";
    return std::get<std::string>(stmt.GetValueFromColumnIndex(0));
}

SqlDb::FailReason SqlDb::GetFailReason() {
    return mFailReason;
}

Statement SqlDb::PrepareStatement(const std::string &stmtStr) {
	return Statement(this, stmtStr);
}
