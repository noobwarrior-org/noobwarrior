// === noobWarrior ===
// File: Identifier.h
// Started by: Hattozo
// Started on: 1/10/2026
// Description:
#pragma once
#include <string>
#include <vector>

namespace NoobWarrior {
enum class ProtocolType {
    Unsupported,
    Http,
    Https,
    Plugin,
    Data,
    RbxAsset,
    RbxThumb
};

class Identifier {
public:
    Identifier(const std::string &str, const std::string &cwd = "");

    ProtocolType GetProtocol();
    std::string GetProtocolString();

    std::string Resolve();
    std::vector<unsigned char> RetrieveData();

    inline operator std::string() const {
        return mStr;
    }

    inline operator const char*() const {
        return mStr.c_str();
    }

    inline operator char*() const {
        return const_cast<char*>(mStr.c_str());
    }

    // inline Identifier& operator =(const std::string &str) {
        // mStr = str;
    // }
protected:
    std::string mStr;
};
}