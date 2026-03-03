#pragma once

#include <nanovg.h>
#include "UiContext.h"
#include "Rect.h"
#include "InputState.h"

namespace gv3::ui2
{

/**
 * @brief Base interface for all UI widgets.
 * 
 * Widgets are self-contained UI elements that:
 * - Accept a Rect during layout
 * - Draw themselves within that Rect
 * - Handle mouse/keyboard input
 * 
 * All coordinates are in logical pixels (before DPI scaling).
 */
class IWidget
{
public:
    virtual ~IWidget() = default;

    /**
     * @brief Compute layout for this widget within given bounds.
     * 
     * Called when parent container resizes or UI scale changes.
     * Widget should cache its layout Rect for drawing/hit-testing.
     * 
     * @param bounds Available rectangle for this widget
     * @param ctx UI context (scale, viewport, etc.)
     */
    virtual void layout(const Rect& bounds, const UiContext& ctx) = 0;

    /**
     * @brief Draw widget using NanoVG.
     * 
     * Must respect bounds set during layout().
     * Should use ScissorGuard if drawing clipped content.
     * 
     * @param vg NanoVG context
     * @param ctx UI context
     */
    virtual void draw(NVGcontext* vg, const UiContext& ctx) = 0;

    /**
     * @brief Test if point is inside widget bounds.
     * 
     * @param x X coordinate (logical pixels)
     * @param y Y coordinate (logical pixels)
     * @return true if point is inside
     */
    virtual bool hitTest(float x, float y) const = 0;

    // --- Input event handlers (return true if handled) ---

    virtual bool onMouseDown(float x, float y, const InputState& input) { return false; }
    virtual bool onMouseUp(float x, float y, const InputState& input) { return false; }
    virtual bool onMouseMove(float x, float y, const InputState& input) { return false; }
    virtual bool onMouseWheel(float deltaX, float deltaY, const InputState& input) { return false; }
    
    virtual bool onKeyDown(int key, int scancode, const InputState& input) { return false; }
    virtual bool onKeyUp(int key, int scancode, const InputState& input) { return false; }
};

} // namespace gv3::ui2
