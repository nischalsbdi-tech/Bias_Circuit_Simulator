#ifndef OSCILLOSCOPE_HPP
#define OSCILLOSCOPE_HPP

#include "raylib.h"
#include "Circuit.hpp"
#include <algorithm>
#include <sstream>
#include <iomanip>

using namespace std;

inline void DrawOscilloscope(const Circuit& circuit, Rectangle bounds) {
    DrawRectangleRec(bounds, Color{ 15, 20, 25, 230 });
    DrawRectangleLinesEx(bounds, 2, GREEN);
    DrawText("OSCILLOSCOPE PROBE", static_cast<int>(bounds.x + 10), static_cast<int>(bounds.y + 8), 12, GREEN);

    if (circuit.probedNodeId < 0) {
        DrawText("Click 'PROBE' and select a node", static_cast<int>(bounds.x + 20), static_cast<int>(bounds.y + 70), 11, GRAY);
        return;
    }

    const auto& hist = circuit.oscilloscopeHistory;
    if (hist.size() < 2) return;

    float minV = *min_element(hist.begin(), hist.end()) - 0.5f;
    float maxV = *max_element(hist.begin(), hist.end()) + 0.5f;

    for (size_t i = 1; i < hist.size(); ++i) {
        float x1 = bounds.x + (i - 1) * (bounds.width / 300.0f);
        float y1 = bounds.y + bounds.height - ((hist[i - 1] - minV) / (maxV - minV)) * bounds.height;
        float x2 = bounds.x + i * (bounds.width / 300.0f);
        float y2 = bounds.y + bounds.height - ((hist[i] - minV) / (maxV - minV)) * bounds.height;
        DrawLineEx(Vector2{ x1, y1 }, Vector2{ x2, y2 }, 2, LIME);
    }

    stringstream ss; ss << "Node " << circuit.probedNodeId << ": " << fixed << setprecision(2) << hist.back() << " V";
    DrawText(ss.str().c_str(), static_cast<int>(bounds.x + bounds.width - 130), static_cast<int>(bounds.y + 8), 12, YELLOW);
}

#endif
