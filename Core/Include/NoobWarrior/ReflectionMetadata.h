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

#define REFLECT_ID_TYPE(idType) \
    /* This is used as a cheap way to autorun our code */ \
    struct idType##Registrar { \
        idType##Registrar() { \
            NoobWarrior::Reflection::GetIdTypes()[#idType] = { \
                .Name = #idType, \
                .Class = &typeid(idType), \
                .Create = []() { \
                    return idType(); \
                } \
            }; \
        } \
    }; \
    static idType##Registrar s##idType##RegistrarInstance; \

namespace NoobWarrior::Reflection {
struct IdType {
    std::string Name;
    const std::type_info *Class { nullptr };
    std::function<IdRecord()> Create;
};

inline std::map<std::string, IdType> &GetIdTypes() {
    static std::map<std::string, IdType> m;
    return m;
}

template<typename T>
std::string GetIdTypeName() {
    return typeid(T).name();
}
}