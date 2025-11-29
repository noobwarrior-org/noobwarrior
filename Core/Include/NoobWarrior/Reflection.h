// === noobWarrior ===
// File: Reflection.h
// Started by: Hattozo
// Started on: 9/19/2025
// Description: Anything that requires runtime to dynamically understand the structure of data
// without having to manually specify it everywhere should go here
//
// ID types for example; you don't want to add checks in 30 different code paths because you added a new ID type.
// Just declare it here and those code paths will automatically discover it.
//
// Note: If you are adding a new ID type, you will still have to manually add glue code between the SQL database schema and
// the actual C structs. Check the Database.cpp & Database.h file in order to see how we did this.
#pragma once
#include "Database/Item/Item.h"
#include "Database/Item/Asset.h"
#include "Database/Common.h"

#include <functional>
#include <map>
#include <string>
#include <typeindex>
#include <any>
#include <optional>
#include <memory>

#include <entt/entt.hpp>

namespace NoobWarrior::Reflection {
inline const std::vector<entt::meta_type> GetItemTypes() {
    std::vector<entt::meta_type> ItemTypes;
    ItemTypes.push_back(entt::resolve<Asset>());
    return ItemTypes;
}

inline const std::vector<entt::meta_type> GetEnums() {
    std::vector<entt::meta_type> Enums;
    Enums.push_back(entt::resolve<Roblox::AssetType>());
    return Enums;
}

void Register();
}