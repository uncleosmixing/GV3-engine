#pragma once

namespace gv3::ui2
{

/**
 * @brief Mouse/keyboard input state (snapshot for single frame).
 * 
 * Captured by platform layer and passed to UI event handlers.
 */
struct InputState
{
    // Mouse position (logical pixels)
    float mouseX = 0.0f;
    float mouseY = 0.0f;

    // Mouse buttons (bit flags)
    bool leftButtonDown = false;
    bool rightButtonDown = false;
    bool middleButtonDown = false;

    // Mouse wheel delta (accumulated since last frame)
    float wheelDeltaX = 0.0f;
    float wheelDeltaY = 0.0f;

    // Modifier keys
    bool shiftDown = false;
    bool ctrlDown = false;
    bool altDown = false;

    InputState() = default;

    void reset()
    {
        wheelDeltaX = 0.0f;
        wheelDeltaY = 0.0f;
    }
};

} // namespace gv3::ui2
