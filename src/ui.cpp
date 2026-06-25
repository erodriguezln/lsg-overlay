#include "ui.h"
#include "main.h"

// ImGui headers live here — only this TU needs to compile them.
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlrenderer3.h"

static void RenderAttribute(const char *name, double value, double max_val, float width) {
    ImGui::Text("%s", name);

    char valStr[32];
    std::snprintf(valStr, sizeof(valStr), "%.2f", value);
    float textWidth = ImGui::CalcTextSize(valStr).x;

    ImGui::SameLine();
    ImGui::SetCursorPosX(width - textWidth - 30.0f);
    ImGui::Text("%s", valStr);

    ImGui::ProgressBar(static_cast<float>(value) / static_cast<float>(max_val), ImVec2(-1, 8.0f), "");
    ImGui::Spacing();
}

void UI_Init(SDL_Window *window, SDL_Renderer *renderer) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    ImGui::StyleColorsDark();

    constexpr float UI_SCALE = 1.5f;
    io.FontGlobalScale = UI_SCALE;

    ImGuiStyle &style = ImGui::GetStyle();
    style.ScaleAllSizes(UI_SCALE);

    style.WindowRounding = 12.0f;
    style.FrameRounding = 6.0f;
    style.GrabRounding = 6.0f;
    style.PopupRounding = 6.0f;
    style.ChildRounding = 8.0f;
    style.ScrollbarRounding = 6.0f;

    style.WindowPadding = ImVec2(20.0f, 20.0f);
    style.FramePadding = ImVec2(10.0f, 8.0f);
    style.ItemSpacing = ImVec2(12.0f, 12.0f);
    style.ItemInnerSpacing = ImVec2(8.0f, 8.0f);

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
    const auto player = state.network.getPlayer();
    const auto playerAttributes = state.network.getPlayerAttributePoints();
    const auto mechanics = state.network.getMechanics();

    // 1. Dear ImGui demo window (toggle from the overlay panel).
    if (state.show_demo_window)
        ImGui::ShowDemoWindow(&state.show_demo_window);

    // 2. Custom overlay panel (top-left corner).
    if (state.show_overlay) {
        // ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);
        // ImGui::SetNextWindowBgAlpha(0.75f);
        // ImGui::Begin("##overlay", &state.show_overlay,
        //              ImGuiWindowFlags_NoDecoration |
        //              ImGuiWindowFlags_AlwaysAutoResize |
        //              ImGuiWindowFlags_NoSavedSettings |
        //              ImGuiWindowFlags_NoFocusOnAppearing |
        //              ImGuiWindowFlags_NoNav |
        //              ImGuiWindowFlags_NoMove);
        //
        //
        //
        // ImGui::Text("SDL3 + Dear ImGui");
        // ImGui::Separator();
        // ImGui::Text("Frame: %d", state.frame_count);
        // ImGui::Text("Delta Time: %f", state.delta_time);
        // ImGui::Text("FPS:   %.1f", ImGui::GetIO().Framerate);
        // ImGui::Separator();
        // ImGui::ColorEdit3("Clear color", state.clear_color);
        // ImGui::Separator();
        // ImGui::Checkbox("Show demo window", &state.show_demo_window);
        //
        // ImGui::End();

        const ImVec2 displaySize = ImGui::GetIO().DisplaySize;
        const float scale = ImGui::GetIO().FontGlobalScale;

        const float padding = 24.0f * scale;
        const float headerH = 60.0f * scale;
        const float bottomH = 40.0f * scale;

        ImGui::SetNextWindowPos(ImVec2(padding, padding), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(displaySize.x - padding * 2, headerH), ImGuiCond_Always);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

        ImGui::Begin("##header", nullptr,
                     ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoSavedSettings);

        // TODO Poner el juego?
        ImGui::SetWindowFontScale(2.0f);
        // if (player.name.empty()) {
        //     ImGui::Text("Cargando jugador...");
        // } else {
        //     ImGui::TextColored(ImVec4(0.45f, 0.55f, 0.65f, 1.0f), player.name.c_str());
        // }
        if (state.info.selfHwnd) {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.45f, 0.55f, 0.65f, 1.0f), "(HWND: %p)", state.info.selfHwnd);
        }

        if (state.info.targetHwnd) {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.45f, 0.55f, 0.65f, 1.0f), "(Target HWND: %p)", state.info.targetHwnd);
        }
        ImGui::SetWindowFontScale(1.0f);

        ImGui::End();
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();

        const float leftPanelWidth = 380.0f * scale;
        const float panelsY = padding + headerH;
        const float panelsH = displaySize.y - panelsY - padding - bottomH;

        // Left Panel: Atributos
        ImGui::SetNextWindowPos(ImVec2(padding, panelsY), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(leftPanelWidth, panelsH), ImGuiCond_Always);

        ImGui::Begin("Atributos", nullptr,
                     ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

        ImGui::TextColored(ImVec4(0.45f, 0.55f, 0.65f, 1.0f), "Balance calculado desde puntos");
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (playerAttributes.empty()) {
            ImGui::Text("Cargando atributos...");
        } else {
            for (const auto &attr: playerAttributes) {
                RenderAttribute(attr.attribute_name->c_str(), attr.balance_ledger, 2000.0, leftPanelWidth);
            }
        }

        ImGui::End();


        // Right Panel: Mecánicas canjeables
        const float rightPanelX = padding * 2 + leftPanelWidth;
        const float rightPanelWidth = displaySize.x - rightPanelX - padding;

        ImGui::SetNextWindowPos(ImVec2(rightPanelX, panelsY), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(rightPanelWidth, panelsH), ImGuiCond_Always);

        ImGui::Begin("Mecánicas canjeables", nullptr,
                     ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

        // // Header row
        char leftText[64];
        std::snprintf(leftText, sizeof(leftText), "Mecánicas totales: %zu", mechanics.size());
        ImGui::TextColored(ImVec4(0.45f, 0.55f, 0.65f, 1.0f), "%s", leftText);
        ImGui::SameLine();

        // Server status
        bool serverOnline = state.network.getIsVideogameServerOnline();
        const char* statusText = serverOnline ? "Videogame Server Online" : "Videogame Server Offline";
        ImVec4 statusColor = serverOnline ? ImVec4(0.0f, 1.0f, 0.5f, 1.0f) : ImVec4(1.0f, 0.3f, 0.3f, 1.0f);
        ImU32 dotColor = serverOnline ? IM_COL32(0, 255, 128, 255) : IM_COL32(255, 64, 64, 255);

        float statusWidth = ImGui::CalcTextSize(statusText).x + 35.0f * scale;
        ImGui::SetCursorPosX(rightPanelWidth - statusWidth);

        // Draw status dot
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        ImVec2 pos = ImGui::GetCursorScreenPos();
        drawList->AddCircleFilled(ImVec2(pos.x + 8.0f * scale, pos.y + 8.0f * scale), 4.0f * scale, dotColor);
        ImGui::Dummy(ImVec2(15.0f * scale, 1.0f)); 
        ImGui::SameLine();

        ImGui::TextColored(statusColor, "%s", statusText);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Child container scrolleable para cards
        if (ImGui::BeginChild("##cards_scroller", ImVec2(0, 0), false, ImGuiWindowFlags_AlwaysUseWindowPadding)) {
            if (mechanics.empty()) {
                ImGui::Text("No hay mecánicas disponibles.");
            } else {
                for (const auto &m: mechanics) {
                    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 8.0f);
                    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.08f, 0.12f, 0.20f, 0.40f));

                    char card_id[64];
                    std::snprintf(card_id, sizeof(card_id), "##card_%d", m.id_modifiable_mechanic);

                    if (ImGui::BeginChild(card_id, ImVec2(0, 115.0f * scale), true, ImGuiWindowFlags_NoScrollbar)) {
                        if (ImGui::BeginTable("##card_table", 2, ImGuiTableFlags_None)) {
                            ImGui::TableSetupColumn("details", ImGuiTableColumnFlags_WidthStretch, 0.82f);
                            ImGui::TableSetupColumn("action", ImGuiTableColumnFlags_WidthStretch, 0.18f);
                            ImGui::TableNextRow();

                            // Columna 1
                            ImGui::TableNextColumn();

                            // Name de la mecanica
                            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
                            ImGui::Text("%s", m.modifiable_mechanic_name.c_str());
                            ImGui::PopStyleColor();

                            ImGui::SameLine();
                            // TODO: Ver cuantos type de badges existen y poner un color distinto para cada uno
                            if (m.modifiable_mechanic_type == "buff") {
                                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.30f, 0.60f, 0.25f));
                                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.30f, 0.70f, 1.0f, 1.0f));
                                ImGui::Button(m.modifiable_mechanic_type->c_str());
                                ImGui::PopStyleColor(2);
                            } else {
                                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.60f, 0.30f, 0.0f, 0.25f));
                                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.60f, 0.30f, 1.0f));
                                ImGui::Button(m.modifiable_mechanic_type->c_str());
                                ImGui::PopStyleColor(2);
                            }

                            // Descripcion de la mecanica
                            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.60f, 0.65f, 0.75f, 1.0f));
                            ImGui::TextWrapped("%s", m.modifiable_mechanic_description
                                                         ? m.modifiable_mechanic_description->c_str()
                                                         : "Sin descripción");
                            ImGui::PopStyleColor();

                            ImGui::Spacing();

                            // Badges row: Costo y Dimension de la mecanica
                            int pointCost = -1;
                            int pointDimensionId = -1;
                            if (m.options.is_object()) {
                                if (m.options.contains("cost_amount")) {
                                    pointCost = m.options["cost_amount"].get<int>();
                                }
                                if (m.options.contains("point_dimension_id")) {
                                    pointDimensionId = m.options["point_dimension_id"].get<int>();
                                }
                            }

                            if (pointCost == -1 || pointDimensionId == -1) {
                                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.40f, 0.20f, 0.0f, 0.20f));
                                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.60f, 0.30f, 1.0f));
                                ImGui::Button("Falta point_dimension_id/cost_amount en options");
                                ImGui::PopStyleColor(2);
                            } else {
                                char costStr[32];
                                std::snprintf(costStr, sizeof(costStr), "Costo %d", pointCost);
                                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.12f, 0.18f, 0.28f, 0.60f));
                                ImGui::Button(costStr);
                                ImGui::PopStyleColor();

                                ImGui::SameLine();
                                char dimensionStr[32];
                                std::snprintf(dimensionStr, sizeof(dimensionStr), "Dimension %d", pointDimensionId);
                                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.12f, 0.18f, 0.28f, 0.60f));
                                ImGui::Button(dimensionStr);
                                ImGui::PopStyleColor();

                            }

                            // Columna 2: Redeem
                            ImGui::TableNextColumn();

                            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 24.0f * scale);

                            // Obtiene el id de recompensa desde options "reward_id"
                            int rewardId = -1;
                            if (m.options.is_object() && m.options.contains("reward_id") && !m.options["reward_id"].is_null()) {
                                rewardId = m.options["reward_id"].get<int>();
                            }

                            bool isAvailable = (pointCost != -1 && pointDimensionId != -1 && rewardId != -1);

                            if (!isAvailable || !serverOnline) {
                                // Botón deshabilitado si no está disponible o el servidor está offline
                                ImGui::BeginDisabled();
                                ImGui::Button("No disponible", ImVec2(-1, 35.0f * scale));
                                ImGui::EndDisabled();

                                // Tooltip indicativo para avisar si el servidor está caído
                                if (isAvailable && !serverOnline && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
                                    ImGui::SetTooltip("Servidor del videojuego desconectado");
                                }
                            } else {
                                // Botón "Redeem" activo
                                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.50f, 0.70f, 0.50f));
                                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 0.60f, 0.90f, 0.70f));

                                char btnLabel[64];
                                std::snprintf(btnLabel, sizeof(btnLabel), "Redeem##%d", m.id_modifiable_mechanic);

                                if (ImGui::Button(btnLabel, ImVec2(-1, 35.0f * scale))) {
                                    state.network.redeem(m.id_modifiable_mechanic_videogame, pointDimensionId, pointCost, rewardId);
                                }

                                ImGui::PopStyleColor(2);
                            }

                            ImGui::EndTable();
                        }
                    }
                    ImGui::EndChild();

                    ImGui::PopStyleColor();
                    ImGui::PopStyleVar();
                    ImGui::Spacing();
                }
            }
        }
        ImGui::EndChild();

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
