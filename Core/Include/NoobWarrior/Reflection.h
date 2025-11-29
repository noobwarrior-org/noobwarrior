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

#define NOOBWARRIOR_REFLECT_ITEMTYPE_BEGIN(itemType) \
    struct itemType##Registrar { \
        itemType##Registrar() { \
            auto itemTypePtr = std::make_shared<NoobWarrior::Reflection::ItemType>(); \
            itemTypePtr->Name = #itemType; \
            itemTypePtr->Class = &typeid(itemType); \
            itemTypePtr->Create = []() { return itemType(); }; \
            NoobWarrior::Reflection::GetItemTypesInternal().push_back(itemTypePtr); \
            NoobWarrior::Reflection::GetItemTypeMap()[typeid(itemType)] = NoobWarrior::Reflection::GetItemTypesInternal().back(); \
            itemType itemTypeClass;

#define NOOBWARRIOR_REFLECT_ITEMTYPE_END(itemType) \
        } \
    }; \
    static itemType##Registrar s##itemType##RegistrarInstance;

/*
#define NOOBWARRIOR_REFLECT_FIELD(fieldName, prettyName, desc, getterName, setterName) \
    itemTypePtr->Fields[#fieldName] = NoobWarrior::Reflection::Field { \
        .Name = #fieldName, \
        .PrettyName = prettyName, \
        .Description = desc \
        .Getter = [](NoobWarrior::Item *item) -> std::any { \
            return std::any(dynamic_cast<decltype(itemTypeClass)*>(item)->getterName()); \
        } \
        .Setter = [](NoobWarrior::Item *item, const std::any &val) -> void { \
            dynamic_cast<decltype(itemTypeClass)*>(item)->setterName(std::any_cast<decltype(dynamic_cast<decltype(sClass)*>(item)->getterName())>(val)); \
        } \
    };
*/

#define NOOBWARRIOR_REFLECT_PROPERTY(fieldName, prettyName, desc) \
    itemTypePtr->Fields[#fieldName] = NoobWarrior::Reflection::Field { \
        .Name = #fieldName, \
        .PrettyName = prettyName, \
        .Description = desc, \
        .Getter = [](NoobWarrior::Item *item) -> std::any { \
            return std::any(dynamic_cast<decltype(itemTypeClass)*>(item)->fieldName); \
        }, \
        .Setter = [](NoobWarrior::Item *item, const std::any &val) -> bool { \
            try { \
                dynamic_cast<decltype(itemTypeClass)*>(item)->fieldName = std::any_cast<decltype(dynamic_cast<decltype(itemTypeClass)*>(item)->fieldName)>(val); \
                return true; \
            } catch (std::exception &ex) { \
                return false; \
            } \
        } \
    };

#define NOOBWARRIOR_REFLECT_DEFAULT_IMAGE(data, dataSize) \
    itemTypePtr->DefaultImage = data; \
    itemTypePtr->DefaultImageSize = dataSize;

#define NOOBWARRIOR_REFLECT_ENUM_BEGIN(enumName) \
    struct enumName##Registrar { \
        enumName##Registrar() { \
            auto enumeration = std::make_shared<NoobWarrior::Reflection::Enum>(); \
            enumeration->Name = #enumName;

#define NOOBWARRIOR_REFLECT_ENUM_END(enumName) \
            NoobWarrior::Reflection::GetEnumsInternal().push_back(enumeration); \
            NoobWarrior::Reflection::GetEnumMap()[typeid(enumName)] = NoobWarrior::Reflection::GetEnumsInternal().back(); \
        } \
    }; \
    static enumName##Registrar s##enumName##RegistrarInstance;

#define NOOBWARRIOR_REFLECT_ENUM_VALUE(enumName, enumVal) \
    enumeration->Values[#enumName] = static_cast<int>(enumVal);

#define NOOBWARRIOR_REFLECT_COMMON_BEGIN \
    struct CommonRegistrar { \
        CommonRegistrar() {

#define NOOBWARRIOR_REFLECT_COMMON_END \
        } \
    }; \
    static CommonRegistrar sCommonRegistrarInstance;

#define NOOBWARRIOR_REFLECT_COMMON_FIELD(fieldName, tableName, prettyName, datatype, desc, getDefaultVal) \
    for (auto itemTypePtr : NoobWarrior::Reflection::GetItemTypesInternal()) \
        itemTypePtr->Fields[#fieldName] = NoobWarrior::Reflection::Field { .Name = #fieldName, .TableName = #tableName, .PrettyName = prettyName, .Description = desc, .Type = &typeid(datatype), .GetDefaultValue = getDefaultVal };

namespace NoobWarrior {
class Database;
enum class DatabaseResponse;
}
namespace NoobWarrior::Reflection {
struct Field {
    std::string Name;
    std::string PrettyName;
    std::string Description;
    std::function<std::any(Item*)> Getter;
    std::function<bool(Item*, const std::any&)> Setter;
};

struct ItemType {
    std::string Name {};
    const std::type_info *Class { nullptr };
    std::function<Item()> Create;
    std::map<std::string, Field> Fields;
    int* DefaultImage = nullptr;
    int DefaultImageSize {};
};

struct Enum {
    std::string Name;
    std::map<std::string, int> Values;
};

/**
 * @brief Raw version of GetItemTypes() meant only for internal use. The difference is that it contains a vector of shared pointers.
 * This is so that the memory locations of each reflected ItemType will always stay the same, even if the vector reallocates elements to different memory locations.
 * @return std::vector<std::shared_ptr<ItemType>>& 
 */
inline std::vector<std::shared_ptr<ItemType>> &GetItemTypesInternal() {
    static std::vector<std::shared_ptr<ItemType>> ItemTypes;
    return ItemTypes;
}

/**
 * @brief A vector that consists of each available ItemType. 
 * 
 * @return std::vector<ItemType>& 
 */
inline const std::vector<entt::meta_type> GetItemTypes() {
    std::vector<entt::meta_type> ItemTypes;
    ItemTypes.push_back(entt::resolve<Asset>());
    return ItemTypes;
}

inline std::unordered_map<std::type_index, std::shared_ptr<ItemType>> &GetItemTypeMap() {
    static std::unordered_map<std::type_index, std::shared_ptr<ItemType>> ItemTypeNameMap;
    return ItemTypeNameMap;
}

template<typename T>
const ItemType &GetItemType() {
    return const_cast<const ItemType&>(*GetItemTypeMap()[std::type_index(typeid(T))].get());
}

template<typename T>
std::string GetItemTypeName() {
    return GetItemType<T>().Name;
}

template<typename T>
std::map<std::string, Field> &GetFields() {
    ItemType &reflectedType = GetItemType<T>();
    return reflectedType.Fields;
}

inline std::vector<std::shared_ptr<Enum>> &GetEnumsInternal() {
    static std::vector<std::shared_ptr<Enum>> Enums;
    return Enums;
}

inline const std::vector<Enum> GetEnums() {
    std::vector<Enum> Enums;
    auto enumsRaw = GetEnumsInternal();
    for (auto &enumeration : enumsRaw) {
        Enums.push_back(*enumeration.get());
    }
    return Enums;
}

inline std::unordered_map<std::type_index, std::shared_ptr<Enum>> &GetEnumMap() {
    static std::unordered_map<std::type_index, std::shared_ptr<Enum>> EnumMap;
    return EnumMap;
}

template<typename T>
Enum &GetEnum() {
    return *GetEnumMap()[std::type_index(typeid(T))].get();
}

template<typename T>
std::string GetEnumName() {
    return GetEnum<T>().Name;
}

void hi();
void Register();
}