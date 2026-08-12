#ifndef POINT_KEY_HPP
#define POINT_KEY_HPP

#include "raylib.h"
#include <cmath>

using namespace std;

struct PointKey {
    int x, y;
    bool operator<(const PointKey& o) const {
        if (x != o.x) return x < o.x;
        return y < o.y;
    }
    bool operator==(const PointKey& o) const {
        return x == o.x && y == o.y;
    }
};

inline PointKey makePointKey(Vector2 v) {
    return { static_cast<int>(round(v.x / 20.0f)) * 20,
             static_cast<int>(round(v.y / 20.0f)) * 20 };
}

struct Terminal {
    Vector2 pos;
    int nodeId = -1;
};

#endif
