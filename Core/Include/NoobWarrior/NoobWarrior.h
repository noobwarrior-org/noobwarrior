// === noobWarrior ===
// File: NoobWarrior.h
// Started by: Hattozo
// Started on: 12/15/2024
// Description: Contains global constants and useful macros
#pragma once

#define NOOBWARRIOR_VERSION "0.0.1"
#define NOOBWARRIOR_ARCHIVE_VERSION 1
#define NOOBWARRIOR_FREE_PTR(ptr) delete ptr; ptr = nullptr;
#define NOOBWARRIOR_ARRAY_SIZE(arr) sizeof(arr) / sizeof(arr[0])