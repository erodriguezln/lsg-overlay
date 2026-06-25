#include "ui.h"
#include "main.h"

// ImGui headers live here — only this TU needs to compile them.
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlrenderer3.h"

void UI_Init(SDL_Window *window, SDL_Renderer *renderer) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    ImGui::StyleColorsDark();

    constexpr float UI_SCALE = 1.5f;
    io.FontGlobalScale = UI_SCALE;
    ImGui::GetStyle().ScaleAllSizes(UI_SCALE);

    ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer3_Init(renderer);
}

void UI_ProcessEvent(const SDL_Event *event) {
    ImGui_ImplSDL3_ProcessEvent(event);
}

void UI_BeginFrame() {
    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
}

void UI_BuildWindows(AppState &state) {
    auto mechanics = state.network.getMechanics();
    // 1. Dear ImGui demo window (toggle from the overlay panel).
    if (state.show_demo_window)
        ImGui::ShowDemoWindow(&state.show_demo_window);

    // 2. Custom overlay panel (top-left corner).
    if (state.show_overlay) {
        ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);
        ImGui::SetNextWindowBgAlpha(0.75f);
        ImGui::Begin("##overlay", &state.show_overlay,
                     ImGuiWindowFlags_NoDecoration |
                     ImGuiWindowFlags_AlwaysAutoResize |
                     ImGuiWindowFlags_NoSavedSettings |
                     ImGuiWindowFlags_NoFocusOnAppearing |
                     ImGuiWindowFlags_NoNav |
                     ImGuiWindowFlags_NoMove);

        ImGui::Text("SDL3 + Dear ImGui");
        ImGui::Separator();
        ImGui::Text("Frame: %d", state.frame_count);
        ImGui::Text("Delta Time: %f", state.delta_time);
        ImGui::Text("FPS:   %.1f", ImGui::GetIO().Framerate);
        ImGui::Separator();
        ImGui::ColorEdit3("Clear color", state.clear_color);
        ImGui::Separator();
        ImGui::Checkbox("Show demo window", &state.show_demo_window);

        ImGui::Separator();
        if (mechanics.empty()) {
            ImGui::Text("Cargando mecanicas...");
        } else {
            for (const auto &m: mechanics) {
                ImGui::Text("Mecanica: %s", m.modifiable_mechanic_name.c_str());

            }
        }


        ImGui::End();
    }
}

void UI_Render(SDL_Renderer *renderer) {
    // Finalise the ImGui draw list (no GPU calls yet).
    ImGui::Render();
    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
}

void UI_Shutdown() {
    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
}
