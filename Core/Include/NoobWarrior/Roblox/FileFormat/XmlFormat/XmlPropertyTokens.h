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
// File: XmlPropertyTokens.h
// Description: Registry mapping an XML element name to its token.
#pragma once

#include <NoobWarrior/Roblox/FileFormat/Tokens/Axes.h>
#include <NoobWarrior/Roblox/FileFormat/Tokens/BinaryString.h>
#include <NoobWarrior/Roblox/FileFormat/Tokens/Boolean.h>
#include <NoobWarrior/Roblox/FileFormat/Tokens/BrickColor.h>
#include <NoobWarrior/Roblox/FileFormat/Tokens/CFrame.h>
#include <NoobWarrior/Roblox/FileFormat/Tokens/Color3.h>
#include <NoobWarrior/Roblox/FileFormat/Tokens/Color3uint8.h>
#include <NoobWarrior/Roblox/FileFormat/Tokens/ColorSequence.h>
#include <NoobWarrior/Roblox/FileFormat/Tokens/Content.h>
#include <NoobWarrior/Roblox/FileFormat/Tokens/ContentId.h>
#include <NoobWarrior/Roblox/FileFormat/Tokens/Double.h>
#include <NoobWarrior/Roblox/FileFormat/Tokens/Enum.h>
#include <NoobWarrior/Roblox/FileFormat/Tokens/Faces.h>
#include <NoobWarrior/Roblox/FileFormat/Tokens/Float.h>
#include <NoobWarrior/Roblox/FileFormat/Tokens/Font.h>
#include <NoobWarrior/Roblox/FileFormat/Tokens/Int.h>
#include <NoobWarrior/Roblox/FileFormat/Tokens/Int64.h>
#include <NoobWarrior/Roblox/FileFormat/Tokens/NumberRange.h>
#include <NoobWarrior/Roblox/FileFormat/Tokens/NumberSequence.h>
#include <NoobWarrior/Roblox/FileFormat/Tokens/OptionalCFrame.h>
#include <NoobWarrior/Roblox/FileFormat/Tokens/PhysicalProperties.h>
#include <NoobWarrior/Roblox/FileFormat/Tokens/ProtectedString.h>
#include <NoobWarrior/Roblox/FileFormat/Tokens/Ray.h>
#include <NoobWarrior/Roblox/FileFormat/Tokens/Rect.h>
#include <NoobWarrior/Roblox/FileFormat/Tokens/Ref.h>
#include <NoobWarrior/Roblox/FileFormat/Tokens/SecurityCapabilities.h>
#include <NoobWarrior/Roblox/FileFormat/Tokens/SharedString.h>
#include <NoobWarrior/Roblox/FileFormat/Tokens/String.h>
#include <NoobWarrior/Roblox/FileFormat/Tokens/UDim.h>
#include <NoobWarrior/Roblox/FileFormat/Tokens/UDim2.h>
#include <NoobWarrior/Roblox/FileFormat/Tokens/UniqueId.h>
#include <NoobWarrior/Roblox/FileFormat/Tokens/Vector2.h>
#include <NoobWarrior/Roblox/FileFormat/Tokens/Vector3.h>
#include <NoobWarrior/Roblox/FileFormat/Tokens/Vector3int16.h>

#include <string_view>
#include <vector>

namespace NoobWarrior::Roblox::XmlFormat {
// The token classes live in RobloxFiles.Tokens; the registry that indexes them lives here.
using namespace Tokens;

// CFrame owns two element names, so lookup is by the element the file actually uses rather than
// by a one-to-one map from PropertyType.
inline const IXmlPropertyToken *FindToken(std::string_view xmlToken) {
    static const AxesToken kAxes;
    static const BinaryStringToken kBinaryString;
    static const BooleanToken kBoolean;
    static const BrickColorToken kBrickColor;
    static const CFrameToken kCFrame;
    static const Color3Token kColor3;
    static const Color3uint8Token kColor3uint8;
    static const ColorSequenceToken kColorSequence;
    static const ContentToken kContent;
    static const ContentIdToken kContentId;
    static const DoubleToken kDouble;
    static const EnumToken kEnum;
    static const FacesToken kFaces;
    static const FloatToken kFloat;
    static const FontToken kFont;
    static const IntToken kInt;
    static const Int64Token kInt64;
    static const NumberRangeToken kNumberRange;
    static const NumberSequenceToken kNumberSequence;
    static const OptionalCFrameToken kOptionalCFrame;
    static const PhysicalPropertiesToken kPhysicalProperties;
    static const ProtectedStringToken kProtectedString;
    static const RayToken kRay;
    static const RectToken kRect;
    static const RefToken kRef;
    static const SecurityCapabilitiesToken kSecurityCapabilities;
    static const SharedStringToken kSharedString;
    static const StringToken kString;
    static const UDimToken kUDim;
    static const UDim2Token kUDim2;
    static const UniqueIdToken kUniqueId;
    static const Vector2Token kVector2;
    static const Vector3Token kVector3;
    static const Vector3int16Token kVector3int16;

    static const std::vector<const IXmlPropertyToken *> kTokens = {
        &kAxes,
        &kBinaryString,
        &kBoolean,
        &kBrickColor,
        &kCFrame,
        &kColor3,
        &kColor3uint8,
        &kColorSequence,
        &kContent,
        &kContentId,
        &kDouble,
        &kEnum,
        &kFaces,
        &kFloat,
        &kFont,
        &kInt,
        &kInt64,
        &kNumberRange,
        &kNumberSequence,
        &kOptionalCFrame,
        &kPhysicalProperties,
        &kProtectedString,
        &kRay,
        &kRect,
        &kRef,
        &kSecurityCapabilities,
        &kSharedString,
        &kString,
        &kUDim,
        &kUDim2,
        &kUniqueId,
        &kVector2,
        &kVector3,
        &kVector3int16,
    };

    for (const IXmlPropertyToken *token : kTokens) {
        if (token->XmlPropertyToken() == xmlToken)
            return token;
    }
    // CoordinateFrame is the modern spelling; older files write CFrame.
    if (xmlToken == "CFrame")
        return &kCFrame;
    return nullptr;
}
}
