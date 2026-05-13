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
// File: CreateAccountHandler.h
// Started by: Hattozo
// Started on: 5/1/2026
// Description:
#include <NoobWarrior/HttpServer/Emulator/CreateAccountHandler.h>
#include <NoobWarrior/Log.h>
#include <NoobWarrior/HttpServer/Emulator/ServerEmulator.h>
#include <NoobWarrior/NoobWarrior.h>

using namespace NoobWarrior;

CreateAccountHandler::CreateAccountHandler(ServerEmulator* emu) : mEmu(emu) {

}

void CreateAccountHandler::OnRequest(evhttp_request *req, void *userdata) {
    std::map<std::string, std::string> params = GetPostFormParameters(req);
    for (auto& [k, v] : params) {
        Out("CreateAccountHandler", "{} {}", k, v);
    }
    EmuDb* masterDb = mEmu->GetCore()->GetEmuDbManager()->GetMasterDatabase();
    Statement checkUserStmt = masterDb->PrepareStatement("SELECT 1 FROM User WHERE Name = ? COLLATE NOCASE;");
    if (checkUserStmt.Step() == SQLITE_ROW) {
        evhttp_send_error(req, HTTP_FORBIDDEN, "Username already exists!");
        return;
    }
    Statement createUserStmt = masterDb->PrepareStatement("INSERT INTO User (Id, Name, PasswordHash) VALUES (?, ?, ?);");
    evhttp_send_error(req, 500, "oops");
}
