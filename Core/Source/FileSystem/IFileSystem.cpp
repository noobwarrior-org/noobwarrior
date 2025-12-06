// === noobWarrior ===
// File: IFileSystem.cpp
// Started by: Hattozo
// Started on: 12/5/2025
// Description: An abstract interface for a file system.
#include <NoobWarrior/FileSystem/IFileSystem.h>
#include <NoobWarrior/FileSystem/StdFileSystem.h>

#include <cstring>

using namespace NoobWarrior;

IFileSystem::Response IFileSystem::CreateFromFile(IFileSystem** vfsPtr, const std::filesystem::path &path) {
    if (std::filesystem::is_directory(path)) {
        *vfsPtr = new StdFileSystem(path);
        return Response::Success;
    } else {
        std::ifstream file(fullDir, std::ios::in | std::ios::binary);
        if (file.fail())
            return Response::FileReadFailed;

        char magic[4];
        file.read(magic, 4);
        file.close();

        // Only ZIP files are allowed for now.
        // TODO: Add support for other file types in the future
        if (strncmp(magic, "\x50\x4B\x03\x04", 4) != 0) // Check if file contains magic number for ZIP archive
            return Response::InvalidFile; // Not a valid ZIP file
        
        zip_open();
    }
    return Response::Failed;
}