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

    // Use a virtual filesystem so that we can use both compressed archives and regular folders.
    IFileSystem* vfs;
    IFileSystem::Response fsRes = IFileSystem::CreateFromFile(&vfs, fullDir);

    if (fsRes != IFileSystem::Response::Success || vfs == nullptr) {
        return Response::Failed;
    }

    std::string pluginLuaString;

    FSEntryHandle handle = vfs->OpenHandle("/plugin.lua");
    std::string buf;
    while (vfs->ReadHandleLine(handle, &buf)) {
        pluginLuaString.append(buf);
    }
    vfs->CloseHandle(handle);

    NOOBWARRIOR_FREE_PTR(vfs)

    mCore->GetLuaState();

    return Response::Success;
}

void Plugin::Close() {

}