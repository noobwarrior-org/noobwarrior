// === noobWarrior ===
// File: Instance.h
// Started by: Hattozo
// Started on: 3/9/2025
// Description:
#pragma once

#include <string>

namespace NoobWarrior::Roblox {
    class Instance {
    public:
        Instance();

        /**
         * @brief The ClassName of this Instance.
         */
        std::string ClassName;

        /**
         * @brief The Name of this Instance.
         */
        std::string Name;

        /**
         * @brief A context-dependent unique identifier for this instance when being serialized.
         */
        std::string Referent;

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
    private:
    };
}