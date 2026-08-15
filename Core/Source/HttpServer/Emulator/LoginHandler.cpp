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
#include <NoobWarrior/HttpServer/Emulator/AuthUtil.h>
#include <NoobWarrior/Log.h>
#include <NoobWarrior/NoobWarrior.h>
#include <NoobWarrior/Registry.h>
#include <nlohmann/json.hpp>

#include <string_view>

using namespace NoobWarrior;

LoginHandler::LoginHandler(ServerEmulator* emu) : mEmu(emu) {

}

namespace {
bool IsPlayerLoginRequest(evhttp_request *req) {
    const char *uri = evhttp_request_get_uri(req);
    if (uri == nullptr)
        return false;

    std::string_view path(uri);
    if (const size_t query = path.find('?'); query != std::string_view::npos)
        path = path.substr(0, query);
    return path == "/v2/login";
}

void SendJson(evhttp_request *req, int status, const nlohmann::json &body) {
    const std::string encoded = body.dump();
    evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type", "application/json");
    evbuffer *reply = evbuffer_new();
    evbuffer_add(reply, encoded.data(), encoded.size());
    evhttp_send_reply(req, status, nullptr, reply);
    evbuffer_free(reply);
}

std::map<std::string, std::string> GetPlayerLoginParameters(evhttp_request *req) {
    std::map<std::string, std::string> params;
    evbuffer *input = evhttp_request_get_input_buffer(req);
    const size_t length = evbuffer_get_length(input);
    std::string body(length, '\0');
    if (length != 0)
        evbuffer_copyout(input, body.data(), length);

    const nlohmann::json json = nlohmann::json::parse(body, nullptr, false);
    if (!json.is_discarded() && json.is_object()) {
        // Roblox's /v2/login schema calls the username field cvalue. Accept the ordinary names as
        // well so this endpoint remains easy to exercise without the Player.
        for (const char *key : {"cvalue", "username", "userName"}) {
            if (json.contains(key) && json[key].is_string()) {
                params["username"] = json[key].get<std::string>();
                break;
            }
        }
        if (json.contains("password") && json["password"].is_string())
            params["password"] = json["password"].get<std::string>();
        return params;
    }

    return Handler::GetPostFormParameters(req);
}
}

void LoginHandler::OnRequest(evhttp_request *req, void *userdata) {
    const bool playerLogin = IsPlayerLoginRequest(req);
    std::map<std::string, std::string> params = playerLogin
        ? GetPlayerLoginParameters(req)
        : GetPostFormParameters(req);

    Registry *registry = mEmu->GetCore()->GetRegistry();
    const bool authEnabled = registry != nullptr
        && registry->GetKeyValue<bool>("emu.auth.enabled").value_or(false);

    // With account authentication disabled, the desktop app signs into the configured local
    // identity. This mirrors the rest of the emulator's legacy trust-local-user behavior and,
    // importantly, gives the app a cross-subdomain .ROBLOSECURITY cookie before any game launch.
    if (playerLogin && !authEnabled) {
        const int64_t userId = registry != nullptr
            ? registry->GetKeyValue<int64_t>("user.id").value_or(1) : 1;
        const std::string userName = registry != nullptr
            ? registry->GetKeyValue<std::string>("user.name").value_or("Player") : "Player";
        const std::string displayName = registry != nullptr
            ? registry->GetKeyValue<std::string>("user.display_name").value_or(userName) : userName;

        evhttp_add_header(evhttp_request_get_output_headers(req), "Set-Cookie",
            ".ROBLOSECURITY=noobwarrior-local-session; Domain=.roblox.com; Path=/; "
            "Max-Age=2592000; Secure; HttpOnly; SameSite=None");
        Out("LoginHandler", "Desktop Player signed into local user \"{}\" (id {})",
            userName, userId);
        SendJson(req, HTTP_OK, {
            {"user", {{"id", userId}, {"name", userName}, {"displayName", displayName}}},
            {"userId", userId},
            {"isBanned", false}
        });
        return;
    }

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

    // Password login can be turned off in favor of OAuth2-only auth.
    if (registry != nullptr &&
        !registry->GetKeyValue<bool>("emu.auth.password_based").value_or(true)) {
        evhttp_send_error(req, HTTP_FORBIDDEN, "Password-based login is disabled on this server");
        return;
    }

    EmuDb* masterDb = mEmu->GetCore()->GetEmuDbManager()->GetMasterDatabase();
    if (masterDb == nullptr || masterDb->Fail()) {
        evhttp_send_error(req, HTTP_INTERNAL, "No usable master database is mounted");
        return;
    }

    Statement lookupStmt = masterDb->PrepareStatement(
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

    std::string token = AuthUtil::CreateLoginSession(masterDb, userId,
        peerAddress ? peerAddress : "", userAgent ? userAgent : "");
    if (token.empty()) {
        evhttp_send_error(req, HTTP_INTERNAL, "Failed to create session");
        return;
    }
    Out("LoginHandler", "User \"{}\" (id {}) logged in from {}", username, userId, peerAddress);

    // Opportunistically sweep sessions idle past the TTL so the table doesn't grow without bound.
    if (Registry *reg = mEmu->GetCore()->GetRegistry()) {
        int64_t ttlSeconds = reg->GetKeyValue<int64_t>("emu.auth.session_ttl_days").value_or(30) * 86400;
        int reaped = AuthUtil::ReapExpiredSessions(masterDb, ttlSeconds);
        if (reaped > 0)
            Out("LoginHandler", "Reaped {} expired login session(s)", reaped);
    }

    if (playerLogin) {
        const std::string playerCookie = std::format(
            ".ROBLOSECURITY={}; Domain=.roblox.com; Path=/; Max-Age=2592000; "
            "Secure; HttpOnly; SameSite=None", token);
        evhttp_add_header(evhttp_request_get_output_headers(req), "Set-Cookie",
                          playerCookie.c_str());
        SendJson(req, HTTP_OK, {
            {"user", {{"id", userId}, {"name", username}, {"displayName", username}}},
            {"userId", userId},
            {"isBanned", false}
        });
        return;
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
