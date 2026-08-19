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
// File: Content.h
// Started by: Hattozo
// Started on: 8/19/2026
// Description: Ported from RobloxFiles.DataTypes.Content (MaximumADHD/Roblox-File-Format).
#pragma once

#include <cstdint>
#include <string>
#include <utility>

namespace NoobWarrior::Roblox::DataTypes {
// Mirrors Enum.ContentSourceType. Object and ExternalObject carry a referent rather than a URI.
enum class ContentSourceType : int32_t {
    None = 0,
    Uri = 1,
    Object = 2,
    ExternalObject = 3,
};

struct Content {
    ContentSourceType SourceType {ContentSourceType::None};
    std::string Uri;
    int32_t ObjectReferent {-1};
    // Content.cs:14 (`internal readonly string RefId`). The two file formats name the same target
    // in incompatible ways: a binary PROP column stores an int32 index into the object table
    // (read by ReadReferents), while XML stores referent *text* such as "RBX1A2B3C", which has no
    // numeric reading at all. Only one of the two is ever populated, decided by the format the
    // value came from -- reading the XML spelling as an int32 yields 0, which silently aliases
    // the file's first instance.
    std::string RefId;

    Content() = default;
    explicit Content(std::string uri) :
        SourceType(ContentSourceType::Uri), Uri(std::move(uri)) {}

    // Content.cs:47-51: the XML reader only ever knows the referent text. The instance it names
    // is matched up afterwards, by the document's reference-resolution pass.
    static Content FromRefId(std::string refId) {
        Content content;
        content.SourceType = ContentSourceType::Object;
        content.RefId = std::move(refId);
        return content;
    }

    // True while this value points at an instance named only by XML referent text, i.e. while it
    // is still waiting on the resolution pass.
    bool HasRefId() const {
        return SourceType == ContentSourceType::Object && !RefId.empty();
    }

    // XmlRobloxFile.cs:105 nulls a Ref property whose referent names nothing in the document.
    // Content.None (Content.cs:10) is the same "points at nothing" value for this type, so an
    // unresolvable Content collapses to it rather than keeping a referent nobody can follow.
    void ClearReferent() {
        SourceType = ContentSourceType::None;
        ObjectReferent = -1;
        RefId.clear();
    }

    friend bool operator==(const Content &, const Content &) = default;
};
}
