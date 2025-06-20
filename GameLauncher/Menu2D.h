// === noobWarrior ===
// File: Menu2D.h
// Started by: Hattozo
// Started on: 6/17/2025
// Description:
#pragma once

#include <blend2d.h>

namespace NoobWarrior {
class Menu2D {
public:
    Menu2D();
    void RedrawImage(int width, int height);

    int GetWidth();
    int GetHeight();
    int GetStride();
    void *GetPixelData();
private:
    BLImage Image;
    BLContext Context;
    BLImageData ImageData;
};
}
