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

#include <algorithm>

using namespace NoobWarrior::Roblox;

Instance::Instance() : RbxObject(), ParentLocked(false), IsService(false),
    ParentUnsafe(nullptr) {}

RbxAttributes::Response Instance::LoadAttributes(RbxAttributes &attributes) const {
    const auto found = props.find(std::string(kAttributesProperty));
    if (found == props.end()) {
        attributes.Clear();
        return RbxAttributes::Response::Empty;
    }

    // The blob is a BinaryString, which this port stores as a std::string of raw bytes. Anything
    // else under that name is not an attribute blob, and rewriting it would destroy whatever is.
    const std::string *blob = found->second.CastValue<std::string>();
    if (blob == nullptr) {
        attributes.Clear();
        return RbxAttributes::Response::Malformed;
    }

    return attributes.Load(*blob);
}

void Instance::StoreAttributes(const RbxAttributes &attributes) {
    const std::string blob = attributes.Save();
    if (blob.empty()) {
        RemoveProperty(std::string(kAttributesProperty));
        return;
    }

    SetPropertyValue<std::string>(std::string(kAttributesProperty), PropertyType::String,
                                  "BinaryString", blob);
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
        auto it = std::find(ParentUnsafe->Children.begin(), ParentUnsafe->Children.end(), this);
        if (it != ParentUnsafe->Children.end()) {
            ParentUnsafe->Children.erase(it);
        } else return false;
    }
    if (inst != nullptr)
        inst->Children.push_back(this);
    ParentUnsafe = inst;
    return true;
}

bool Instance::IsAncestorOf(Instance* descendant) {
    // Reference Tree/Instance.cs:176-186 starts the walk at the descendant itself, so an
    // instance counts as its own ancestor. That is what makes the SetParent guard above
    // reject parenting an instance to itself, and what makes IsDescendantOf agree.
    Instance* at = descendant;
    while (at != nullptr) {
        if (at == this) return true;
        at = at->GetParent();
    }
    return false;
}

bool Instance::IsDescendantOf(Instance* ancestor) {
    return ancestor != nullptr && ancestor->IsAncestorOf(this);
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

std::vector<Instance*> Instance::GetChildren() const {
    return Children;
}

std::vector<Instance*> Instance::GetDescendants() const {
    std::vector<Instance*> descendants;
    for (Instance *child : Children) {
        descendants.push_back(child);
        std::vector<Instance*> children = child->GetDescendants();
        descendants.insert(descendants.end(), children.begin(), children.end());
    }
    return descendants;
}

void Instance::SetName(const std::string &name) {
    Name = name;
    // The Name field and the serialized "Name" property are separate stores, synced only one
    // way (property -> field) when a file is read, so a rename has to write both or the save
    // puts the old name back. The "string" XML token is what XmlRobloxFile::CreateInstance
    // writes for this property.
    SetPropertyValue<std::string>("Name", PropertyType::String, "string", name);
}

void Instance::Destroy() {
    // Reference Tree/Instance.cs:447-462, with two deviations the port forces:
    //  - the reference throws out of the Parent setter when the instance is locked, which
    //    aborts Destroy; SetParent here only returns false, so the lock is checked up front.
    //  - the reference drains children with "while (Children.Any())", which terminates only
    //    because of that throw. A silently failing detach would spin forever, so this walks a
    //    snapshot of the children exactly once.
    if (Destroyed)
        return;

    if (ParentLocked) {
        Out("Instance", "The Parent property of {} is locked, it cannot be destroyed", Name);
        return;
    }

    RbxObject::Destroy();

    SetParent(nullptr);
    ParentLocked = true;
    Destroyed = true;

    // Each child detaches itself from Children as it goes, hence the by-value snapshot.
    for (Instance *child : GetChildren())
        child->Destroy();
}
