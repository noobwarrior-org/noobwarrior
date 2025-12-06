// === noobWarrior ===
// File: StdFileSystem.cpp
// Started by: Hattozo
// Started on: 12/5/2025
// Description:
#include <NoobWarrior/FileSystem/StdFileSystem.h>
#include <cassert>

using namespace NoobWarrior;

StdFileSystem::StdFileSystem(const std::filesystem::path &root) {
    assert(std::filesystem::is_directory(root) && "Root is not a directory");

}

FSEntry StdFileSystem::GetEntryFromPath(const std::string &path) {
    FSEntry entry {};
    entry.Exists = std::filesystem::exists(path);
    if (!entry.Exists)
        goto finish;
    entry.Symlink = 
    entry.Type = std::filesystem::is_directory(path) ? FSEntry::Type::Directory : FSEntry::Type::File;
finish:
    return entry;
}