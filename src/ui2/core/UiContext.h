#pragma once

#include "Rect.h"

namespace gv3::ui2
{

/**
 * @brief UI rendering context (passed to layout/draw methods).
 * 
 * Contains global UI state like DPI scale, delta time, and viewport.
 * Immutable during single frame render.
 */
struct UiContext
{
    /// DPI/Retina scale factor (e.g., 1.0 for standard, 2.0 for Retina)
    float uiScale = 1.0f;

    /// Delta time since last frame (seconds) - for animations
    float dt = 0.0f;

    /// Current viewport rectangle (logical pixels)
    Rect viewRect;

    UiContext() = default;
    UiContext(float scale, float deltaTime, const Rect& view)
        : uiScale(scale), dt(deltaTime), viewRect(view)
    {}
};

} // namespace gv3::ui2
