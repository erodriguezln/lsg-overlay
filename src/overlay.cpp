#include "overlay.h"
#include <SDL3/SDL.h>

bool Overlay::init(SDL_Window *sdlWindow, HWND targetWindowHwnd) {
    if (!sdlWindow) {
        SDL_Log("SDL window is null.");
        return false;
    }

    window = sdlWindow;
    targetHwnd = targetWindowHwnd;

    // Obtiene el HWND de la ventana SDL y lo almacena en el estado de la aplicación
    selfHwnd = static_cast<HWND>(SDL_GetPointerProperty(SDL_GetWindowProperties(sdlWindow),
                                                        SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr));

    if (!selfHwnd || !IsWindow(selfHwnd)) {
        SDL_Log("SDL window handle is invalid.");
        return false;
    }

    if (!targetWindowIsValid()) {
        SDL_Log("Target window not found or invalid.");
        return false;
    }

    return true;
}

bool Overlay::attach() {
    if (attached) {
        SDL_Log("Overlay is already attached.");
        return true;
    }

    if (!targetHwnd || !IsWindow(targetHwnd)) {
        SDL_Log("Target window not found or invalid.");
        return false;
    }

    if (!selfHwnd || !IsWindow(selfHwnd)) {
        SDL_Log("SDL window handle is invalid.");
        return false;
    }

    return attachSdlWindowAsChild();
}

void Overlay::resize() const {
    if (!targetHwnd || !IsWindow(targetHwnd)) {
        SDL_Log("Target window not found or invalid.");
        return;
    }

    if (!selfHwnd || !IsWindow(selfHwnd)) {
        SDL_Log("SDL window handle is invalid.");
        return;
    }

    if (!attached) {
        SDL_Log("SDL window is not attached to the target window.");
        return;
    }

    if (!overlayVisible) {
        return;
    }

    RECT targetRect{};
    GetClientRect(targetHwnd, &targetRect);

    const int targetWidth = targetRect.right - targetRect.left;
    const int targetHeight = targetRect.bottom - targetRect.top;
    if (targetWidth <= 0 || targetHeight <= 0) {
        SDL_Log("Target window has invalid dimensions.");
        return;
    }

    // Se sincroniza el tamaño interno de la ventana SDL con el área cliente de la ventana objetivo
    SDL_SetWindowSize(window, targetWidth, targetHeight);

    // Se posiciona y redimensiona la ventana SDL para cubrir el área cliente de la ventana objetivo
    SetWindowPos(
        selfHwnd,
        HWND_TOP,
        0,
        0,
        targetWidth,
        targetHeight,
        SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_FRAMECHANGED
    );
}

void Overlay::applyVisibility() const {
    if (overlayVisible) {
        // Permite que la ventana SDL reciba eventos de entrada
        EnableWindow(selfHwnd, TRUE);
        // Muestra la ventana SDL sin activarla (no toma el foco)
        ShowWindow(selfHwnd, SW_SHOWNOACTIVATE);
    } else {
        EnableWindow(selfHwnd, FALSE);
        ShowWindow(selfHwnd, SW_HIDE);
    }
}

void Overlay::toggleVisibility() {
    overlayVisible = !overlayVisible;
    applyVisibility();
}

bool Overlay::isAttached() const {
    return attached;
}

bool Overlay::isVisible() const {
    return overlayVisible;
}

bool Overlay::targetWindowIsValid() const {
    return targetHwnd && IsWindow(targetHwnd);
}

bool Overlay::attachSdlWindowAsChild() {
    SetLastError(0);
    // Se obtiene el estilo actual de la ventana SDL
    LONG_PTR style = GetWindowLongPtr(selfHwnd, GWL_STYLE);
    DWORD error = GetLastError();
    if (style == 0 && error != 0) {
        SDL_Log("Failed to get window style. Error code: %lu", error);
        return false;
    }
    // Se eliminan los estilos de ventana independiente y con marco
    style &= ~(WS_POPUP | WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU | WS_BORDER);
    // Se agrega el estilo WS_CHILD para preparar la ventana SDL como ventana hija
    style |= WS_CHILD;

    SetLastError(0);
    // Se establece el nuevo estilo de la ventana SDL
    const LONG_PTR previousStyle = SetWindowLongPtr(selfHwnd, GWL_STYLE, style);
    error = GetLastError();
    if (previousStyle == 0 && error != 0) {
        SDL_Log("Failed to set window style. Error code: %lu", error);
        return false;
    }

    // Se limpia el código de error antes de establecer el nuevo estilo extendido
    SetLastError(0);
    // Se obtiene el estilo extendido actual de la ventana SDL
    LONG_PTR exStyle = GetWindowLongPtr(selfHwnd, GWL_EXSTYLE);
    error = GetLastError();
    if (exStyle == 0 && error != 0) {
        SDL_Log("Failed to get extended window style. Error code: %lu", error);
        return false;
    }

    // Se eliminan los estilos extendidos de ventana independiente, topmost y bordes especiales
    exStyle &= ~(WS_EX_APPWINDOW | WS_EX_TOPMOST | WS_EX_DLGMODALFRAME | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE |
                 WS_EX_WINDOWEDGE);
    // Se agrega el estilo WS_EX_LAYERED para permitir transparencia en la ventana SDL
    exStyle |= WS_EX_LAYERED;

    SetLastError(0);
    // Se establece el nuevo estilo extendido de la ventana SDL
    const LONG_PTR previousExStyle = SetWindowLongPtr(selfHwnd, GWL_EXSTYLE, exStyle);
    error = GetLastError();
    if (previousExStyle == 0 && error != 0) {
        SDL_Log("Failed to set extended window style. Error code: %lu", error);
        return false;
    }

    SetLastError(0);
    // Convierte ventana SDL en CHILD de ventana objetiva
    HWND previousParent = SetParent(selfHwnd, targetHwnd);
    error = GetLastError();
    if (!previousParent && error != 0) {
        SDL_Log("Failed to set parent window. Error code: %lu", error);
        return false;
    }

    SetLastError(0);
    // Se configura la opacidad global de la ventana SDL como completamente opaca
    if (!SetLayeredWindowAttributes(selfHwnd, 0, 255, LWA_ALPHA)) {
        SDL_Log("Failed to set layered window attributes. Error code: %lu", GetLastError());
        return false;
    }

    // Se marca la ventana SDL como adjunta a la ventana objetiva
    attached = true;
    resize();

    return true;
}
