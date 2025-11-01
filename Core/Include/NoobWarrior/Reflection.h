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

#define NOOBWARRIOR_REFLECT_ID_TYPE_BEGIN(idType) \
    struct idType##Registrar { \
        idType##Registrar() { \
            NoobWarrior::Reflection::GetIdTypes().push_back({ \
                .Name = #idType, \
                .Class = &typeid(idType), \
                .Create = []() { \
                    return idType(); \
                } \
            }); \
            NoobWarrior::Reflection::GetIdTypeMap()[typeid(idType)] = NoobWarrior::Reflection::GetIdTypes().back(); \
            auto *fields = NoobWarrior::Reflection::GetFields<idType>();

#define NOOBWARRIOR_REFLECT_ID_TYPE_END(idType) \
        } \
    }; \
    static idType##Registrar s##idType##RegistrarInstance;

#define NOOBWARRIOR_REFLECT_FIELD(fieldName, datatype, desc, defaultVal, getter, setter) \
    (*fields)[#fieldName] = { .Name = #fieldName, .Description = desc, .Type = &typeid(datatype), .Getter = getter, .Setter = setter };

#define NOOBWARRIOR_REFLECT_ENUM_BEGIN(enumName) \
    struct enumName##Registrar { \
        enumName##Registrar() { \
            NoobWarrior::Reflection::Enum enumeration {}; \
            enumeration.Name = #enumName;

#define NOOBWARRIOR_REFLECT_ENUM_END(enumName) \
            NoobWarrior::Reflection::GetEnums().push_back(enumeration); \
            NoobWarrior::Reflection::GetEnumMap()[typeid(enumName)] = NoobWarrior::Reflection::GetEnums().back(); \
        } \
    }; \
    static enumName##Registrar s##enumName##RegistrarInstance;

#define NOOBWARRIOR_REFLECT_ENUM_VALUE(enumName, enumVal) \
    enumeration.Values[#enumName] = static_cast<int>(enumVal);

/*
#define NOOBWARRIOR_REFLECT_COMMON_BEGIN \
    struct CommonRegistrar { \
        CommonRegistrar() {

#define NOOBWARRIOR_REFLECT_COMMON_END \
        } \
    }; \
    static CommonRegistrar sCommonRegistrarInstance;

#define NOOBWARRIOR_REFLECT_COMMON_FIELD(fieldName, datatype, desc, getter, setter) \
    for (auto &pair : NoobWarrior::Reflection::GetIdTypes()) \
        pair.second.Fields[#fieldName] = { .Name = #fieldName, .Description = desc, .Type = &typeid(datatype), .Getter = getter, .Setter = setter };
*/

namespace NoobWarrior {
class Database;
enum class DatabaseResponse;
}
namespace NoobWarrior::Reflection {
struct Field {
    std::string Name;
    std::string Description;
    const std::type_info* Type { nullptr };
    std::function<std::any(Database *db, int64_t id, std::optional<int64_t> version)> Getter;
    std::function<DatabaseResponse(Database *db, int64_t id, std::optional<int64_t> version, std::any val)> Setter;
};

struct IdType {
    std::string Name;
    const std::type_info *Class { nullptr };
    std::function<IdRecord()> Create;
    std::unordered_map<std::string, Field> Fields;
};

struct Enum {
    std::string Name;
    std::map<std::string, int> Values;
};

inline std::vector<IdType> &GetIdTypes() {
    static std::vector<IdType> IdTypes;
    return IdTypes;
}

inline std::unordered_map<std::type_index, IdType> &GetIdTypeMap() {
    static std::unordered_map<std::type_index, IdType> IdTypeNameMap;
    return IdTypeNameMap;
}

template<typename T>
IdType &GetIdType() {
    return GetIdTypeMap()[std::type_index(typeid(T))];
}

template<typename T>
std::string GetIdTypeName() {
    return GetIdType<T>().Name;
}

template<typename T>
std::unordered_map<std::string, Field> *GetFields() {
    IdType &reflectedType = GetIdType<T>();
    return &reflectedType.Fields;
}

inline std::vector<Enum> &GetEnums() {
    static std::vector<Enum> Enums;
    return Enums;
}

inline std::unordered_map<std::type_index, Enum> &GetEnumMap() {
    static std::unordered_map<std::type_index, Enum> EnumMap;
    return EnumMap;
}

template<typename T>
Enum &GetEnum() {
    return GetEnumMap()[std::type_index(typeid(T))];
}

template<typename T>
std::string GetEnumName() {
    return GetEnum<T>().Name;
}

void hi();
}