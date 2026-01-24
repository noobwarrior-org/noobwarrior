/*
 * Copyright (C) 2026 Hattozo
 *
 * This file is part of noobWarrior.
 *
 * noobWarrior is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * noobWarrior is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with noobWarrior; if not, see
 * <https://www.gnu.org/licenses/>.
 */
// === noobWarrior ===
// File: Url.h
// Started by: Hattozo
// Started on: 1/10/2026
// Description: Url class that supports a bunch of cool custom protocols and shit
#pragma once
#include <NoobWarrior/FileSystem/VirtualFileSystem.h>

#include <string>

namespace NoobWarrior {
class Core;

enum class ProtocolType {
    Unsupported,
    File,
    Http,
    Https,
    InstallData,
    UserData,
    Database,
    Plugin,
    RbxAssetId,
    RbxThumb
};

/*
 * Usually the string in a url does not have enough context
 * for us to understand what resource it is exactly pointing to.
 *
 * Say if a Lua script in a plugin decides to include a file named "myscript.lua"
 * with no absolute path & no name of the plugin and its protocol, how do we
 * know where to find it?
 *
 * This gives us some parameters to work with in-case that is the case.
 */
struct UrlContext {
    std::string Cwd { "/" };
    ProtocolType DefaultProtocolType { ProtocolType::Plugin }; // The default protocol type if one is not specified in the string.
    std::string DefaultHostName {};

    /* If set to true, will force the user submitted url string to have this protocol.
       Any other protocol will result in an error. */
    bool EnforceProtocolType { false };

    /* If set to true, will force the user submitted url string to have this host name if not set.
       Any other host name will result in an error.
       This does nothing if DefaultProtocolType is not set to either Plugin or Http/Https.
       It will also do nothing if HostName is an empty string */
    bool EnforceHostName { false };
};

class Url {
public:
    enum class FailReason {
        None,
        Unknown,
        Malformed,
        UnsupportedProtocol,
        ForbiddenProtocol,
        ForbiddenPlugin
    };

    Url(const std::string &str = "", const UrlContext ctx = {});

    bool Fail() const;
    bool IsBlank() const;

    bool DoesStringHaveProtocol() const;
    bool DoesStringHaveHostName() const;
    bool IsStringRelative() const;

    FailReason GetFailReason() const;

    ProtocolType GetProtocol() const;
    std::string GetProtocolString() const;
    std::string GetHostName() const;
    std::string GetCwd() const;

    /* Returns an absolute path to the resource */
    std::string Resolve() const;

    /* Returns an absolute path to the resource without a protocol */
    std::string ResolveAsProtocolRelative() const;

    /* Returns an absolute path to the resource without its origin */
    std::string ResolveAsPathName() const;

    /* This works for only URLs that use completely offline protocols that rely on virtual file systems like plugin://, file://, or db://
       Anything else will fail. Use NetClient for that. */
    VirtualFileSystem::Response OpenHandle(Core* core, VirtualFileSystem **vfsPtr, FSEntryHandle *handlePtr) const;

    /* Same thing here, except it will return nullptr instead */
    VirtualFileSystem* GetVfs(Core* core) const;

    inline operator std::string() const {
        return mStr;
    }

    inline operator const char*() const {
        return mStr.c_str();
    }

    inline operator char*() const {
        return const_cast<char*>(mStr.c_str());
    }

    inline Url& operator =(const std::string &str) {
        mStr = str;
        return *this;
    }

    inline Url& operator =(const char* str) {
        mStr = str;
        return *this;
    }
protected:
    std::string mStr;
    FailReason mFailReason;
    UrlContext mCtx;
};
}