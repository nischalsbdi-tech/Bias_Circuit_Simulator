#ifndef WIRE_HPP
#define WIRE_HPP

#include "raylib.h"
#include <cmath>

using namespace std;

struct Wire {
    Vector2 startPos;
    Vector2 endPos;
    bool selected = false;
    double current = 0.0;
    float particleProgress = 0.0f;

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

    void draw() const {
        Color wireColor = selected ? GOLD : DARKBLUE;
        DrawLineEx(startPos, endPos, 3, wireColor);
        DrawCircleV(startPos, 3, MAROON);
        DrawCircleV(endPos, 3, MAROON);

        if (abs(current) > 1e-4) {
            Vector2 pPos = Vector2{ startPos.x + (endPos.x - startPos.x) * particleProgress,
                                   startPos.y + (endPos.y - startPos.y) * particleProgress };
            DrawCircleV(pPos, 3, YELLOW);
        }
    }
};

#endif