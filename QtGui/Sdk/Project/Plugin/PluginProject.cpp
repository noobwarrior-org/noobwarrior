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
// File: PluginProject.cpp
// Started by: Hattozo
// Started on: 2/2/2026
// Description:
#include "PluginProject.h"
#include <NoobWarrior/FileSystem/VirtualFileSystem.h>

using namespace NoobWarrior;

PluginProject::PluginProject(const std::string& path) {
    VirtualFileSystem::Response res = VirtualFileSystem::New(&mVfs, path);
    if (res != VirtualFileSystem::Response::Success) {
        // idk do something
    }
}

PluginProject::~PluginProject() {
    VirtualFileSystem::Free(mVfs);
}

bool PluginProject::Fail() {
    return mVfs->Fail();
}

QString PluginProject::GetFailMsg() {
    return "The plugin's virtual file system failed to initialize.";
}

QString PluginProject::GetOpenFailMsg() {
    return "unknown error";
}

QString PluginProject::GetSaveFailMsg() {
    return "unknown error";
}

QString PluginProject::GetTitle() {
    return "Plugin";
}

QIcon PluginProject::GetIcon() {
    return QIcon(":/images/silk/plugin.png");
}

bool PluginProject::IsDirty() {
    return false;
}

std::filesystem::path PluginProject::GetFilePath() {
    return {};
}

bool PluginProject::Save() {
    return false;
}

void PluginProject::OnShown() {
}
void PluginProject::OnHidden() { }
