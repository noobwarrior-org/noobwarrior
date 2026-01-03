// === noobWarrior ===
// File: RobloxFile.h
// Started by: Hattozo
// Started on: 3/9/2025
// Description:
#pragma once

#include "Tree/Instance.h"

#include <string_view>
#include <vector>

namespace NoobWarrior::Roblox {
enum class FileResponse {
    Failed,
    Success,
    InvalidHeader,
    CouldNotParse,
    InvalidVersion,
    VersionTooLow
};

class RobloxFile : public Instance {
public:
    static bool LogErrors;

    // RobloxFile(char *buffer);
    // RobloxFile(const std::filesystem::path &filePath);

    static FileResponse Open(RobloxFile **file, std::vector<unsigned char> buffer);
    static FileResponse Open(RobloxFile **file, std::string_view filePath);

    virtual void Save() = 0;
protected:
    virtual FileResponse ReadFile(std::vector<unsigned char> buffer) = 0;
};
}