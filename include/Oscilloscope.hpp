#ifndef OSCILLOSCOPE_HPP
#define OSCILLOSCOPE_HPP

#include "raylib.h"
#include <vector>
#include <map>
#include <cmath>
#include <string>
#include <sstream>

// ---------- Ring Buffer (efficient, fixed capacity) ----------
template<typename T>
class RingBuffer {
    std::vector<T> data;
    size_t head = 0;
    size_t count = 0;
public:
    explicit RingBuffer(size_t capacity = 2000) : data(capacity) {}

    void push(const T& value) {
        data[head] = value;
        head = (head + 1) % data.size();
        if (count < data.size()) count++;
    }

    size_t size() const { return count; }
    size_t capacity() const { return data.size(); }
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

// ---------- Oscilloscope class ----------
class Oscilloscope {
public:
    struct Trace {
        int nodeId = -1;
        Color color = WHITE;
        RingBuffer<float> history{1000};
        float voltsPerDiv = 1.0f;
        float verticalOffset = 0.0f;
    };

    std::vector<Trace> traces;
    float timePerDiv = 0.01f;           // seconds per horizontal division
    int horizontalDivs = 10;
    int verticalDivs = 8;
    bool paused = false;

    // Trigger (simple placeholder)
    enum TriggerMode { AUTO, NORMAL, SINGLE };
    TriggerMode triggerMode = AUTO;
    float triggerLevel = 0.0f;
    bool triggerRising = true;

    // Cursors (placeholder)
    bool showCursors = false;
    float cursor1Time = 0.0f, cursor2Time = 0.0f;

    // -------- API --------
    void addTrace(int nodeId, Color color) {
        traces.push_back({nodeId, color, RingBuffer<float>(2000), 1.0f, 0.0f});
    }

    void removeTrace(int index) {
        if (index >= 0 && index < (int)traces.size())
            traces.erase(traces.begin() + index);
    }

    void clearTraces() {
        traces.clear();
    }

    // Called every simulation step to push new data
    void update(const std::map<int, double>& nodeVoltages, float dt) {
        if (paused) return;
        for (auto& trace : traces) {
            auto it = nodeVoltages.find(trace.nodeId);
            if (it != nodeVoltages.end()) {
                trace.history.push(static_cast<float>(it->second));
            }
        }
    }

    // Draw the oscilloscope panel
    void draw(Rectangle bounds) const {
        // Background
        DrawRectangleRec(bounds, Color{15, 20, 25, 230});
        DrawRectangleLinesEx(bounds, 2, GREEN);
        DrawText("OSCILLOSCOPE", (int)bounds.x + 10, (int)bounds.y + 6, 12, GREEN);

        if (traces.empty()) {
            DrawText("Use PROBE tool on a node", (int)bounds.x + 20, (int)bounds.y + 70, 11, GRAY);
            return;
        }

        // Grid area
        int left   = (int)bounds.x + 40;
        int right  = (int)bounds.x + bounds.width - 20;
        int top    = (int)bounds.y + 30;
        int bottom = (int)bounds.y + bounds.height - 20;
        float gridW = (float)(right - left);
        float gridH = (float)(bottom - top);
        float divW = gridW / horizontalDivs;
        float divH = gridH / verticalDivs;

        // Grid lines
        for (int i = 0; i <= horizontalDivs; ++i) {
            float x = left + i * divW;
            DrawLine((int)x, top, (int)x, bottom, Color{60, 80, 80, 100});
        }
        for (int i = 0; i <= verticalDivs; ++i) {
            float y = top + i * divH;
            DrawLine(left, (int)y, right, (int)y, Color{60, 80, 80, 100});
        }

        // Time labels (bottom)
        for (int i = 0; i <= horizontalDivs; ++i) {
            float time = i * timePerDiv;
            std::string label = std::to_string(time) + "s";
            DrawText(label.c_str(), (int)(left + i*divW - 10), bottom + 2, 8, GRAY);
        }
        // Voltage labels (left) – assume centre = 0V, scale = 1V/div for display
        for (int i = 0; i <= verticalDivs; ++i) {
            float v = (verticalDivs/2.0f - i) * 1.0f;
            std::string label = std::to_string(v);
            DrawText(label.c_str(), (int)(left - 30), (int)(top + i*divH - 4), 8, GRAY);
        }

        // Draw each trace
        for (const auto& trace : traces) {
            if (trace.history.size() < 2) continue;

            size_t n = trace.history.size();
            // Map all history points to the full width of the grid
            for (size_t i = 1; i < n; ++i) {
                float x1 = left + ((float)(i-1) / (n-1)) * gridW;
                float x2 = left + ((float)i / (n-1)) * gridW;
                float v1 = trace.history[i-1] - trace.verticalOffset;
                float v2 = trace.history[i] - trace.verticalOffset;
                float y1 = top + (verticalDivs/2.0f - v1 / trace.voltsPerDiv) * divH;
                float y2 = top + (verticalDivs/2.0f - v2 / trace.voltsPerDiv) * divH;
                DrawLine((int)x1, (int)y1, (int)x2, (int)y2, trace.color);
            }

            // Show node ID and current voltage in the legend
            if (!trace.history.empty()) {
                std::string info = "N" + std::to_string(trace.nodeId) + " " +
                                   std::to_string(trace.history.back()) + "V";
                DrawText(info.c_str(), (int)(bounds.x + bounds.width - 130),
                         (int)(bounds.y + 8), 10, trace.color);
            }
        }

        // Trigger indicator
        if (triggerMode != AUTO) {
            float triggerY = top + (verticalDivs/2.0f - triggerLevel / 1.0f) * divH;
            DrawLine(left, triggerY, right, triggerY, RED);
            DrawText("TRIG", (int)left+5, (int)triggerY-8, 10, RED);
        }

        // Cursors placeholder
        if (showCursors) {
            // draw vertical lines – to be implemented
        }

        // Info line
        std::string info = "Time/Div: " + std::to_string(timePerDiv) + "s";
        DrawText(info.c_str(), (int)bounds.x + 10, (int)bounds.y + bounds.height - 18, 10, LIGHTGRAY);
    }

    // Draw small control buttons inside the oscilloscope panel
    void drawControls(Rectangle bounds) {
        int y = (int)(bounds.y + bounds.height - 18);
        int x = (int)(bounds.x + bounds.width - 200);

        // Pause button
        const char* pauseLabel = paused ? "RESUME" : "PAUSE";
        Rectangle btn = { (float)x, (float)y, 50, 16 };
        DrawRectangleRec(btn, paused ? ORANGE : DARKGREEN);
        DrawText(pauseLabel, (int)btn.x+5, (int)btn.y+3, 10, WHITE);
        if (CheckCollisionPointRec(GetMousePosition(), btn) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            paused = !paused;

        // Time/Div arrows
        x += 55;
        if (drawSmallButton(x, y, "<", LIGHTGRAY)) {
            timePerDiv = std::max(0.001f, timePerDiv * 0.5f);
        }
        x += 20;
        DrawText((std::to_string(timePerDiv) + "s").c_str(), x, y+2, 10, RAYWHITE);
        x += 55;
        if (drawSmallButton(x, y, ">", LIGHTGRAY)) {
            timePerDiv = std::min(1.0f, timePerDiv * 2.0f);
        }
    }

private:
    bool drawSmallButton(int x, int y, const char* label, Color color) {
        Rectangle rect = { (float)x, (float)y, 16, 16 };
        Vector2 mouse = GetMousePosition();
        bool hover = CheckCollisionPointRec(mouse, rect);
        DrawRectangleRec(rect, hover ? RAYWHITE : color);
        DrawRectangleLinesEx(rect, 1, DARKGRAY);
        DrawText(label, (int)rect.x+4, (int)rect.y+3, 10, BLACK);
        return hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    }
};

#endif