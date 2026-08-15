#ifndef COMPONENT_HPP
#define COMPONENT_HPP

#include "raylib.h"
#include "PointKey.hpp"
#include <cmath>

using namespace std;

enum class ComponentType {
    RESISTOR, CAPACITOR, INDUCTOR, VOLTAGE_SOURCE,
    GROUND, LED, AMMETER, VOLTMETER, SWITCH, TWO_WAY_SWITCH,
    AND_GATE, OR_GATE,
    NOT_GATE, NAND_GATE, NOR_GATE, XOR_GATE, XNOR_GATE
};

class Component {
public:
    ComponentType type;
    Vector2 pos;
    double value;
    bool selected = false;
    bool isHorizontal = true;

    double vPrev = 0.0;
    double iPrev = 0.0;
    double current = 0.0;
    float particleProgress = 0.0f;

    // Switch state. SWITCH (SPST): switchOn = closed/conducting.
    // TWO_WAY_SWITCH (SPDT): switchPos = false -> common(A) connects to throw1(B),
    //                        switchPos = true  -> common(A) connects to throw2(C).
    bool switchOn = false;
    bool switchPos = false;

    Component(ComponentType t, Vector2 p, double val = 100.0)
        : type(t), pos(p), value(val) {}

    virtual ~Component() = default;

    bool isLogicGate() const {
        return type >= ComponentType::AND_GATE && type <= ComponentType::XNOR_GATE;
    }

    bool isSwitch() const {
        return type == ComponentType::SWITCH || type == ComponentType::TWO_WAY_SWITCH;
    }

    virtual Rectangle getBounds() const {
        if (isLogicGate()) return Rectangle{ pos.x - 40, pos.y - 20, 60, 40 };
        return isHorizontal ? Rectangle{ pos.x - 40, pos.y - 20, 80, 40 }
                            : Rectangle{ pos.x - 20, pos.y - 40, 40, 80 };
    }

    virtual Terminal getTerminalA() const {
        if (type == ComponentType::GROUND) return Terminal{ Vector2{ pos.x, pos.y - 20 }, -1 };
        if (type == ComponentType::NOT_GATE) return Terminal{ Vector2{ pos.x - 40, pos.y }, -1 };
        if (isLogicGate()) return Terminal{ Vector2{ pos.x - 40, pos.y - 20 }, -1 };
        return isHorizontal ? Terminal{ Vector2{ pos.x - 40, pos.y }, -1 }
                            : Terminal{ Vector2{ pos.x, pos.y - 40 }, -1 };
    }

    virtual Terminal getTerminalB() const {
        if (type == ComponentType::GROUND || type == ComponentType::NOT_GATE) return Terminal{ pos, -1 };
        if (isLogicGate()) return Terminal{ Vector2{ pos.x - 40, pos.y + 20 }, -1 };
        if (type == ComponentType::TWO_WAY_SWITCH) return Terminal{ Vector2{ pos.x + 40, pos.y - 20 }, -1 };
        return isHorizontal ? Terminal{ Vector2{ pos.x + 40, pos.y }, -1 }
                            : Terminal{ Vector2{ pos.x, pos.y + 40 }, -1 };
    }

    virtual Terminal getTerminalC() const {
        if (isLogicGate()) return Terminal{ Vector2{ pos.x + 40, pos.y }, -1 };
        if (type == ComponentType::TWO_WAY_SWITCH) return Terminal{ Vector2{ pos.x + 40, pos.y + 20 }, -1 };
        return Terminal{ Vector2{ 0, 0 }, -1 };
    }

    void updateParticles(float dt) {
        float speed = static_cast<float>(abs(current) * 15.0);
        if (speed > 50.0f) speed = 50.0f;
        if (current >= 0) {
            particleProgress += speed * dt;
            if (particleProgress > 1.0f) particleProgress -= 1.0f;
        } else {
            particleProgress -= speed * dt;
            if (particleProgress < 0.0f) particleProgress += 1.0f;
        }
    }
};

#endif
