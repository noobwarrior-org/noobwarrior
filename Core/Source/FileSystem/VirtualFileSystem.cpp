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

VirtualFileSystem::Format VirtualFileSystem::GetFormatFromPath(const std::filesystem::path &path) {
    if (std::filesystem::is_directory(path))
        return Format::Standard;
    else if (std::filesystem::exists(path))
        return Format::Zip;
    else
        return Format::Invalid;
}

VirtualFileSystem::Response VirtualFileSystem::New(VirtualFileSystem** vfsPtr, const std::filesystem::path &path) {
    *vfsPtr = nullptr;
    switch (GetFormatFromPath(path)) {
    default:
        return Response::InvalidFile;
    case Format::Standard:
        *vfsPtr = new StdFileSystem(path);
        break;
    case Format::Zip:
        *vfsPtr = new ZipFileSystem(path);
        break;
    }
    return !(*vfsPtr)->Fail() ? Response::Success : Response::Failed;
}

void VirtualFileSystem::Free(VirtualFileSystem* vfs) {
    NOOBWARRIOR_FREE_PTR(vfs)
}