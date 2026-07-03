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
// File: AvatarSetHandler.cpp
// Started by: Hattozo
// Started on: 7/3/2026
// Description:
#include <NoobWarrior/HttpServer/Emulator/AvatarSetHandler.h>
#include <NoobWarrior/HttpServer/Emulator/ServerEmulator.h>
#include <NoobWarrior/HttpServer/Emulator/AuthUtil.h>
#include <NoobWarrior/HttpServer/Emulator/AvatarAppearance.h>
#include <NoobWarrior/NoobWarrior.h>
#include <NoobWarrior/Registry.h>
#include <NoobWarrior/EmuDb/EmuDb.h>
#include <NoobWarrior/Log.h>
#include <nlohmann/json.hpp>

#include <optional>

using namespace NoobWarrior;

AvatarSetHandler::AvatarSetHandler(ServerEmulator* emu) : mEmu(emu) {}

// "RRGGBB" (or "#RRGGBB") -> packed 0xRRGGBB, or -1 if unparseable.
static int ParseHexColor(const std::string& hex) {
    std::string s = (!hex.empty() && hex.front() == '#') ? hex.substr(1) : hex;
    if (s.empty())
        return -1;
    try {
        return static_cast<int>(std::stoul(s, nullptr, 16)) & 0xFFFFFF;
    } catch (...) {
        return -1;
    }
}

void AvatarSetHandler::OnRequest(evhttp_request *req, void *userdata) {
    auto sendJson = [req](int code, const std::string& body) {
        evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type", "application/json");
        evbuffer* reply = evbuffer_new();
        evbuffer_add(reply, body.data(), body.size());
        evhttp_send_reply(req, code, nullptr, reply);
        evbuffer_free(reply);
    };
    
    const char* cookie = evhttp_find_header(evhttp_request_get_input_headers(req), "Cookie");
    std::string token = AuthUtil::ExtractCookieValue(cookie, ".LOGINSESSION");
    Registry* reg = mEmu->GetCore()->GetRegistry();
    int64_t ttl = reg != nullptr ? reg->GetKeyValue<int64_t>("emu.auth.session_ttl_days").value_or(30) * 86400 : 0;
    EmuDb* master = mEmu->GetCore()->GetEmuDbManager()->GetMasterDatabase();
    std::optional<AuthUtil::SessionUser> user = AuthUtil::ResolveSessionUser(master, token, ttl);
    if (!user) {
        sendJson(401, R"({"error":"You must be signed in to edit your avatar"})");
        return;
    }
    if (master == nullptr || master->Fail()) {
        sendJson(500, R"({"error":"No master database"})");
        return;
    }
    int64_t userId = user->id;

    // GET returns my current avatar (avatar-fetch shape) so the editor can load it; POST saves it.
    if (evhttp_request_get_command(req) == EVHTTP_REQ_GET) {
        sendJson(200, AvatarAppearance::BuildAvatarFetchJsonForUser(mEmu->GetCore(), userId).dump());
        return;
    }

    std::string body;
    if (evbuffer* buf = evhttp_request_get_input_buffer(req)) {
        size_t len = evbuffer_get_length(buf);
        body.resize(len);
        if (len > 0)
            evbuffer_copyout(buf, body.data(), len);
    }
    nlohmann::json j;
    try {
        j = nlohmann::json::parse(body);
    } catch (const nlohmann::json::exception&) {
        sendJson(400, R"({"error":"Malformed JSON"})");
        return;
    }

    Statement clear = master->PrepareStatement("DELETE FROM UserCharacterItem WHERE Id = ?;");
    clear.Bind(1, userId);
    clear.Step();
    if (j.contains("assetAndAssetTypeIds") && j["assetAndAssetTypeIds"].is_array()) {
        for (const auto& e : j["assetAndAssetTypeIds"]) {
            int64_t assetId = e.value("assetId", static_cast<int64_t>(0));
            if (assetId > 0)
                master->AddAssetToUserCharacter(userId, assetId);
        }
    }

    if (j.contains("bodyColor3s") && j["bodyColor3s"].is_object()) {
        const auto& c = j["bodyColor3s"];
        const std::pair<const char*, int> parts[] = {
            {"headColor3", 0}, {"torsoColor3", 1}, {"rightArmColor3", 2},
            {"leftArmColor3", 3}, {"rightLegColor3", 4}, {"leftLegColor3", 5},
        };
        for (const auto& [key, part] : parts) {
            if (c.contains(key) && c[key].is_string()) {
                int packed = ParseHexColor(c[key].get<std::string>());
                if (packed >= 0)
                    master->SetUserCharacterBodyColor(userId, part, packed);
            }
        }
    }

    if (j.contains("scales") && j["scales"].is_object()) {
        const auto& s = j["scales"];
        Statement up = master->PrepareStatement(
            "UPDATE User SET CharacterBodyType = ?, CharacterWidth = ?, CharacterHeight = ?, "
            "CharacterHead = ?, CharacterProportions = ? WHERE Id = ?;");
        up.Bind(1, s.value("bodyType", 0.0));
        up.Bind(2, s.value("width", 1.0));
        up.Bind(3, s.value("height", 1.0));
        up.Bind(4, s.value("head", 1.0));
        up.Bind(5, s.value("proportion", 0.0));
        up.Bind(6, userId);
        up.Step();
    }

    master->MarkDirty();
    Out("AvatarSetHandler", "Updated avatar for {} (id {})", user->name, userId);
    sendJson(200, R"({"ok":true})");
}
