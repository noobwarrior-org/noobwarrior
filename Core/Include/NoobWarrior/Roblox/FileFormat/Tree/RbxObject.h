// === noobWarrior ===
// File: RbxObject.h
// Started by: Hattozo
// Started on: 11/3/2025
// Description: This file is derived from Roblox-File-Format (https://github.com/MaximumADHD/Roblox-File-Format/blob/main/Tree/RbxObject.cs)
#pragma once
#include <NoobWarrior/Roblox/FileFormat/Tree/Property.h>

#include <string>
#include <map>

namespace NoobWarrior::Roblox {
class RbxObject {
public:
    RbxObject();

    /**
     * @brief The ClassName of this Instance.
     */
    std::string ClassName;

    /**
     * @brief A context-dependent unique identifier for this instance when being serialized.
     */
    std::string Referent;

    std::map<std::string, Property> &GetProperties();

    bool Destroyed;

    template<typename T>
    bool IsA() {
        return dynamic_cast<T*>(this) != nullptr;
    }

    template<typename T>
    T CastAs() {
        return dynamic_cast<T*>(this);
    }

    virtual void Destroy() {
        props.clear();
    }

    Property* GetProperty(const std::string &name) {
        if (props.contains(name))
            return &props[name];
        return nullptr;
    }

    void AddProperty(Property prop) {
        std::string name = prop.Name;
        RemoveProperty(name);

        prop.Object = this;
        props[name] = prop;
    }

    bool RemoveProperty(const std::string &name) {
        if (props.contains(name)) {
            Property prop = props[name];
            prop.Object = nullptr;
        }
        auto it = props.find(name);
        if (it != props.end())
            props.erase(it);
        return it != props.end();
    }
protected:
    std::map<std::string, Property> props;
};
}