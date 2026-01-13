// === noobWarrior ===
// File: Identifier.cpp
// Started by: Hattozo
// Started on: 1/10/2026
// Description:
#include <NoobWarrior/Identifier.h>

#define CHECK_PROTOCOL(str, type) if (protocolStr.compare(str) == 0) return type; // how to mask bad code 101

using namespace NoobWarrior;

Identifier::Identifier(const std::string &str, const std::string &cwd) : mStr(str) {

}

ProtocolType Identifier::GetProtocol() {
    std::string protocolStr = GetProtocolString();
    CHECK_PROTOCOL("http", ProtocolType::Http)
    CHECK_PROTOCOL("https", ProtocolType::Https)
    CHECK_PROTOCOL("plugin", ProtocolType::Plugin)
    CHECK_PROTOCOL("data", ProtocolType::Data)
    CHECK_PROTOCOL("rbxasset", ProtocolType::RbxAsset)
    CHECK_PROTOCOL("rbxthumb", ProtocolType::RbxThumb)
    return ProtocolType::Unsupported;
}

std::string Identifier::GetProtocolString() {
    int pos = mStr.find("://");
    if (pos) {
        std::string protocol = mStr.substr(0 , pos);
        return protocol;
    } else return mStr;
}