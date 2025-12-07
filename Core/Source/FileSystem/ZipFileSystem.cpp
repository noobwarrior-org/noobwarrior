// === noobWarrior ===
// File: ZipFileSystem.cpp
// Started by: Hattozo
// Started on: 12/6/2025
// Description:
#include <NoobWarrior/FileSystem/ZipFileSystem.h>

#include <zip.h>

#include <fstream>

using namespace NoobWarrior;

ZipFileSystem::ZipFileSystem() = default;

IFileSystem::Response ZipFileSystem::Open(const std::vector<unsigned char> &zipData) {
    std::vector<unsigned char> magic_number = {0x50, 0x4B, 0x03, 0x04};

    bool fits_magic_number = std::equal(zipData.begin(), zipData.begin() + 3, magic_number.begin());

    if (!fits_magic_number) // Check if file contains magic number for ZIP archive
        return Response::InvalidFile; // Not a valid ZIP file
    
    // zip_open();
    return Response::Success;
}

IFileSystem::Response ZipFileSystem::Open(const std::filesystem::path &zipPath) {
    std::ifstream file(zipPath, std::ios::in | std::ios::binary);
    if (file.fail())
        return Response::FileReadFailed;

    char magic[4];
    file.read(magic, 4);
    file.close();

    return Response::Success;
}