// === noobWarrior ===
// File: IdRecord.h
// Started by: Hattozo
// Started on: N/A
// Description:
#pragma once
#include "Record.h"
#include "../ContentImages.h"

namespace NoobWarrior {
struct IdRecord : Record {
    int64_t             Id;
    int                 Version = 1;
    std::string         Name;

    static constexpr const int* DefaultImage = g_icon_content_deleted;
    inline static const int DefaultImageSize = g_icon_content_deleted_size;
};
}
