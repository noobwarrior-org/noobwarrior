// === noobWarrior ===
// File: DatabaseManager.h
// Started by: Hattozo
// Started on: 3/17/2025
// Description:
#pragma once

#include <NoobWarrior/Database/Database.h>
#include "IdType/IdType.h"

#include <filesystem>
#include <vector>

namespace NoobWarrior {
class DatabaseManager {
public:
    DatabaseResponse Mount(const std::filesystem::path &filePath, unsigned int priority);
    void Mount(Database *database, unsigned int priority);
    int Unmount(Database *database);
    std::vector<unsigned char> RetrieveContentData(int64_t id, IdType type);
private:
    std::vector<Database*> MountedDatabases;
};
}