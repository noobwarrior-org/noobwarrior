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
// File: RobloxFile.cpp
// Started by: Hattozo
// Started on: 3/9/2025
// Description: Represents a loaded Roblox place/model file. RobloxFile is an Instance and its children are the contents of the file.
// This file is derived from Roblox-File-Format (https://github.com/MaximumADHD/Roblox-File-Format/blob/main/RobloxFile.cs)
#include <NoobWarrior/Roblox/FileFormat/RobloxFile.h>
#include <NoobWarrior/Roblox/FileFormat/BinaryFormat/BinaryRobloxFile.h>
#include <NoobWarrior/Roblox/FileFormat/XmlFormat/XmlRobloxFile.h>

#include <algorithm>
#include <fstream>
#include <iterator>
#include <memory>
#include <utility>

using namespace NoobWarrior::Roblox;

FileResponse RobloxFile::Open(std::unique_ptr<RobloxFile> &file,
                              const std::vector<unsigned char> &buffer) {
    file.reset();
    if (buffer.empty())
        return FileResponse::InvalidHeader;

    const bool binary = buffer.size() >= 8 &&
        std::equal(buffer.begin(), buffer.begin() + 8, "<roblox!");
    if (binary) {
        file = std::make_unique<BinaryFormat::BinaryRobloxFile>();
    } else {
        size_t offset = buffer.size() >= 3 && buffer[0] == 0xef &&
            buffer[1] == 0xbb && buffer[2] == 0xbf ? 3 : 0;
        while (offset < buffer.size() &&
               (buffer[offset] == ' ' || buffer[offset] == '\t' ||
                buffer[offset] == '\r' || buffer[offset] == '\n')) {
            ++offset;
        }
        if (offset >= buffer.size() || buffer[offset] != '<')
            return FileResponse::InvalidHeader;
        file = std::make_unique<XmlFormat::XmlRobloxFile>();
    }

    const FileResponse response = file->ReadFile(buffer);
    if (response != FileResponse::Success)
        file.reset();
    return response;
}

FileResponse RobloxFile::Open(RobloxFile **file, std::vector<unsigned char> buffer) {
    if (file == nullptr)
        return FileResponse::Failed;
    *file = nullptr;
    std::unique_ptr<RobloxFile> opened;
    const FileResponse response = Open(opened, buffer);
    *file = opened.release();
    return response;
}

FileResponse RobloxFile::Open(std::unique_ptr<RobloxFile> &file, std::string_view filePath) {
    std::ifstream stream(std::string(filePath), std::ios::binary);
    if (!stream)
        return FileResponse::Failed;
    std::vector<unsigned char> buffer((std::istreambuf_iterator<char>(stream)),
                                      std::istreambuf_iterator<char>());
    if (!stream.good() && !stream.eof())
        return FileResponse::Failed;
    return Open(file, buffer);
}

FileResponse RobloxFile::Open(RobloxFile **file, std::string_view filePath) {
    if (file == nullptr)
        return FileResponse::Failed;
    *file = nullptr;
    std::unique_ptr<RobloxFile> opened;
    const FileResponse response = Open(opened, filePath);
    *file = opened.release();
    return response;
}

FileResponse RobloxFile::Save(std::string_view filePath) const {
    std::vector<unsigned char> buffer;
    const FileResponse response = Save(buffer);
    if (response != FileResponse::Success)
        return response;

    std::ofstream stream(std::string(filePath), std::ios::binary | std::ios::trunc);
    if (!stream)
        return FileResponse::Failed;
    stream.write(reinterpret_cast<const char *>(buffer.data()),
                 static_cast<std::streamsize>(buffer.size()));
    return stream ? FileResponse::Success : FileResponse::Failed;
}

bool RobloxFile::AppendLuaSourceContainer(std::string_view className,
                                          std::string_view name,
                                          std::string_view source,
                                          bool disabled,
                                          std::string *error,
                                          std::string_view parentClassName) {
    const LuaSourceContainerSpec container {
        className,
        name,
        source,
        disabled,
        parentClassName,
    };
    return AppendLuaSourceContainers(std::span(&container, 1), error);
}

const std::string &RobloxFile::GetLastError() const {
    return mLastError;
}

void RobloxFile::SetLastError(std::string error) {
    mLastError = std::move(error);
}
