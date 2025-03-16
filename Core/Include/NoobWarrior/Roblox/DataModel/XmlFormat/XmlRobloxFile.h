// === noobWarrior ===
// File: XmlRobloxFile.h
// Started by: Hattozo
// Started on: 3/9/2025
// Description:
#pragma once

#include <NoobWarrior/Roblox/DataModel/RobloxFile.h>

namespace NoobWarrior::Roblox {
    class XmlRobloxFile : public RobloxFile {
    public:
        XmlRobloxFile();
    private:
        int RefCounter;
    };
}