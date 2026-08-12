#ifndef DISJOINT_SET_HPP
#define DISJOINT_SET_HPP

#include "PointKey.hpp"
#include <map>

using namespace std;

class DisjointSet {
public:
    map<PointKey, PointKey> parent;

    PointKey find(PointKey p) {
        if (parent.find(p) == parent.end()) return parent[p] = p;
        if (parent[p] == p) return p;
        return parent[p] = find(parent[p]);
    }

    void unite(PointKey p1, PointKey p2) {
        PointKey r1 = find(p1), r2 = find(p2);
        if (!(r1 == r2)) parent[r1] = r2;
    }
};

#endif
