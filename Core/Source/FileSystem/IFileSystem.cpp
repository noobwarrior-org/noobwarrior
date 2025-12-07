// === noobWarrior ===
// File: IFileSystem.cpp
// Started by: Hattozo
// Started on: 12/5/2025
// Description: An abstract interface for a file system.
#include <NoobWarrior/FileSystem/IFileSystem.h>
#include <NoobWarrior/FileSystem/StdFileSystem.h>
#include <NoobWarrior/FileSystem/ZipFileSystem.h>

#include <cstring>
#include <fstream>

using namespace NoobWarrior;

IFileSystem::Response IFileSystem::CreateFromFile(IFileSystem** vfsPtr, const std::filesystem::path &path) {
    if (std::filesystem::is_directory(path)) {
        *vfsPtr = new StdFileSystem(path);
        return Response::Success;
    } else {
        *vfsPtr = new ZipFileSystem();
        return dynamic_cast<ZipFileSystem*>(*vfsPtr)->Open(path);
    }
    *vfsPtr = nullptr;
    return Response::Failed;
}