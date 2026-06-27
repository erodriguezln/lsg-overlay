#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <string>
#include <exception>

#include "main.h"
#include "ui.h"

// TODO: Evaluar si es necesario usar algo como subclass, aun no estoy seguro si al cerrar un juego y forzar el cierre de la overlay si esta se destruye correctamente.

static constexpr int WIDTH = 1280, HEIGHT = 800;


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


    // Inicializa la overlay con la ventana SDL y la ventana objetivo
    if (state->overlay.init(state->window, targetHwnd)) {
        // TODO: Podria encapsular esto
        if (state->overlay.attachToTargetWindow()) {
            state->overlay.resizeToTargetClientArea();
            state->overlay.applyVisibility();
            SDL_Log("Overlay attached successfully.");
        } else {
            SDL_Log("Failed to attach overlay to target window.");
            return SDL_APP_FAILURE;
        }
    } else {
        SDL_Log("Failed to initialize overlay.");
        return SDL_APP_FAILURE;
    }

    state->renderer = SDL_CreateRenderer(state->window, nullptr);
    if (!state->renderer) {
        SDL_Log("Couldn't create renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    // SDL_SetRenderVSync(state->renderer, 1);
    // SDL_SetWindowPosition(state->window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    // SDL_ShowWindow(state->window);

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

    // TODO: Podria encapsular esto
    static bool f3WasDown = false;
    // Detecta si la tecla F3 está presionada actualmente
    const bool f3IsDown = (GetAsyncKeyState(VK_F3) & 0x8000) != 0;
    if (f3IsDown && !f3WasDown) {
        state->overlay.toggleVisibility();
        SDL_Log("F3 pressed: toggling overlay visibility.");
    }
    //
    f3WasDown = f3IsDown;

    // TODO: Podria encapsular esto
    if (state->overlay.isAttached()) {
        if (!state->overlay.isTargetWindowValid()) {
            SDL_Log("Target window is no longer valid. Exiting overlay.");
            return SDL_APP_SUCCESS;
        }

        state->overlay.resizeToTargetClientArea();
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
