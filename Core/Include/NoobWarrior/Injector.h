// === noobWarrior ===
// File: Injector.h
// Started by: Hattozo
// Started on: 3/16/2025
// Description:
#pragma once
#include <NoobWarrior/Roblox/EngineType.h>

#include <filesystem>

namespace NoobWarrior {
    int Inject(unsigned long pid, char *dllPath);
    int LaunchInjectProcess(const std::filesystem::path &filePath);
    int LaunchRoblox(Roblox::EngineType type, std::string version);
}