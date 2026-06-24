#pragma once

#include <SDL3/SDL.h>

enum class AppStateFlow {
    START,
    PLAYING,
    PAUSED,
    GAME_OVER
};

class App {
public:
    App();
    ~App() = default;

    void handleEvent(const SDL_Event &event);
    void update(float delta_time);
    void render(SDL_Renderer *renderer, int window_width, int window_height);
    void reset();

private:
    AppStateFlow currentState;
};
