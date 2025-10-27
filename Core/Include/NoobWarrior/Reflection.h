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
            NoobWarrior::Reflection::GetIdTypes()[#idType] = { \
                .Name = #idType, \
                .Class = &typeid(idType), \
                .Create = []() { \
                    return idType(); \
                } \
            }; \
            NoobWarrior::Reflection::GetIdTypeNames()[std::type_index(typeid(idType))] = #idType; \
            auto *fields = NoobWarrior::Reflection::GetFields<idType>();

#define NOOBWARRIOR_REFLECT_ID_TYPE_END(idType) \
        } \
    }; \
    static idType##Registrar s##idType##RegistrarInstance;

#define NOOBWARRIOR_REFLECT_FIELD(fieldName, datatype, desc, getter, setter) \
    (*fields)[#fieldName] = { .Name = #fieldName, .Description = desc, .Type = &typeid(datatype), .Getter = getter, .Setter = setter };

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
    std::string TableName; // To what SQL table does this field belong to?
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

inline std::map<std::string, IdType> &GetIdTypes() {
    static std::map<std::string, IdType> IdTypeMap;
    return IdTypeMap;
}

inline std::unordered_map<std::type_index, std::string> &GetIdTypeNames() {
    static std::unordered_map<std::type_index, std::string> IdTypeNameMap;
    return IdTypeNameMap;
}

template<typename T>
std::string GetIdTypeName() {
    auto &m = GetIdTypeNames();
    auto it = m.find(std::type_index(typeid(T)));
    return (it != m.end()) ? it->second : std::string {};
}

template<typename T>
IdType &GetIdType() {
    return GetIdTypes()[GetIdTypeName<T>()];
}

template<typename T>
std::unordered_map<std::string, Field> *GetFields() {
    IdType &reflectedType = GetIdType<T>();
    return &reflectedType.Fields;
}

void hi();
}