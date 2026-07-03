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
// File: LogoutHandler.cpp
// Started by: Hattozo
// Started on: 5/13/2026
// Description:
#include <NoobWarrior/HttpServer/Emulator/LogoutHandler.h>
#include <NoobWarrior/HttpServer/Emulator/ServerEmulator.h>
#include <NoobWarrior/HttpServer/Emulator/AuthUtil.h>
#include <NoobWarrior/Log.h>
#include <NoobWarrior/NoobWarrior.h>

using namespace NoobWarrior;

LogoutHandler::LogoutHandler(ServerEmulator* emu) : mEmu(emu) {

}

void LogoutHandler::OnRequest(evhttp_request *req, void *userdata) {
    const char* cookieHeader = evhttp_find_header(evhttp_request_get_input_headers(req), "Cookie");
    std::string token = AuthUtil::ExtractCookieValue(cookieHeader, ".LOGINSESSION");

    if (!token.empty()) {
        EmuDb* masterDb = mEmu->GetCore()->GetEmuDbManager()->GetMasterDatabase();
        if (masterDb != nullptr && !masterDb->Fail()) {
            Statement deleteStmt = masterDb->PrepareStatement("DELETE FROM LoginSession WHERE Token = ?;");
            deleteStmt.Bind(1, token);
            if (deleteStmt.Step() == SQLITE_DONE)
                masterDb->MarkDirty();
            else
                Out("LogoutHandler", "Failed to delete session: {}", masterDb->GetLastErrorMsg());
        }
    }

    // delete it
    evhttp_add_header(evhttp_request_get_output_headers(req), "Set-Cookie",
                      ".LOGINSESSION=deleted; Path=/; Max-Age=0; HttpOnly; SameSite=Lax");
    evhttp_add_header(evhttp_request_get_output_headers(req), "Location", "/");
    evbuffer *reply = evbuffer_new();
    evhttp_send_reply(req, 302, "Found", reply);
    evbuffer_free(reply);
}
