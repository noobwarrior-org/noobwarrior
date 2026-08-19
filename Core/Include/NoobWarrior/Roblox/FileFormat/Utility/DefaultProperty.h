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
// File: DefaultProperty.h
// Started by: Hattozo
// Started on: 8/19/2026
// Description: This file is derived from Roblox-File-Format (https://github.com/MaximumADHD/Roblox-File-Format/blob/main/Utility/DefaultProperty.cs)
#pragma once
#include <NoobWarrior/Roblox/FileFormat/Tree/Property.h>
#include <NoobWarrior/Roblox/FileFormat/Tree/RbxObject.h>

#include <any>
#include <optional>
#include <string_view>

#ifdef NOOBWARRIOR_HAVE_GENERATED_ROBLOX_API
#include <NoobWarrior/Roblox/FileFormat/Generated/Registry.h>
#endif

namespace NoobWarrior::Roblox::Utility {
class DefaultProperty {
public:
    // Empty when the dump describes neither the class nor the property, and also when it
    // describes the property but records no default -- the reference collapses both onto null,
    // and so does every caller. Callers that must write *something* pick their own zero; that
    // choice is type-driven and stays with the writer.
    static std::optional<std::any> Get(std::string_view className, std::string_view propertyName) {
#ifdef NOOBWARRIOR_HAVE_GENERATED_ROBLOX_API
        if (const PropertyDescriptor *declared =
                ::NoobWarrior::Roblox::FindProperty(className, propertyName)) {
            if (declared->Default != nullptr)
                return declared->Default();
        }
#else
        (void)className;
        (void)propertyName;
#endif
        return std::nullopt;
    }

    static std::optional<std::any> Get(const RbxObject &object, std::string_view propertyName) {
        return Get(object.ClassName, propertyName);
    }

    static std::optional<std::any> Get(const RbxObject &object, const Property &property) {
        return Get(object.ClassName, property.Name);
    }

    static std::optional<std::any> Get(std::string_view className, const Property &property) {
        return Get(className, property.Name);
    }

    // Typed convenience the reference cannot express, since its Get returns a bare object.
    // Empty when there is no default, or when the default is not a T -- the dump stores an enum
    // default as uint32_t and a BrickColor default as int32_t, so asking for the wrong type is a
    // real mistake rather than a theoretical one.
    template<typename T>
    static std::optional<T> GetAs(std::string_view className, std::string_view propertyName) {
        std::optional<std::any> value = Get(className, propertyName);
        if (!value.has_value())
            return std::nullopt;
        if (const T *typed = std::any_cast<T>(&*value))
            return *typed;
        return std::nullopt;
    }

    template<typename T>
    static std::optional<T> GetAs(const RbxObject &object, std::string_view propertyName) {
        return GetAs<T>(object.ClassName, propertyName);
    }
};
} // namespace NoobWarrior::Roblox::Utility
