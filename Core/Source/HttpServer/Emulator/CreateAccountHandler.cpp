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
// File: CreateAccountHandler.cpp
// Started by: Hattozo
// Started on: 5/1/2026
// Description:
#include <NoobWarrior/HttpServer/Emulator/CreateAccountHandler.h>
#include <NoobWarrior/HttpServer/Emulator/AuthUtil.h>
#include <NoobWarrior/Log.h>
#include <NoobWarrior/HttpServer/Emulator/ServerEmulator.h>
#include <NoobWarrior/NoobWarrior.h>
#include <NoobWarrior/Registry.h>

using namespace NoobWarrior;

CreateAccountHandler::CreateAccountHandler(ServerEmulator* emu) : mEmu(emu) {

}

void CreateAccountHandler::OnRequest(evhttp_request *req, void *userdata) {
    std::map<std::string, std::string> params = GetPostFormParameters(req);

    auto userIt = params.find("username");
    auto passIt = params.find("password");
    auto iam13It = params.find("iam13");

    if (userIt == params.end() || passIt == params.end()) {
        evhttp_send_error(req, HTTP_BADREQUEST, "Missing username or password");
        return;
    }

    const std::string &username = userIt->second;
    const std::string &password = passIt->second;

    if (username.empty()) {
        evhttp_send_error(req, HTTP_BADREQUEST, "Username cannot be empty");
        return;
    }
    if (password.empty()) {
        evhttp_send_error(req, HTTP_BADREQUEST, "Password cannot be empty");
        return;
    }
    if (username.size() > 20) {
        evhttp_send_error(req, HTTP_BADREQUEST, "Username cannot be longer than 20 characters");
        return;
    }
    if (password.size() > 128) {
        evhttp_send_error(req, HTTP_BADREQUEST, "Password cannot be longer than 128 characters");
        return;
    }
    if (iam13It == params.end()) {
        evhttp_send_error(req, HTTP_BADREQUEST, "13+ box not ticked");
        return;
    }
    if (iam13It->second != "on") {
        evhttp_send_error(req, HTTP_BADREQUEST, "You must confirm you are 13 years or older");
        return;
    }

    // Guests registering their own accounts can be disabled so an operator seeds accounts manually.
    if (Registry* reg = mEmu->GetCore()->GetRegistry();
        reg != nullptr && !reg->GetKeyValue<bool>("emu.auth.allow_registration").value_or(false)) {
        evhttp_send_error(req, HTTP_FORBIDDEN, "Registration is disabled on this server");
        return;
    }

    EmuDb* masterDb = mEmu->GetCore()->GetEmuDbManager()->GetMasterDatabase();
    if (masterDb == nullptr || masterDb->Fail()) {
        evhttp_send_error(req, HTTP_INTERNAL, "No usable master database is mounted");
        return;
    }

    if (AuthUtil::LocalAccountExists(masterDb, username)) {
        evhttp_send_error(req, 409, "Username already exists!");
        return;
    }

    std::optional<int64_t> newUserId = AuthUtil::CreateLocalAccount(masterDb, username, password);
    if (!newUserId) {
        evhttp_send_error(req, HTTP_INTERNAL, "Failed to create account");
        return;
    }
    mCore->Out("CreateAccountHandler", "Created account \"{}\" with ID {}", username, *newUserId);

    evhttp_add_header(evhttp_request_get_output_headers(req), "Location", "/login");
    evbuffer *reply = evbuffer_new();
    evhttp_send_reply(req, 302, "Found", reply);
    evbuffer_free(reply);
}
