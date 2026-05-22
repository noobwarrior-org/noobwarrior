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
#include <NoobWarrior/Log.h>
#include <NoobWarrior/HttpServer/Emulator/ServerEmulator.h>
#include <NoobWarrior/NoobWarrior.h>

#include <openssl/ssl.h>
#include <openssl/rand.h>
#include <openssl/kdf.h>
#include <openssl/params.h>
#include <openssl/core_names.h>

#define HASH_LENGTH 32
#define SALT_LENGTH 16

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

    EmuDb* masterDb = mEmu->GetCore()->GetEmuDbManager()->GetMasterDatabase();
    if (masterDb == nullptr || masterDb->Fail()) {
        evhttp_send_error(req, HTTP_INTERNAL, "No usable master database is mounted");
        return;
    }

    Statement checkUserStmt = masterDb->PrepareStatement("SELECT 1 FROM User WHERE Name = ? COLLATE NOCASE;");
    checkUserStmt.Bind(1, username);
    if (checkUserStmt.Step() == SQLITE_ROW) {
        evhttp_send_error(req, 409, "Username already exists!");
        return;
    }

    unsigned char salt[SALT_LENGTH];
    if (RAND_bytes(salt, sizeof(salt)) != 1) {
        evhttp_send_error(req, HTTP_INTERNAL, "Failed to generate password salt");
        return;
    }
    std::string saltStr;
	for (int i = 0; i < SALT_LENGTH; i++) {
		saltStr += std::format("{:02x}", salt[i]);
	}

    unsigned char hash[HASH_LENGTH];
    {
        uint32_t t_cost = 2, m_cost = 1u << 16, lanes = 1, threads = 1;
        EVP_KDF *kdf = EVP_KDF_fetch(nullptr, "ARGON2ID", nullptr);
        EVP_KDF_CTX *kctx = kdf ? EVP_KDF_CTX_new(kdf) : nullptr;
        EVP_KDF_free(kdf);
        OSSL_PARAM params[] = {
            OSSL_PARAM_octet_string(OSSL_KDF_PARAM_PASSWORD, const_cast<char*>(password.data()), password.size()),
            OSSL_PARAM_octet_string(OSSL_KDF_PARAM_SALT, salt, SALT_LENGTH),
            OSSL_PARAM_uint32(OSSL_KDF_PARAM_ITER, &t_cost),
            OSSL_PARAM_uint32(OSSL_KDF_PARAM_ARGON2_MEMCOST, &m_cost),
            OSSL_PARAM_uint32(OSSL_KDF_PARAM_ARGON2_LANES, &lanes),
            OSSL_PARAM_uint32(OSSL_KDF_PARAM_THREADS, &threads),
            OSSL_PARAM_END
        };
        bool ok = kctx && EVP_KDF_derive(kctx, hash, HASH_LENGTH, params) > 0;
        EVP_KDF_CTX_free(kctx);
        if (!ok) {
            evhttp_send_error(req, HTTP_INTERNAL, "Failed to hash password");
            return;
        }
    }
    std::string hashStr;
	for (int i = 0; i < HASH_LENGTH; i++) {
		hashStr += std::format("{:02x}", hash[i]);
	}

    Statement createUserStmt = masterDb->PrepareStatement(
        "INSERT INTO User (Name, DisplayName, PasswordHash, PasswordSalt, JoinDate) VALUES (?, ?, ?, ?, unixepoch());"
    );
    createUserStmt.Bind(1, username);
    createUserStmt.Bind(2, username);
    createUserStmt.Bind(3, hashStr);
    createUserStmt.Bind(4, saltStr);
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
