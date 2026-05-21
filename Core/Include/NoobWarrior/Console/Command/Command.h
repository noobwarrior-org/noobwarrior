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
// File: Command.h
// Started by: Hattozo
// Started on: 5/9/2026
// Description:
#pragma once
#include <vector>
#include <string>

namespace NoobWarrior {
class Console;
class Core;
struct CommandContext {
    CommandContext(Console* console);

    void Reply(const std::string& str);
    Console* GetConsole();
    Core* GetCore();
    std::vector<std::string> Args;
private:
    Console* mConsole;
};

class Command {
public:
    Command() = default;
    virtual ~Command() = default;
    virtual void Main(CommandContext& ctx) = 0;
};
}