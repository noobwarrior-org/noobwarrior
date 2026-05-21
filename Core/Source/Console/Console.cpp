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
// File: Console.cpp
// Started by: Hattozo
// Started on: 5/9/2026
// Description:
#include <NoobWarrior/Console/Console.h>
#include <NoobWarrior/Console/Command/HelpCommand.h>
#include <NoobWarrior/Console/Command/ExitCommand.h>
#include <NoobWarrior/Console/Command/StudioCommand.h>
#include <NoobWarrior/NoobWarrior.h>
#include <sstream>

using namespace NoobWarrior;

Console::Console(Core* core, std::ostream* out, std::istream* in) :
    mRunning(true),
    mCore(core)
{
    mOut = out;
    mIn = in;
    if (mOut == nullptr) {
        mOut = new std::ostringstream();
        mOwningOutStream = true;
    }
    if (mIn == nullptr) {
        mIn = new std::istringstream();
        mOwningInStream = true;
    }

    RegisterCommand("help", std::make_unique<HelpCommand>(), "Gives a list of all available commands.");
    RegisterAlias("help", "cmds");

    RegisterCommand("exit", std::make_unique<ExitCommand>(), "Stops the console.");
    RegisterAlias("exit", "quit");

    RegisterCommand("studio", std::make_unique<StudioCommand>(), "Launches Roblox Studio.");
    RegisterAlias("studio", "launch-studio");

    mCore->GetConsoleAddedSignal()->Fire(this);
}

Console::~Console() {
    if (mOwningOutStream) {
        NOOBWARRIOR_FREE_PTR(mOut)
    }
    if (mOwningInStream) {
        NOOBWARRIOR_FREE_PTR(mIn)
    }
}

int Console::Exec() {
    *mOut << "\x1b[33mnoobWarrior v"
            NOOBWARRIOR_VERSION
            " \x1b[35mShell\n\x1b[36mType \"help\" for commands. Type \"about\" to see credits. Type \"exit\" to quit.\x1b[0m\n";
    while (mRunning) {
        *mOut << "> ";
        std::string input;

        std::getline(*mIn, input);

        CommandContext ctx(this);
        std::string cmdName = input.substr(0, input.find_first_of(' '));
        if (mCommands.contains(cmdName))
            mCommands[cmdName]->Main(ctx);
        else if (mAliases.contains(cmdName) && mCommands.contains(mAliases[cmdName]))
            mCommands[mAliases[cmdName]]->Main(ctx);
        else
            *mOut << "Command " << cmdName << " not found" << std::endl;
    }
    return 0;
}

void Console::Stop() {
    mRunning = false;
}

void Console::RegisterCommand(const std::string &name, std::unique_ptr<Command> cmd, const std::string &desc) {
    mCommands[name] = std::move(cmd);
    mDescriptions[name] = !desc.empty() ? desc : "No description available.";
}

void Console::RegisterAlias(const std::string &name, const std::string &alias) {
    mAliases[alias] = name;
}

Core* Console::GetCore() {
    return mCore;
}

const std::unordered_map<std::string, std::unique_ptr<Command>>& Console::GetCommands() const {
    return mCommands;
}

const std::unordered_map<std::string, std::string>& Console::GetAliases() const {
    return mAliases;
}

const std::unordered_map<std::string, std::string>& Console::GetDescriptions() const {
    return mDescriptions;
}

std::ostream* Console::GetOutputStream() {
    return mOut;
}

std::istream* Console::GetInputStream() {
    return mIn;
}
