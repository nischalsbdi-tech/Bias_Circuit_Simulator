#ifndef OSCILLOSCOPE_HPP
#define OSCILLOSCOPE_HPP

#include "raylib.h"
#include "Utils.hpp"   // for g_font, VOLT_TEXT_COLOR
#include <vector>
#include <map>
#include <string>
#include <sstream>
#include <iomanip>

// ---------- Ring Buffer ----------
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

    T operator[](size_t idx) const {
        return data[(head - count + idx + data.size()) % data.size()];
    }

    T back() const {
        if (count == 0) return T{};
        return data[(head - 1 + data.size()) % data.size()];
    }

    void clear() { count = 0; head = 0; }
};

// ---------- Oscilloscope ----------
class Oscilloscope {
public:
    bool visible = false;
    int nodeId = -1;
    RingBuffer<float> history{400};
    float voltsPerDiv = 5.0f;
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

    void draw() const {
        if (!visible) return;

        DrawRectangleRec(bounds, Color{ 12, 16, 20, 235 });
        DrawRectangleLinesEx(bounds, 1, Color{ 90, 200, 120, 220 });

        // title bar
        Rectangle titleBar = { bounds.x, bounds.y, bounds.width, TITLE_H };
        DrawRectangleRec(titleBar, Color{ 30, 40, 45, 255 });
        Vector2 titlePos = { bounds.x + 6, bounds.y + 3 };
        DrawTextEx(g_font, "OSCILLOSCOPE  (drag here to move)", titlePos, 10, 1, LIGHTGRAY);

        float left = bounds.x + 4, right = bounds.x + bounds.width - 4;
        float top = bounds.y + TITLE_H + 20, bottom = bounds.y + bounds.height - 6;
        float w = right - left, h = bottom - top;
        float midY = top + h / 2.0f;

        if (nodeId < 0) {
            Vector2 pos = { bounds.x + 10, midY - 5 };
            DrawTextEx(g_font, "Use PROBE to pick a node", pos, 10, 1, GRAY);
            return;
        }

        // centre gridline
        DrawLine((int)left, (int)midY, (int)right, (int)midY, Color{ 60, 90, 90, 140 });

        // label: node id + latest reading
        std::string label = "N" + std::to_string(nodeId);
        if (!history.empty()) {
            std::ostringstream ss;
            ss << std::fixed << std::setprecision(2) << history.back() << "V";
            label += "  " + ss.str();
        }
        Vector2 labelPos = { bounds.x + 8, bounds.y + TITLE_H + 4 };
        DrawTextEx(g_font, label.c_str(), labelPos, 12, 1, VOLT_TEXT_COLOR);

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