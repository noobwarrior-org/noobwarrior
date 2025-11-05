// === noobWarrior ===
// File: RobloxFile.cpp
// Started by: Hattozo
// Started on: 3/9/2025
// Description: Represents a loaded Roblox place/model file. RobloxFile is an Instance and its children are the contents of the file.
// This file is derived from Roblox-File-Format (https://github.com/MaximumADHD/Roblox-File-Format/blob/main/RobloxFile.cs)
#include <NoobWarrior/Roblox/FileFormat/RobloxFile.h>
#include <NoobWarrior/Roblox/FileFormat/BinaryFormat/BinaryRobloxFile.h>
#include <NoobWarrior/Roblox/FileFormat/XmlFormat/XmlRobloxFile.h>

using namespace NoobWarrior::Roblox;

FileResponse RobloxFile::Open(RobloxFile **file, std::vector<unsigned char> buffer) {
    *file = nullptr;
    if (buffer.size() > 14) {
        std::string header;
        header.assign(buffer.begin(), buffer.begin() + 8);

        if (header.compare("<roblox!") == 0)
            *file = new BinaryRobloxFile();
        else if (header.starts_with("<roblox"))
            *file = new XmlRobloxFile();

        if (*file != nullptr) {
            return (*file)->ReadFile(buffer);
        }
    }
    return FileResponse::InvalidHeader;
}

FileResponse RobloxFile::Open(RobloxFile **file, std::string_view filePath) {
    return FileResponse::Failed;
}