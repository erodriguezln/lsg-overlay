#pragma once

#include <SDL3/SDL.h>
#include <network.h>

#include "overlay.h"

// AppState holds every object that must survive between SDL callbacks.
struct AppState {
    SDL_Window *window = nullptr;
    SDL_Renderer *renderer = nullptr;

    Network network;

    Overlay overlay;

    // Constructor que inicializa el objeto Network con el player_id y videogame_id proporcionados
    AppState(int playerId, int videogameId) : network(playerId, videogameId) {
    }

    // Demo window toggle — press the button in the overlay to show/hide it.
    bool show_demo_window = false;

    // Example state for the custom overlay panel.
    float clear_color[3] = {0.117f, 0.117f, 0.157f}; // matches the original dark bg
    int frame_count = 0;

    Uint64 last_time = 0;
    float delta_time = 0.0f;
};
