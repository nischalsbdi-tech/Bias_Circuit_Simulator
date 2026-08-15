#ifndef OSCILLOSCOPE_HPP
#define OSCILLOSCOPE_HPP

#include "raylib.h"
#include "Utils.hpp"
#include <vector>
#include <map>
#include <string>
#include <sstream>
#include <iomanip>

// ---------- Ring Buffer (fixed capacity) ----------
template<typename T>
class RingBuffer {
    std::vector<T> data;
    size_t head = 0;
    size_t count = 0;
public:
    explicit RingBuffer(size_t capacity = 400) : data(capacity) {}

    void push(const T& value) {
        data[head] = value;
        head = (head + 1) % data.size();
        if (count < data.size()) count++;
    }

    size_t size() const { return count; }
    bool empty() const { return count == 0; }

    // operator[]: 0 = oldest, size()-1 = newest
    T operator[](size_t idx) const {
        return data[(head - count + idx + data.size()) % data.size()];
    }

    T back() const {
        if (count == 0) return T{};
        return data[(head - 1 + data.size()) % data.size()];
    }

    void clear() { count = 0; head = 0; }
};

// ---------- Minimalistic single-node, draggable Oscilloscope ----------
// Only ever tracks ONE node at a time. It costs nothing and draws nowhere
// unless "visible" is turned on from the sidebar toggle. Drag it by its
// title bar to reposition it anywhere on the canvas.
class Oscilloscope {
public:
    bool visible = false;      // toggled on/off from the sidebar
    int nodeId = -1;           // -1 = nothing probed yet
    RingBuffer<float> history{400};
    float voltsPerDiv = 5.0f;  // fixed vertical scale (+-2 divs shown)
    Rectangle bounds = { 900, 480, 480, 200 };

private:
    bool dragging = false;
    Vector2 dragOffset = { 0, 0 };
    static constexpr float TITLE_H = 18.0f;

public:
    void setNode(int id) {
        nodeId = id;
        history.clear();
    }

    void clearNode() {
        nodeId = -1;
        history.clear();
    }

    void update(const std::map<int, double>& nodeVoltages, float /*dt*/) {
        if (!visible || nodeId < 0) return;
        auto it = nodeVoltages.find(nodeId);
        if (it != nodeVoltages.end()) history.push(static_cast<float>(it->second));
    }

    // Call once per frame (before draw) so dragging the title bar works.
    void handleDrag() {
        if (!visible) return;
        Vector2 m = GetMousePosition();
        Rectangle titleBar = { bounds.x, bounds.y, bounds.width, TITLE_H };

        if (!dragging && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(m, titleBar)) {
            dragging = true;
            dragOffset = { m.x - bounds.x, m.y - bounds.y };
        }
        if (dragging && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            bounds.x = m.x - dragOffset.x;
            bounds.y = m.y - dragOffset.y;
        }
        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) dragging = false;
    }

    // Draws the panel. Does nothing if not visible - it simply isn't on the canvas.
    void draw() const {
        if (!visible) return;

        DrawRectangleRec(bounds, Color{ 12, 16, 20, 235 });
        DrawRectangleLinesEx(bounds, 1, Color{ 90, 200, 120, 220 });

        // title bar (drag handle)
        Rectangle titleBar = { bounds.x, bounds.y, bounds.width, TITLE_H };
        DrawRectangleRec(titleBar, Color{ 30, 40, 45, 255 });
        DrawText("OSCILLOSCOPE  (drag here to move)", (int)bounds.x + 6, (int)bounds.y + 3, 10, LIGHTGRAY);

        float left = bounds.x + 4, right = bounds.x + bounds.width - 4;
        float top = bounds.y + TITLE_H + 20, bottom = bounds.y + bounds.height - 6;
        float w = right - left, h = bottom - top;
        float midY = top + h / 2.0f;

        if (nodeId < 0) {
            DrawText("Use PROBE to pick a node", (int)bounds.x + 10, (int)midY - 5, 10, GRAY);
            return;
        }

        // single centre gridline - minimalistic, no ruled grid
        DrawLine((int)left, (int)midY, (int)right, (int)midY, Color{ 60, 90, 90, 140 });

        // label: node id + latest reading, bold dark-green for legibility
        std::string label = "N" + std::to_string(nodeId);
        if (!history.empty()) {
            std::ostringstream ss;
            ss << std::fixed << std::setprecision(2) << history.back() << "V";
            label += "  " + ss.str();
        }
        DrawTextBold(label.c_str(), (int)bounds.x + 8, (int)bounds.y + TITLE_H + 4, 12, VOLT_TEXT_COLOR);

        if (history.size() < 2) return;

        size_t n = history.size();
        for (size_t i = 1; i < n; ++i) {
            float x1 = left + ((float)(i - 1) / (n - 1)) * w;
            float x2 = left + ((float)i / (n - 1)) * w;
            float y1 = midY - (history[i - 1] / voltsPerDiv) * (h / 2.0f);
            float y2 = midY - (history[i]     / voltsPerDiv) * (h / 2.0f);
            DrawLine((int)x1, (int)y1, (int)x2, (int)y2, GREEN);
        }
    }
};

#endif
