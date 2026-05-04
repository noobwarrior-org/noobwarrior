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
// File: NetFileSystem.h
// Started by: Hattozo
// Started on: 3/10/2026
// Description: VirtualFileSystem that relies on a remote location
// Note: As the name suggests, this only works for URLs that rely on remote locations.
// Since local URLs already depend on a VFS, they do not require a wrapper class like this one.
// If you want to retrieve a VFS for those kinds of URLs, use Url::GetVfs() instead.
#pragma once
#include "VirtualFileSystem.h"
#include <NoobWarrior/Url.h>

namespace NoobWarrior {
class NetFileSystem : public VirtualFileSystem {
public:
    NetFileSystem(Url& rootUrl);
};
}