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
// File: Shell.h
// Started by: Hattozo
// Started on: 3/7/2025
// Description:
#pragma once
#include <NoobWarrior/NoobWarrior.h>
#include <cstdio>

namespace NoobWarrior::Shell {
    typedef struct {
        void* Main;
        const char* Name;
        const char* Description;
    } Command;

    extern FILE* gOut;
    int Open(Core *core, FILE* out = stdout, FILE* in = stdin);
    void Close();
    void ParseCommand(char* cmd);
}