#include "raylib.h"
#include "Circuit.hpp"

using namespace std;

enum class ToolMode {
    SELECT, RESISTOR, CAPACITOR, INDUCTOR, VOLTAGE_SOURCE,
    LED, AMMETER, SWITCH, TWO_WAY_SWITCH, AND_GATE, OR_GATE, NOT_GATE, NAND_GATE,
    NOR_GATE, XOR_GATE, XNOR_GATE, GROUND, WIRE, PROBE
};

bool isEditableValueType(ComponentType t) {
    return t == ComponentType::RESISTOR || t == ComponentType::CAPACITOR ||
           t == ComponentType::INDUCTOR || t == ComponentType::VOLTAGE_SOURCE;
}

string editUnitSuffix(ComponentType t) {
    switch (t) {
        case ComponentType::RESISTOR: return "Ohm";
        case ComponentType::CAPACITOR: return "uF";
        case ComponentType::INDUCTOR: return "mH";
        case ComponentType::VOLTAGE_SOURCE: return "V";
        default: return "";
    }
}

// Converts the component's internal value into the friendly unit shown/edited in the UI.
string valueToEditString(const shared_ptr<Component>& c) {
    double v = c->value;
    if (c->type == ComponentType::CAPACITOR) v = c->value * 1e6;
    else if (c->type == ComponentType::INDUCTOR) v = c->value * 1e3;
    ostringstream ss;
    ss << v;
    return ss.str();
}

// Parses the edit buffer (in the friendly unit) and, if valid, applies it live to the component.
void applyEditBuffer(const shared_ptr<Component>& c, const string& buf) {
    if (!c || buf.empty() || buf == "-" || buf == ".") return;
    try {
        size_t consumed = 0;
        double v = stod(buf, &consumed);
        if (consumed == 0) return;
        switch (c->type) {
            case ComponentType::RESISTOR:       c->value = max(v, 0.01); break;
            case ComponentType::CAPACITOR:      c->value = max(v, 0.001) * 1e-6; break;
            case ComponentType::INDUCTOR:       c->value = max(v, 0.001) * 1e-3; break;
            case ComponentType::VOLTAGE_SOURCE:  c->value = v; break;
            default: break;
        }
    } catch (...) {}
}

Vector2 snapVector(Vector2 v, float gridStep = 20.0f) {
    return Vector2{
        static_cast<float>(round(v.x / gridStep) * gridStep),
        static_cast<float>(round(v.y / gridStep) * gridStep)
    };
}

bool DrawButton(Rectangle rect, const char* text, bool active = false, Color baseColor = LIGHTGRAY) {
    Vector2 mousePos = GetMousePosition();
    bool hovered = CheckCollisionPointRec(mousePos, rect);
    Color color = active ? SKYBLUE : (hovered ? RAYWHITE : baseColor);
    DrawRectangleRec(rect, color);
    DrawRectangleLinesEx(rect, 2, active ? BLUE : DARKGRAY);
    int textWidth = MeasureText(text, 12);
    DrawText(text, static_cast<int>(rect.x + (rect.width - textWidth) / 2.0f), static_cast<int>(rect.y + (rect.height - 12) / 2.0f), 12, BLACK);
    return hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

void DrawOscilloscope(const Circuit& circuit, Rectangle bounds) {
    DrawRectangleRec(bounds, Color{ 15, 20, 25, 230 });
    DrawRectangleLinesEx(bounds, 2, GREEN);
    DrawText("OSCILLOSCOPE PROBE", static_cast<int>(bounds.x + 10), static_cast<int>(bounds.y + 8), 12, GREEN);

    if (circuit.probedNodeId < 0) {
        DrawText("Click 'PROBE' and select a node", static_cast<int>(bounds.x + 20), static_cast<int>(bounds.y + 70), 11, GRAY);
        return;
    }

    const auto& hist = circuit.oscilloscopeHistory;
    if (hist.size() < 2) return;

    float minV = *min_element(hist.begin(), hist.end()) - 0.5f;
    float maxV = *max_element(hist.begin(), hist.end()) + 0.5f;

    for (size_t i = 1; i < hist.size(); ++i) {
        float x1 = bounds.x + (i - 1) * (bounds.width / 300.0f);
        float y1 = bounds.y + bounds.height - ((hist[i - 1] - minV) / (maxV - minV)) * bounds.height;
        float x2 = bounds.x + i * (bounds.width / 300.0f);
        float y2 = bounds.y + bounds.height - ((hist[i] - minV) / (maxV - minV)) * bounds.height;
        DrawLineEx(Vector2{ x1, y1 }, Vector2{ x2, y2 }, 2, LIME);
    }

    stringstream ss; ss << "Node " << circuit.probedNodeId << ": " << fixed << setprecision(2) << hist.back() << " V";
    DrawText(ss.str().c_str(), static_cast<int>(bounds.x + bounds.width - 130), static_cast<int>(bounds.y + 8), 12, YELLOW);
}

int main() {
    InitWindow(1280, 720, "Transient Circuit Simulator - Logic & Analog");
    SetTargetFPS(60);

    Circuit circuit;
    ToolMode currentTool = ToolMode::SELECT;
    shared_ptr<Component> selectedComp = nullptr;
    int selectedWireIdx = -1;
    bool isDragging = false, isWiring = false;
    Vector2 wireStartPos = { 0, 0 };

    // Switch click-to-toggle tracking (a click that doesn't drag toggles the switch)
    bool pressedOnSwitch = false;
    Vector2 pressPos = { 0, 0 };

    // Value editing state
    bool isEditingValue = false;
    string editBuffer = "";

    while (!WindowShouldClose()) {
        Vector2 mousePos = GetMousePosition();

        if (isEditingValue && (!selectedComp || !isEditableValueType(selectedComp->type))) {
            isEditingValue = false;
        }

        if (circuit.isRunning) {
            for (int i = 0; i < 5; ++i) circuit.stepSimulation(0.001);
        } else {
            circuit.stepSimulation(0.0);
        }

        if (!isEditingValue && mousePos.x > 240) {
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                Vector2 snapped = snapVector(mousePos);

                if (currentTool == ToolMode::RESISTOR) circuit.components.push_back(make_shared<Component>(ComponentType::RESISTOR, snapped, 100.0));
                else if (currentTool == ToolMode::CAPACITOR) circuit.components.push_back(make_shared<Component>(ComponentType::CAPACITOR, snapped, 100e-6));
                else if (currentTool == ToolMode::INDUCTOR) circuit.components.push_back(make_shared<Component>(ComponentType::INDUCTOR, snapped, 10e-3));
                else if (currentTool == ToolMode::VOLTAGE_SOURCE) circuit.components.push_back(make_shared<Component>(ComponentType::VOLTAGE_SOURCE, snapped, 5.0));
                else if (currentTool == ToolMode::LED) circuit.components.push_back(make_shared<Component>(ComponentType::LED, snapped, 0.0));
                else if (currentTool == ToolMode::AMMETER) circuit.components.push_back(make_shared<Component>(ComponentType::AMMETER, snapped, 0.0));
                else if (currentTool == ToolMode::SWITCH) circuit.components.push_back(make_shared<Component>(ComponentType::SWITCH, snapped, 0.0));
                else if (currentTool == ToolMode::TWO_WAY_SWITCH) circuit.components.push_back(make_shared<Component>(ComponentType::TWO_WAY_SWITCH, snapped, 0.0));
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
                    selectedWireIdx = -1;
                    for (auto& c : circuit.components) c->selected = false;

                    // 1. Select Component
                    for (auto& c : circuit.components) {
                        if (CheckCollisionPointRec(mousePos, c->getBounds())) {
                            selectedComp = c; 
                            c->selected = true; 
                            isDragging = true; 
                            pressPos = mousePos;
                            pressedOnSwitch = c->isSwitch();
                            break;
                        }
                    }

                    // 2. Select Wire (if no component clicked)
                    if (!selectedComp) {
                        for (size_t i = 0; i < circuit.wires.size(); ++i) {
                            if (CheckCollisionPointLine(mousePos, circuit.wires[i].startPos, circuit.wires[i].endPos, 8)) {
                                selectedWireIdx = static_cast<int>(i);
                                break;
                            }
                        }
                    }
                }
            }
        }

        if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) { 
            isWiring = false; 
            currentTool = ToolMode::SELECT; 
            selectedWireIdx = -1;
        }

        // Dragging component and dragging attached wires together
        if (isDragging && selectedComp && IsMouseButtonDown(MOUSE_BUTTON_LEFT) && mousePos.x > 240) {
            Vector2 newPos = snapVector(mousePos);
            if (newPos.x != selectedComp->pos.x || newPos.y != selectedComp->pos.y) {
                PointKey kA_old = makePointKey(selectedComp->getTerminalA().pos);
                PointKey kB_old = makePointKey(selectedComp->getTerminalB().pos);
                PointKey kC_old = makePointKey(selectedComp->getTerminalC().pos);

                selectedComp->pos = newPos;

                Vector2 tA_new = selectedComp->getTerminalA().pos;
                Vector2 tB_new = selectedComp->getTerminalB().pos;
                Vector2 tC_new = selectedComp->getTerminalC().pos;

                for (auto& wire : circuit.wires) {
                    PointKey kStart = makePointKey(wire.startPos);
                    PointKey kEnd   = makePointKey(wire.endPos);

                    if (kStart == kA_old)      wire.startPos = tA_new;
                    else if (kStart == kB_old) wire.startPos = tB_new;
                    else if (kStart == kC_old) wire.startPos = tC_new;

                    if (kEnd == kA_old)      wire.endPos = tA_new;
                    else if (kEnd == kB_old) wire.endPos = tB_new;
                    else if (kEnd == kC_old) wire.endPos = tC_new;
                }
            }
        }
        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
            // A click (press+release with little movement) on a switch toggles it;
            // a click-and-drag instead moves it, like every other component.
            if (pressedOnSwitch && selectedComp) {
                float dx = mousePos.x - pressPos.x, dy = mousePos.y - pressPos.y;
                if ((dx * dx + dy * dy) < 36.0f) {
                    if (selectedComp->type == ComponentType::SWITCH) selectedComp->switchOn = !selectedComp->switchOn;
                    else if (selectedComp->type == ComponentType::TWO_WAY_SWITCH) selectedComp->switchPos = !selectedComp->switchPos;
                }
            }
            isDragging = false;
            pressedOnSwitch = false;
        }

        // Delete component OR selected wire (disabled while typing a value)
        if (!isEditingValue && (IsKeyPressed(KEY_DELETE) || IsKeyPressed(KEY_BACKSPACE))) {
            if (selectedComp) {
                circuit.components.erase(remove(circuit.components.begin(), circuit.components.end(), selectedComp), circuit.components.end());
                selectedComp = nullptr;
            } else if (selectedWireIdx >= 0 && selectedWireIdx < static_cast<int>(circuit.wires.size())) {
                circuit.wires.erase(circuit.wires.begin() + selectedWireIdx);
                selectedWireIdx = -1;
            }
        }

        BeginDrawing();
        ClearBackground(RAYWHITE);

        for (int x = 240; x <= 1280; x += 20)
            for (int y = 0; y <= 720; y += 20) DrawPixel(x, y, LIGHTGRAY);

        // Draw wires (highlighting the selected wire in blue)
        for (size_t i = 0; i < circuit.wires.size(); ++i) {
            if (static_cast<int>(i) == selectedWireIdx) {
                DrawLineEx(circuit.wires[i].startPos, circuit.wires[i].endPos, 4.0f, BLUE);
                DrawCircleV(circuit.wires[i].startPos, 4, BLUE);
                DrawCircleV(circuit.wires[i].endPos, 4, BLUE);
            } else {
                circuit.wires[i].draw();
            }
        }

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
        #define BTN(lbl, mode) if (DrawButton(Rectangle{ 15, (float)yPos, 210, 22 }, lbl, currentTool == mode)) { currentTool = mode; selectedWireIdx = -1; isEditingValue = false; } yPos += 25;
        BTN("SELECT / DRAG", ToolMode::SELECT);
        BTN("+ RESISTOR", ToolMode::RESISTOR);
        BTN("+ CAPACITOR", ToolMode::CAPACITOR);
        BTN("+ INDUCTOR", ToolMode::INDUCTOR);
        BTN("+ VOLTAGE SRC (5V)", ToolMode::VOLTAGE_SOURCE);
        BTN("+ LED", ToolMode::LED);
        BTN("+ AMMETER", ToolMode::AMMETER);
        BTN("+ SWITCH", ToolMode::SWITCH);
        BTN("+ TWO-WAY SWITCH", ToolMode::TWO_WAY_SWITCH);
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
            circuit.clear(); selectedComp = nullptr; selectedWireIdx = -1; isEditingValue = false;
        }
        yPos += 34;

        // Edit-value panel: only shows up for components with an editable value
        // (resistor, capacitor, inductor, voltage source). Switches/gates/etc. skip it.
        bool editableSelected = selectedComp && isEditableValueType(selectedComp->type);
        if (editableSelected) {
            DrawLine(15, yPos, 225, yPos, GRAY); yPos += 10;

            const char* editLabel = isEditingValue ? "CONFIRM (ENTER)" : "EDIT VALUE";
            if (DrawButton(Rectangle{ 15, (float)yPos, 210, 26 }, editLabel, isEditingValue, SKYBLUE)) {
                if (!isEditingValue) {
                    isEditingValue = true;
                    editBuffer = valueToEditString(selectedComp);
                } else {
                    applyEditBuffer(selectedComp, editBuffer);
                    isEditingValue = false;
                }
            }
            yPos += 30;

            if (isEditingValue) {
                // Capture typed characters
                int key = GetCharPressed();
                while (key > 0) {
                    bool isDigit = (key >= '0' && key <= '9');
                    bool isDot = (key == '.' && editBuffer.find('.') == string::npos);
                    bool isMinus = (key == '-' && editBuffer.empty() && selectedComp->type == ComponentType::VOLTAGE_SOURCE);
                    if ((isDigit || isDot || isMinus) && editBuffer.size() < 10) editBuffer += static_cast<char>(key);
                    key = GetCharPressed();
                }
                if (IsKeyPressed(KEY_BACKSPACE) && !editBuffer.empty()) editBuffer.pop_back();
                if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) {
                    applyEditBuffer(selectedComp, editBuffer);
                    isEditingValue = false;
                }
                if (IsKeyPressed(KEY_ESCAPE)) isEditingValue = false;

                // Apply live so the component updates on the canvas as you type
                applyEditBuffer(selectedComp, editBuffer);

                Rectangle box{ 15, (float)yPos, 210, 28 };
                DrawRectangleRec(box, RAYWHITE);
                DrawRectangleLinesEx(box, 2, BLUE);
                bool cursorOn = (static_cast<int>(GetTime() * 2.0) % 2) == 0;
                string display = editBuffer + (cursorOn ? "_" : "");
                DrawText(display.c_str(), static_cast<int>(box.x + 8), static_cast<int>(box.y + 7), 14, BLACK);
                string unit = editUnitSuffix(selectedComp->type);
                int uw = MeasureText(unit.c_str(), 12);
                DrawText(unit.c_str(), static_cast<int>(box.x + box.width - uw - 8), static_cast<int>(box.y + 9), 12, DARKGRAY);
                yPos += 32;
            } else {
                stringstream ss;
                ss << "Value: " << valueToEditString(selectedComp) << " " << editUnitSuffix(selectedComp->type);
                DrawText(ss.str().c_str(), 15, yPos, 12, RAYWHITE);
                yPos += 18;
            }
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
