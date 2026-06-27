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

};
