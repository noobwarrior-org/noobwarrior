// === noobWarrior ===
// File: Plugin.cpp
// Started by: Hattozo
// Started on: 12/3/2025
// Description:
#include <NoobWarrior/Plugin.h>
#include <NoobWarrior/NoobWarrior.h>
#include <NoobWarrior/FileSystem/IFileSystem.h>

#include <zip.h>

#include <fstream>
#include <cstring>

using namespace NoobWarrior;

/* NOTE: File names are relative to the path of the plugins folder in noobWarrior's user directory folder. */
Plugin::Plugin(const std::string &fileName, Core* core, bool includedInInstall) : mFileName(fileName), mCore(core), mIncludedInInstall(includedInInstall) {

}

Plugin::Response Plugin::Open() {
    std::filesystem::path fullDir = (!mIncludedInInstall ? mCore->GetUserDataDir() : mCore->GetInstallationDir()) / "plugins" / mFileName;

    IFileSystem* vfs;
    IFileSystem::Response fsRes = IFileSystem::CreateFromFile(&vfs, fullDir);

    if (fsRes != IFileSystem::Response::Success) {
        return Response::Failed;
    }
    
    if (std::filesystem::is_directory(fullDir)) {

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

    return Response::Success;
}
