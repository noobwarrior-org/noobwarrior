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
// File: VfsBridge.cpp
// Started by: Hattozo
// Started on: 1/10/2026
// Description:
#include <NoobWarrior/Lua/Bridge/VfsBridge.h>
#include <NoobWarrior/Lua/LuaState.h>
#include <NoobWarrior/FileSystem/VirtualFileSystem.h>
#include <NoobWarrior/FileSystem/StdFileSystem.h>
#include <NoobWarrior/FileSystem/ZipFileSystem.h>

using namespace NoobWarrior;

static int VirtualFileSystem_new(lua_State *L) {
    const char* pathStr = luaL_checkstring(L, 1);
    VirtualFileSystem* vfs = nullptr;

    switch (VirtualFileSystem::GetFormatFromPath(pathStr)) {
    default:
        return luaL_error(L, "invalid format for virtual filesystem");
    case VirtualFileSystem::Format::Standard:
        vfs = (StdFileSystem*)lua_newuserdata(L, sizeof(StdFileSystem));
        new(vfs) StdFileSystem(pathStr);
        break;
    case VirtualFileSystem::Format::Zip:
        vfs = (ZipFileSystem*)lua_newuserdata(L, sizeof(ZipFileSystem));
        new(vfs) ZipFileSystem(pathStr);
        break;
    }
    return 1;
}

static int VirtualFileSystem_gc(lua_State *L) {
    VirtualFileSystem* vfs = (VirtualFileSystem*)luaL_checkudata(L, 1, "VirtualFileSystem");
    if (vfs != nullptr) {
        vfs->~VirtualFileSystem();
    }
    return 0;
}

VfsBridge::VfsBridge(LuaState* lua) : LuaObjectBridge(lua, "VirtualFileSystem") {}

LuaReg VfsBridge::GetStaticFuncs() {
    return {
        {"new", VirtualFileSystem_new}
    };
}

LuaReg VfsBridge::GetObjectMetaFuncs() {
    return {
        {"__gc", VirtualFileSystem_gc}
    };
}