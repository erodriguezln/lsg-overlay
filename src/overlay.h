#pragma once

#include <windows.h>

struct SDL_Window;


class Overlay {
public:
    bool init(SDL_Window *sdlWindow, HWND targetWindowHwnd);

    bool attach();

    void resize() const;

    void applyVisibility() const;

    void toggleVisibility();

    bool isAttached() const;

    bool isVisible() const;

    bool targetWindowIsValid() const;

private:
    SDL_Window *window{nullptr};
    HWND selfHwnd = nullptr;
    HWND targetHwnd = nullptr;
    bool attached{false};
    bool overlayVisible{true};

    bool attachSdlWindowAsChild();
};
