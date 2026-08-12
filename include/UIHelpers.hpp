#ifndef UI_HELPERS_HPP
#define UI_HELPERS_HPP

#include "raylib.h"
#include <cmath>

using namespace std;

inline Vector2 snapVector(Vector2 v, float gridStep = 20.0f) {
    return Vector2{
        static_cast<float>(round(v.x / gridStep) * gridStep),
        static_cast<float>(round(v.y / gridStep) * gridStep)
    };
}

inline bool DrawButton(Rectangle rect, const char* text, bool active = false, Color baseColor = LIGHTGRAY) {
    Vector2 mousePos = GetMousePosition();
    bool hovered = CheckCollisionPointRec(mousePos, rect);
    Color color = active ? SKYBLUE : (hovered ? RAYWHITE : baseColor);
    DrawRectangleRec(rect, color);
    DrawRectangleLinesEx(rect, 2, active ? BLUE : DARKGRAY);
    int textWidth = MeasureText(text, 12);
    DrawText(text, static_cast<int>(rect.x + (rect.width - textWidth) / 2.0f), static_cast<int>(rect.y + (rect.height - 12) / 2.0f), 12, BLACK);
    return hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

#endif
