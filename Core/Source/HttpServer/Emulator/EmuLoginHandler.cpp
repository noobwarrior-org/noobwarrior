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
// File: EmuLoginHandler.cpp
// Started by: Hattozo
// Started on: 8/23/2026
// Description:
#include <NoobWarrior/HttpServer/Emulator/EmuLoginHandler.h>
#include <NoobWarrior/HttpServer/Emulator/ServerEmulator.h>
#include <NoobWarrior/HttpServer/Emulator/AuthUtil.h>
#include <NoobWarrior/Log.h>
#include <NoobWarrior/NoobWarrior.h>
#include <NoobWarrior/Registry.h>

using namespace NoobWarrior;

static bool UserExistsIn(EmuDb *db, const std::string &username) {
    if (db == nullptr || db->Fail())
        return false;
    Statement stmt = db->PrepareStatement(
        "SELECT EXISTS(SELECT 1 FROM User WHERE Name = ? COLLATE NOCASE);"
    );
    stmt.Bind(1, username);
    return stmt.Step() == SQLITE_ROW && stmt.GetIntFromColumnIndex(0) != 0;
}

EmuLoginHandler::EmuLoginHandler(ServerEmulator* emu) : mEmu(emu) {

}

void EmuLoginHandler::OnRequest(evhttp_request *req, void *userdata) {
    std::map<std::string, std::string> params = GetPostFormParameters(req);

    auto userIt = params.find("username");
    auto passIt = params.find("password");
    auto rememberIt = params.find("remember");

    if (userIt == params.end() || passIt == params.end()) {
        evhttp_send_error(req, HTTP_BADREQUEST, "Missing username or password");
        return;
    }

    const std::string &username = userIt->second;
    const std::string &password = passIt->second;

    if (username.empty() || password.empty()) {
        evhttp_send_error(req, HTTP_BADREQUEST, "Username and password cannot be empty");
        return;
    }

    Registry *registry = mEmu->GetCore()->GetRegistry();

    // Password login can be turned off in favor of OAuth2-only auth.
    if (registry != nullptr &&
        !registry->GetKeyValue<bool>("emu.auth.password_based").value_or(true)) {
        evhttp_send_error(req, HTTP_FORBIDDEN, "Password-based login is disabled on this server");
        return;
    }

    const bool allowAccountsFromAllMountedDbs = registry != nullptr &&
        registry->GetKeyValue<bool>("emu.auth.allow_accounts_from_all_mounted_databases").value_or(false);

    EmuDbManager* dbManager = mEmu->GetCore()->GetEmuDbManager();
    EmuDb* master = dbManager->GetMasterDatabase();
    EmuDb* db = master;
    
    if (allowAccountsFromAllMountedDbs && !UserExistsIn(master, username)) {
        for (EmuDb* otherDb : dbManager->GetMountedDatabases()) {
            if (otherDb != master && UserExistsIn(otherDb, username)) {
                db = otherDb;
                break;
            }
        }
    }
    if (db == nullptr || db->Fail()) {
        evhttp_send_error(req, HTTP_INTERNAL, "No usable master database is mounted");
        return;
    }

    Statement lookupStmt = db->PrepareStatement(
        "SELECT Id, PasswordHash, PasswordSalt FROM User WHERE Name = ? COLLATE NOCASE;"
    );
    lookupStmt.Bind(1, username);
    if (lookupStmt.Step() != SQLITE_ROW) {
        evhttp_send_error(req, HTTP_FORBIDDEN, "Invalid username or password");
        return;
    }

    int64_t userId = lookupStmt.GetInt64FromColumnIndex(0);
    std::string storedHash = lookupStmt.GetStringFromColumnIndex(1);
    std::string storedSalt = lookupStmt.GetStringFromColumnIndex(2);

    if (!AuthUtil::VerifyPassword(password, storedSalt, storedHash)) {
        evhttp_send_error(req, HTTP_FORBIDDEN, "Invalid username or password");
        return;
    }

    const char* peerAddress = "";
    uint16_t peerPort {};
    evhttp_connection* conn = evhttp_request_get_connection(req);
    if (conn != nullptr)
        evhttp_connection_get_peer(conn, &peerAddress, &peerPort);

    const char* userAgent = evhttp_find_header(evhttp_request_get_input_headers(req), "User-Agent");

    std::string token = AuthUtil::CreateLoginSession(db, userId,
        peerAddress ? peerAddress : "", userAgent ? userAgent : "");
    if (token.empty()) {
        evhttp_send_error(req, HTTP_INTERNAL, "Failed to create session");
        return;
    }
    Out("EmuLoginHandler", "User \"{}\" (id {}) logged in from {}", username, userId, peerAddress);

    // Opportunistically sweep sessions idle past the TTL so the table doesn't grow without bound.
    if (registry != nullptr) {
        int64_t ttlSeconds = registry->GetKeyValue<int64_t>("emu.auth.session_ttl_days").value_or(30) * 86400;
        int reaped = AuthUtil::ReapExpiredSessions(db, ttlSeconds);
        if (reaped > 0)
            Out("EmuLoginHandler", "Reaped {} expired login session(s)", reaped);
    }

    std::string cookie = std::format(".LOGINSESSION={}; Path=/; HttpOnly; SameSite=Lax", token);
    if (rememberIt != params.end())
        cookie += "; Max-Age=2592000";

    evhttp_add_header(evhttp_request_get_output_headers(req), "Set-Cookie", cookie.c_str());
    evhttp_add_header(evhttp_request_get_output_headers(req), "Location", "/");
    evbuffer *reply = evbuffer_new();
    evhttp_send_reply(req, 302, "Found", reply);
    evbuffer_free(reply);
}
