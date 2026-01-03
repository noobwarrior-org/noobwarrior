// === noobWarrior ===
// File: Instance.h
// Started by: Hattozo
// Started on: 3/9/2025
// Description: This file is derived from Roblox-File-Format (https://github.com/MaximumADHD/Roblox-File-Format/blob/main/Tree/Instance.cs)
#pragma once
#include <NoobWarrior/Roblox/FileFormat/Tree/RbxObject.h>

#include <string>
#include <unordered_set>

namespace NoobWarrior::Roblox {
class Instance : public RbxObject {
public:
    Instance();

    /**
     * @brief The Name of this Instance.
     */
    std::string Name;

    /**
     * @brief Indicates whether the parent of this object is locked.
     */
    bool ParentLocked;

    /**
        * @brief Indicates whether this Instance is a Service.
        */
    bool IsService;

    /**
        * @brief Indicates whether this Instance has been destroyed.
        */
    bool Destroyed;

    template <typename T>
    bool GetAttribute(std::string key, T* value);
    
    template <typename T>
    bool SetAttribute(std::string key, T value);

    bool IsAncestorOf(Instance* ancestor);
    bool IsDescendantOf(Instance* descendant);

    Instance* GetParent();
    bool SetParent(Instance* inst);

    std::string GetFullName(const std::string &separator = "\\");
private:
    Instance* ParentUnsafe;
    std::unordered_set<Instance*> Children;
};
}