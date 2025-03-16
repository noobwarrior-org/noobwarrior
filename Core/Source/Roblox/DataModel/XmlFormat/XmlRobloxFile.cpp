// === noobWarrior ===
// File: XmlRobloxFile.cpp
// Started by: Hattozo
// Started on: 3/9/2025
// Description: This file is derived from Roblox-File-Format (https://github.com/MaximumADHD/Roblox-File-Format/blob/main/XmlFormat/XmlRobloxFile.cs)
#include <NoobWarrior/Roblox/DataModel/XmlFormat/XmlRobloxFile.h>

using namespace NoobWarrior::Roblox;

XmlRobloxFile::XmlRobloxFile() : RefCounter(0) {
    Name = "Xml:";
    Referent = "null";
    ParentLocked = true;
}