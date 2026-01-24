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
// File: Url.cpp
// Started by: Hattozo
// Started on: 1/10/2026
// Description: Url class that supports a bunch of cool custom protocols and shit
#include "NoobWarrior/FileSystem/VirtualFileSystem.h"
#include <NoobWarrior/Url.h>
#include <NoobWarrior/Log.h>

#include <map>

#define PROTOCOL_BEGIN() static std::map<std::string, NoobWarrior::ProtocolType> sProtocolMap = {
#define PROTOCOL_END() };
#define PROTOCOL(name, type) {name, type},

using namespace NoobWarrior;

PROTOCOL_BEGIN()
    PROTOCOL("file", ProtocolType::File)
    PROTOCOL("http", ProtocolType::Http)
    PROTOCOL("https", ProtocolType::Https)
    PROTOCOL("installdata", ProtocolType::InstallData)
    PROTOCOL("userdata", ProtocolType::UserData)
    PROTOCOL("db", ProtocolType::Database)
    PROTOCOL("plugin", ProtocolType::Plugin)
    PROTOCOL("rbxassetid", ProtocolType::RbxAssetId)
    PROTOCOL("rbxthumb", ProtocolType::RbxThumb)
PROTOCOL_END()

Url::Url(const std::string &str, const UrlContext ctx) :
    mStr(str),
    mFailReason(FailReason::None),
    mCtx(ctx)
{}

bool Url::Fail() const {
    return mFailReason != FailReason::None;
}

bool Url::IsBlank() const {
    bool containsNotSlash = false;
    for (auto it = mStr.begin(); it != mStr.end(); it++) {
        if (*it != '/' && *it != '\\')
            containsNotSlash = true;
    }
    return mStr.empty() || !containsNotSlash;
}

bool Url::DoesStringHaveProtocol() const {
    std::string::size_type pos = mStr.find_first_of("://");
    return pos != std::string::npos;
}

bool Url::DoesStringHaveHostName() const {
    // YOU MUST NEED THE FUCKING PROTOCOL FIRST IF YOU GUARANTEE THAT THIS HAS A HOST NAME
    return DoesStringHaveProtocol() && !GetHostName().empty();
}

bool Url::IsStringRelative() const {
    return !DoesStringHaveProtocol() && !mStr.starts_with("/");
}

Url::FailReason Url::GetFailReason() const {
    return mFailReason;
}

ProtocolType Url::GetProtocol() const {
    std::string protocolStr = GetProtocolString();
    if (sProtocolMap.contains(protocolStr))
        return sProtocolMap.at(protocolStr);
    return ProtocolType::Unsupported;
}

std::string Url::GetProtocolString() const {
    std::string protocol;
    std::string::size_type pos = mStr.find_first_of("://");
    if (pos != std::string::npos) {
        protocol = mStr.substr(0 , pos);
        return protocol;
    }
    if (protocol.empty()) {
        // user did not include a protocol in url, use default submitted in context
        for (const auto& pair : sProtocolMap) {
            if (pair.second == mCtx.DefaultProtocolType)
                return pair.first;
        }
    }
    return ""; // give up
}

std::string Url::GetHostName() const {
    bool doesItHaveAProtocol = DoesStringHaveProtocol();
    if (!doesItHaveAProtocol)
        return ""; // MAKE NO FALSE ASSUMPTIONS. WE HAVE NO IDEA IF THIS IS EITHER A RELATIVE PATH OR HOST NAME. NO I DONT WANT TO RELY ON "WWW" OR ".COM"

    ProtocolType protocol = GetProtocol();
    if (protocol == ProtocolType::File) {
        return ""; // local file, no hostname required
    }

    std::string hostName;
    std::string::size_type protocolPos = mStr.find_first_of("://");
    if (protocolPos != std::string::npos) {
        hostName = mStr.substr(protocolPos + 3);
    }

    std::string::size_type rootPos = hostName.find_first_of("/");
    if (rootPos != std::string::npos) {
        hostName = mStr.substr(0, rootPos - 1);
    }
    
    if (hostName.empty())
        hostName = mCtx.DefaultHostName;
    Out("Url", "Host name: {}", hostName);
    return hostName;
}

std::string Url::GetCwd() const {
    return mCtx.Cwd;
}

std::string Url::Resolve() const {
    std::string protocolStr = GetProtocolString();
    std::string hostName = GetHostName();
    std::string cwd = GetCwd();

    std::string origin = protocolStr + "://" + hostName;

    /* if user-submitted string starts with "/", assume they are bypassing the
       current working directory completely and start from root */
    if (mStr.starts_with("/"))
        return origin + mStr;
    
    return origin + "/" + cwd + "/" + mStr;
}

std::string Url::ResolveAsProtocolRelative() const {
    std::string hostName = GetHostName();
    std::string cwd = GetCwd();

    if (mStr.starts_with("/"))
        return hostName + mStr;

    return hostName + "/" + cwd + "/" + mStr;
}

std::string Url::ResolveAsPathName() const {
    std::string cwd = GetCwd();
    if (mStr.starts_with("/"))
        return mStr;
    return cwd + "/" + mStr;
}

VirtualFileSystem::Response Url::OpenHandle(Core* core, VirtualFileSystem **vfsPtr, FSEntryHandle *handlePtr) const {
    ProtocolType protocol = GetProtocol();

    switch (protocol) {
    default:
        Out("Url", "Core::OpenHandle() called but submitted protocol type relies on networking!");
        return VirtualFileSystem::Response::Failed;
    case ProtocolType::Unsupported:
        Out("Url", "Core::OpenHandle() called on a URL with an unsupported protocol");
        return VirtualFileSystem::Response::Failed;
    case ProtocolType::Database:
        Out("Url", "Database protocol URLs are WIP");
        return VirtualFileSystem::Response::Failed;
    case ProtocolType::File:
        Out("Url", "File protocol URLs are WIP");
        return VirtualFileSystem::Response::Failed;
     case ProtocolType::InstallData:
        Out("Url", "Installdata protocol URLs are WIP");
        return VirtualFileSystem::Response::Failed;
    case ProtocolType::UserData:
        Out("Url", "Userdata protocol URLs are WIP");
        return VirtualFileSystem::Response::Failed;
    case ProtocolType::Plugin: break;
    }

    /* maybe ill finish this one day
    if (protocol == ProtocolType::File) {
        std::string path = ResolveAsProtocolRelative();
        std::ifstream file(path);
        file.read
    }
    */

    VirtualFileSystem* vfs = GetVfs(core);
    FSEntryHandle handle;
    if (vfs != nullptr)
        handle = vfs->OpenHandle(ResolveAsPathName());
    
    *vfsPtr = vfs;
    *handlePtr = handle;

    if (vfs == nullptr)
        return VirtualFileSystem::Response::InvalidFileSystem;
    if (handle == 0)
        return VirtualFileSystem::Response::InvalidFile;
    return VirtualFileSystem::Response::Success;
}

VirtualFileSystem* Url::GetVfs(Core* core) const {
    ProtocolType protocol = GetProtocol();
    if (protocol == ProtocolType::Plugin) {
        GetHostName();
    }
    return nullptr;
}