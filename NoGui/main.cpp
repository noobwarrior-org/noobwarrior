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
#define NOOBWARRIOR_NOSHELL_HINT 1
#include <NoobWarrior/Shell.h>

#include <cstring>

int main(int argc, char* argv[]) {
    int ret = 0;
    bool startShell = true;
    for (int i = 0; i < argc; i++) {
        if (strncmp(argv[i], "--no-shell", strlen(argv[i])) == 0)
            startShell = false;
    }
    if (startShell) {
        puts("\x1b[34mDon't want the CLI to open a shell? Start the program with the --no-shell flag.\x1b[0m");
        NoobWarrior::Core core;
        ret = NoobWarrior::Shell::Open(&core, stdout);
    }
    return ret;
}