#include "raylib.h"
#include "Circuit.hpp"
#include "UIHelpers.hpp"
#include "Oscilloscope.hpp"

using namespace std;

enum class ToolMode {
    SELECT, RESISTOR, CAPACITOR, INDUCTOR, VOLTAGE_SOURCE,
    LED, AMMETER, AND_GATE, OR_GATE, NOT_GATE, NAND_GATE,
    NOR_GATE, XOR_GATE, XNOR_GATE, GROUND, WIRE, PROBE
};

int main() {
    InitWindow(1280, 720, "Transient Circuit Simulator - Logic & Analog");
    SetTargetFPS(60);

    Circuit circuit;
    ToolMode currentTool = ToolMode::SELECT;
    shared_ptr<Component> selectedComp = nullptr;
    bool isDragging = false, isWiring = false;
    Vector2 wireStartPos = { 0, 0 };

    while (!WindowShouldClose()) {
        Vector2 mousePos = GetMousePosition();

        if (circuit.isRunning) {
            for (int i = 0; i < 5; ++i) circuit.stepSimulation(0.001);
        } else {
            circuit.stepSimulation(0.0);
        }

        if (mousePos.x > 240) {
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                Vector2 snapped = snapVector(mousePos);

                if (currentTool == ToolMode::RESISTOR) circuit.components.push_back(make_shared<Component>(ComponentType::RESISTOR, snapped, 100.0));
                else if (currentTool == ToolMode::CAPACITOR) circuit.components.push_back(make_shared<Component>(ComponentType::CAPACITOR, snapped, 100e-6));
                else if (currentTool == ToolMode::INDUCTOR) circuit.components.push_back(make_shared<Component>(ComponentType::INDUCTOR, snapped, 10e-3));
                else if (currentTool == ToolMode::VOLTAGE_SOURCE) circuit.components.push_back(make_shared<Component>(ComponentType::VOLTAGE_SOURCE, snapped, 5.0));
                else if (currentTool == ToolMode::LED) circuit.components.push_back(make_shared<Component>(ComponentType::LED, snapped, 0.0));
                else if (currentTool == ToolMode::AMMETER) circuit.components.push_back(make_shared<Component>(ComponentType::AMMETER, snapped, 0.0));
                else if (currentTool == ToolMode::AND_GATE) circuit.components.push_back(make_shared<Component>(ComponentType::AND_GATE, snapped, 0.0));
                else if (currentTool == ToolMode::OR_GATE) circuit.components.push_back(make_shared<Component>(ComponentType::OR_GATE, snapped, 0.0));
                else if (currentTool == ToolMode::NOT_GATE) circuit.components.push_back(make_shared<Component>(ComponentType::NOT_GATE, snapped, 0.0));
                else if (currentTool == ToolMode::NAND_GATE) circuit.components.push_back(make_shared<Component>(ComponentType::NAND_GATE, snapped, 0.0));
                else if (currentTool == ToolMode::NOR_GATE) circuit.components.push_back(make_shared<Component>(ComponentType::NOR_GATE, snapped, 0.0));
                else if (currentTool == ToolMode::XOR_GATE) circuit.components.push_back(make_shared<Component>(ComponentType::XOR_GATE, snapped, 0.0));
                else if (currentTool == ToolMode::XNOR_GATE) circuit.components.push_back(make_shared<Component>(ComponentType::XNOR_GATE, snapped, 0.0));
                else if (currentTool == ToolMode::GROUND) circuit.components.push_back(make_shared<Component>(ComponentType::GROUND, snapped, 0.0));
                else if (currentTool == ToolMode::WIRE) {
                    if (!isWiring) { wireStartPos = snapped; isWiring = true; }
                    else {
                        if (snapped.x != wireStartPos.x || snapped.y != wireStartPos.y)
                            circuit.wires.push_back(Wire{ wireStartPos, snapped });
                        isWiring = false;
                    }
                }
                else if (currentTool == ToolMode::PROBE) {
                    PointKey key = makePointKey(snapped);
                    if (circuit.pointToNodeMap.find(key) != circuit.pointToNodeMap.end()) {
                        circuit.probedNodeId = circuit.pointToNodeMap[key];
                    }
                }
                else if (currentTool == ToolMode::SELECT) {
                    selectedComp = nullptr;
                    for (auto& c : circuit.components) c->selected = false;
                    for (auto& c : circuit.components) {
                        if (CheckCollisionPointRec(mousePos, c->getBounds())) {
                            selectedComp = c; c->selected = true; isDragging = true; break;
                        }
                    }
                }
            }
        }

        if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) { isWiring = false; currentTool = ToolMode::SELECT; }
        if (isDragging && selectedComp && IsMouseButtonDown(MOUSE_BUTTON_LEFT) && mousePos.x > 240) {
            selectedComp->pos = snapVector(mousePos);
        }
        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) isDragging = false;

        if (IsKeyPressed(KEY_DELETE) || IsKeyPressed(KEY_BACKSPACE)) {
            if (selectedComp) {
                circuit.components.erase(remove(circuit.components.begin(), circuit.components.end(), selectedComp), circuit.components.end());
                selectedComp = nullptr;
            }
        }

        BeginDrawing();
        ClearBackground(RAYWHITE);

        // Graphical improvement: Draw 2x2 rectangles instead of single faint pixels for the grid
        for (int x = 240; x <= 1280; x += 20) {
            for (int y = 0; y <= 720; y += 20) {
                DrawRectangle(x - 1, y - 1, 2, 2, LIGHTGRAY);
            }
        }

        for (const auto& wire : circuit.wires) wire.draw();
        if (isWiring) DrawLineEx(wireStartPos, snapVector(mousePos), 2, RED);
        for (const auto& comp : circuit.components) circuit.drawComponent(*comp);

        for (const auto& pair : circuit.pointToNodeMap) {
            PointKey pt = pair.first;
            int nodeId = pair.second;
            double vVal = (circuit.nodeVoltages.find(nodeId) != circuit.nodeVoltages.end()) ? circuit.nodeVoltages[nodeId] : 0.0;
            stringstream ss; ss << "N" << nodeId << " (" << fixed << setprecision(2) << vVal << "V)";
            DrawRectangle(pt.x - 4, pt.y - 18, MeasureText(ss.str().c_str(), 10) + 8, 14, Color{ 255, 255, 255, 200 });
            DrawText(ss.str().c_str(), pt.x - 2, pt.y - 16, 10, DARKGREEN);
        }

        DrawOscilloscope(circuit, Rectangle{ 900, 530, 360, 170 });

        DrawRectangle(0, 0, 240, 720, Color{ 30, 34, 42, 255 });
        DrawText("LOGIC & ANALOG LAB", 15, 12, 16, RAYWHITE);

        int yPos = 38;
        #define BTN(lbl, mode) if (DrawButton(Rectangle{ 15, (float)yPos, 210, 22 }, lbl, currentTool == mode)) currentTool = mode; yPos += 25;
        BTN("SELECT / DRAG", ToolMode::SELECT);
        BTN("+ RESISTOR", ToolMode::RESISTOR);
        BTN("+ CAPACITOR", ToolMode::CAPACITOR);
        BTN("+ INDUCTOR", ToolMode::INDUCTOR);
        BTN("+ VOLTAGE SRC (5V)", ToolMode::VOLTAGE_SOURCE);
        BTN("+ LED", ToolMode::LED);
        BTN("+ AMMETER", ToolMode::AMMETER);
        BTN("+ AND GATE", ToolMode::AND_GATE);
        BTN("+ OR GATE", ToolMode::OR_GATE);
        BTN("+ NOT GATE", ToolMode::NOT_GATE);
        BTN("+ NAND GATE", ToolMode::NAND_GATE);
        BTN("+ NOR GATE", ToolMode::NOR_GATE);
        BTN("+ XOR GATE", ToolMode::XOR_GATE);
        BTN("+ XNOR GATE", ToolMode::XNOR_GATE);
        BTN("+ GROUND (0V)", ToolMode::GROUND);
        BTN("ADD WIRE", ToolMode::WIRE);
        BTN("PROBE NODE", ToolMode::PROBE);
        #undef BTN

        yPos += 5;
        DrawLine(15, yPos, 225, yPos, GRAY); yPos += 10;

        const char* simBtnLabel = circuit.isRunning ? "PAUSE SIMULATION" : "RUN SIMULATION";
        if (DrawButton(Rectangle{ 15, (float)yPos, 210, 30 }, simBtnLabel, false, circuit.isRunning ? ORANGE : GREEN)) {
            circuit.isRunning = !circuit.isRunning;
        } yPos += 35;

        if (DrawButton(Rectangle{ 15, (float)yPos, 210, 24 }, "CLEAR ALL", false, MAROON)) {
            circuit.clear(); selectedComp = nullptr;
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
