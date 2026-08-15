#ifndef APP_STATE_HPP
#define APP_STATE_HPP

#include "raylib.h"
#include "Component.hpp"
#include "ToolMode.hpp"
#include <memory>
#include <string>

using namespace std;

// All of the editor's mutable UI state, grouped in one place so the
// input-handling and drawing code don't need a long list of loose locals.
struct AppState {
    ToolMode currentTool = ToolMode::SELECT;
    shared_ptr<Component> selectedComp = nullptr;
    int selectedWireIdx = -1;

    bool isDragging = false;
    bool isWiring = false;
    Vector2 wireStartPos = { 0, 0 };

    bool pressedOnSwitch = false;
    Vector2 pressPos = { 0, 0 };

    bool isEditingValue = false;
    string editBuffer = "";
};

#endif
