// === noobWarrior ===
// File: VirtualFileSystem.cpp
// Started by: Hattozo
// Started on: 12/5/2025
// Description: An abstract interface for a file system.
#include <NoobWarrior/FileSystem/VirtualFileSystem.h>
#include <NoobWarrior/FileSystem/StdFileSystem.h>
#include <NoobWarrior/FileSystem/ZipFileSystem.h>
#include <NoobWarrior/NoobWarrior.h>

using namespace NoobWarrior;

VirtualFileSystem::~VirtualFileSystem() {}

VirtualFileSystem::Response VirtualFileSystem::New(VirtualFileSystem** vfsPtr, const std::filesystem::path &path) {
    *vfsPtr = nullptr;
    if (std::filesystem::is_directory(path))
        *vfsPtr = new StdFileSystem(path);
    else if (std::filesystem::exists(path))
        *vfsPtr = new ZipFileSystem(path);
    if (*vfsPtr == nullptr)
        return Response::InvalidFile;
    return !(*vfsPtr)->Fail() ? Response::Success : Response::Failed;
}

void VirtualFileSystem::Free(VirtualFileSystem* vfs) {
    NOOBWARRIOR_FREE_PTR(vfs)
}