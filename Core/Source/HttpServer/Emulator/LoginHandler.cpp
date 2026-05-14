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
// File: LoginHandler.cpp
// Started by: Hattozo
// Started on: 5/13/2026
// Description:
#include <NoobWarrior/HttpServer/Emulator/LoginHandler.h>
#include <NoobWarrior/HttpServer/Emulator/ServerEmulator.h>
#include <NoobWarrior/Log.h>
#include <NoobWarrior/NoobWarrior.h>

#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/rand.h>

#include <charconv>

using namespace NoobWarrior;

static std::string ToHex(const unsigned char *bytes, size_t len) {
    std::string out;
    out.reserve(len * 2);
    for (size_t i = 0; i < len; i++)
        out += std::format("{:02x}", bytes[i]);
    return out;
}

static bool FromHex(const std::string &hex, std::vector<unsigned char> &out) {
    if (hex.size() % 2 != 0) return false;
    out.clear();
    out.reserve(hex.size() / 2);
    for (size_t i = 0; i < hex.size(); i += 2) {
        unsigned int byte = 0;
        auto [ptr, ec] = std::from_chars(hex.data() + i, hex.data() + i + 2, byte, 16);
        if (ec != std::errc() || ptr != hex.data() + i + 2) return false;
        out.push_back(static_cast<unsigned char>(byte));
    }
    return true;
}

// Splits "pbkdf2_sha256$<iters>$<saltHex>$<hashHex>" into its parts.
static bool ParseStoredHash(const std::string &stored, int &iters,
                            std::vector<unsigned char> &salt,
                            std::vector<unsigned char> &hash) {
    constexpr std::string_view prefix = "pbkdf2_sha256$";
    if (!stored.starts_with(prefix)) return false;

    size_t a = prefix.size();
    size_t b = stored.find('$', a);
    if (b == std::string::npos) return false;
    size_t c = stored.find('$', b + 1);
    if (c == std::string::npos) return false;

    auto [ptr, ec] = std::from_chars(stored.data() + a, stored.data() + b, iters);
    if (ec != std::errc() || ptr != stored.data() + b) return false;

    if (!FromHex(stored.substr(b + 1, c - (b + 1)), salt)) return false;
    if (!FromHex(stored.substr(c + 1), hash)) return false;
    return true;
}

LoginHandler::LoginHandler(ServerEmulator* emu) : mEmu(emu) {

}

void LoginHandler::OnRequest(evhttp_request *req, void *userdata) {
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

    EmuDb* masterDb = mEmu->GetCore()->GetEmuDbManager()->GetMasterDatabase();
    if (masterDb == nullptr || masterDb->Fail()) {
        evhttp_send_error(req, HTTP_INTERNAL, "No usable master database is mounted");
        return;
    }

    Statement lookupStmt = masterDb->PrepareStatement(
        "SELECT Id, PasswordHash FROM User WHERE Name = ? COLLATE NOCASE;"
    );
    lookupStmt.Bind(1, username);
    if (lookupStmt.Step() != SQLITE_ROW) {
        evhttp_send_error(req, HTTP_FORBIDDEN, "Invalid username or password");
        return;
    }

    int64_t userId = lookupStmt.GetInt64FromColumnIndex(0);
    std::string storedHash = lookupStmt.GetStringFromColumnIndex(1);

    int iters = 0;
    std::vector<unsigned char> salt;
    std::vector<unsigned char> expected;
    if (!ParseStoredHash(storedHash, iters, salt, expected) || expected.empty()) {
        Out("LoginHandler", "Stored hash for user \"{}\" is malformed or uses an unsupported scheme", username);
        evhttp_send_error(req, HTTP_FORBIDDEN, "Invalid username or password");
        return;
    }

    std::vector<unsigned char> derived(expected.size());
    if (PKCS5_PBKDF2_HMAC(password.data(), static_cast<int>(password.size()),
                          salt.data(), static_cast<int>(salt.size()),
                          iters, EVP_sha256(),
                          static_cast<int>(derived.size()), derived.data()) != 1) {
        evhttp_send_error(req, HTTP_INTERNAL, "Failed to verify password");
        return;
    }

    if (CRYPTO_memcmp(derived.data(), expected.data(), expected.size()) != 0) {
        evhttp_send_error(req, HTTP_FORBIDDEN, "Invalid username or password");
        return;
    }

    unsigned char tokenBytes[32];
    if (RAND_bytes(tokenBytes, sizeof(tokenBytes)) != 1) {
        evhttp_send_error(req, HTTP_INTERNAL, "Failed to generate session token");
        return;
    }
    std::string token = ToHex(tokenBytes, sizeof(tokenBytes));

    const char* peerAddress = "";
    uint16_t peerPort {};
    evhttp_connection* conn = evhttp_request_get_connection(req);
    if (conn != nullptr)
        evhttp_connection_get_peer(conn, &peerAddress, &peerPort);

    const char* userAgent = evhttp_find_header(evhttp_request_get_input_headers(req), "User-Agent");

    Statement sessionStmt = masterDb->PrepareStatement(
        "INSERT INTO LoginSession (Token, UserId, Ip, Device) VALUES (?, ?, ?, ?);"
    );
    sessionStmt.Bind(1, token);
    sessionStmt.Bind(2, userId);
    sessionStmt.Bind(3, std::string(peerAddress ? peerAddress : ""));
    sessionStmt.Bind(4, std::string(userAgent ? userAgent : ""));
    if (sessionStmt.Step() != SQLITE_DONE) {
        Out("LoginHandler", "Failed to create login session for user {}: {}", userId, masterDb->GetLastErrorMsg());
        evhttp_send_error(req, HTTP_INTERNAL, "Failed to create session");
        return;
    }
    masterDb->MarkDirty();
    Out("LoginHandler", "User \"{}\" (id {}) logged in from {}", username, userId, peerAddress);

    std::string cookie = std::format(".LOGINSESSION={}; Path=/; HttpOnly; SameSite=Lax", token);
    if (rememberIt != params.end())
        cookie += "; Max-Age=2592000";

    evhttp_add_header(evhttp_request_get_output_headers(req), "Set-Cookie", cookie.c_str());
    evhttp_add_header(evhttp_request_get_output_headers(req), "Location", "/");
    evbuffer *reply = evbuffer_new();
    evhttp_send_reply(req, 302, "Found", reply);
    evbuffer_free(reply);
}
