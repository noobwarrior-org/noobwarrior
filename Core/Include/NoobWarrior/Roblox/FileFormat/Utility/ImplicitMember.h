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
// File: ImplicitMember.h
// Started by: Hattozo
// Started on: 8/19/2026
// Description: This file is derived from Roblox-File-Format (https://github.com/MaximumADHD/Roblox-File-Format/blob/main/Utility/ImplicitMember.cs)
#pragma once
#include <string_view>

#ifdef NOOBWARRIOR_HAVE_GENERATED_ROBLOX_API
#include <NoobWarrior/Roblox/FileFormat/Generated/Registry.h>
#endif

// ImplicitMember.cs exists only to paper over a C# split: a serialized member of a generated
// class is a *field*, a scripting alias is a *property*, and reflection needs two different
// calls to find them. Its callers never want the member itself -- they want one fact about it,
// almost always "what type does class X declare for member Y" (Tokens/Content.cs:20-24,
// Tokens/Int.cs:21-24, Tokens/Enum.cs:23-30, BinaryFormat/Chunks/PROP.cs:126 and :439).
//
// The generated registry answers that directly, so this is a lookup facade rather than a member
// wrapper: there is nothing to hold, and PropertyDescriptor already carries the type name. The
// field/property split survives as CanSave -- the generator emits an accessor and a saved value
// only for CanSave properties, so CanSave=true is a C# field and CanSave=false is a C# property
// (BasePart.BrickColor, HumanoidDescription.Head, GuiObject.BackgroundColor are all CanSave=false).
//
// Reference calls that read or write a value through the member (Tree/Property.cs:225 and :252)
// have no analogue here: a port Property stores its own std::any, it is not a view onto a field.
namespace NoobWarrior::Roblox::Utility {
class ImplicitMember {
public:
#ifdef NOOBWARRIOR_HAVE_GENERATED_ROBLOX_API
    // Null when the dump describes neither the class nor the member, mirroring
    // ImplicitMember.Get returning null. Resolution walks Superclass, so Part.Anchored answers
    // through BasePart the way FlattenHierarchy does in the reference.
    static const PropertyDescriptor *Get(std::string_view className, std::string_view memberName) {
        return ::NoobWarrior::Roblox::FindProperty(className, memberName);
    }
#endif

    // The declared type name, e.g. "BrickColor", "ContentId", "SurfaceType". Empty when the
    // member is unknown -- the stand-in for ImplicitMember.MemberType on a null member, which
    // every reference caller treats as "no opinion" and falls back to its generic path.
    static std::string_view MemberType(std::string_view className, std::string_view memberName) {
#ifdef NOOBWARRIOR_HAVE_GENERATED_ROBLOX_API
        if (const PropertyDescriptor *descriptor = Get(className, memberName))
            return descriptor->Type;
#else
        (void)className;
        (void)memberName;
#endif
        return {};
    }

    static bool Exists(std::string_view className, std::string_view memberName) {
        return !MemberType(className, memberName).empty();
    }

    // True for a member the engine actually serializes -- the equivalent of the reference's
    // Type.GetField, which is what Content.cs:47 and Int.cs:21 use on their write/read paths
    // precisely to exclude aliases.
    static bool IsSerializedMember(std::string_view className, std::string_view memberName) {
#ifdef NOOBWARRIOR_HAVE_GENERATED_ROBLOX_API
        if (const PropertyDescriptor *descriptor = Get(className, memberName))
            return descriptor->CanSave;
#else
        (void)className;
        (void)memberName;
#endif
        return false;
    }

    // True for a name the dump knows but the engine never writes: a scripting-only spelling of
    // some other member (BasePart.BrickColor over Color3uint8, HumanoidDescription.Head over a
    // BodyPartDescription child). An old file can still carry one, because the property was
    // genuinely serialized before the engine replaced it -- so "alias" means "not saved by the
    // engine the dump came from", never "cannot appear in a file".
    static bool IsScriptingAlias(std::string_view className, std::string_view memberName) {
        return Exists(className, memberName) && !IsSerializedMember(className, memberName);
    }
};
} // namespace NoobWarrior::Roblox::Utility
