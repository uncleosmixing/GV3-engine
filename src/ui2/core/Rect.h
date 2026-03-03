#pragma once

namespace gv3::ui2
{

/**
 * @brief Simple rectangle for UI layout.
 * 
 * Used throughout ui2 layer for positioning and bounds checking.
 * All coordinates are in logical pixels (before DPI scaling).
 */
struct Rect
{
    float x = 0.0f;
    float y = 0.0f;
    float w = 0.0f;
    float h = 0.0f;

    Rect() = default;
    Rect(float x_, float y_, float w_, float h_) : x(x_), y(y_), w(w_), h(h_) {}

    // Getters
    float right() const { return x + w; }
    float bottom() const { return y + h; }
    float centerX() const { return x + w * 0.5f; }
    float centerY() const { return y + h * 0.5f; }

    // Bounds checking
    bool contains(float px, float py) const
    {
        return px >= x && px < right() && py >= y && py < bottom();
    }

    // Create inset rect (shrink by padding)
    Rect inset(float padding) const
    {
        return Rect(x + padding, y + padding, 
                   w - padding * 2.0f, h - padding * 2.0f);
    }

    // Create rect with adjusted dimensions
    Rect withSize(float newW, float newH) const
    {
        return Rect(x, y, newW, newH);
    }

    // Split horizontally (left part)
    Rect splitLeft(float width) const
    {
        return Rect(x, y, width, h);
    }

    // Split horizontally (right part)
    Rect splitRight(float width) const
    {
        return Rect(right() - width, y, width, h);
    }

    // Check if valid (non-negative size)
    bool isValid() const { return w > 0.0f && h > 0.0f; }
};

} // namespace gv3::ui2
