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
// File: PluginTreeMaterializer.h
// Started by: Hattozo
// Started on: 8/18/2026
// Description: Writes a plugin's datamodel tree straight into a place
#pragma once

#include <NoobWarrior/Roblox/FileFormat/RobloxFile.h>
#include <NoobWarrior/Roblox/FileFormat/DataTypes/DataTypes.h>

#include <nlohmann/json.hpp>

#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace NoobWarrior {
// The one service-name list this pipeline keeps. There were three of them: this one, a copy in
// PluginDataModel.cpp's plan builder, and a third in BinaryRobloxFile.cpp:741-753 that this
// header cannot reach -- that one should be pointed here as well.
bool IsDataModelServiceClass(std::string_view className);

// Plugins mount over one another like Source game content: they are applied in load order and the
// last one to define an instance wins.
//
// Folders and services merge instead of replacing, so several plugins can contribute into the same
// container. Anything else is replaced outright -- a half-overwritten Model that kept the previous
// plugin's children is worse than either version, and "later wins" is only predictable if total.
// Instances are matched by Name alone, matching Rojo.
struct PluginMountResult {
    int Created {};
    int Merged {};
    int Replaced {};
    std::vector<std::string> Warnings;
};

class PluginTreeMaterializer {
public:
    explicit PluginTreeMaterializer(Roblox::RobloxFile *place);

    // plan is the node array produced by the Rojo directory walker.
    bool Mount(const nlohmann::json &plan, PluginMountResult &result, std::string *error = nullptr);

    // Every instance this materializer created, plus every instance it merged plugin properties
    // into. A merged instance came out of the place file, so BinaryRobloxFile::BuildTables counts
    // everything on it as written by the file -- its test is referent < mLoadedObjectCount --
    // including a property the plugin only just added, which then becomes a column that has to
    // carry a stand-in default on every other instance of the class. The writer has to read
    // ownership from here rather than infer it from referent order.
    const std::unordered_set<const Roblox::Instance *> &MountedInstances() const {
        return mMounted;
    }

private:
    Roblox::Instance *MountNode(const nlohmann::json &node, Roblox::Instance *parent,
                                PluginMountResult &result, std::string *error);
    // strict is set for properties the project file asked for by name, which must apply or
    // fail the mount. Properties harvested out of a .rbxmx are best-effort and only warn.
    bool ApplyProperties(Roblox::Instance &instance, const nlohmann::json &properties,
                         bool strict,
                         PluginMountResult &result, std::string *error);

    // Ref values name a plan referent, which only becomes a real instance once the whole plan
    // has been mounted, so they are recorded and resolved in a second pass.
    struct PendingReference {
        Roblox::Instance *Owner;
        std::string PropertyName;
        std::string Target;
    };
    void ResolveReferences(PluginMountResult &result);
    // Mounts every "children" entry of node beneath parent. Returns false only when a child
    // failed hard enough to abort the whole mount.
    bool MountChildren(const nlohmann::json &node, Roblox::Instance *parent,
                       PluginMountResult &result, std::string *error);
    // DestroySubtree frees the instances a replaced node owned, so every map still pointing at
    // one has to let go first. Two plan nodes can carry the same name within a single plan -- a
    // Foo.rbxmx and a Foo.txt in one directory both emit a node called Foo, and the .rbxmx sorts
    // first, registers its referents and is then destroyed by the .txt.
    void ForgetSubtree(Roblox::Instance *instance);

    Roblox::RobloxFile *mPlace;
    // True when the place stores referents as integers rather than strings.
    bool mBinaryReferents {};
    std::unordered_map<std::string, Roblox::Instance *> mReferents;
    std::vector<PendingReference> mPending;
    std::unordered_set<const Roblox::Instance *> mMounted;
};

// Exposed for testing: converts one tagged plan value into a typed property value. Returns false
// when the shape is not one the plan builder emits.
//
// className/propertyName are optional and only decide the width of an *untagged* JSON number,
// which carries none of its own; a tag written into the value always wins.
bool ConvertPlanValue(const nlohmann::json &value, Roblox::PropertyType &type, std::any &out,
                      std::string_view className = {}, std::string_view propertyName = {});
}
