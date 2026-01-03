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
