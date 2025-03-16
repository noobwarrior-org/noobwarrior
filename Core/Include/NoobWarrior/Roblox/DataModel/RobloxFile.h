// === noobWarrior ===
// File: RobloxFile.h
// Started by: Hattozo
// Started on: 3/9/2025
// Description:
#pragma once

#include "Tree/Instance.h"

#include <string_view>
#include <filesystem>

namespace NoobWarrior::Roblox {
    class RobloxFile : public Instance {
    public:
        static bool LogErrors;

        // RobloxFile(char *buffer);
        // RobloxFile(const std::filesystem::path &filePath);

        static RobloxFile Open(char* buffer);
        static RobloxFile Open(std::string_view filePath);

        virtual void Save() = 0;
    protected:
        virtual void ReadFile(char* buffer) = 0;
    };
}