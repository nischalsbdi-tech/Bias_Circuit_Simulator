#ifndef CIRCUIT_HPP
#define CIRCUIT_HPP

#include "Component.hpp"
#include "LogicGates.hpp"
#include "Wire.hpp"
#include "Solver.hpp"
#include <vector>
#include <memory>
#include <map>
#include <algorithm>
#include <sstream>
#include <iomanip>

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

class Circuit {
public:
    vector<shared_ptr<Component>> components;
    vector<Wire> wires;
    map<PointKey, int> pointToNodeMap;
    map<int, double> nodeVoltages;

    bool isRunning = false;
    double simTime = 0.0;
    int probedNodeId = -1;
    vector<float> oscilloscopeHistory;

    void clear() {
        components.clear();
        wires.clear();
        pointToNodeMap.clear();
        nodeVoltages.clear();
        oscilloscopeHistory.clear();
        probedNodeId = -1;
    }

    void drawComponent(const Component& comp) const {
        Terminal tA = comp.getTerminalA();
        Terminal tB = comp.getTerminalB();
        Color bodyColor = comp.selected ? GOLD : DARKGRAY;

        if (comp.isLogicGate()) {
            drawLogicGate(comp);
        } else if (comp.type == ComponentType::RESISTOR) {
            Rectangle rect = Rectangle{ comp.pos.x - 25, comp.pos.y - 12, 50, 24 };
            DrawLineEx(tA.pos, Vector2{ comp.pos.x - 25, comp.pos.y }, 2, DARKGRAY);
            DrawLineEx(Vector2{ comp.pos.x + 25, comp.pos.y }, tB.pos, 2, DARKGRAY);
            DrawRectangleRec(rect, LIGHTGRAY);
            DrawRectangleLinesEx(rect, 2, bodyColor);
            stringstream ss; ss << static_cast<int>(comp.value) << " Ohm";
            DrawText(ss.str().c_str(), static_cast<int>(comp.pos.x - 22), static_cast<int>(comp.pos.y + 16), 11, DARKBLUE);
        } else if (comp.type == ComponentType::CAPACITOR) {
            DrawLineEx(tA.pos, Vector2{ comp.pos.x - 6, comp.pos.y }, 2, DARKGRAY);
            DrawLineEx(Vector2{ comp.pos.x + 6, comp.pos.y }, tB.pos, 2, DARKGRAY);
            DrawLineEx(Vector2{ comp.pos.x - 6, comp.pos.y - 18 }, Vector2{ comp.pos.x - 6, comp.pos.y + 18 }, 4, bodyColor);
            DrawLineEx(Vector2{ comp.pos.x + 6, comp.pos.y - 18 }, Vector2{ comp.pos.x + 6, comp.pos.y + 18 }, 4, bodyColor);
            stringstream ss; ss << static_cast<int>(comp.value * 1e6) << " uF";
            DrawText(ss.str().c_str(), static_cast<int>(comp.pos.x + 15), static_cast<int>(comp.pos.y - 6), 11, DARKBLUE);
        } else if (comp.type == ComponentType::INDUCTOR) {
            DrawLineEx(tA.pos, tB.pos, 2, DARKGRAY);
            for (int i = -2; i <= 2; ++i) {
                DrawCircleLines(static_cast<int>(comp.pos.x + i * 10.0f), static_cast<int>(comp.pos.y), 7, bodyColor);
            }
            stringstream ss; ss << static_cast<int>(comp.value * 1e3) << " mH";
            DrawText(ss.str().c_str(), static_cast<int>(comp.pos.x + 15), static_cast<int>(comp.pos.y - 6), 11, DARKBLUE);
        } else if (comp.type == ComponentType::VOLTAGE_SOURCE) {
            DrawCircleV(comp.pos, 22, LIGHTGRAY);
            DrawCircleLines(static_cast<int>(comp.pos.x), static_cast<int>(comp.pos.y), 22, bodyColor);
            DrawLineEx(tA.pos, Vector2{ comp.pos.x - 22, comp.pos.y }, 2, DARKGRAY);
            DrawLineEx(Vector2{ comp.pos.x + 22, comp.pos.y }, tB.pos, 2, DARKGRAY);
            DrawText("+", static_cast<int>(comp.pos.x - 14), static_cast<int>(comp.pos.y - 8), 16, RED);
            DrawText("-", static_cast<int>(comp.pos.x + 6), static_cast<int>(comp.pos.y - 8), 16, BLUE);
            stringstream ss; ss << comp.value << " V";
            DrawText(ss.str().c_str(), static_cast<int>(comp.pos.x + 25), static_cast<int>(comp.pos.y - 6), 11, DARKGREEN);
        } else if (comp.type == ComponentType::GROUND) {
            DrawLineEx(Vector2{ comp.pos.x, comp.pos.y - 20 }, comp.pos, 2, DARKGRAY);
            DrawLineEx(Vector2{ comp.pos.x - 15, comp.pos.y }, Vector2{ comp.pos.x + 15, comp.pos.y }, 3, bodyColor);
            DrawLineEx(Vector2{ comp.pos.x - 10, comp.pos.y + 5 }, Vector2{ comp.pos.x + 10, comp.pos.y + 5 }, 2, bodyColor);
            DrawLineEx(Vector2{ comp.pos.x - 5, comp.pos.y + 10 }, Vector2{ comp.pos.x + 5, comp.pos.y + 10 }, 1, bodyColor);
            DrawText("GND", static_cast<int>(comp.pos.x + 18), static_cast<int>(comp.pos.y - 10), 11, DARKGRAY);
        } else if (comp.type == ComponentType::LED) {
            DrawLineEx(tA.pos, Vector2{ comp.pos.x - 12, comp.pos.y }, 2, DARKGRAY);
            DrawLineEx(Vector2{ comp.pos.x + 12, comp.pos.y }, tB.pos, 2, DARKGRAY);
            bool isLit = comp.current > 0.0005;
            if (isLit) DrawCircleV(comp.pos, 22, Color{ 255, 50, 50, 100 });
            DrawTriangle(Vector2{ comp.pos.x - 10, comp.pos.y - 12 }, Vector2{ comp.pos.x - 10, comp.pos.y + 12 }, Vector2{ comp.pos.x + 10, comp.pos.y }, isLit ? RED : DARKGRAY);
            DrawLineEx(Vector2{ comp.pos.x + 10, comp.pos.y - 12 }, Vector2{ comp.pos.x + 10, comp.pos.y + 12 }, 3, isLit ? RED : bodyColor);
            DrawText("LED", static_cast<int>(comp.pos.x - 10), static_cast<int>(comp.pos.y + 16), 10, RED);
        } else if (comp.type == ComponentType::AMMETER) {
            DrawLineEx(tA.pos, Vector2{ comp.pos.x - 18, comp.pos.y }, 2, DARKGRAY);
            DrawLineEx(Vector2{ comp.pos.x + 18, comp.pos.y }, tB.pos, 2, DARKGRAY);
            DrawCircleV(comp.pos, 18, LIGHTGRAY);
            DrawCircleLines(static_cast<int>(comp.pos.x), static_cast<int>(comp.pos.y), 18, bodyColor);
            DrawText("A", static_cast<int>(comp.pos.x - 4), static_cast<int>(comp.pos.y - 7), 14, DARKBLUE);
            stringstream ss;
            if (abs(comp.current) < 1.0) ss << fixed << setprecision(1) << (comp.current * 1000.0) << " mA";
            else ss << fixed << setprecision(2) << comp.current << " A";
            DrawText(ss.str().c_str(), static_cast<int>(comp.pos.x - 20), static_cast<int>(comp.pos.y + 22), 11, PURPLE);
        }

        if (abs(comp.current) > 1e-4 && comp.type != ComponentType::GROUND && !comp.isLogicGate()) {
            Vector2 pPos = Vector2{ tA.pos.x + (tB.pos.x - tA.pos.x) * comp.particleProgress,
                                   tA.pos.y + (tB.pos.y - tA.pos.y) * comp.particleProgress };
            DrawCircleV(pPos, 4, YELLOW);
        }

        DrawCircleV(tA.pos, 4, RED);
        if (comp.type != ComponentType::GROUND && comp.type != ComponentType::NOT_GATE) DrawCircleV(tB.pos, 4, BLUE);
    }

    void stepSimulation(double dt = 0.001) {
        if (components.empty()) return;

        DisjointSet ds;
        PointKey firstGndKey;
        bool hasGnd = false;

        for (const auto& comp : components) {
            if (comp->type == ComponentType::GROUND) {
                PointKey gKey = makePointKey(comp->getTerminalA().pos);
                ds.find(gKey);
                if (!hasGnd) { firstGndKey = gKey; hasGnd = true; }
                else { ds.unite(firstGndKey, gKey); }
            } else {
                ds.find(makePointKey(comp->getTerminalA().pos));
                if (comp->type != ComponentType::NOT_GATE) ds.find(makePointKey(comp->getTerminalB().pos));
                if (comp->isLogicGate()) ds.find(makePointKey(comp->getTerminalC().pos));
            }
        }

        for (const auto& wire : wires) {
            PointKey kStart = makePointKey(wire.startPos);
            PointKey kEnd = makePointKey(wire.endPos);
            ds.find(kStart); ds.find(kEnd);
            ds.unite(kStart, kEnd);
        }

        map<PointKey, vector<PointKey>> clusters;
        for (const auto& pair : ds.parent) clusters[ds.find(pair.first)].push_back(pair.first);

        PointKey groundRoot = hasGnd ? ds.find(firstGndKey) : (clusters.empty() ? PointKey{-999, -999} : clusters.begin()->first);

        map<PointKey, int> rootToNodeId;
        int nextNodeId = 1;
        rootToNodeId[groundRoot] = 0;

        for (const auto& pair : clusters) {
            if (rootToNodeId.find(pair.first) == rootToNodeId.end()) {
                rootToNodeId[pair.first] = nextNodeId++;
            }
        }

        pointToNodeMap.clear();
        for (const auto& pair : ds.parent) pointToNodeMap[pair.first] = rootToNodeId[ds.find(pair.first)];

        int numNonGroundNodes = static_cast<int>(clusters.size()) - (clusters.find(groundRoot) != clusters.end() ? 1 : 0);
        vector<shared_ptr<Component>> vSources;
        for (const auto& comp : components) {
            if (comp->type == ComponentType::VOLTAGE_SOURCE) vSources.push_back(comp);
        }

        int matrixDim = numNonGroundNodes + static_cast<int>(vSources.size());
        if (matrixDim == 0) return;

        MNAMatrix A(matrixDim, matrixDim, 0.0);
        vector<double> z(matrixDim, 0.0);

        for (const auto& comp : components) {
            int nA = pointToNodeMap[makePointKey(comp->getTerminalA().pos)];
            int nB = pointToNodeMap[makePointKey(comp->getTerminalB().pos)];
            double G = 0.0, reqCurrent = 0.0;

            if (comp->type == ComponentType::RESISTOR) G = 1.0 / max(comp->value, 1e-6);
            else if (comp->type == ComponentType::CAPACITOR) { G = comp->value / max(dt, 1e-6); reqCurrent = G * comp->vPrev; }
            else if (comp->type == ComponentType::INDUCTOR) { G = dt / max(comp->value, 1e-6); reqCurrent = -comp->iPrev; }
            else if (comp->type == ComponentType::AMMETER) G = 1.0 / 0.0001;
            else if (comp->type == ComponentType::LED) {
                double vDiff = nodeVoltages[nA] - nodeVoltages[nB];
                if (vDiff > 1.8) { G = 1.0 / 20.0; reqCurrent = G * 1.8; }
                else G = 1e-6;
            }

            if (comp->type == ComponentType::RESISTOR || comp->type == ComponentType::CAPACITOR ||
                comp->type == ComponentType::INDUCTOR || comp->type == ComponentType::AMMETER ||
                comp->type == ComponentType::LED) {
                if (nA > 0) { A[nA - 1][nA - 1] += G; z[nA - 1] += reqCurrent; }
                if (nB > 0) { A[nB - 1][nB - 1] += G; z[nB - 1] -= reqCurrent; }
                if (nA > 0 && nB > 0) { A[nA - 1][nB - 1] -= G; A[nB - 1][nA - 1] -= G; }
            }
            else if (comp->isLogicGate()) {
                int nC = pointToNodeMap[makePointKey(comp->getTerminalC().pos)];
                if (nA > 0) A[nA - 1][nA - 1] += 1e-6;
                if (nB > 0 && comp->type != ComponentType::NOT_GATE) A[nB - 1][nB - 1] += 1e-6;

                bool inA = (nodeVoltages[nA] > 2.5);
                bool inB = (nodeVoltages[nB] > 2.5);
                bool outLogic = evaluateLogicGate(comp->type, inA, inB);

                double Gout = 0.1;
                double targetV = outLogic ? 5.0 : 0.0;
                if (nC > 0) {
                    A[nC - 1][nC - 1] += Gout;
                    z[nC - 1] += Gout * targetV;
                }
            }
        }

        for (size_t k = 0; k < vSources.size(); ++k) {
            auto vSrc = vSources[k];
            int p = pointToNodeMap[makePointKey(vSrc->getTerminalA().pos)];
            int q = pointToNodeMap[makePointKey(vSrc->getTerminalB().pos)];
            int vIdx = numNonGroundNodes + static_cast<int>(k);

            if (p > 0) { A[p - 1][vIdx] += 1.0; A[vIdx][p - 1] += 1.0; }
            if (q > 0) { A[q - 1][vIdx] -= 1.0; A[vIdx][q - 1] -= 1.0; }
            z[vIdx] = vSrc->value;
        }

        vector<double> x;
        if (!MNAMatrix::solve(A, z, x)) return;

        nodeVoltages[0] = 0.0;
        for (int i = 0; i < numNonGroundNodes; ++i) nodeVoltages[i + 1] = x[i];

        for (auto& comp : components) {
            int nA = pointToNodeMap[makePointKey(comp->getTerminalA().pos)];
            int nB = pointToNodeMap[makePointKey(comp->getTerminalB().pos)];
            double vA = nodeVoltages[nA], vB = nodeVoltages[nB];

            if (comp->type == ComponentType::RESISTOR || comp->type == ComponentType::AMMETER) {
                double R = (comp->type == ComponentType::AMMETER) ? 0.0001 : comp->value;
                comp->current = (vA - vB) / R;
            } else if (comp->type == ComponentType::LED) {
                double vDiff = vA - vB;
                comp->current = (vDiff > 1.8) ? (vDiff - 1.8) / 20.0 : 0.0;
            } else if (comp->type == ComponentType::CAPACITOR) {
                double vDiff = vA - vB;
                comp->current = (comp->value / max(dt, 1e-6)) * (vDiff - comp->vPrev);
                comp->vPrev = vDiff;
            } else if (comp->type == ComponentType::INDUCTOR) {
                double vDiff = vA - vB;
                comp->current = comp->iPrev + (dt / comp->value) * vDiff;
                comp->iPrev = comp->current;
            }
            comp->updateParticles(static_cast<float>(dt));
        }

        for (auto& wire : wires) {
            int nA = pointToNodeMap[makePointKey(wire.startPos)];
            int nB = pointToNodeMap[makePointKey(wire.endPos)];
            wire.current = (nodeVoltages[nA] - nodeVoltages[nB]) * 0.1;
            wire.updateParticles(static_cast<float>(dt));
        }

        if (probedNodeId >= 0 && nodeVoltages.find(probedNodeId) != nodeVoltages.end()) {
            oscilloscopeHistory.push_back(static_cast<float>(nodeVoltages[probedNodeId]));
            if (oscilloscopeHistory.size() > 300) oscilloscopeHistory.erase(oscilloscopeHistory.begin());
        }

        simTime += dt;
    }
};

#endif