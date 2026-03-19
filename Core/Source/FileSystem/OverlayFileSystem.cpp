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
// File: OverlayFileSystem.cpp
// Started by: Hattozo
// Started on: 12/5/2025
// Description: An pseudo file system that overlays each file system over one another.
#include <NoobWarrior/FileSystem/OverlayFileSystem.h>
#include <NoobWarrior/FileSystem/VirtualFileSystem.h>

using namespace NoobWarrior;

OverlayFileSystem::OverlayFileSystem() {

}

OverlayFileSystem::~OverlayFileSystem() {

}

VirtualFileSystem::Response OverlayFileSystem::Mount(const std::string &vol, std::unique_ptr<VirtualFileSystem> vfs) {

    return VirtualFileSystem::Response::Failed;
}

VirtualFileSystem::Response OverlayFileSystem::Mount(const std::string &vol, const std::filesystem::path &realPath) {
    return VirtualFileSystem::Response::Failed;
}

VirtualFileSystem::Response OverlayFileSystem::Unmount(VirtualFileSystem* vfs) {
    return VirtualFileSystem::Response::Failed;
}

VirtualFileSystem::Response OverlayFileSystem::Unmount(const std::string &vol) {
    return VirtualFileSystem::Response::Failed;
}

FSEntryInfo OverlayFileSystem::GetEntryFromPath(const std::string &path) {
    return {};
}

std::vector<FSEntryInfo> OverlayFileSystem::GetEntriesInDirectory(const std::string &path) {
    return {};
}

FSEntryHandle OverlayFileSystem::OpenHandle(const std::string &path) {
    return {};
}

VirtualFileSystem::Response OverlayFileSystem::CloseHandle(FSEntryHandle handle) {
    return VirtualFileSystem::Response::Failed;
}

bool OverlayFileSystem::IsHandleEOF(FSEntryHandle handle) {
    return false;
}

bool OverlayFileSystem::ReadHandleChunk(FSEntryHandle handle, std::vector<unsigned char> *buf, unsigned int size) {
    return false;
}

bool OverlayFileSystem::ReadHandleLine(FSEntryHandle handle, std::string *buf) {
    return false;
}

bool OverlayFileSystem::EntryExists(const std::string &path) {
    return false;
}

VirtualFileSystem::Response OverlayFileSystem::DeleteEntry(const std::string &path) {
    return VirtualFileSystem::Response::Failed;
}

std::filesystem::path OverlayFileSystem::ConstructRealPath(std::string submittedPath) {
    return {};
}