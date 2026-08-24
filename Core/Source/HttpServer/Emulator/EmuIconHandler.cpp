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
// File: EmuIconHandler.cpp
// Started by: Hattozo
// Started on: 8/24/2026
// Description: Serves the icon for the emulator.
#include <NoobWarrior/HttpServer/Emulator/EmuIconHandler.h>
#include <NoobWarrior/HttpServer/Emulator/ServerEmulator.h>
#include <NoobWarrior/Log.h>
#include <NoobWarrior/NoobWarrior.h>
#include <NoobWarrior/Registry.h>
#include <NoobWarrior/Url.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <iterator>
#include <string>
#include <system_error>
#include <vector>

using namespace NoobWarrior;

// A 68-byte fully transparent 1x1 PNG.
static const unsigned char kBlankPng[] = {
    0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A, 0x00, 0x00, 0x00, 0x0D, 0x49, 0x48, 0x44, 0x52,
    0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x08, 0x06, 0x00, 0x00, 0x00, 0x1F, 0x15, 0xC4,
    0x89, 0x00, 0x00, 0x00, 0x0B, 0x49, 0x44, 0x41, 0x54, 0x78, 0xDA, 0x63, 0x60, 0x00, 0x02, 0x00,
    0x00, 0x05, 0x00, 0x01, 0xE9, 0xFA, 0xDC, 0xD8, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4E, 0x44,
    0xAE, 0x42, 0x60, 0x82
};

static const char* SniffImageMimeType(const std::vector<unsigned char> &data) {
    auto starts = [&data](std::initializer_list<unsigned char> magic) {
        if (data.size() < magic.size()) return false;
        return std::equal(magic.begin(), magic.end(), data.begin());
    };

    if (starts({0x89, 0x50, 0x4E, 0x47}))             return "image/png";
    if (starts({0xFF, 0xD8, 0xFF}))                   return "image/jpeg";
    if (starts({0x47, 0x49, 0x46, 0x38}))             return "image/gif";
    if (starts({0x42, 0x4D}))                         return "image/bmp";
    if (starts({0x00, 0x00, 0x01, 0x00}))             return "image/x-icon";
    // RIFF....WEBP
    if (starts({0x52, 0x49, 0x46, 0x46}) && data.size() >= 12 &&
        data[8] == 'W' && data[9] == 'E' && data[10] == 'B' && data[11] == 'P')
        return "image/webp";
    if (starts({0x3C, 0x3F, 0x78, 0x6D, 0x6C}) || starts({0x3C, 0x73, 0x76, 0x67})) // "<?xml" / "<svg"
        return "image/svg+xml";
    return "image/png";
}

static bool ReadWholeLocalFile(const std::filesystem::path &path, std::vector<unsigned char> *out) {
    std::error_code ec;
    if (!std::filesystem::is_regular_file(path, ec))
        return false;
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open())
        return false;
    out->assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
    return !out->empty();
}

static bool ReadWholeVfsFile(VirtualFileSystem *vfs, const std::string &path, std::vector<unsigned char> *out) {
    if (vfs == nullptr)
        return false;
    FSEntryInfo info = vfs->GetEntryFromPath(path);
    if (info.Failed || !info.Exists || info.Type != FSEntryInfo::Type::File || info.Size == 0)
        return false;
    FSEntryHandle handle = vfs->OpenHandle(path);
    if (handle == 0)
        return false;
    bool read = vfs->ReadHandleChunk(handle, out, static_cast<unsigned int>(info.Size));
    vfs->CloseHandle(handle);
    return read && !out->empty();
}

EmuIconHandler::EmuIconHandler(ServerEmulator* emu) : mEmu(emu) {

}

bool EmuIconHandler::LoadConfiguredIcon(std::vector<unsigned char> *out) {
    Registry* reg = mEmu->GetCore()->GetRegistry();
    if (reg == nullptr)
        return false;

    std::string icon = reg->GetKeyValue<std::string>("emu.branding.icon").value_or("");
    if (icon.empty())
        return false;

    // No protocol means a path on the website, which is served out of the volumes plugins mount on
    // this server, that is where the default "/img/icon1024.png" lives.
    if (icon.find("://") == std::string::npos) {
        if (icon.front() != '/')
            icon.insert(icon.begin(), '/');
        return ReadWholeVfsFile(mEmu->GetVfs(), icon, out);
    }

    Url url(icon);
    if (url.Fail())
        return false;

    switch (url.GetProtocol()) {
    case ProtocolType::Plugin: {
        VirtualFileSystem* vfs = url.GetVfs(mEmu->GetCore());
        return ReadWholeVfsFile(vfs, url.ResolveAsPath(), out);
    }
    case ProtocolType::PluginData:
    case ProtocolType::UserData:
    case ProtocolType::InstallData:
    case ProtocolType::File:
        return ReadWholeLocalFile(url.ResolveAsLocalPath(mEmu->GetCore()), out);
    default:
        // local files only for now
        Out("EmuIconHandler", "emu.branding.icon uses a protocol that cannot be served from: {}", icon);
        return false;
    }
}

void EmuIconHandler::OnRequest(evhttp_request *req, void *userdata) {
    std::vector<unsigned char> data;
    const char* mimeType = "image/png";

    if (LoadConfiguredIcon(&data)) {
        mimeType = SniffImageMimeType(data);
    } else {
        data.assign(std::begin(kBlankPng), std::end(kBlankPng));
    }

    evkeyvalq* headers = evhttp_request_get_output_headers(req);
    evhttp_add_header(headers, "Content-Type", mimeType);
    evhttp_add_header(headers, "Cache-Control", "max-age=60");

    evbuffer* reply = evbuffer_new();
    evbuffer_add(reply, data.data(), data.size());
    evhttp_send_reply(req, HTTP_OK, nullptr, reply);
    evbuffer_free(reply);
}
