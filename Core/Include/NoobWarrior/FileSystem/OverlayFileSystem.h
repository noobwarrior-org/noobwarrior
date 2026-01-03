// === noobWarrior ===
// File: OverlayFileSystem.h
// Started by: Hattozo
// Started on: 12/5/2025
// Description: An psuedo file system that overlays each file system over one another.
#pragma once
#include "VirtualFileSystem.h"

namespace NoobWarrior {
class OverlayFileSystem {
public:
    enum class Response {
        Failed,
        Success
    };

    Response Mount(const VirtualFileSystem &fs);
    Response Unmount(const VirtualFileSystem &fs);
};
}