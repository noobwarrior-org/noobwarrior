// === noobWarrior ===
// File: DatabaseManager.cpp
// Started by: Hattozo
// Started on: 3/17/2025
// Description: Loads in multiple databases with different priorities over one another.
// This is used in situations where you want to have multiple databases loaded at the same time for different reasons,
// but these databases may have conflicting IDs in them. In this case, a system to manage priority is required.
//
// This also handles authentication, but will outsource it to a master server if set.
#include <NoobWarrior/Database/DatabaseManager.h>
#include <NoobWarrior/Database/Database.h>
#include <NoobWarrior/Config.h>

#include <vector>

using namespace NoobWarrior;

DatabaseResponse DatabaseManager::AutocreateMasterDatabase() {
    if (GetMasterDatabase() != nullptr)
        return DatabaseResponse::Success;

}

DatabaseResponse DatabaseManager::Mount(const std::filesystem::path &filePath, unsigned int priority) {
    auto *database = new Database();
    DatabaseResponse res = database->Open(filePath.string());
    if (res != DatabaseResponse::Success) return res;
    MountedDatabases.insert(MountedDatabases.begin() + priority, database);
    return DatabaseResponse::Success;
}

void DatabaseManager::Mount(Database *database, unsigned int priority) {
    MountedDatabases.insert(MountedDatabases.begin() + priority, database);
}

Database *DatabaseManager::GetMasterDatabase() {
    return MountedDatabases.size() > 0 ? MountedDatabases.at(0) : nullptr;
}

std::vector<unsigned char> DatabaseManager::RetrieveAssetData(int64_t id) {
    for (int i = 0; i < MountedDatabases.size(); i++) {
        Database *database = MountedDatabases[i];
        auto data = database->RetrieveAssetData(id);
        if (!data.empty())
            return data;
    }
    return {};
}