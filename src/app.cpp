#include "app.h"

App::App() : currentState(AppStateFlow::START) {
}

void App::handleEvent(const SDL_Event &event) {
    if (event.type == SDL_EVENT_KEY_DOWN) {
        switch (event.key.scancode) {
            // Movement Controls
            case SDL_SCANCODE_UP:
            case SDL_SCANCODE_W:
                SDL_Log("UP");
                break;
            case SDL_SCANCODE_DOWN:
            case SDL_SCANCODE_S:
                SDL_Log("DOWN");
                break;
            case SDL_SCANCODE_LEFT:
            case SDL_SCANCODE_A:
                SDL_Log("LEFT");
                break;
            case SDL_SCANCODE_RIGHT:
            case SDL_SCANCODE_D:
                SDL_Log("RIGHT");
                break;

            // State Controls
            case SDL_SCANCODE_SPACE:
                SDL_Log("SPACE");
                [[fallthrough]];
            case SDL_SCANCODE_RETURN:
                if (currentState == AppStateFlow::START) {
                    currentState = AppStateFlow::PLAYING;
                } else if (currentState == AppStateFlow::PLAYING) {
                    currentState = AppStateFlow::PAUSED;
                } else if (currentState == AppStateFlow::PAUSED) {
                    currentState = AppStateFlow::PLAYING;
                } else if (currentState == AppStateFlow::GAME_OVER) {
                    reset();
                    currentState = AppStateFlow::PLAYING;
                }
                break;
            default:
                break;
        }
    }
}

void App::update(float delta_time) {
    (void) delta_time;
}

void App::render(SDL_Renderer *renderer, int window_width, int window_height) {
    (void) renderer;
    (void) window_width;
    (void) window_height;
}

void App::reset() {
}
