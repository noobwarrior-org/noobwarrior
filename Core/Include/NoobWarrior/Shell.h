// === noobWarrior ===
// File: Shell.h
// Started by: Hattozo
// Started on: 3/7/2025
// Description:
#pragma once

#include <cstdio>

namespace NoobWarrior::Shell {
    typedef struct {
        void* Main;
        const char* Name;
        const char* Description;
    } Command;

    extern FILE* gOut;
    int Open(FILE* out = stdout, FILE* in = stdin);
    void Close();
    void ParseCommand(char* cmd);
}