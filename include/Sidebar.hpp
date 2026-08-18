#ifndef SIDEBAR_HPP
#define SIDEBAR_HPP

#include "raylib.h"
#include "Circuit.hpp"
#include "AppState.hpp"
#include "ToolMode.hpp"
#include "Utils.hpp"
#include <vector>
#include <algorithm>
#include <sstream>
#include <iomanip>

using namespace std;

// Node table (uses custom font)
inline void DrawNodeTable(const Circuit& circuit, Rectangle bounds) {
    DrawRectangleRec(bounds, Color{ 20, 25, 35, 220 });
    DrawRectangleLinesEx(bounds, 2, Color{ 100, 200, 255, 200 });
    DrawTextEx(g_font, "NODE VOLTAGES", { bounds.x + 10, bounds.y + 6 }, 12, 1, LIGHTGRAY);

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
        Vector2 pos = { bounds.x + 12, bounds.y + yOffset };
        Color col = (id == 0) ? ORANGE : RAYWHITE;
        DrawTextEx(g_font, ss.str().c_str(), pos, 11, 1, col);
        yOffset += rowHeight; displayed++;
    }
    if (static_cast<int>(nodes.size()) > maxRows) {
        Vector2 pos = { bounds.x + 12, bounds.y + yOffset };
        DrawTextEx(g_font, "...", pos, 11, 1, GRAY);
    }
}

// Value editing helpers
inline void handleValueEditing(AppState& state) {
    int key = GetCharPressed();
    while (key > 0) {
        bool isDigit = (key >= '0' && key <= '9');
        bool isDot = (key == '.' && state.editBuffer.find('.') == string::npos);
        bool isMinus = (key == '-' && state.editBuffer.empty() && state.selectedComp->type == ComponentType::VOLTAGE_SOURCE);
        if ((isDigit || isDot || isMinus) && state.editBuffer.size() < 10) state.editBuffer += static_cast<char>(key);
        key = GetCharPressed();
    }
    if (IsKeyPressed(KEY_BACKSPACE) && !state.editBuffer.empty()) state.editBuffer.pop_back();
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) {
        applyEditBuffer(state.selectedComp, state.editBuffer);
        state.isEditingValue = false;
    }
    if (IsKeyPressed(KEY_ESCAPE)) state.isEditingValue = false;

    applyEditBuffer(state.selectedComp, state.editBuffer);
}

inline void drawValueEditBox(AppState& state, int yPos) {
    Rectangle box{ 15, (float)yPos, 210, 28 };
    DrawRectangleRec(box, RAYWHITE);
    DrawRectangleLinesEx(box, 2, BLUE);
    bool cursorOn = (static_cast<int>(GetTime() * 2.0) % 2) == 0;
    string display = state.editBuffer + (cursorOn ? "_" : "");
    Vector2 pos = { box.x + 8, box.y + 7 };
    DrawTextEx(g_font, display.c_str(), pos, 14, 1, BLACK);
    string unit = editUnitSuffix(state.selectedComp->type);
    Vector2 unitPos = { box.x + box.width - 8 - MeasureTextEx(g_font, unit.c_str(), 12, 1).x, box.y + 9 };
    DrawTextEx(g_font, unit.c_str(), unitPos, 12, 1, DARKGRAY);
}

// Sidebar drawing
inline void drawSidebar(Circuit& circuit, AppState& state, int screenHeight) {
    DrawRectangle(0, 0, 240, screenHeight, Color{ 30, 34, 42, 255 });
    DrawTextEx(g_font, "COMPONETS", { 15, 12 }, 16, 1, RAYWHITE);
    DrawTextEx(g_font, "Drag from a red/blue pin to wire", { 15, 30 }, 10, 1, GRAY);

    int yPos = 46;
    #define BTN(lbl, mode) if (DrawButton(Rectangle{ 15, (float)yPos, 210, 22 }, lbl, state.currentTool == mode)) { state.currentTool = mode; state.selectedWireIdx = -1; state.isEditingValue = false; } yPos += 25;
    BTN("SELECT / DRAG", ToolMode::SELECT);
    BTN("+ RESISTOR", ToolMode::RESISTOR);
    BTN("+ CAPACITOR", ToolMode::CAPACITOR);
    BTN("+ INDUCTOR", ToolMode::INDUCTOR);
    BTN("+ VOLTAGE SRC (5V)", ToolMode::VOLTAGE_SOURCE);
    BTN("+ LED", ToolMode::LED);
    BTN("+ AMMETER", ToolMode::AMMETER);
    BTN("+ VOLTMETER", ToolMode::VOLTMETER);
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

    const char* scopeBtnLabel = circuit.scope.visible ? "OSCILLOSCOPE: ON" : "OSCILLOSCOPE: OFF";
    if (DrawButton(Rectangle{ 15, (float)yPos, 210, 24 }, scopeBtnLabel, circuit.scope.visible, SKYBLUE)) {
        circuit.scope.visible = !circuit.scope.visible;
    }
    yPos += 30;

    DrawLine(15, yPos, 225, yPos, GRAY); yPos += 10;

    const char* simBtnLabel = circuit.isRunning ? "PAUSE SIMULATION" : "RUN SIMULATION";
    if (DrawButton(Rectangle{ 15, (float)yPos, 210, 30 }, simBtnLabel, false, circuit.isRunning ? ORANGE : GREEN)) {
        circuit.isRunning = !circuit.isRunning;
    }
    yPos += 35;

    if (DrawButton(Rectangle{ 15, (float)yPos, 210, 24 }, "CLEAR ALL", false, MAROON)) {
        circuit.clear();
        state.selectedComp = nullptr;
        state.selectedWireIdx = -1;
        state.isEditingValue = false;
    }
    yPos += 34;

    bool editableSelected = state.selectedComp && isEditableValueType(state.selectedComp->type);
    if (!editableSelected) return;

    DrawLine(15, yPos, 225, yPos, GRAY); yPos += 10;

    const char* editLabel = state.isEditingValue ? "CONFIRM (ENTER)" : "EDIT VALUE";
    if (DrawButton(Rectangle{ 15, (float)yPos, 210, 26 }, editLabel, state.isEditingValue, SKYBLUE)) {
        if (!state.isEditingValue) {
            state.isEditingValue = true;
            state.editBuffer = valueToEditString(state.selectedComp);
        } else {
            applyEditBuffer(state.selectedComp, state.editBuffer);
            state.isEditingValue = false;
        }
    }
    yPos += 30;

    if (state.isEditingValue) {
        handleValueEditing(state);
        drawValueEditBox(state, yPos);
    } else {
        stringstream ss;
        ss << "Value: " << valueToEditString(state.selectedComp) << " " << editUnitSuffix(state.selectedComp->type);
        Vector2 pos = { 15, (float)yPos };
        DrawTextEx(g_font, ss.str().c_str(), pos, 12, 1, RAYWHITE);
    }
}

#endif