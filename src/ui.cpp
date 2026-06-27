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

    // Colores custom
    ImVec4 *colors = style.Colors;
    colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.08f, 0.12f, 0.85f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.08f, 0.11f, 0.18f, 0.70f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.11f, 0.18f, 0.95f);
    colors[ImGuiCol_Border] = ImVec4(0.12f, 0.20f, 0.35f, 0.60f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.10f, 0.14f, 0.22f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.15f, 0.20f, 0.30f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.18f, 0.25f, 0.38f, 1.00f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.08f, 0.11f, 0.18f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.12f, 0.18f, 0.28f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.08f, 0.11f, 0.18f, 0.50f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.08f, 0.11f, 0.18f, 1.00f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.08f, 0.11f, 0.18f, 0.30f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.15f, 0.22f, 0.35f, 0.80f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.20f, 0.30f, 0.45f, 0.80f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.25f, 0.38f, 0.55f, 0.80f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.00f, 0.70f, 1.00f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.00f, 0.70f, 1.00f, 0.80f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.00f, 0.70f, 1.00f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.00f, 0.45f, 0.70f, 0.40f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.00f, 0.55f, 0.85f, 0.60f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.00f, 0.70f, 1.00f, 0.80f);
    colors[ImGuiCol_Header] = ImVec4(0.12f, 0.18f, 0.28f, 1.00f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.15f, 0.25f, 0.40f, 1.00f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.20f, 0.30f, 0.50f, 1.00f);
    colors[ImGuiCol_Separator] = ImVec4(0.12f, 0.20f, 0.35f, 0.60f);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.15f, 0.25f, 0.45f, 0.80f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.20f, 0.35f, 0.55f, 1.00f);
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.12f, 0.18f, 0.28f, 0.20f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.15f, 0.25f, 0.40f, 0.60f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.20f, 0.30f, 0.50f, 0.90f);
    colors[ImGuiCol_PlotLines] = ImVec4(0.00f, 0.70f, 1.00f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.00f, 0.80f, 1.00f, 1.00f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.00f, 0.65f, 0.95f, 1.00f); // Glowing cyan fill
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.00f, 0.80f, 1.00f, 1.00f);
    colors[ImGuiCol_Text] = ImVec4(0.85f, 0.88f, 0.94f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.45f, 0.50f, 0.60f, 1.00f);

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
    const auto videogame = state.network.getVideogame();

    if (state.overlay.isVisible()) {
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

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
        ImGui::SetWindowFontScale(1.4f);
        if (videogame.name.empty()) {
            ImGui::Text("Cargando juego...");
        } else {
            ImGui::Text("%s", videogame.name.c_str());
        }
        ImGui::SetWindowFontScale(1.0f);
        ImGui::PopStyleColor();

        if (player.name.empty()) {
            ImGui::Text("Cargando jugador...");
        } else {
            ImGui::TextColored(ImVec4(0.45f, 0.55f, 0.65f, 1.0f), "%s", player.name.c_str());
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.45f, 0.55f, 0.65f, 1.0f), " - ");
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.45f, 0.55f, 0.65f, 1.0f), "%s", player.email.c_str());
        }

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
                // Al ser optional puede ser null, en ese caso se muestra "Atributo desconocido" y se evita crash
                const char *attributeName = attr.attribute_name ? attr.attribute_name->c_str() : "Atributo desconocido";

                RenderAttribute(attributeName, attr.balance_ledger, 2000.0, leftPanelWidth);
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
        const char *statusText = serverOnline ? "Videogame Server Online" : "Videogame Server Offline";
        ImVec4 statusColor = serverOnline ? ImVec4(0.0f, 1.0f, 0.5f, 1.0f) : ImVec4(1.0f, 0.3f, 0.3f, 1.0f);
        ImU32 dotColor = serverOnline ? IM_COL32(0, 255, 128, 255) : IM_COL32(255, 64, 64, 255);

        float statusWidth = ImGui::CalcTextSize(statusText).x + 35.0f * scale;
        ImGui::SetCursorPosX(rightPanelWidth - statusWidth);

        // Draw status dot
        ImDrawList *drawList = ImGui::GetWindowDrawList();
        ImVec2 pos = ImGui::GetCursorScreenPos();
        drawList->AddCircleFilled(ImVec2(pos.x + 8.0f * scale, pos.y + 8.0f * scale), 4.0f * scale, dotColor);
        ImGui::Dummy(ImVec2(15.0f * scale, 1.0f));
        ImGui::SameLine();

        ImGui::TextColored(statusColor, "%s", statusText);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Child container scrolleable para cards
        if (ImGui::BeginChild("##cards_scroller", ImVec2(0, 0), ImGuiChildFlags_AlwaysUseWindowPadding)) {
            if (mechanics.empty()) {
                ImGui::Text("No hay mecánicas disponibles.");
            } else {
                for (const auto &m: mechanics) {
                    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 8.0f);
                    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.08f, 0.12f, 0.20f, 0.40f));

                    char card_id[64];
                    std::snprintf(card_id, sizeof(card_id), "##card_%d", m.id_modifiable_mechanic);

                    if (ImGui::BeginChild(card_id, ImVec2(0, 125.0f * scale), ImGuiChildFlags_Borders,
                                          ImGuiWindowFlags_NoScrollWithMouse)) {
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
                            // Si no hay type, se pone "unknown"
                            const std::string mechanicType = m.modifiable_mechanic_type.value_or("unknown");

                            if (mechanicType == "buff") {
                                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.30f, 0.60f, 0.25f));
                                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.30f, 0.70f, 1.0f, 1.0f));
                                ImGui::Button(mechanicType.c_str());
                                ImGui::PopStyleColor(2);
                            } else {
                                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.60f, 0.30f, 0.0f, 0.25f));
                                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.60f, 0.30f, 1.0f));
                                ImGui::Button(mechanicType.c_str());
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
                            if (m.options.is_object() && m.options.contains("reward_id") && !m.options["reward_id"].
                                is_null()) {
                                rewardId = m.options["reward_id"].get<int>();
                            }

                            bool isAvailable = (pointCost != -1 && pointDimensionId != -1 && rewardId != -1);

                            if (!isAvailable || !serverOnline) {
                                // Botón deshabilitado si no está disponible o el servidor está offline
                                ImGui::BeginDisabled();
                                ImGui::Button("No disponible", ImVec2(-1, 35.0f * scale));
                                ImGui::EndDisabled();

                                // Tooltip indicativo para avisar si el servidor está caído
                                if (isAvailable && !serverOnline && ImGui::IsItemHovered(
                                        ImGuiHoveredFlags_AllowWhenDisabled)) {
                                    ImGui::SetTooltip("Servidor del videojuego desconectado");
                                }
                            } else {
                                // Botón "Redeem" activo
                                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.50f, 0.70f, 0.50f));
                                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 0.60f, 0.90f, 0.70f));

                                char btnLabel[64];
                                std::snprintf(btnLabel, sizeof(btnLabel), "Redeem##%d", m.id_modifiable_mechanic);

                                if (ImGui::Button(btnLabel, ImVec2(-1, 35.0f * scale))) {
                                    state.network.redeem(m.id_modifiable_mechanic_videogame, pointDimensionId,
                                                         pointCost, rewardId);
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
