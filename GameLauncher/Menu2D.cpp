// === noobWarrior ===
// File: Menu2D.cpp
// Started by: Hattozo
// Started on: 6/17/2025
// Description:
#include "Menu2D.h"

#include <NoobWarrior/Log.h>

NoobWarrior::Menu2D::Menu2D() :
    Image(1280, 720, BL_FORMAT_PRGB32),
    Context(Image)
{}

void NoobWarrior::Menu2D::RedrawImage(int width, int height) {
    Context.clearAll();
    // Image.create(width, height, BL_FORMAT_PRGB32);

    BLFontFace face;
    BLResult result = face.createFromFile("gamelauncher/fonts/SourceSansPro-Regular.ttf");
    if (result != BL_SUCCESS) {
        Out("GameLauncher::Menu2D", "Failed to load Source Sans Pro font");
        return;
    }

    BLFont font;
    font.createFromFace(face, 50.0f);

    Context.setFillStyle(BLRgba32(0xFFFFFFFF));
    Context.fillUtf8Text(BLPoint(60, 80), font, "noobWarrior");

    Context.rotate(0.785398);
    Context.fillUtf8Text(BLPoint(250, 80), font, "I am rotating");

    Context.end();

    Image.getData(&ImageData);
}

int NoobWarrior::Menu2D::GetWidth() {
    return Image.width();
}

int NoobWarrior::Menu2D::GetHeight() {
    return Image.height();
}

int NoobWarrior::Menu2D::GetStride() {
    return ImageData.stride;
}

void * NoobWarrior::Menu2D::GetPixelData() {
    return ImageData.pixelData;
}
