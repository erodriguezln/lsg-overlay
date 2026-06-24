#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "main.h"
#include "ui.h"

static constexpr int WIDTH = 1280, HEIGHT = 800;

SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv) {
    (void) argc;
    (void) argv;
    SDL_SetAppMetadata(APP_NAME, "1.0", "com.example.sdl3-boilerplate");

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    auto *state = new AppState();
    *appstate = state;

    // Window & Renderer
    SDL_WindowFlags window_flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN;
    state->window = SDL_CreateWindow(APP_NAME, WIDTH, HEIGHT, window_flags);
    if (!state->window) {
        SDL_Log("Couldn't create window: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    state->renderer = SDL_CreateRenderer(state->window, nullptr);
    if (!state->renderer) {
        SDL_Log("Couldn't create renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    SDL_SetRenderVSync(state->renderer, 1);
    SDL_SetWindowPosition(state->window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    SDL_ShowWindow(state->window);

    state->last_time = SDL_GetTicksNS();

    UI_Init(state->window, state->renderer);

    if (state->network.fetchMechanics()) {
        state->network.printMechanics();
    }

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

    if (event->type == SDL_EVENT_KEY_DOWN) {
        if (event->key.scancode == SDL_SCANCODE_ESCAPE) {
            return SDL_APP_SUCCESS;
        }
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

    UI_BeginFrame();
    UI_BuildWindows(*state);

    // Render
    SDL_SetRenderDrawColorFloat(state->renderer,
                                state->clear_color[0],
                                state->clear_color[1],
                                state->clear_color[2],
                                1.0f);
    SDL_RenderClear(state->renderer);

    // render game/scene before imgui overlay
    int w, h;
    SDL_GetRenderOutputSize(state->renderer, &w, &h);

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
