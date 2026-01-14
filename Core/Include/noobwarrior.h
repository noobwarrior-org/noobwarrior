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
// File: noobwarrior.h
// Started by: Hattozo
// Started on: 1/2/2026
// Description: A C wrapper for the noobWarrior API
#ifndef NOOBWARRIOR_H
#define NOOBWARRIOR_H

#include "NoobWarrior/Macros.h"

#ifdef __cplusplus
extern "C" {
#endif

struct nw_core;

struct nw_core *nw_core_create();
void nw_core_free(struct nw_core*);

#ifdef __cplusplus
}
#endif

#endif // NOOBWARRIOR_H
