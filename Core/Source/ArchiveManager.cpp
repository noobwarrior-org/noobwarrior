// === noobWarrior ===
// File: ArchiveManager.cpp
// Started by: Hattozo
// Started on: 3/17/2025
// Description: Loads in multiple Archives with different priorities over one another.
// This is used in situations where you want to have multiple archives loaded at the same time for different reasons,
// but these archives may have conflicting IDs in them. In this case, a system to manage priority is required.
#include <NoobWarrior/ArchiveManager.h>
#include <NoobWarrior/Archive.h>
#include <NoobWarrior/Config.h>

#include <vector>

using namespace NoobWarrior;

static std::vector<Archive*> sMountedArchives;

int ArchiveManager::AddArchive(const std::filesystem::path &filePath, unsigned int priority) {
    Archive *archive = new Archive();
    int ret = archive->Open(filePath.string());
    if (ret != 0) return ret;
    sMountedArchives.insert(sMountedArchives.begin() + priority, archive);
    gConfig.MountedArchives.push_back(archive->GetFilePath());
    return 0;
}

void ArchiveManager::AddArchive(Archive *archive, unsigned int priority) {
    sMountedArchives.insert(sMountedArchives.begin() + priority, archive);
    gConfig.MountedArchives.push_back(archive->GetFilePath());
}

std::vector<unsigned char> ArchiveManager::RetrieveFile(int64_t id, Roblox::IdType type) {
    for (int i = 0; i < sMountedArchives.size(); i++) {
        Archive *archive = sMountedArchives[i];
        auto data = archive->RetrieveFile(id, type);
        if (!data.empty())
            return data;
    }
    return {};
}