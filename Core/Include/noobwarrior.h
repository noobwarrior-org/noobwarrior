// === noobWarrior ===
// File: noobwarrior.h
// Started by: Hattozo
// Started on: 6/23/2025
// Description: A C wrapper for the noobWarrior API
#ifndef NOOBWARRIOR_H
#define NOOBWARRIOR_H

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
