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
// File: RbxObject.h
// Started by: Hattozo
// Started on: 11/3/2025
// Description: This file is derived from Roblox-File-Format (https://github.com/MaximumADHD/Roblox-File-Format/blob/main/Tree/RbxObject.cs)
#pragma once
#include <NoobWarrior/Roblox/FileFormat/Tree/Property.h>

#include <string>
#include <map>

namespace NoobWarrior::Roblox {
class RbxObject {
public:
    RbxObject();

    /**
     * @brief The ClassName of this Instance.
     */
    std::string ClassName;

    /**
     * @brief A context-dependent unique identifier for this instance when being serialized.
     */
    std::string Referent;

    std::map<std::string, Property> &GetProperties();

    bool Destroyed;

    template<typename T>
    bool IsA() {
        return dynamic_cast<T*>(this) != nullptr;
    }

    template<typename T>
    T CastAs() {
        return dynamic_cast<T*>(this);
    }

    virtual void Destroy() {
        props.clear();
    }

    Property* GetProperty(const std::string &name) {
        if (props.contains(name))
            return &props[name];
        return nullptr;
    }

    void AddProperty(Property prop) {
        std::string name = prop.Name;
        RemoveProperty(name);

        prop.Object = this;
        props[name] = prop;
    }

    bool RemoveProperty(const std::string &name) {
        if (props.contains(name)) {
            Property prop = props[name];
            prop.Object = nullptr;
        }
        auto it = props.find(name);
        if (it != props.end())
            props.erase(it);
        return it != props.end();
    }
protected:
    std::map<std::string, Property> props;
};
}