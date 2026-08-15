#ifndef TOOL_MODE_HPP
#define TOOL_MODE_HPP

#include "Component.hpp"
#include "Circuit.hpp"
#include <string>
#include <memory>
#include <sstream>

using namespace std;

enum class ToolMode {
    SELECT, RESISTOR, CAPACITOR, INDUCTOR, VOLTAGE_SOURCE,
    LED, AMMETER, VOLTMETER, SWITCH, TWO_WAY_SWITCH, AND_GATE, OR_GATE, NOT_GATE, NAND_GATE,
    NOR_GATE, XOR_GATE, XNOR_GATE, GROUND, PROBE, ROTATE
};

inline bool isPlaceableTool(ToolMode t) {
    switch (t) {
        case ToolMode::RESISTOR: case ToolMode::CAPACITOR: case ToolMode::INDUCTOR:
        case ToolMode::VOLTAGE_SOURCE: case ToolMode::LED: case ToolMode::AMMETER:
        case ToolMode::VOLTMETER:
        case ToolMode::SWITCH: case ToolMode::TWO_WAY_SWITCH:
        case ToolMode::AND_GATE: case ToolMode::OR_GATE: case ToolMode::NOT_GATE:
        case ToolMode::NAND_GATE: case ToolMode::NOR_GATE: case ToolMode::XOR_GATE:
        case ToolMode::XNOR_GATE: case ToolMode::GROUND:
            return true;
        default:
            return false;
    }
}

inline bool isEditableValueType(ComponentType t) {
    return t == ComponentType::RESISTOR || t == ComponentType::CAPACITOR ||
           t == ComponentType::INDUCTOR || t == ComponentType::VOLTAGE_SOURCE;
}

inline string editUnitSuffix(ComponentType t) {
    switch (t) {
        case ComponentType::RESISTOR: return "Ohm";
        case ComponentType::CAPACITOR: return "uF";
        case ComponentType::INDUCTOR: return "mH";
        case ComponentType::VOLTAGE_SOURCE: return "V";
        default: return "";
    }
}

inline string valueToEditString(const shared_ptr<Component>& c) {
    double v = c->value;
    if (c->type == ComponentType::CAPACITOR) v = c->value * 1e6;
    else if (c->type == ComponentType::INDUCTOR) v = c->value * 1e3;
    ostringstream ss;
    ss << v;
    return ss.str();
}

inline void applyEditBuffer(const shared_ptr<Component>& c, const string& buf) {
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

// Places a new component of the given tool type at a snapped position.
inline void placeComponent(Circuit& circuit, ToolMode tool, Vector2 snapped) {
    switch (tool) {
        case ToolMode::RESISTOR:       circuit.components.push_back(make_shared<Component>(ComponentType::RESISTOR, snapped, 100.0)); break;
        case ToolMode::CAPACITOR:      circuit.components.push_back(make_shared<Component>(ComponentType::CAPACITOR, snapped, 100e-6)); break;
        case ToolMode::INDUCTOR:       circuit.components.push_back(make_shared<Component>(ComponentType::INDUCTOR, snapped, 10e-3)); break;
        case ToolMode::VOLTAGE_SOURCE: circuit.components.push_back(make_shared<Component>(ComponentType::VOLTAGE_SOURCE, snapped, 5.0)); break;
        case ToolMode::LED:            circuit.components.push_back(make_shared<Component>(ComponentType::LED, snapped, 0.0)); break;
        case ToolMode::AMMETER:        circuit.components.push_back(make_shared<Component>(ComponentType::AMMETER, snapped, 0.0)); break;
        case ToolMode::VOLTMETER:      circuit.components.push_back(make_shared<Component>(ComponentType::VOLTMETER, snapped, 0.0)); break;
        case ToolMode::SWITCH:         circuit.components.push_back(make_shared<Component>(ComponentType::SWITCH, snapped, 0.0)); break;
        case ToolMode::TWO_WAY_SWITCH: circuit.components.push_back(make_shared<Component>(ComponentType::TWO_WAY_SWITCH, snapped, 0.0)); break;
        case ToolMode::AND_GATE:       circuit.components.push_back(make_shared<Component>(ComponentType::AND_GATE, snapped, 0.0)); break;
        case ToolMode::OR_GATE:        circuit.components.push_back(make_shared<Component>(ComponentType::OR_GATE, snapped, 0.0)); break;
        case ToolMode::NOT_GATE:       circuit.components.push_back(make_shared<Component>(ComponentType::NOT_GATE, snapped, 0.0)); break;
        case ToolMode::NAND_GATE:      circuit.components.push_back(make_shared<Component>(ComponentType::NAND_GATE, snapped, 0.0)); break;
        case ToolMode::NOR_GATE:       circuit.components.push_back(make_shared<Component>(ComponentType::NOR_GATE, snapped, 0.0)); break;
        case ToolMode::XOR_GATE:       circuit.components.push_back(make_shared<Component>(ComponentType::XOR_GATE, snapped, 0.0)); break;
        case ToolMode::XNOR_GATE:      circuit.components.push_back(make_shared<Component>(ComponentType::XNOR_GATE, snapped, 0.0)); break;
        case ToolMode::GROUND:         circuit.components.push_back(make_shared<Component>(ComponentType::GROUND, snapped, 0.0)); break;
        default: break;
    }
}

#endif
