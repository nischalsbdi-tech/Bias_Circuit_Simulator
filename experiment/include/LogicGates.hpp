#ifndef LOGIC_GATES_HPP
#define LOGIC_GATES_HPP

#include "Component.hpp"
#include <string>

using namespace std;

inline bool evaluateLogicGate(ComponentType type, bool inA, bool inB) {
    switch (type) {
        case ComponentType::AND_GATE:  return inA && inB;
        case ComponentType::OR_GATE:   return inA || inB;
        case ComponentType::NOT_GATE:  return !inA;
        case ComponentType::NAND_GATE: return !(inA && inB);
        case ComponentType::NOR_GATE:  return !(inA || inB);
        case ComponentType::XOR_GATE:  return inA ^ inB;
        case ComponentType::XNOR_GATE: return !(inA ^ inB);
        default: return false;
    }
}

inline string getGateLabel(ComponentType type) {
    switch (type) {
        case ComponentType::AND_GATE:  return "AND";
        case ComponentType::OR_GATE:   return "OR";
        case ComponentType::NOT_GATE:  return "NOT";
        case ComponentType::NAND_GATE: return "NAND";
        case ComponentType::NOR_GATE:  return "NOR";
        case ComponentType::XOR_GATE:  return "XOR";
        case ComponentType::XNOR_GATE: return "XNOR";
        default: return "";
    }
}

inline void drawLogicGate(const Component& gate) {
    Color bodyColor = gate.selected ? GOLD : DARKGRAY;
    Terminal tA = gate.getTerminalA();
    Terminal tB = gate.getTerminalB();
    Terminal tC = gate.getTerminalC();

    DrawRectangleRec(Rectangle{ gate.pos.x - 20, gate.pos.y - 18, 20, 36 }, LIGHTGRAY);
    DrawCircleSector(Vector2{ gate.pos.x, gate.pos.y }, 18, -90, 90, 16, LIGHTGRAY);
    DrawRectangleLinesEx(Rectangle{ gate.pos.x - 20, gate.pos.y - 18, 20, 36 }, 2, bodyColor);

    if (gate.type == ComponentType::NOT_GATE) {
        DrawLineEx(tA.pos, Vector2{ gate.pos.x - 20, gate.pos.y }, 2, DARKGRAY);
    } else {
        DrawLineEx(tA.pos, Vector2{ gate.pos.x - 20, gate.pos.y - 20 }, 2, DARKGRAY);
        DrawLineEx(tB.pos, Vector2{ gate.pos.x - 20, gate.pos.y + 20 }, 2, DARKGRAY);
    }

    DrawLineEx(Vector2{ gate.pos.x + 20, gate.pos.y }, tC.pos, 2, DARKGRAY);
    DrawText(getGateLabel(gate.type).c_str(), static_cast<int>(gate.pos.x - 16), static_cast<int>(gate.pos.y - 6), 10, BLACK);
    DrawCircleV(tC.pos, 3, BLUE);
}

#endif
