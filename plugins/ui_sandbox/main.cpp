#include <glad/gl.h>         // MUST be before GLFW
#include <GLFW/glfw3.h>

#include <nanovg.h>

#define NANOVG_GL3_IMPLEMENTATION
#include <nanovg_gl.h>

#include "src/ui/MeterWidget.h"

// NEW UI2 LAYER
#include "src/ui2/core/Rect.h"
#include "src/ui2/core/UiContext.h"
#include "src/ui2/scenes/GainScene.h"

#include <cstdio>
#include <cmath>
#include <algorithm>

namespace gv3::sandbox
{

// Simple UI state for demo
struct SandboxState
{
    float knobValue = 0.5f;      // 0.0 - 1.0
    
    // Simulated meter values (normalized 0-1 like from DSP)
    float meterInL = 0.7f;
    float meterInR = 0.6f;
    float meterOutL = 0.5f;
    float meterOutR = 0.4f;
    
    // Animation time for simulated signal
    float animTime = 0.0f;
    
    double lastTime = 0.0;
    
    // Mouse interaction
    bool knobDragging = false;
    double dragStartY = 0.0;
    float dragStartValue = 0.0f;
    
    // MeterWidget instances (OLD UI)
    ui::MeterWidget meterWidgetInL;
    ui::MeterWidget meterWidgetInR;
    ui::MeterWidget meterWidgetOutL;
    ui::MeterWidget meterWidgetOutR;
    
    // NEW UI2 LAYER (correct namespace: gv3::ui2, not ui2::scenes)
    gv3::ui2::GainScene scene;
    bool showNewUI = true;  // Toggle between old/new UI for comparison
};

// Forward declarations
void renderUI(NVGcontext* vg, int width, int height, SandboxState& state, float deltaTime);
void drawKnob(NVGcontext* vg, float x, float y, float radius, float value);

// GLFW callbacks
static SandboxState* g_state = nullptr;

void errorCallback(int error, const char* description)
{
    fprintf(stderr, "[GLFW Error %d] %s\n", error, description);
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
    
    // Toggle between old/new UI
    if (key == GLFW_KEY_TAB && action == GLFW_PRESS && g_state)
    {
        g_state->showNewUI = !g_state->showNewUI;
        printf("[GV3 Sandbox] UI mode: %s\n", g_state->showNewUI ? "NEW (ui2 zones)" : "OLD (MeterWidget)");
    }
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    if (!g_state) return;
    
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    
    float mx = static_cast<float>(xpos);
    float my = static_cast<float>(ypos);
    
    // Build InputState
    gv3::ui2::InputState inputState;
    inputState.mouseX = mx;
    inputState.mouseY = my;
    inputState.leftButtonDown = (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS);
    inputState.shiftDown = (mods & GLFW_MOD_SHIFT) != 0;
    inputState.ctrlDown = (mods & GLFW_MOD_CONTROL) != 0;
    inputState.altDown = (mods & GLFW_MOD_ALT) != 0;
    
    if (button == GLFW_MOUSE_BUTTON_LEFT)
    {
        if (action == GLFW_PRESS)
        {
            // Try NEW UI2 scene first
            if (g_state->showNewUI && g_state->scene.onMouseDown(mx, my, inputState))
            {
                return; // Event handled by ui2
            }
            
            // Fallback to OLD UI logic
            float dx = mx - 640.0f;
            float dy = my - 200.0f;
            float dist = std::sqrt(dx*dx + dy*dy);
            
            if (dist < 50.0f)
            {
                g_state->knobDragging = true;
                g_state->dragStartY = ypos;
                g_state->dragStartValue = g_state->knobValue;
            }
            else if (g_state->meterWidgetInL.isInsidePeakHoldBounds(mx, my))
            {
                g_state->meterWidgetInL.resetPeakHold();
            }
            else if (g_state->meterWidgetInR.isInsidePeakHoldBounds(mx, my))
            {
                g_state->meterWidgetInR.resetPeakHold();
            }
            else if (g_state->meterWidgetOutL.isInsidePeakHoldBounds(mx, my))
            {
                g_state->meterWidgetOutL.resetPeakHold();
            }
            else if (g_state->meterWidgetOutR.isInsidePeakHoldBounds(mx, my))
            {
                g_state->meterWidgetOutR.resetPeakHold();
            }
        }
        else if (action == GLFW_RELEASE)
        {
            // Try NEW UI2 scene first
            if (g_state->showNewUI && g_state->scene.onMouseUp(mx, my, inputState))
            {
                return;
            }
            
            // OLD UI
            g_state->knobDragging = false;
        }
    }
}

void cursorPosCallback(GLFWwindow* window, double xpos, double ypos)
{
    if (!g_state) return;
    
    float mx = static_cast<float>(xpos);
    float my = static_cast<float>(ypos);
    
    // Build InputState
    gv3::ui2::InputState inputState;
    inputState.mouseX = mx;
    inputState.mouseY = my;
    inputState.leftButtonDown = (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS);
    
    // Try NEW UI2 scene first
    if (g_state->showNewUI && g_state->scene.onMouseMove(mx, my, inputState))
    {
        return; // Event handled
    }
    
    // OLD UI: knob dragging
    if (g_state->knobDragging)
    {
        double delta = g_state->dragStartY - ypos;
        float newValue = g_state->dragStartValue + static_cast<float>(delta) / 200.0f;
        g_state->knobValue = std::clamp(newValue, 0.0f, 1.0f);
    }
}

void scrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    if (!g_state) return;
    
    // Scroll adjusts all input meters
    g_state->meterInL += static_cast<float>(yoffset) * 0.05f;
    g_state->meterInL = std::clamp(g_state->meterInL, 0.0f, 1.0f);
    
    g_state->meterInR += static_cast<float>(yoffset) * 0.05f;
    g_state->meterInR = std::clamp(g_state->meterInR, 0.0f, 1.0f);
}

int runSandbox()
{
    // Initialize GLFW
    glfwSetErrorCallback(errorCallback);
    
    if (!glfwInit())
    {
        fprintf(stderr, "[GV3 Sandbox] Failed to initialize GLFW\n");
        return -1;
    }
    
    // Configure OpenGL context
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
    glfwWindowHint(GLFW_SAMPLES, 4); // MSAA
    
    // Create window
    GLFWwindow* window = glfwCreateWindow(1280, 720, "GV3 UI Sandbox", nullptr, nullptr);
    if (!window)
    {
        fprintf(stderr, "[GV3 Sandbox] Failed to create GLFW window\n");
        glfwTerminate();
        return -1;
    }
    
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable VSync
    
    // Setup callbacks
    glfwSetKeyCallback(window, keyCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetCursorPosCallback(window, cursorPosCallback);
    glfwSetScrollCallback(window, scrollCallback);
    
    // Load OpenGL functions via glad
    int version = gladLoadGL(glfwGetProcAddress);
    if (version == 0)
    {
        fprintf(stderr, "[GV3 Sandbox] Failed to initialize OpenGL via glad\n");
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }
    
    printf("[GV3 Sandbox] OpenGL %d.%d loaded\n", GLAD_VERSION_MAJOR(version), GLAD_VERSION_MINOR(version));
    
    // Create NanoVG context
    NVGcontext* vg = nvgCreateGL3(NVG_ANTIALIAS | NVG_STENCIL_STROKES);
    if (!vg)
    {
        fprintf(stderr, "[GV3 Sandbox] Failed to create NanoVG context\n");
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }
    
    printf("[GV3 Sandbox] NanoVG initialized\n");
    
    // Load system font for UI rendering
    int fontId = nvgCreateFont(vg, "sans", "C:/Windows/Fonts/segoeui.ttf");
    if (fontId == -1)
    {
        fontId = nvgCreateFont(vg, "sans", "C:/Windows/Fonts/arial.ttf");
    }
    if (fontId == -1)
    {
        printf("[GV3 Sandbox] WARNING: Failed to load system font, text may not render\n");
    }
    else
    {
        printf("[GV3 Sandbox] Font loaded successfully (ID: %d)\n", fontId);
    }
    
    // Load bold variant if available
    int fontBoldId = nvgCreateFont(vg, "sans-bold", "C:/Windows/Fonts/segoeuib.ttf");
    if (fontBoldId == -1)
    {
        fontBoldId = nvgCreateFont(vg, "sans-bold", "C:/Windows/Fonts/arialbd.ttf");
    }
    if (fontBoldId == -1)
    {
        // Fallback: use regular font for bold
        printf("[GV3 Sandbox] WARNING: Bold font not found, using regular font\n");
    }
    
    // Initialize state
    SandboxState state;
    state.lastTime = glfwGetTime();
    g_state = &state;
    
    // NEW: Initialize scene layout with initial window size
    int initialWinWidth, initialWinHeight;
    glfwGetWindowSize(window, &initialWinWidth, &initialWinHeight);
    
    gv3::ui2::Rect viewRect{ 0.0f, 0.0f, static_cast<float>(initialWinWidth), static_cast<float>(initialWinHeight) };
    gv3::ui2::UiContext uiContext;
    uiContext.viewRect = viewRect;
    uiContext.uiScale = 1.0f;  // Will update each frame with pixelRatio
    uiContext.dt = 0.016f;
    
    state.scene.layout(viewRect, uiContext);
    
    printf("[GV3 Sandbox] Initial window size: %dx%d\n", initialWinWidth, initialWinHeight);
    printf("[GV3 Sandbox] UI2 scene initialized (zones should be visible)\n");
    printf("[GV3 Sandbox] Press TAB to toggle UI modes, ESC to exit\n");

    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        // Calculate delta time
        double currentTime = glfwGetTime();
        double deltaTime = currentTime - state.lastTime;
        state.lastTime = currentTime;
        
        // Sync knob widget value ↔ state.knobValue
        if (state.showNewUI)
        {
            // UI2: widget drives state
            state.knobValue = state.scene.getKnob().getValue();
        }
        else
        {
            // OLD UI: state drives widget
            state.scene.getKnob().setValue(state.knobValue);
        }
        
        // Simulate animated meter values (like audio signal)
        state.animTime += static_cast<float>(deltaTime);
        float pulse = 0.5f + 0.5f * std::sin(state.animTime * 2.0f);
        
        // Output meters follow knob gain value
        float gain = state.knobValue;
        state.meterOutL = state.meterInL * gain;
        state.meterOutR = state.meterInR * gain;
        
        // Add some animation to input meters
        state.meterInL = std::clamp(state.meterInL + (pulse - 0.5f) * 0.005f, 0.0f, 1.0f);
        state.meterInR = std::clamp(state.meterInR + (pulse - 0.3f) * 0.005f, 0.0f, 1.0f);
        
        // Convert linear amplitude to normalized dB range [0-1] for [-60dB, +6dB]
        auto toNormDb = [](float linearAmp) -> float {
            constexpr float kMinDb = -60.0f;
            constexpr float kMaxDb = 6.0f;
            constexpr float kRange = kMaxDb - kMinDb;
            float db = 20.0f * std::log10(std::max(linearAmp, 1.0e-6f));
            return std::clamp((db - kMinDb) / kRange, 0.0f, 1.0f);
        };
        
        // Update MeterWidget states (with normalized dB values)
        state.meterWidgetInL.update(toNormDb(state.meterInL));
        state.meterWidgetInR.update(toNormDb(state.meterInR));
        state.meterWidgetOutL.update(toNormDb(state.meterOutL));
        state.meterWidgetOutR.update(toNormDb(state.meterOutR));
        
        // Get framebuffer size for HiDPI displays
        int fbWidth, fbHeight;
        glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
        
        int winWidth, winHeight;
        glfwGetWindowSize(window, &winWidth, &winHeight);
        
        float pixelRatio = static_cast<float>(fbWidth) / static_cast<float>(winWidth);
        
        // OpenGL clear
        glViewport(0, 0, fbWidth, fbHeight);
        glClearColor(0.15f, 0.15f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        
        // NanoVG frame
        nvgBeginFrame(vg, static_cast<float>(winWidth), static_cast<float>(winHeight), pixelRatio);
        
        renderUI(vg, winWidth, winHeight, state, static_cast<float>(deltaTime));
        
        nvgEndFrame(vg);
        
        // Swap and poll
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    
    // Cleanup
    g_state = nullptr;
    nvgDeleteGL3(vg);
    glfwDestroyWindow(window);
    glfwTerminate();
    
    printf("[GV3 Sandbox] Shutdown complete\n");
}

void renderUI(NVGcontext* vg, int width, int height, SandboxState& state, float deltaTime)
{
    // Background panel
    nvgBeginPath(vg);
    nvgRect(vg, 0, 0, static_cast<float>(width), static_cast<float>(height));
    nvgFillColor(vg, nvgRGBA(30, 30, 35, 255));
    nvgFill(vg);
    
    // NEW UI2: Update context and redraw scene zones
    gv3::ui2::Rect viewRect{ 0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height) };
    gv3::ui2::UiContext uiContext;
    uiContext.viewRect = viewRect;
    uiContext.uiScale = 1.0f;  // pixelRatio is passed by nvgBeginFrame, so 1.0 here
    uiContext.dt = deltaTime;
    
    // Recalculate layout on each frame (for resize detection)
    state.scene.layout(viewRect, uiContext);
    
    // Draw NEW UI2 zones (outlines + labels + knob)
    state.scene.draw(vg, uiContext);
    
    // Title
    nvgFontSize(vg, 32.0f);
    nvgFontFace(vg, "sans");
    nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_TOP);
    nvgFillColor(vg, nvgRGBA(255, 255, 255, 200));
    nvgText(vg, static_cast<float>(width) * 0.5f, 30.0f, "GV3 UI Sandbox - NEW UI2 Layer (Responsive Zones)", nullptr);
    
    // Instructions
    nvgFontSize(vg, 14.0f);
    nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_TOP);
    nvgFillColor(vg, nvgRGBA(180, 180, 180, 255));
    nvgText(vg, static_cast<float>(width) * 0.5f, 75.0f, "Drag knob to change gain | Resize window to see adaptive layout | TAB to toggle UI | ESC to exit", nullptr);
}

void drawKnob(NVGcontext* vg, float x, float y, float radius, float value)
{
    // (This function is no longer used — KnobWidget handles drawing)
    // Kept for compatibility with old UI mode if needed
}

} // namespace gv3::sandbox

int main()
{
    return gv3::sandbox::runSandbox();
}
