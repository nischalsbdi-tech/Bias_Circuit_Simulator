#include "raylib.h"
#include "Circuit.hpp"
#include <algorithm>
#include <vector>
#include <sstream>

using namespace std;

enum class ToolMode {
    SELECT, RESISTOR, CAPACITOR, INDUCTOR, VOLTAGE_SOURCE,
    LED, AMMETER, SWITCH, TWO_WAY_SWITCH, AND_GATE, OR_GATE, NOT_GATE, NAND_GATE,
    NOR_GATE, XOR_GATE, XNOR_GATE, GROUND, PROBE, ROTATE
};

bool isPlaceableTool(ToolMode t) {
    switch (t) {
        case ToolMode::RESISTOR: case ToolMode::CAPACITOR: case ToolMode::INDUCTOR:
        case ToolMode::VOLTAGE_SOURCE: case ToolMode::LED: case ToolMode::AMMETER:
        case ToolMode::SWITCH: case ToolMode::TWO_WAY_SWITCH:
        case ToolMode::AND_GATE: case ToolMode::OR_GATE: case ToolMode::NOT_GATE:
        case ToolMode::NAND_GATE: case ToolMode::NOR_GATE: case ToolMode::XOR_GATE:
        case ToolMode::XNOR_GATE: case ToolMode::GROUND:
            return true;
        default:
            return false;
    }
}

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

string valueToEditString(const shared_ptr<Component>& c) {
    double v = c->value;
    if (c->type == ComponentType::CAPACITOR) v = c->value * 1e6;
    else if (c->type == ComponentType::INDUCTOR) v = c->value * 1e3;
    ostringstream ss;
    ss << v;
    return ss.str();
}

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

struct TerminalHit {
    bool found = false;
    Vector2 pos = { 0, 0 };
};

TerminalHit findNearestTerminal(const Circuit& circuit, Vector2 mousePos, float radius = 10.0f) {
    float r2 = radius * radius;
    for (const auto& c : circuit.components) {
        Vector2 pts[3];
        int n = 0;
        pts[n++] = c->getTerminalA().pos;
        if (c->type != ComponentType::GROUND && c->type != ComponentType::NOT_GATE) pts[n++] = c->getTerminalB().pos;
        if (c->isLogicGate() || c->type == ComponentType::TWO_WAY_SWITCH) pts[n++] = c->getTerminalC().pos;
        for (int i = 0; i < n; ++i) {
            float dx = mousePos.x - pts[i].x, dy = mousePos.y - pts[i].y;
            if (dx * dx + dy * dy <= r2) return TerminalHit{ true, pts[i] };
        }
    }
    for (const auto& w : circuit.wires) {
        float dx1 = mousePos.x - w.startPos.x, dy1 = mousePos.y - w.startPos.y;
        if (dx1 * dx1 + dy1 * dy1 <= r2) return TerminalHit{ true, w.startPos };
        float dx2 = mousePos.x - w.endPos.x, dy2 = mousePos.y - w.endPos.y;
        if (dx2 * dx2 + dy2 * dy2 <= r2) return TerminalHit{ true, w.endPos };
    }
    return TerminalHit{};
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

void DrawNodeTable(const Circuit& circuit, Rectangle bounds) {
    DrawRectangleRec(bounds, Color{ 20, 25, 35, 220 });
    DrawRectangleLinesEx(bounds, 2, Color{ 100, 200, 255, 200 });
    DrawText("NODE VOLTAGES", static_cast<int>(bounds.x + 10), static_cast<int>(bounds.y + 6), 12, LIGHTGRAY);

    vector<pair<int, double>> nodes;
    for (const auto& kv : circuit.nodeVoltages) nodes.push_back(kv);
    sort(nodes.begin(), nodes.end(), [](auto& a, auto& b){ return a.first < b.first; });

    int yOffset = 26, rowHeight = 16;
    int maxRows = static_cast<int>((bounds.height - 30) / rowHeight);
    int displayed = 0;
    for (const auto& [id, v] : nodes) {
        if (displayed >= maxRows) break;
        stringstream ss;
        ss << "N" << id << " : " << fixed << setprecision(3) << v << " V";
        DrawText(ss.str().c_str(),
                 static_cast<int>(bounds.x + 12),
                 static_cast<int>(bounds.y + yOffset),
                 11, (id == 0) ? ORANGE : RAYWHITE);
        yOffset += rowHeight; displayed++;
    }
    if (static_cast<int>(nodes.size()) > maxRows) {
        DrawText("...", static_cast<int>(bounds.x + 12), static_cast<int>(bounds.y + yOffset), 11, GRAY);
    }
}

int main() {
    const int SCREEN_WIDTH = 1600;
    const int SCREEN_HEIGHT = 900;
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Transient Circuit Simulator - Logic & Analog");
    SetTargetFPS(60);

    Circuit circuit;
    ToolMode currentTool = ToolMode::SELECT;
    shared_ptr<Component> selectedComp = nullptr;
    int selectedWireIdx = -1;
    bool isDragging = false, isWiring = false;
    Vector2 wireStartPos = { 0, 0 };
    bool pressedOnSwitch = false;
    Vector2 pressPos = { 0, 0 };
    bool isEditingValue = false;
    string editBuffer = "";

    Color traceColors[] = { LIME, YELLOW, MAGENTA, GREEN, ORANGE, PINK, SKYBLUE, VIOLET };
    int colorIndex = 0;

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

        // ---- rotate with arrow keys or R ----
        if (selectedComp && !isEditingValue) {
            if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_RIGHT) ||
                IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_DOWN) ||
                IsKeyPressed(KEY_R)) {
                circuit.rotateComponent(selectedComp);
            }
        }

        if (!isEditingValue && mousePos.x > 240) {
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                Vector2 snapped = snapVector(mousePos);

                if (isPlaceableTool(currentTool)) {
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
                }
                else if (currentTool == ToolMode::PROBE) {
                    PointKey key = makePointKey(snapped);
                    if (circuit.pointToNodeMap.find(key) != circuit.pointToNodeMap.end()) {
                        int nodeId = circuit.pointToNodeMap[key];
                        bool exists = false;
                        for (const auto& t : circuit.scope.traces)
                            if (t.nodeId == nodeId) { exists = true; break; }
                        if (!exists) {
                            Color col = traceColors[colorIndex % 8];
                            colorIndex++;
                            circuit.scope.addTrace(nodeId, col);
                        }
                    }
                }
                else if (currentTool == ToolMode::ROTATE) {
                    for (auto& c : circuit.components) {
                        if (CheckCollisionPointRec(mousePos, c->getBounds())) {
                            circuit.rotateComponent(c);
                            selectedComp = c;
                            c->selected = true;
                            break;
                        }
                    }
                }
                else if (currentTool == ToolMode::SELECT) {
                    TerminalHit hit = findNearestTerminal(circuit, mousePos);
                    if (hit.found) {
                        isWiring = true;
                        wireStartPos = hit.pos;
                    } else {
                        selectedComp = nullptr;
                        selectedWireIdx = -1;
                        for (auto& c : circuit.components) c->selected = false;

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

                if (isPlaceableTool(currentTool)) currentTool = ToolMode::SELECT;
            }
        }

        // ---- finish wire ----
        if (isWiring && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
            TerminalHit endHit = findNearestTerminal(circuit, mousePos);
            Vector2 endPos = endHit.found ? endHit.pos : snapVector(mousePos);
            if (mousePos.x > 240 && (endPos.x != wireStartPos.x || endPos.y != wireStartPos.y)) {
                circuit.wires.push_back(Wire{ wireStartPos, endPos });
            }
            isWiring = false;
        }

        if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
            isWiring = false;
            currentTool = ToolMode::SELECT;
            selectedWireIdx = -1;
        }

        // ---- drag component ----
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

        // ---- delete ----
        if (!isEditingValue && (IsKeyPressed(KEY_DELETE) || IsKeyPressed(KEY_BACKSPACE))) {
            if (selectedComp) {
                circuit.components.erase(remove(circuit.components.begin(), circuit.components.end(), selectedComp), circuit.components.end());
                selectedComp = nullptr;
            } else if (selectedWireIdx >= 0 && selectedWireIdx < static_cast<int>(circuit.wires.size())) {
                circuit.wires.erase(circuit.wires.begin() + selectedWireIdx);
                selectedWireIdx = -1;
            }
        }

        // ---- drawing ----
        BeginDrawing();
        ClearBackground(RAYWHITE);

        for (int x = 240; x < SCREEN_WIDTH; x += 20)
            for (int y = 0; y < SCREEN_HEIGHT; y += 20)
                DrawPixel(x, y, LIGHTGRAY);

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

        // ---- oscilloscope and node table ----
        Rectangle scopeBounds = { 850, 450, 700, 400 };
        circuit.scope.draw(scopeBounds);
        circuit.scope.drawControls(scopeBounds);

        DrawNodeTable(circuit, Rectangle{ 250, SCREEN_HEIGHT - 190, 260, 180 });

        // ---- sidebar ----
        DrawRectangle(0, 0, 240, SCREEN_HEIGHT, Color{ 30, 34, 42, 255 });
        DrawText("LOGIC & ANALOG LAB", 15, 12, 16, RAYWHITE);
        DrawText("Drag from a red/blue pin to wire", 15, 30, 10, GRAY);

        int yPos = 46;
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
        BTN("PROBE NODE", ToolMode::PROBE);
        BTN("ROTATE", ToolMode::ROTATE);
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