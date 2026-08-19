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
// File: PluginDataModel.h
// Started by: Hattozo
// Started on: 8/15/2026
// Description: Compiles a Rojo-shaped plugin directory into a Studio bootstrap script.
#pragma once

#include <NoobWarrior/FileSystem/VirtualFileSystem.h>
#include <NoobWarrior/Roblox/FileFormat/Tree/Property.h>

#include <nlohmann/json.hpp>

#include <optional>
#include <string>
#include <vector>

namespace NoobWarrior {
struct StudioServerScript {
    std::string Key;
    std::string ClassName;
    std::string Source;
    std::string ParentClassName {"ServerScriptService"};
    bool Disabled {true};
};

struct StudioServerModelPathElement {
    std::string Name;
    std::string ClassName;
};

struct StudioServerModel {
    // The RBXM roots are serialized directly beneath this Rojo path in the server
    // RBXL. SourcePath is retained for diagnostics and cache invalidation.
    std::string SourcePath;
    std::vector<StudioServerModelPathElement> ParentPath;
    std::vector<unsigned char> Data;
    std::string SingleRootName;
};

struct StudioServerBootstrap {
    // Rojo plans in mount order, materialized straight into the place by
    // PluginTreeMaterializer. Nothing rebuilds the world at run time any more.
    std::vector<std::string> Plans;
    std::vector<StudioServerScript> Scripts;
    std::vector<StudioServerModel> Models;

    bool Empty() const {
        return Plans.empty() && Scripts.empty() && Models.empty();
    }
};

class PluginDataModel {
public:
    PluginDataModel(VirtualFileSystem *vfs, std::string pluginIdentifier);

    StudioServerBootstrap BuildBootstrap(const std::string &rootDirectory);

private:
    VirtualFileSystem *mVfs;
    std::string mPluginIdentifier;
};

// Exposed for testing
std::optional<nlohmann::json> XmlPropertyValue(const Roblox::Property &property,
                                               const std::string &referentNamespace,
                                               std::string &propertyName,
                                               std::string *unsupportedToken = nullptr);
}
