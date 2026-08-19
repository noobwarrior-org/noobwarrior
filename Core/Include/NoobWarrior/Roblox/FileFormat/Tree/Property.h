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
// File: Property.h
// Started by: Hattozo
// Started on: 11/3/2025
// Description: This file is derived from Roblox-File-Format (https://github.com/MaximumADHD/Roblox-File-Format/blob/main/Tree/Property.cs)
#pragma once
#include <string>
#include <vector>
#include <any>
#include <cstdint>

namespace NoobWarrior::Roblox {
enum class PropertyType {
    Unknown,
    String,
    Bool,
    Int,
    Float,
    Double,
    UDim,
    UDim2,
    Ray,
    Faces,
    Axes,
    BrickColor,
    Color3,
    Vector2,
    Vector3,
    CFrame = 16,
    Quaternion,
    Enum,
    Ref,
    Vector3int16,
    NumberSequence,
    ColorSequence,
    NumberRange,
    Rect,
    PhysicalProperties,
    Color3uint8,
    Int64,
    SharedString,
    ProtectedString,
    OptionalCFrame,
    UniqueId,
    FontFace,
    SecurityCapabilities,
    Content
};

class RbxObject;
class RobloxFile;
class Property {
public:
    Property() : Object(nullptr), Type(PropertyType::Unknown), File(nullptr) {}

    enum class BindingFlags {
        Instance = 1 << 0,
        Public = 1 << 1,
        NonPublic = 1 << 2,
        FlattenHierarchy = 1 << 3,
        IgnoreCase = 1 << 4
    };

    std::string Name;
    RbxObject* Object;
    PropertyType Type;
    RobloxFile* File;

    std::string XmlToken;
    std::vector<unsigned char> RawBuffer;

    // The decoded value, typed per the DataTypes layer. std::any is the direct analogue of
    // RobloxFiles.Property.Value, which is a bare object.
    std::any Value;

    // Returns nullptr when the value is absent or holds a different type, so callers branch
    // rather than handling an exception.
    template<typename T>
    const T *CastValue() const {
        return std::any_cast<T>(&Value);
    }

    template<typename T>
    T *CastValue() {
        return std::any_cast<T>(&Value);
    }

    bool HasValue() const {
        return Value.has_value();
    }

    bool HasRawBuffer() const {
        return !RawBuffer.empty();
    }
};
}
