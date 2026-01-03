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
