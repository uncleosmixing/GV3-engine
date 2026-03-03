# GV3 UI Sandbox

## Overview

The GV3 UI Sandbox is a standalone development environment for creating and testing NanoVG-based UI components without requiring a DAW or VST3 host. It provides rapid iteration cycles for UI development while maintaining code compatibility with the VST3 plugin.

## Architecture

### Technology Stack

- **Windowing & Input**: GLFW 3.4 (git submodule at `external/glfw`)
- **OpenGL Loading**: glad (via `external/glad`)
- **Vector Graphics**: NanoVG (via `external/nanovg`) with GL3 backend
- **Graphics API**: OpenGL 3.3+ Core Profile

### Design Principles

1. **Separation of Concerns**: Sandbox code lives in `plugins/ui_sandbox/`, separate from VST3 adapter
2. **Shared UI Code**: UI components can be shared between sandbox and VST3 plugin
3. **No VST3 Dependencies**: Sandbox builds without VST3 SDK
4. **Minimal Dependencies**: Only GLFW added for windowing (NanoVG/glad already present)

## Building

### Enable/Disable Sandbox

The sandbox is enabled by default. To disable:

```bash
cmake --preset x64-release -DGV3_BUILD_UI_SANDBOX=OFF
```

### Build Sandbox

```bash
cmake --build out/build/x64-release --config Release --target GV3UiSandbox
```

Output: `out/build/x64-release/Release/GV3UiSandbox.exe`

### Dependencies

GLFW is a git submodule. If not present after cloning:

```bash
git submodule update --init external/glfw
```

CMake will automatically detect missing submodules and fall back to disabling the sandbox with a warning.

## Running

```bash
# Windows
.\out\build\x64-release\Release\GV3UiSandbox.exe

# Cross-platform
cmake --build out/build/x64-release --config Release --target GV3UiSandbox
```

### Controls

- **Mouse**: Drag knob to adjust gain value
- **Scroll Wheel**: Adjust input meter levels
- **Click**: Click on peak hold value (yellow text at bottom of meter) to reset
- **ESC**: Exit application

## Current Demo Features

The sandbox demonstrates professional dBFS metering with the following features:

1. **Window Management**
   - 1280x720 resizable window
   - OpenGL 3.3 Core Profile context
   - VSync enabled (60 FPS)
   - HiDPI/Retina display support

2. **UI Components**
   - **Knob**: Interactive gain control (maps to -60dB to +24dB)
   - **Professional dBFS Meters**: Four vertical meters (INPUT L/R, OUTPUT L/R) with:
     * Static dBFS scale with tick marks (0, -6, -12, -18, -24, -36, -48, -60 dB)
     * Dynamic momentary peak bar (color-coded: green/yellow/red zones)
     * Peak hold display at bottom (maximum dBFS reached, updates only upward)
     * Peak marker line (bright yellow horizontal line showing max level on scale)
     * Click-to-reset peak hold functionality
   - **Background Panel**: Dark theme with title/instructions

3. **Input Handling**
   - Mouse button events (knob dragging, peak hold reset)
   - Mouse motion events (value updates)
   - Scroll wheel events (input meter control)
   - Keyboard events (ESC to exit)

4. **Signal Simulation**
   - Animated input meters with sine wave modulation
   - Output meters follow input × gain coefficient
   - Real-time peak hold tracking

## Extending the Sandbox

### Adding New UI Components

1. Create component drawing function in `plugins/ui_sandbox/main.cpp`:
```cpp
void drawMyComponent(NVGcontext* vg, float x, float y, float value)
{
    // NanoVG drawing code
}
```

2. Call from `renderUI()`:
```cpp
void renderUI(NVGcontext* vg, int width, int height, SandboxState& state)
{
    // ...existing code...
    drawMyComponent(vg, x, y, state.myValue);
}
```

3. Add state if needed:
```cpp
struct SandboxState
{
    float myValue = 0.0f;
    // ...
};
```

### Sharing Code with VST3 Plugin

To share UI components between sandbox and plugin:

1. **Create shared UI module** in `src/ui/`:
```cpp
// src/ui/Knob.h
#pragma once
#include <nanovg.h>

namespace gv3::ui
{
    void drawKnob(NVGcontext* vg, float x, float y, float radius, float value);
}
```

2. **Use in both sandbox and VST3**:
```cpp
// plugins/ui_sandbox/main.cpp
#include "src/ui/Knob.h"
gv3::ui::drawKnob(vg, x, y, 50.0f, state.knobValue);

// vst3/GV3EditorView.cpp
#include "src/ui/Knob.h"
gv3::ui::drawKnob(vg, x, y, 50.0f, paramValue);
```

3. **Link shared code**:
```cmake
# In CMakeLists.txt
target_sources(GV3UiSandbox PRIVATE src/ui/Knob.cpp)
```

## Best Practices

### Performance

- NanoVG is immediate-mode, so redraw entire scene each frame
- Use VSync to limit framerate (enabled by default)
- Minimize state changes in render loop
- Batch similar drawing operations

### Realtime Safety

- Sandbox runs in separate thread from audio (no realtime constraints)
- Shared UI code must remain allocation-free if used in VST3 editor
- Use atomic/lock-free patterns for audio→UI data transfer in plugin context

### HiDPI Support

```cpp
int fbWidth, fbHeight;
glfwGetFramebufferSize(window, &fbWidth, &fbHeight);

int winWidth, winHeight;
glfwGetWindowSize(window, &winWidth, &winHeight);

float pixelRatio = (float)fbWidth / (float)winWidth;

nvgBeginFrame(vg, winWidth, winHeight, pixelRatio);
```

Always pass correct `pixelRatio` to `nvgBeginFrame()` for sharp rendering on Retina/4K displays.

## CMake Integration

### GLFW Configuration

```cmake
# cmake/Dependencies.cmake
if (GV3_BUILD_UI_SANDBOX)
  set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
  set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
  set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
  
  add_subdirectory("${GLFW_DIR}" "${CMAKE_BINARY_DIR}/glfw")
endif()
```

### Target Definition

```cmake
# CMakeLists.txt
if (GV3_BUILD_UI_SANDBOX AND TARGET glfw)
  add_executable(GV3UiSandbox "plugins/ui_sandbox/main.cpp")
  target_compile_features(GV3UiSandbox PRIVATE cxx_std_17)
  target_link_libraries(GV3UiSandbox PRIVATE glfw opengl32)
  # ... add glad, NanoVG sources ...
endif()
```

## Troubleshooting

### GLFW not found

```
CMake Warning at cmake/Dependencies.cmake:XX (message):
  [GV3] GLFW: Not found at external/glfw
  [GV3] GLFW: Run 'git submodule update --init external/glfw'
```

**Solution**: Initialize submodule:
```bash
git submodule update --init external/glfw
```

### OpenGL header conflicts

```
error C1189: #error: OpenGL (gl.h) header already included
```

**Solution**: Include glad BEFORE GLFW:
```cpp
#include <glad/gl.h>  // MUST be first
#include <GLFW/glfw3.h>
```

### Window doesn't appear

Check console output for initialization errors:
```
[GV3 Sandbox] OpenGL 3.3 loaded
[GV3 Sandbox] NanoVG initialized
```

If missing, check OpenGL driver version (requires 3.3+).

## Future Enhancements

Planned features for UI Sandbox:

- [ ] Hot-reload for UI code (recompile without restart)
- [ ] Parameter automation playback
- [ ] VST3 parameter mapping (live plugin connection)
- [ ] UI layout editor/inspector
- [ ] Theme switcher (light/dark modes)
- [ ] Component library browser
- [ ] Screenshot/recording functionality

## References

- [GLFW Documentation](https://www.glfw.org/documentation.html)
- [NanoVG API](https://github.com/memononen/nanovg)
- [OpenGL Core Profile](https://www.khronos.org/opengl/wiki/Core_Profile)
- [GV3 Copilot Instructions](../.github/copilot-instructions.md)
