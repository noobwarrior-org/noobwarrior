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
#include <NoobWarrior/Url.h>
#include <NoobWarrior/Log.h>
#include <NoobWarrior/NoobWarrior.h>
#include <NoobWarrior/FileSystem/VirtualFileSystem.h>
#include <NoobWarrior/PluginManager.h>

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
{
    if ((mCtx.EnforceProtocolType || !DoesStringHaveProtocol())
            && mCtx.DefaultProtocolType == ProtocolType::Unsupported) // Why would you ever do this?
    {
        mFailReason = FailReason::ForbiddenProtocol;
        return;
    }

    ProtocolType userSubmittedProtocol = GetProtocol();
    if (mCtx.EnforceProtocolType && userSubmittedProtocol != mCtx.DefaultProtocolType) {
        mFailReason = FailReason::ForbiddenProtocol;
        return;
    }

    std::string userSubmittedHostName = GetHostName();
    if (mCtx.EnforceHostName && userSubmittedHostName.compare(mCtx.DefaultHostName) != 0) {
        mFailReason = FailReason::ForbiddenHostName;
        return;
    }
}

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
    std::string::size_type pos = mStr.find("://");
    return pos != std::string::npos;
}

bool Url::DoesStringHaveHostName() const {
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
    std::string fullUrl = Resolve();
    std::string::size_type pos = fullUrl.find("://");
    if (pos != std::string::npos) {
        return fullUrl.substr(0, pos);
    }

    // user did not include a protocol in url, use default submitted in context
    for (const auto& pair : sProtocolMap) {
        if (pair.second == mCtx.DefaultProtocolType) {
            return pair.first;
        }
    }
    return ""; // give up
}

std::string Url::GetHostName() const {
    if (GetProtocol() == ProtocolType::File)
        return ""; // Local files SHOULD NOT have a host name!

    std::string fullUrl = Resolve();
    std::string hostName;
    std::string::size_type protocolPos = fullUrl.find("://");
    
    if (protocolPos != std::string::npos) {
        hostName = fullUrl.substr(protocolPos + 3); // Skip "://"
    }

    std::string::size_type rootPos = hostName.find("/");
    if (rootPos != std::string::npos) {
        hostName = hostName.substr(0, rootPos);
    }
    
    return hostName;
}

std::string Url::GetCwd() const {
    return mCtx.Cwd;
}

/* constructs a full absolute URL using the information from the UrlContext object */
std::string Url::Resolve() const {
    std::string fullUrl;
    bool foundProtocolInUserString = false;

    std::string::size_type protocolPos = mStr.find("://");
    if (protocolPos != std::string::npos) {
        foundProtocolInUserString = true;
    }

    if (!foundProtocolInUserString) {
        for (const auto& pair : sProtocolMap) {
            if (pair.second == mCtx.DefaultProtocolType)
                fullUrl += pair.first;
        }
        fullUrl += "://";
        fullUrl += mCtx.DefaultHostName;
        
        // current working directory
        if (!mStr.starts_with("/"))
            fullUrl += mCtx.Cwd;

        fullUrl += mStr;
    } else {
        fullUrl = mStr;
    }

    return fullUrl;
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
    Out("Url", "String: {}", mStr);
    Out("Url", "Full Resolve: {}", Resolve());
    Out("Url", "Protocol for OpenHandle(): {}", static_cast<int>(protocol));

#define NETWORK_UNSUPPORTED \
    Out("Url", "Core::OpenHandle() called but submitted protocol type relies on networking!"); \
    return VirtualFileSystem::Response::Failed;

    switch (protocol) {
    default:
        Out("Url", "Core::OpenHandle() called on a URL with an unsupported protocol");
        return VirtualFileSystem::Response::Failed;
    case ProtocolType::Http: NETWORK_UNSUPPORTED
    case ProtocolType::Https: NETWORK_UNSUPPORTED
    case ProtocolType::RbxAssetId: NETWORK_UNSUPPORTED
    case ProtocolType::RbxThumb: NETWORK_UNSUPPORTED
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

#undef NETWORK_UNSUPPORTED

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
        std::string hostName = GetHostName();
        PluginManager* plgMgr = core->GetPluginManager();
        Plugin* plugin = plgMgr->GetPluginFromIdentifier(hostName);
        if (plugin != nullptr)
            return plugin->GetVfs();
    // fuck u
    } else if (protocol == ProtocolType::File) {

    } else if (protocol == ProtocolType::Database) {

    } else if (protocol == ProtocolType::InstallData) {

    } else if (protocol == ProtocolType::UserData) {

    }
    return nullptr;
}