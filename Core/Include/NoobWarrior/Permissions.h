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
// File: Permissions.h
// Started by: Hattozo
// Started on: 10/5/2026
// Description: The kinds of permissions that you can grant to Lua scripts
#pragma once

namespace NoobWarrior {
enum class Permissions {
    AccessAllPluginDataUrl, // Can access plugin data outside of this plugins specialized folder
    AccessAllPluginUrl, // Can access all plugin URLs other than its own
    AccessDbUrl, // Can access EmuDb URLs
    AccessLocalFile, // Can access local files on the user's PC through URLs. VERY DANGEROUS!
    NetServer, // Can create HTTP servers
    NetClient, // Can make TCP/IP client requests
    OsShell, // Can execute operating system shell commands. VERY DANGEROUS!
    NoobShell, // Can execute noobWarrior's own shell commands
    // Okay this sounds like a horrible idea but I'm making a MCP server plugin
    // where a bot can play games and take screenshots as they play, that's why I added this
    // That makes it sound even worse, actually
    Screencast // Can take screenshots of the user's desktop
};
}