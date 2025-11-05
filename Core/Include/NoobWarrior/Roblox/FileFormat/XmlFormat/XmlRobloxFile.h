// === noobWarrior ===
// File: XmlRobloxFile.h
// Started by: Hattozo
// Started on: 3/9/2025
// Description:
#pragma once
#include <NoobWarrior/Roblox/FileFormat/RobloxFile.h>
#include <NoobWarrior/Roblox/FileFormat/Tree/Instance.h>

#include <pugixml.hpp>

#include <map>
#include <vector>
#include <unordered_set>

namespace NoobWarrior::Roblox {
class XmlRobloxFile : public RobloxFile {
public:
    XmlRobloxFile();

    pugi::xml_document XmlDocument;

    std::map<std::string, Instance> Instances;
    std::map<std::string, std::string> Metadata;
    std::unordered_set<std::string> SharedStrings;

    void Save() override;
protected:
    FileResponse ReadFile(std::vector<unsigned char> buffer) override;
private:
    int RefCounter;
};
}