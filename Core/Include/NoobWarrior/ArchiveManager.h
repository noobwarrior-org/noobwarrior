// === noobWarrior ===
// File: ArchiveManager.h
// Started by: Hattozo
// Started on: 3/17/2025
// Description:
#pragma once

#include <NoobWarrior/Archive.h>
#include <NoobWarrior/Roblox/IdType.h>

#include <filesystem>
#include <vector>

namespace NoobWarrior::ArchiveManager {
int AddArchive(const std::filesystem::path &filePath, unsigned int priority);
void AddArchive(Archive *archive, unsigned int priority);
std::vector<unsigned char> RetrieveFile(int64_t id, Roblox::IdType type);
}