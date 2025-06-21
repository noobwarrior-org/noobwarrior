// === noobWarrior ===
// File: DatabaseManager.cpp
// Started by: Hattozo
// Started on: 3/17/2025
// Description: Loads in multiple databases with different priorities over one another.
// This is used in situations where you want to have multiple databases loaded at the same time for different reasons,
// but these databases may have conflicting IDs in them. In this case, a system to manage priority is required.
#include <NoobWarrior/DatabaseManager.h>
#include <NoobWarrior/Database.h>
#include <NoobWarrior/Config.h>

#include <vector>

using namespace NoobWarrior;

int DatabaseManager::Mount(const std::filesystem::path &filePath, unsigned int priority) {
    Database *database = new Database();
    int ret = database->Open(filePath.string());
    if (ret != 0) return ret;
    MountedDatabases.insert(MountedDatabases.begin() + priority, database);
    // gConfig.MountedArchives.push_back(database->GetFilePath());
    return 0;
}

void DatabaseManager::Mount(Database *database, unsigned int priority) {
    MountedDatabases.insert(MountedDatabases.begin() + priority, database);
    // gConfig.MountedArchives.push_back(database->GetFilePath());
}

std::vector<unsigned char> DatabaseManager::RetrieveFile(int64_t id, Roblox::IdType type) {
    for (int i = 0; i < MountedDatabases.size(); i++) {
        Database *database = MountedDatabases[i];
        auto data = database->RetrieveFile(id, type);
        if (!data.empty())
            return data;
    }
    return {};
}