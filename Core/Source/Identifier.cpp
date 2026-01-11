// === noobWarrior ===
// File: Identifier.cpp
// Started by: Hattozo
// Started on: 1/10/2026
// Description:
#include <NoobWarrior/Identifier.h>

using namespace NoobWarrior;

Identifier::Identifier(const std::string &str, const std::string &cwd) : mStr(str) {

}

ProtocolType Identifier::GetProtocol() {
    
}

std::string Identifier::GetProtocolString() {
    int pos = mStr.find("://");
    if (pos)
        std::string protocol = mStr.substr(0 , pos);
}