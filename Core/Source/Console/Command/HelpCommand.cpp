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
// File: HelpCommand.cpp
// Started by: Hattozo
// Started on: 5/16/2026
// Description:
#include <NoobWarrior/Console/Command/HelpCommand.h>
#include <NoobWarrior/Console/Console.h>
#include <algorithm>

using namespace NoobWarrior;

void HelpCommand::Main(CommandContext& ctx) {
    const auto& cmds = ctx.GetConsole()->GetCommands();

    std::vector<std::string> cmdNames;
    for (auto &[cmdName, cmd] : cmds) {
        cmdNames.push_back(cmdName);
    }
    std::stable_sort(cmdNames.begin(), cmdNames.end());

    const auto& descs = ctx.GetConsole()->GetDescriptions();

    ctx.Reply("\x1b[32mCommands:");
    for (auto &cmdName : cmdNames) {
        ctx.Reply("\x1b[35m" + cmdName + " - \x1b[36m" + (descs.contains(cmdName) ? descs.at(cmdName) : "No description available.") + "\x1b[0m");
    }
}