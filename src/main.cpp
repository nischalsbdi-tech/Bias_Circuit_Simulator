#include "raylib.h"
#include "Circuit.hpp"
#include "AppState.hpp"
#include "InputHandler.hpp"
#include "Sidebar.hpp"

int main() {
    const int SCREEN_WIDTH = 1600;
    const int SCREEN_HEIGHT = 900;
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Transient Circuit Simulator - Logic & Analog");
    SetTargetFPS(60);

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

        // on-canvas node labels: just "N1" while building, "N1 5.00V" (bold green) while running
        circuit.drawNodeLabels(circuit.isRunning);

        // minimal, draggable oscilloscope - only appears when toggled on from the sidebar
        circuit.scope.draw();

        // node voltage table - only meaningful once the simulation is running
        if (circuit.isRunning) {
            DrawNodeTable(circuit, Rectangle{ 250, SCREEN_HEIGHT - 190, 260, 180 });
        }

        drawSidebar(circuit, state, SCREEN_HEIGHT);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
