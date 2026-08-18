#ifndef UTILS_HPP
#define UTILS_HPP

#include "raylib.h"
#include <string>

using namespace std;

// Global custom font (loaded in main.cpp)
extern Font g_font;
extern Font g_fontBold; 

// Draws voltage/readout text using the custom font and darker green.
inline void DrawTextBold(const char* text, int x, int y, int fontSize, Color color) {
    Vector2 pos = { (float)x, (float)y };
    DrawTextEx(g_font, text, pos, (float)fontSize, 1, color);
}

// Darker, more legible green used for live voltage readouts.
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

    // Use custom font for button text, with proper centering
    float fontSize = 12.0f;
    Vector2 textSize = MeasureTextEx(g_font, text, fontSize, 1);
    Vector2 pos = {
        rect.x + (rect.width - textSize.x) / 2.0f,
        rect.y + (rect.height - textSize.y) / 2.0f
    };
    DrawTextEx(g_font, text, pos, fontSize, 1, BLACK);
    return hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

#endif