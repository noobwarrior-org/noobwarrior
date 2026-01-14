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
// File: noobwarrior_c.cpp
// Started by: Hattozo
// Started on: 1/2/2026
// Description: C wrapper for noobWarrior
#include <noobwarrior.h>
#include <NoobWarrior/NoobWarrior.h>

struct nw_core {
    NoobWarrior::Core inst;
};

struct nw_core *nw_core_create() {
    return new nw_core();
}

void nw_core_free(struct nw_core *core) {
    delete core;
}
