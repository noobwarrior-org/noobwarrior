// === noobWarrior ===
// File: GameLauncher.cpp
// Started by: Hattozo
// Started on: 6/17/2025
// Description:
#include "GameLauncher.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_render.h>
#include <blend2d.h>

#include <cstdio>

using namespace NoobWarrior;

GameLauncher::GameLauncher() :
    Running(true),
    Width(1280), Height(720)
{}

int GameLauncher::Run() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMEPAD | SDL_INIT_JOYSTICK | SDL_INIT_HAPTIC | SDL_INIT_EVENTS) != 1) {
        return 1;
    }

    Menu.RedrawImage(Width, Height);

    SDL_Window *window;
    SDL_Renderer *renderer;

    SDL_CreateWindowAndRenderer("noobWarrior", Width, Height, SDL_WINDOW_RESIZABLE, &window, &renderer);

    SDL_Event event;
    while (Running) {
        SDL_Surface* sdlSurface = SDL_CreateSurfaceFrom(Menu.GetWidth(), Menu.GetHeight(), SDL_PIXELFORMAT_ARGB8888, Menu.GetPixelData(), Menu.GetStride());

        SDL_Texture* sdlTexture = SDL_CreateTextureFromSurface(renderer, sdlSurface);
        SDL_DestroySurface(sdlSurface); // Free the temporary surface

        SDL_FRect destRect = {0, 0, static_cast<float>(Menu.GetWidth()), static_cast<float>(Menu.GetHeight())}; // Position and size
        SDL_RenderTexture(renderer, sdlTexture, nullptr, &destRect);
        SDL_RenderPresent(renderer);

        SDL_DestroyTexture(sdlTexture);

        SDL_PollEvent(&event);
        switch (event.type) {
        case SDL_EVENT_WINDOW_RESIZED:
            Width = event.window.data1;
            Height = event.window.data2;
            break;
        case SDL_EVENT_QUIT:
            Running = false;
            break;
        default: break;
        }
    }
    return 0;
}

int main(int argc, char **argv) {
    GameLauncher app;
    return app.Run();
}