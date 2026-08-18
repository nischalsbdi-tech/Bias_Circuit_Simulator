#include "raylib.h"
#include "Circuit.hpp"
#include "AppState.hpp"
#include "InputHandler.hpp"
#include "Sidebar.hpp"

// Define the global font (used by all drawing functions)
Font g_font;
Font g_fontBold;

int main() {
    const int SCREEN_WIDTH = 1600;
    const int SCREEN_HEIGHT = 900;
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Electronic Circuit Design and Simulation Studio");
    SetTargetFPS(60);

    // Load custom font (e.g., Roboto-Regular.ttf)
  g_font = LoadFont("LiberationSans-Regular.ttf");
    g_fontBold = LoadFont("LiberationSans-Bold.ttf");
    if (g_font.texture.id == 0) g_font = GetFontDefault();
    if (g_fontBold.texture.id == 0) g_fontBold = g_font;

    Circuit circuit;
    AppState state;

    while (!WindowShouldClose()) {
        // ---- simulate ----
        if (circuit.isRunning) {
            for (int i = 0; i < 5; ++i) circuit.stepSimulation(0.001);
        } else {
            circuit.stepSimulation(0.0);
        }

        // ---- input ----
        processInput(circuit, state);
        circuit.scope.handleDrag();

        // ---- draw ----
        BeginDrawing();
        ClearBackground(RAYWHITE);

        for (int x = 240; x < SCREEN_WIDTH; x += 20)
            for (int y = 0; y < SCREEN_HEIGHT; y += 20)
                DrawPixel(x, y, LIGHTGRAY);

        for (size_t i = 0; i < circuit.wires.size(); ++i) {
            if (static_cast<int>(i) == state.selectedWireIdx) {
                DrawLineEx(circuit.wires[i].startPos, circuit.wires[i].endPos, 4.0f, BLUE);
                DrawCircleV(circuit.wires[i].startPos, 4, BLUE);
                DrawCircleV(circuit.wires[i].endPos, 4, BLUE);
            } else {
                circuit.wires[i].draw();
            }
        }

        if (state.isWiring) DrawLineEx(state.wireStartPos, snapVector(GetMousePosition()), 2, RED);
        for (const auto& comp : circuit.components) circuit.drawComponent(*comp);

        circuit.drawNodeLabels(circuit.isRunning);
        circuit.scope.draw();

        if (circuit.isRunning) {
            DrawNodeTable(circuit, Rectangle{ 250, SCREEN_HEIGHT - 190, 260, 180 });
        }

        drawSidebar(circuit, state, SCREEN_HEIGHT);

        EndDrawing();
    }

    // Unload font if it was loaded
    if (g_font.texture.id != 0 && g_font.texture.id != GetFontDefault().texture.id) {
        UnloadFont(g_font);
    }

    CloseWindow();
    return 0;
}