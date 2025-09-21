// === noobWarrior ===
// File: ReflectionMetadata.h
// Started by: Hattozo
// Started on: 9/19/2025
// Description: Anything that requires runtime to dynamically understand the structure of data
// without having to manually specify it everywhere should go here
//
// ID types for example; you don't want to add checks in 30 different code paths because you added a new ID type.
// Just declare it here and those code paths will automatically discover it.
#pragma once
#include "Database/Record/IdRecord.h"

#include <functional>
#include <map>
#include <string>
#include <typeindex>

#define NOOBWARRIOR_REFLECT_ID_TYPE(idType) \
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
            std::puts("Registrar ctor ran for " #idType); \
        } \
    }; \
    static idType##Registrar s##idType##RegistrarInstance;

namespace NoobWarrior::Reflection {
struct IdType {
    std::string Name;
    const std::type_info *Class { nullptr };
    std::function<IdRecord()> Create;
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

void hi();
}