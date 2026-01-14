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
// File: Instance.cpp
// Started by: Hattozo
// Started on: 3/9/2025
// Description:
#include <NoobWarrior/Roblox/FileFormat/Tree/Instance.h>
#include <NoobWarrior/Log.h>

using namespace NoobWarrior::Roblox;

Instance::Instance() : RbxObject(), ParentUnsafe(nullptr) {
    Name = ClassName;
}

Instance* Instance::GetParent() {
    return ParentUnsafe;
}

bool Instance::SetParent(Instance* inst) {
    if (ParentLocked) {
        std::string newParent = inst != nullptr ? inst->Name : "NULL";
        std::string currParent = ParentUnsafe != nullptr ? ParentUnsafe->Name : "NULL";
        Out("Instance", "The Parent property of {} is locked, current parent: {}, new parent {}", Name, currParent, newParent);
        return false;
    }

    if (IsAncestorOf(inst)) {
        std::string pathA = GetFullName(".");
        std::string pathB = inst->GetFullName(".");
        Out("Instance", "Attempt to set parent of {} to {} would result in circular reference", pathA, pathB);
        return false;
    }

    if (inst == this) {
        Out("Instance", "Attempt to set {} as its own parent", Name);
        return false;
    }

    if (ParentUnsafe != nullptr) {
        auto it = ParentUnsafe->Children.find(this);
        if (it != ParentUnsafe->Children.end()) {
            ParentUnsafe->Children.erase(it);
        } else return false;
    }
    if (inst != nullptr)
        inst->Children.insert(this);
    ParentUnsafe = inst;
    return true;
}

std::string Instance::GetFullName(const std::string &separator) {
    std::string fullName = Name;
    Instance* at = GetParent();

    while (at != nullptr) {
        fullName = at->Name + separator + fullName;
        at = at->GetParent();
    }

    return fullName;
}