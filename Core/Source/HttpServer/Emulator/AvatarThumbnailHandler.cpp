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
// File: AvatarThumbnailHandler.cpp
// Started by: Hattozo
// Started on: 7/3/2026
// Description:
#include <NoobWarrior/HttpServer/Emulator/AvatarThumbnailHandler.h>
#include <NoobWarrior/HttpServer/Emulator/ServerEmulator.h>
#include <NoobWarrior/NoobWarrior.h>
#include <NoobWarrior/EmuDb/EmuDbManager.h>

#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

using namespace NoobWarrior;

AvatarThumbnailHandler::AvatarThumbnailHandler(ServerEmulator* emu) : mEmu(emu) {}

// Tolerant single-param lookup (doesn't fail the whole query on a valueless sibling; see AvatarCatalogHandler).
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
        if (pair.substr(0, eq) == key)
            return (eq == std::string::npos) ? std::string() : pair.substr(eq + 1);
        if (amp == std::string::npos)
            break;
        pos = amp + 1;
    }
    return {};
}

void AvatarThumbnailHandler::OnRequest(evhttp_request *req, void *userdata) {
    const char* uri = evhttp_request_get_uri(req);
    std::string idStr = GetQueryParam(uri, "id");
    char* endPtr = nullptr;
    int64_t id = strtoll(idStr.c_str(), &endPtr, 10);
    if (idStr.empty() || endPtr == idStr.c_str() || *endPtr != '\0') {
        evhttp_send_error(req, 400, "Missing or invalid id");
        return;
    }

    std::vector<unsigned char> image = mEmu->GetCore()->GetEmuDbManager()->RetrieveImageData(ItemType::Asset, id);
    if (image.empty()) {
        evhttp_send_error(req, 404, "No thumbnail");
        return;
    }

    evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type", "image/png");
    evhttp_add_header(evhttp_request_get_output_headers(req), "Cache-Control", "max-age=300");
    evbuffer* buf = evbuffer_new();
    evbuffer_add(buf, image.data(), image.size());
    evhttp_send_reply(req, 200, nullptr, buf);
    evbuffer_free(buf);
}
