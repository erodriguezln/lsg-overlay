#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "main.h"
#include "ui.h"

static HWND FindTargetWindow() {
    struct EnumData {
        HWND foundHwnd = nullptr;
    } data;

    EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL {
        auto *data = reinterpret_cast<EnumData *>(lParam);

        if (!IsWindowVisible(hwnd)) {
            return TRUE;
        }

        if (GetWindow(hwnd, GW_OWNER) != nullptr) {
            return TRUE;
        }

        wchar_t title[256]{};
        if (GetWindowTextW(hwnd, title, 256) <= 0) {
            return TRUE;
        }

        if (wcsstr(title, L"Stardew Valley") == nullptr) {
            return TRUE;
        }

        RECT rect{};
        if (!GetWindowRect(hwnd, &rect)) {
            return TRUE;
        }

        int width = rect.right - rect.left;
        int height = rect.bottom - rect.top;

        if (width < 200 || height < 200) {
            return TRUE;
        }

        data->foundHwnd = hwnd;
        return FALSE;
    }, reinterpret_cast<LPARAM>(&data));

    return data.foundHwnd;
}

static constexpr int WIDTH = 1280, HEIGHT = 800;

SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv) {
    (void) argc;
    (void) argv;
    SDL_SetAppMetadata(APP_NAME, "1.0", "com.example.sdl3-boilerplate");

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    // TODO: Tengo que pasarlo de forma dinamica
    int TEMP_PLAYER_ID = 41;
    auto *state = new AppState(TEMP_PLAYER_ID);

    *appstate = state;

    // Window & Renderer
    SDL_WindowFlags window_flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN | SDL_WINDOW_BORDERLESS | SDL_WINDOW_TRANSPARENT;
    state->window = SDL_CreateWindow(APP_NAME, WIDTH, HEIGHT, window_flags);
    if (!state->window) {
        SDL_Log("Couldn't create window: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    // Obtiene el HWND de la ventana SDL y lo almacena en el estado de la aplicación
    state->info.selfHwnd = static_cast<HWND>(SDL_GetPointerProperty(SDL_GetWindowProperties(state->window),
                                                                    SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr));
    // TODO: Temporal, esto debera ser dinamnico...
    state->info.targetHwnd = FindTargetWindow();

    if (state->info.targetHwnd && IsWindow(state->info.targetHwnd)) {
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
        exStyle &= ~(WS_EX_APPWINDOW | WS_EX_TOPMOST | WS_EX_DLGMODALFRAME | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE | WS_EX_WINDOWEDGE);
        // Se agrega el estilo WS_EX_LAYERED para permitir transparencia en la ventana SDL
        exStyle |= WS_EX_LAYERED;
        // Se establece el nuevo estilo extendido de la ventana SDL
        SetWindowLongPtr(state->info.selfHwnd, GWL_EXSTYLE, exStyle);

        // Convierte ventana SDL en CHILD de ventana objetiva
        SetParent(state->info.selfHwnd, state->info.targetHwnd);

        SetLayeredWindowAttributes(state->info.selfHwnd, 0, 255, LWA_ALPHA);

        // Hace que la ventana SDL sea transparente y click-through
        SetWindowPos(
            state->info.selfHwnd,
            HWND_TOPMOST,
            0,
            0,
            0,
            0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_FRAMECHANGED
        );

        // Muestra la ventana SDL sin robar el foco de la ventana objetivo
        ShowWindow(state->info.selfHwnd, SW_SHOWNOACTIVATE);
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
    SDL_SetRenderDrawColor(state->renderer, 15, 15, 15, 100);
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
