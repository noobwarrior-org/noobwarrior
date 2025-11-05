// === noobWarrior ===
// File: BinaryRobloxFile.h
// Started by: Hattozo
// Started on: 11/3/2025
// Description:
#pragma once

#include <NoobWarrior/Roblox/FileFormat/RobloxFile.h>
#include <NoobWarrior/Roblox/FileFormat/Tree/Instance.h>

#include <vector>

namespace NoobWarrior::Roblox {
class BinaryRobloxFile : public RobloxFile {
public:
    BinaryRobloxFile();

    void Save() override;
protected:
    FileResponse ReadFile(std::vector<unsigned char> buffer) override;
};
}