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

#include <openssl/evp.h>
#include <openssl/rand.h>

using namespace NoobWarrior;

static std::string ToHex(const unsigned char *bytes, size_t len) {
    std::string out;
    out.reserve(len * 2);
    for (size_t i = 0; i < len; i++)
        out += std::format("{:02x}", bytes[i]);
    return out;
}

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

    if (iam13It == params.end()) {
        evhttp_send_error(req, HTTP_BADREQUEST, "You must confirm you are 13 years or older");
        return;
    }

    EmuDb* masterDb = mEmu->GetCore()->GetEmuDbManager()->GetMasterDatabase();
    if (masterDb == nullptr || masterDb->Fail()) {
        evhttp_send_error(req, HTTP_INTERNAL, "No usable master database is mounted");
        return;
    }

    Statement checkUserStmt = masterDb->PrepareStatement("SELECT 1 FROM User WHERE Name = ? COLLATE NOCASE;");
    checkUserStmt.Bind(1, username);
    if (checkUserStmt.Step() == SQLITE_ROW) {
        evhttp_send_error(req, HTTP_FORBIDDEN, "Username already exists!");
        return;
    }

    unsigned char salt[16];
    if (RAND_bytes(salt, sizeof(salt)) != 1) {
        evhttp_send_error(req, HTTP_INTERNAL, "Failed to generate password salt");
        return;
    }

    constexpr int iterations = 100000;
    constexpr int keyLen = 32;
    unsigned char derived[keyLen];
    if (PKCS5_PBKDF2_HMAC(password.data(), static_cast<int>(password.size()),
                          salt, sizeof(salt), iterations, EVP_sha256(),
                          keyLen, derived) != 1) {
        evhttp_send_error(req, HTTP_INTERNAL, "Failed to hash password");
        return;
    }
    
    std::string passwordHash = std::format(
        "pbkdf2_sha256${}${}${}",
        iterations,
        ToHex(salt, sizeof(salt)),
        ToHex(derived, keyLen)
    );

    Statement createUserStmt = masterDb->PrepareStatement(
        "INSERT INTO User (Name, DisplayName, PasswordHash, JoinDate) VALUES (?, ?, ?, unixepoch());"
    );
    createUserStmt.Bind(1, username);
    createUserStmt.Bind(2, username);
    createUserStmt.Bind(3, passwordHash);
    if (createUserStmt.Step() != SQLITE_DONE) {
        Out("CreateAccountHandler", "Failed to insert new user \"{}\": {}", username, masterDb->GetLastErrorMsg());
        evhttp_send_error(req, HTTP_INTERNAL, "Failed to create account");
        return;
    }

    int64_t newUserId = sqlite3_last_insert_rowid(masterDb->Get());
    masterDb->MarkDirty();
    Out("CreateAccountHandler", "Created account \"{}\" with ID {}", username, newUserId);

    evhttp_add_header(evhttp_request_get_output_headers(req), "Location", "/login");
    evbuffer *reply = evbuffer_new();
    evhttp_send_reply(req, 302, "Found", reply);
    evbuffer_free(reply);
}
