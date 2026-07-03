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
// File: AvatarCatalogHandler.cpp
// Started by: Hattozo
// Started on: 7/3/2026
// Description:
#include <NoobWarrior/HttpServer/Emulator/AvatarCatalogHandler.h>
#include <NoobWarrior/HttpServer/Emulator/ServerEmulator.h>
#include <NoobWarrior/NoobWarrior.h>
#include <NoobWarrior/EmuDb/EmuDbManager.h>
#include <NoobWarrior/EmuDb/EmuDb.h>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <set>
#include <string>
#include <vector>

using namespace NoobWarrior;

AvatarCatalogHandler::AvatarCatalogHandler(ServerEmulator* emu) : mEmu(emu) {}

static std::string GetQueryParam(const char* uri, const char* key) {
    if (uri == nullptr)
        return {};
    const char* q = std::strchr(uri, '?');
    if (q == nullptr)
        return {};
    std::string query(q + 1);
    for (size_t pos = 0; pos < query.size();) {
        size_t amp = query.find('&', pos);
        std::string pair = query.substr(pos, amp == std::string::npos ? std::string::npos : amp - pos);
        size_t eq = pair.find('=');
        std::string k = pair.substr(0, eq);
        if (k == key) {
            std::string v = (eq == std::string::npos) ? std::string() : pair.substr(eq + 1);
            if (char* dec = evhttp_uridecode(v.c_str(), 1, nullptr)) {
                std::string out(dec);
                free(dec);
                return out;
            }
            return v;
        }
        if (amp == std::string::npos)
            break;
        pos = amp + 1;
    }
    return {};
}

static std::string EscapeLike(const std::string& s) {
    std::string out;
    for (char c : s) {
        if (c == '%' || c == '_' || c == '\\')
            out += '\\';
        out += c;
    }
    return out;
}

void AvatarCatalogHandler::OnRequest(evhttp_request *req, void *userdata) {
    const char* uri = evhttp_request_get_uri(req);
    int type = std::atoi(GetQueryParam(uri, "type").c_str());
    std::string search = GetQueryParam(uri, "search");
    int page = std::max(0, std::atoi(GetQueryParam(uri, "page").c_str()));
    constexpr int kPageSize = 60;

    struct Item { int64_t id; std::string name; };
    std::vector<Item> items;
    std::set<int64_t> seen;
    std::string escaped = EscapeLike(search);

    EmuDbManager* mgr = mEmu->GetCore()->GetEmuDbManager();
    for (EmuDb* db : mgr->GetMountedDatabases()) {
        std::string sql = "SELECT Id, Name FROM Asset WHERE Type = ?";
        if (!search.empty())
            sql += " AND Name LIKE ? ESCAPE '\\'";
        sql += ";";
        Statement stmt = db->PrepareStatement(sql);
        stmt.Bind(1, type);
        if (!search.empty())
            stmt.Bind(2, "%" + escaped + "%");
        while (stmt.Step() == SQLITE_ROW) {
            int64_t id = stmt.GetInt64FromColumnIndex(0);
            if (seen.insert(id).second)
                items.push_back({ id, stmt.GetStringFromColumnIndex(1) });
        }
    }

    std::sort(items.begin(), items.end(),
              [](const Item& a, const Item& b) { return a.name < b.name; });

    int total = static_cast<int>(items.size());
    int pageCount = std::max(1, (total + kPageSize - 1) / kPageSize);
    if (page >= pageCount)
        page = pageCount - 1;
    int start = page * kPageSize;
    int end = std::min(total, start + kPageSize);

    nlohmann::json out;
    out["page"] = page;
    out["pageCount"] = pageCount;
    out["total"] = total;
    nlohmann::json arr = nlohmann::json::array();
    for (int i = start; i < end; i++) {
        nlohmann::json e;
        e["id"] = items[i].id;
        e["name"] = items[i].name;
        e["assetTypeId"] = type;
        arr.push_back(e);
    }
    out["items"] = arr;

    std::string body = out.dump();
    evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type", "application/json");
    evbuffer* reply = evbuffer_new();
    evbuffer_add(reply, body.data(), body.size());
    evhttp_send_reply(req, 200, nullptr, reply);
    evbuffer_free(reply);
}
