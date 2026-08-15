#ifndef UTILS_HPP
#define UTILS_HPP

#include "raylib.h"
#include <string>

using namespace std;

// Draws voltage/readout text - just a plain draw using the darker, more
// legible green (VOLT_TEXT_COLOR). Previously this doubled the draw call to
// fake bold, but that made small text look thick and jumbled - so it's back
// to a single draw and lets the darker color do the legibility work.
inline void DrawTextBold(const char* text, int x, int y, int fontSize, Color color) {
    DrawText(text, x, y, fontSize, color);
}

// Darker, more legible green used for live voltage readouts (plain GREEN is
// too light/thin to read comfortably against the canvas or scope background).
inline const Color VOLT_TEXT_COLOR = Color{ 0, 140, 0, 255 };

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
