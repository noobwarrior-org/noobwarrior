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

#include <map>
#include <string>
#include <string_view>
#include <utility>

namespace NoobWarrior::Roblox {
class RbxObject {
public:
    RbxObject();
    virtual ~RbxObject() = default;

    /**
     * @brief The ClassName of this Instance.
     */
    std::string ClassName;

    /**
     * @brief A context-dependent unique identifier for this instance when being serialized.
     */
    std::string Referent;

    std::map<std::string, Property> &GetProperties();
    const std::map<std::string, Property> &GetProperties() const;

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
    /**
     * @brief Whether this object carries a property under this name.
     */
    bool HasProperty(const std::string &name) const {
        return props.contains(name);
    }

    /**
     * @brief The decoded value of a property, or @p fallback when it is absent or holds another
     * type. Generated classes wrap this so callers can name a property at compile time.
     */
    template<typename T>
    T GetPropertyValue(const std::string &name, T fallback = T{}) const {
        const auto found = props.find(name);
        if (found == props.end())
            return fallback;
        if (const T *value = std::any_cast<T>(&found->second.Value))
            return *value;
        return fallback;
    }

    /**
     * @brief Sets a property, creating it when absent.
     *
     * The XML token has to be supplied because the XML writer falls back to "string" for a
     * property that carries none, which would mis-serialize anything else. Generated setters pass
     * the token their declared type maps to.
     */
    template<typename T>
    void SetPropertyValue(const std::string &name, PropertyType type,
                          std::string_view xmlToken, T value) {
        Property &slot = props[name];
        slot.Name = name;
        slot.Object = this;
        slot.Type = type;
        slot.XmlToken = xmlToken;
        // The raw buffer describes the value being replaced -- a CFrame's orientation id, a
        // PhysicalProperties flag byte -- so keeping it would make the writer emit the encoding
        // of something that is no longer there.
        slot.RawBuffer.clear();
        slot.Value = std::move(value);
    }

protected:
    std::map<std::string, Property> props;
};
}
