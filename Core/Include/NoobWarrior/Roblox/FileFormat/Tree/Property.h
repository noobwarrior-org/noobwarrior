// === noobWarrior ===
// File: Property.h
// Started by: Hattozo
// Started on: 11/3/2025
// Description: This file is derived from Roblox-File-Format (https://github.com/MaximumADHD/Roblox-File-Format/blob/main/Tree/Property.cs)
#pragma once
#include <string>
#include <vector>
#include <any>

namespace NoobWarrior::Roblox {
enum PropertyType {
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
protected:
    void* RawValue { nullptr };
};
}