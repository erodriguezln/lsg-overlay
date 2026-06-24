#pragma once

#include <SDL3/SDL.h>

// Forward declaration — avoids pulling in AppState definition here.
struct AppState;

// Initializes Dear ImGui context, style, and backends.
// Must be called after the SDL window and renderer are created.
void UI_Init(SDL_Window *window, SDL_Renderer *renderer);

// Forwards an SDL event to ImGui. Call at the top of SDL_AppEvent,
// before any game-side event handling.
void UI_ProcessEvent(const SDL_Event *event);

// Begins a new ImGui frame. Call at the top of SDL_AppIterate.
void UI_BeginFrame();

// Builds and submits all ImGui windows for this frame.
void UI_BuildWindows(AppState &state);

// Renders the ImGui draw list via the SDL_Renderer backend.
// Call after your scene is drawn, but before SDL_RenderPresent.
void UI_Render(SDL_Renderer *renderer);

// Shuts down ImGui backends and destroys the context.
// Must be called before destroying the SDL renderer/window.
void UI_Shutdown();
