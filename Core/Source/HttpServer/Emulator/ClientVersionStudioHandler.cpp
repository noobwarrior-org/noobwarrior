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
// File: ClientVersionStudioHandler.cpp
// Started by: Hattozo
// Started on: 8/8/2026
// Description:
#include <NoobWarrior/HttpServer/Emulator/ClientVersionStudioHandler.h>
#include <NoobWarrior/HttpServer/Emulator/ServerEmulator.h>
#include <NoobWarrior/Log.h>

#include <nlohmann/json.hpp>

using namespace NoobWarrior;

ClientVersionStudioHandler::ClientVersionStudioHandler(ServerEmulator* emu) : mEmu(emu) {}

void ClientVersionStudioHandler::OnRequest(evhttp_request *req, void *userdata) {
    const char* uri = evhttp_request_get_uri(req);
    evhttp_connection* conn = evhttp_request_get_connection(req);

    const char* peer_address = "";
    uint16_t peer_port {};
    if (conn != NULL)
        evhttp_connection_get_peer(conn, &peer_address, &peer_port);
    Out("ClientVersionStudioHandler", "{}:{} requested {}", peer_address, peer_port, uri);

    // Answer with the version of the Studio that's actually asking, so it never self-updates.
    std::string version, hash;
    {
        auto pinned = mEmu->GetLaunchedStudioVersion();
        version = pinned.first;
        hash = pinned.second;
    }

    // If that fails then check the newest running instance carying a version.
    if (version.empty()) {
        time_t newest = 0;
        for (const auto &inst : mEmu->GetRunningInstances()) {
            if (!inst.Version.empty() && inst.FirstSeen >= newest) {
                newest = inst.FirstSeen;
                version = inst.Version;
                hash = inst.Hash;
            }
        }
    }

    // As a last resort, the launching engine's version is unknown.
    // Return a version that is never newer than any real client so Studio never tries to self-update.
    if (version.empty()) version = "0.1.0.1";

    nlohmann::json j;
    j["version"] = version;
    j["clientVersionUpload"] = hash.empty() ? nlohmann::json(nullptr) : nlohmann::json(hash);
    j["bootstrapperVersion"] = version;
    j["nextClientVersionUpload"] = nullptr;
    j["nextClientVersion"] = nullptr;

    const std::string body = j.dump();
    evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type", "application/json");
    evbuffer* reply = evbuffer_new();
    evbuffer_add_printf(reply, "%s", body.c_str());
    evhttp_send_reply(req, 200, nullptr, reply);
    evbuffer_free(reply);
}
