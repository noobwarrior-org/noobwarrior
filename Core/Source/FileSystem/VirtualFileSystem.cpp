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
// File: VirtualFileSystem.cpp
// Started by: Hattozo
// Started on: 12/5/2025
// Description: An abstract interface for a file system.
#include <NoobWarrior/FileSystem/VirtualFileSystem.h>
#include <NoobWarrior/FileSystem/StdFileSystem.h>
#include <NoobWarrior/FileSystem/ZipFileSystem.h>
#include <NoobWarrior/NoobWarrior.h>

using namespace NoobWarrior;

VirtualFileSystem::~VirtualFileSystem() {}

VirtualFileSystem::Format VirtualFileSystem::GetFormatFromPath(const std::filesystem::path &path) {
    if (std::filesystem::is_directory(path))
        return Format::Standard;
    else if (std::filesystem::exists(path))
        return Format::Zip;
    else
        return Format::Invalid;
}

VirtualFileSystem::Response VirtualFileSystem::New(VirtualFileSystem** vfsPtr, const std::filesystem::path &path) {
    *vfsPtr = nullptr;
    switch (GetFormatFromPath(path)) {
    default:
        return Response::InvalidFile;
    case Format::Standard:
        *vfsPtr = new StdFileSystem(path);
        break;
    case Format::Zip:
        *vfsPtr = new ZipFileSystem(path);
        break;
    }
    return !(*vfsPtr)->Fail() ? Response::Success : Response::Failed;
}

void VirtualFileSystem::Free(VirtualFileSystem* vfs) {
    NOOBWARRIOR_FREE_PTR(vfs)
}