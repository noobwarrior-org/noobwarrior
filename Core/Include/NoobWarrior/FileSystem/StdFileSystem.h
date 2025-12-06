// === noobWarrior ===
// File: StdFileSystem.h
// Started by: Hattozo
// Started on: 12/5/2025
// Description:
#pragma once
#include "IFileSystem.h"

#include <filesystem>

namespace NoobWarrior {
class StdFileSystem : public IFileSystem {
public:
    StdFileSystem(const std::filesystem::path &root);
    
};
}