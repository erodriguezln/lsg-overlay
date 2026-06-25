#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <string>
#include <exception>

#include "main.h"
#include "ui.h"

// TODO: Evaluar si es necesario usar algo como subclass, aun no estoy seguro si al cerrar un juego y forzar el cierre de la overlay si esta se destruye correctamente.

static constexpr int WIDTH = 1280, HEIGHT = 800;

void setOverlayVisibility(AppState *state) {
    if (state->show_overlay) {
        // Permite que la ventana SDL reciba eventos de entrada
        EnableWindow(state->info.selfHwnd, TRUE);
        // Muestra la ventana SDL sin activarla (no toma el foco)
        ShowWindow(state->info.selfHwnd, SW_SHOWNOACTIVATE);
    } else {
        EnableWindow(state->info.selfHwnd, FALSE);
        ShowWindow(state->info.selfHwnd, SW_HIDE);
    }
}

void resizeSdlWindow(AppState *state) {
    if (!state->info.targetHwnd || !IsWindow(state->info.targetHwnd)) {
        SDL_Log("Target window not found or invalid.");
        return;
    }

    if (!state->info.selfHwnd || !IsWindow(state->info.selfHwnd)) {
        SDL_Log("SDL window handle is invalid.");
        return;
    }

    if (!state->info.attached) {
        SDL_Log("SDL window is not attached to the target window.");
        return;
    }

    if (!state->show_overlay) {
        return;
    }

    RECT targetRect{};
    GetClientRect(state->info.targetHwnd, &targetRect);

    const int targetWidth = targetRect.right - targetRect.left;
    const int targetHeight = targetRect.bottom - targetRect.top;
    if (targetWidth <= 0 || targetHeight <= 0) {
        SDL_Log("Target window has invalid dimensions.");
        return;
    }

    // Se sincroniza el tamaño interno de la ventana SDL con el área cliente de la ventana objetivo
    SDL_SetWindowSize(state->window, targetWidth, targetHeight);

    // Se posiciona y redimensiona la ventana SDL para cubrir el área cliente de la ventana objetivo
    SetWindowPos(
        state->info.selfHwnd,
        HWND_TOP,
        0,
        0,
        targetWidth,
        targetHeight,
        SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_FRAMECHANGED
    );
}

static bool attachSdlWindowAsChild(AppState *state) {
    // Se obtiene el estilo actual de la ventana SDL
    LONG_PTR style = GetWindowLongPtr(state->info.selfHwnd, GWL_STYLE);
    // Se eliminan los estilos de ventana independiente y con marco
    style &= ~(WS_POPUP | WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU | WS_BORDER);
    // Se agrega el estilo WS_CHILD para preparar la ventana SDL como ventana hija
    style |= WS_CHILD;
    // Se establece el nuevo estilo de la ventana SDL
    SetWindowLongPtr(state->info.selfHwnd, GWL_STYLE, style);

    // Se obtiene el estilo extendido actual de la ventana SDL
    LONG_PTR exStyle = GetWindowLongPtr(state->info.selfHwnd, GWL_EXSTYLE);
    // Se eliminan los estilos extendidos de ventana independiente, topmost y bordes especiales
    exStyle &= ~(WS_EX_APPWINDOW | WS_EX_TOPMOST | WS_EX_DLGMODALFRAME | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE |
                 WS_EX_WINDOWEDGE);
    // Se agrega el estilo WS_EX_LAYERED para permitir transparencia en la ventana SDL
    exStyle |= WS_EX_LAYERED;
    // Se establece el nuevo estilo extendido de la ventana SDL
    SetWindowLongPtr(state->info.selfHwnd, GWL_EXSTYLE, exStyle);

    // Convierte ventana SDL en CHILD de ventana objetiva
    SetParent(state->info.selfHwnd, state->info.targetHwnd);

    // Se configura la opacidad global de la ventana SDL como completamente opaca
    SetLayeredWindowAttributes(state->info.selfHwnd, 0, 255, LWA_ALPHA);

    // Se marca la ventana SDL como adjunta a la ventana objetivo
    state->info.attached = true;

    resizeSdlWindow(state);

    return true;
}

bool attachOverlay(AppState *state) {
    if (state->info.attached) {
        SDL_Log("Overlay is already attached.");
        return true;
    }

    if (!state->info.targetHwnd || !IsWindow(state->info.targetHwnd)) {
        SDL_Log("Target window not found or invalid.");
        return false;
    }

    if (!state->info.selfHwnd || !IsWindow(state->info.selfHwnd)) {
        SDL_Log("SDL window handle is invalid.");
        return false;
    }

    return attachSdlWindowAsChild(state);
}


SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv) {
    SDL_SetAppMetadata(APP_NAME, "1.0", "com.example.sdl3-boilerplate");

    if (argc < 4) {
        SDL_Log("Error: Missing required arguments.");
        return SDL_APP_FAILURE;
    }

    int playerId = 0;
    HWND targetHwnd = nullptr;
    int videogameId = 0;

    try {
        playerId = std::stoi(argv[1]);
        unsigned long long hwndVal = std::stoull(argv[2], nullptr, 0);
        targetHwnd = reinterpret_cast<HWND>(hwndVal);
        videogameId = std::stoi(argv[3]);
    } catch (const std::exception &e) {
        SDL_Log("Error: Invalid arguments: %s", e.what());
        SDL_Log("Usage: %s <playerId> <targetHWND> <videogameId>", argv[0]);
        return SDL_APP_FAILURE;
    }

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    auto *state = new AppState(playerId, videogameId);

    *appstate = state;

    // Window & Renderer
    SDL_WindowFlags window_flags =
            SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN | SDL_WINDOW_BORDERLESS | SDL_WINDOW_TRANSPARENT;

    //TODO: podria crear con tamanio de videojuego o 0, 0 revisar.
    state->window = SDL_CreateWindow(APP_NAME, WIDTH, HEIGHT, window_flags);
    if (!state->window) {
        SDL_Log("Couldn't create window: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    // Obtiene el HWND de la ventana SDL y lo almacena en el estado de la aplicación
    state->info.selfHwnd = static_cast<HWND>(SDL_GetPointerProperty(SDL_GetWindowProperties(state->window),
                                                                    SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr));
    state->info.targetHwnd = targetHwnd;

    if (state->info.targetHwnd && IsWindow(state->info.targetHwnd)) {
        if (attachOverlay(state)) {
            setOverlayVisibility(state);
            SDL_Log("Overlay attached successfully.");
        } else {
            SDL_Log("Failed to attach overlay.");
        }
    }

    state->renderer = SDL_CreateRenderer(state->window, nullptr);
    if (!state->renderer) {
        SDL_Log("Couldn't create renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    // SDL_SetRenderVSync(state->renderer, 1);
    SDL_SetWindowPosition(state->window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    SDL_ShowWindow(state->window);

    state->last_time = SDL_GetTicksNS();

    UI_Init(state->window, state->renderer);

    return SDL_APP_CONTINUE;
}

// Called for every OS / input event.
SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
    auto *state = static_cast<AppState *>(appstate);
    (void) state; // Available for future use (e.g. pausing ImGui input on game events)

    // Forward ALL events to ImGui first so it can consume mouse/keyboard when hovering its windows.
    UI_ProcessEvent(event);

    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;
    }

    return SDL_APP_CONTINUE;
}

// Called once per frame: update game logic + render everything.
SDL_AppResult SDL_AppIterate(void *appstate) {
    auto *state = static_cast<AppState *>(appstate);
    state->frame_count++;

    const Uint64 now = SDL_GetTicksNS();
    state->delta_time = static_cast<float>(now - state->last_time) * 1e-9f;
    state->last_time = now;

    static bool f3WasDown = false;
    // Detecta si la tecla F3 está presionada actualmente
    const bool f3IsDown = (GetAsyncKeyState(VK_F3) & 0x8000) != 0;
    if (f3IsDown && !f3WasDown) {
        state->show_overlay = !state->show_overlay;
        setOverlayVisibility(state);
        SDL_Log("F3 pressed: toggling overlay visibility.");
    }
    //
    f3WasDown = f3IsDown;

    if (state->info.attached) {
        if (!IsWindow(state->info.targetHwnd)) {
            SDL_Log("Target window is no longer valid. Exiting overlay.");
            return SDL_APP_SUCCESS;
        }

        // TODO: Hacer que se redimensione solo si cambia el tamaño de la ventana objetivo
        resizeSdlWindow(state);
    }

    UI_BeginFrame();
    UI_BuildWindows(*state);

    // Render
    // Se limpia la ventana SDL con un color oscuro semitransparente;
    // el alpha se mezcla con la ventana objetivo mediante DWM
    SDL_SetRenderDrawColor(state->renderer, 15, 15, 15, 100);
    SDL_RenderClear(state->renderer);

    UI_Render(state->renderer);
    SDL_RenderPresent(state->renderer);

    return SDL_APP_CONTINUE;
}

// Called once when the app is closing (regardless of success/failure).
void SDL_AppQuit(void *appstate, SDL_AppResult result) {
    (void) result;
    // Shutdown ImGui backends before destroying the SDL objects they reference.
    UI_Shutdown();

    if (auto *state = static_cast<AppState *>(appstate)) {
        if (state->renderer) {
            SDL_DestroyRenderer(state->renderer);
        }
        if (state->window) {
            SDL_DestroyWindow(state->window);
        }
        delete state;
    }
    SDL_Quit();
}
