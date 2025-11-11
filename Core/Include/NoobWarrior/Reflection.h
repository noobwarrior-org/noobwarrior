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
#include "Database/Record/IdRecord.h"

#include <functional>
#include <map>
#include <string>
#include <typeindex>
#include <any>
#include <optional>
#include <memory>

#define NOOBWARRIOR_REFLECT_ID_TYPE_BEGIN(idType) \
    struct idType##Registrar { \
        idType##Registrar() { \
            auto idTypePtr = std::make_shared<NoobWarrior::Reflection::IdType>(); \
            idTypePtr->Name = #idType; \
            idTypePtr->Class = &typeid(idType); \
            idTypePtr->Create = []() { return idType(); }; \
            NoobWarrior::Reflection::GetIdTypesInternal().push_back(idTypePtr); \
            NoobWarrior::Reflection::GetIdTypeMap()[typeid(idType)] = NoobWarrior::Reflection::GetIdTypesInternal().back();

#define NOOBWARRIOR_REFLECT_ID_TYPE_END(idType) \
        } \
    }; \
    static idType##Registrar s##idType##RegistrarInstance;

#define NOOBWARRIOR_REFLECT_FIELD(fieldName, tableName, prettyName, datatype, desc, getDefaultVal) \
    idTypePtr->Fields[#fieldName] = NoobWarrior::Reflection::Field { .Name = #fieldName, .TableName = #tableName, .PrettyName = prettyName, .Description = desc, .Type = &typeid(datatype), .GetDefaultValue = getDefaultVal };

#define NOOBWARRIOR_REFLECT_DEFAULT_IMAGE(data, dataSize) \
    idTypePtr->DefaultImage = data; \
    idTypePtr->DefaultImageSize = dataSize;

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
    for (auto idTypePtr : NoobWarrior::Reflection::GetIdTypesInternal()) \
        idTypePtr->Fields[#fieldName] = NoobWarrior::Reflection::Field { .Name = #fieldName, .TableName = #tableName, .PrettyName = prettyName, .Description = desc, .Type = &typeid(datatype), .GetDefaultValue = getDefaultVal };

namespace NoobWarrior {
class Database;
enum class DatabaseResponse;
}
namespace NoobWarrior::Reflection {
struct Field {
    std::string Name;
    std::string TableName;
    std::string PrettyName;
    std::string Description;
    const std::type_info* Type { nullptr };
    std::function<std::any(Database *db)> GetDefaultValue;
};

struct IdType {
    std::string Name {};
    const std::type_info *Class { nullptr };
    std::function<IdRecord()> Create;
    std::map<std::string, Field> Fields;
    int* DefaultImage = nullptr;
    int DefaultImageSize {};
};

struct Enum {
    std::string Name;
    std::map<std::string, int> Values;
};

/**
 * @brief Raw version of GetIdTypes() meant only for internal use. The difference is that it contains a vector of shared pointers.
 * This is so that the memory locations of each reflected IdType will always stay the same, even if the vector reallocates elements to different memory locations.
 * @return std::vector<std::shared_ptr<IdType>>& 
 */
inline std::vector<std::shared_ptr<IdType>> &GetIdTypesInternal() {
    static std::vector<std::shared_ptr<IdType>> IdTypes;
    return IdTypes;
}

/**
 * @brief A vector that consists of each available IdType. 
 * 
 * @return std::vector<IdType>& 
 */
inline const std::vector<IdType> GetIdTypes() {
    std::vector<IdType> IdTypes;
    auto idTypesRaw = GetIdTypesInternal();
    for (auto &idtype : idTypesRaw) {
        IdTypes.push_back(*idtype.get());
    }
    return IdTypes;
}

inline std::unordered_map<std::type_index, std::shared_ptr<IdType>> &GetIdTypeMap() {
    static std::unordered_map<std::type_index, std::shared_ptr<IdType>> IdTypeNameMap;
    return IdTypeNameMap;
}

template<typename T>
const IdType &GetIdType() {
    return const_cast<const IdType&>(*GetIdTypeMap()[std::type_index(typeid(T))].get());
}

template<typename T>
std::string GetIdTypeName() {
    return GetIdType<T>().Name;
}

template<typename T>
std::map<std::string, Field> &GetFields() {
    IdType &reflectedType = GetIdType<T>();
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
}