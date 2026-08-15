#ifndef INPUT_HANDLER_HPP
#define INPUT_HANDLER_HPP

#include "raylib.h"
#include "Circuit.hpp"
#include "AppState.hpp"
#include "ToolMode.hpp"
#include "Utils.hpp"
#include <algorithm>

using namespace std;

struct TerminalHit {
    bool found = false;
    Vector2 pos = { 0, 0 };
};

inline TerminalHit findNearestTerminal(const Circuit& circuit, Vector2 mousePos, float radius = 10.0f) {
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

// Handles component/tool rotation via keyboard.
inline void handleRotateKeys(Circuit& circuit, AppState& state) {
    if (state.selectedComp && !state.isEditingValue) {
        if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_RIGHT) ||
            IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_R)) {
            circuit.rotateComponent(state.selectedComp);
        }
    }
}

// Handles a left click on the canvas: placing parts, probing, rotating, selecting, or starting a wire.
inline void handleCanvasClick(Circuit& circuit, AppState& state, Vector2 mousePos) {
    Vector2 snapped = snapVector(mousePos);

    if (isPlaceableTool(state.currentTool)) {
        placeComponent(circuit, state.currentTool, snapped);
    }
    else if (state.currentTool == ToolMode::PROBE) {
        PointKey key = makePointKey(snapped);
        auto it = circuit.pointToNodeMap.find(key);
        if (it != circuit.pointToNodeMap.end()) {
            circuit.scope.setNode(it->second);
        }
    }
    else if (state.currentTool == ToolMode::ROTATE) {
        for (auto& c : circuit.components) {
            if (CheckCollisionPointRec(mousePos, c->getBounds())) {
                circuit.rotateComponent(c);
                state.selectedComp = c;
                c->selected = true;
                break;
            }
        }
    }
    else if (state.currentTool == ToolMode::SELECT) {
        TerminalHit hit = findNearestTerminal(circuit, mousePos);
        if (hit.found) {
            state.isWiring = true;
            state.wireStartPos = hit.pos;
        } else {
            state.selectedComp = nullptr;
            state.selectedWireIdx = -1;
            for (auto& c : circuit.components) c->selected = false;

            for (auto& c : circuit.components) {
                if (CheckCollisionPointRec(mousePos, c->getBounds())) {
                    state.selectedComp = c;
                    c->selected = true;
                    state.isDragging = true;
                    state.pressPos = mousePos;
                    state.pressedOnSwitch = c->isSwitch();
                    break;
                }
            }

            if (!state.selectedComp) {
                for (size_t i = 0; i < circuit.wires.size(); ++i) {
                    if (CheckCollisionPointLine(mousePos, circuit.wires[i].startPos, circuit.wires[i].endPos, 8)) {
                        state.selectedWireIdx = static_cast<int>(i);
                        break;
                    }
                }
            }
        }
    }

    if (isPlaceableTool(state.currentTool)) state.currentTool = ToolMode::SELECT;
}

// Finishes a wire drag when the mouse is released.
inline void handleWireRelease(Circuit& circuit, AppState& state, Vector2 mousePos) {
    TerminalHit endHit = findNearestTerminal(circuit, mousePos);
    Vector2 endPos = endHit.found ? endHit.pos : snapVector(mousePos);
    if (mousePos.x > 240 && (endPos.x != state.wireStartPos.x || endPos.y != state.wireStartPos.y)) {
        circuit.wires.push_back(Wire{ state.wireStartPos, endPos });
    }
    state.isWiring = false;
}

// Drags the currently selected component, re-attaching any wires connected to its terminals.
inline void handleComponentDrag(Circuit& circuit, AppState& state, Vector2 mousePos) {
    Vector2 newPos = snapVector(mousePos);
    if (newPos.x == state.selectedComp->pos.x && newPos.y == state.selectedComp->pos.y) return;

    PointKey kA_old = makePointKey(state.selectedComp->getTerminalA().pos);
    PointKey kB_old = makePointKey(state.selectedComp->getTerminalB().pos);
    PointKey kC_old = makePointKey(state.selectedComp->getTerminalC().pos);

    state.selectedComp->pos = newPos;

    Vector2 tA_new = state.selectedComp->getTerminalA().pos;
    Vector2 tB_new = state.selectedComp->getTerminalB().pos;
    Vector2 tC_new = state.selectedComp->getTerminalC().pos;

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

// Toggles a switch if it was clicked (not dragged).
inline void handleSwitchToggle(AppState& state, Vector2 mousePos) {
    if (!(state.pressedOnSwitch && state.selectedComp)) return;
    float dx = mousePos.x - state.pressPos.x, dy = mousePos.y - state.pressPos.y;
    if ((dx * dx + dy * dy) < 36.0f) {
        if (state.selectedComp->type == ComponentType::SWITCH) state.selectedComp->switchOn = !state.selectedComp->switchOn;
        else if (state.selectedComp->type == ComponentType::TWO_WAY_SWITCH) state.selectedComp->switchPos = !state.selectedComp->switchPos;
    }
}

inline void handleDelete(Circuit& circuit, AppState& state) {
    if (state.selectedComp) {
        circuit.components.erase(remove(circuit.components.begin(), circuit.components.end(), state.selectedComp), circuit.components.end());
        state.selectedComp = nullptr;
    } else if (state.selectedWireIdx >= 0 && state.selectedWireIdx < static_cast<int>(circuit.wires.size())) {
        circuit.wires.erase(circuit.wires.begin() + state.selectedWireIdx);
        state.selectedWireIdx = -1;
    }
}

// Runs every frame's full input handling. Assumes the oscilloscope's own
// drag handling is done separately (it owns its own bounds/dragging).
inline void processInput(Circuit& circuit, AppState& state) {
    Vector2 mousePos = GetMousePosition();

    if (state.isEditingValue && (!state.selectedComp || !isEditableValueType(state.selectedComp->type))) {
        state.isEditingValue = false;
    }

    handleRotateKeys(circuit, state);

    if (!state.isEditingValue && mousePos.x > 240) {
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            handleCanvasClick(circuit, state, mousePos);
        }
    }

    if (state.isWiring && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        handleWireRelease(circuit, state, mousePos);
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
        state.isWiring = false;
        state.currentTool = ToolMode::SELECT;
        state.selectedWireIdx = -1;
    }

    if (state.isDragging && state.selectedComp && IsMouseButtonDown(MOUSE_BUTTON_LEFT) && mousePos.x > 240) {
        handleComponentDrag(circuit, state, mousePos);
    }

    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        handleSwitchToggle(state, mousePos);
        state.isDragging = false;
        state.pressedOnSwitch = false;
    }

    if (!state.isEditingValue && (IsKeyPressed(KEY_DELETE) || IsKeyPressed(KEY_BACKSPACE))) {
        handleDelete(circuit, state);
    }
}

#endif
