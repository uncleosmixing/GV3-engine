#pragma once

#include "../core/Widget.h"
#include "../core/Rect.h"
#include <nanovg.h>

namespace gv3::ui2
{

/**
 * @brief Rotary knob widget with drag interaction.
 * 
 * Features:
 * - Smooth drag response (vertical mouse movement changes value)
 * - Visual indicator showing current value
 * - Radial gradient styling
 * - Configurable range (maps internal 0-1 to any dB/linear range)
 * 
 * Usage:
 *   KnobWidget knob;
 *   knob.layout(bounds, ctx);
 *   knob.draw(vg, ctx);
 *   if (knob.onMouseDown(...)) { handle drag start }
 */
class KnobWidget : public IWidget
{
public:
    KnobWidget();

    // Value accessors (normalized 0-1)
    void setValue(float value);
    float getValue() const { return m_value; }

    // Drag sensitivity (pixels per full range sweep)
    void setSensitivity(float pixels) { m_sensitivity = pixels; }

    // IWidget interface
    void layout(const Rect& bounds, const UiContext& ctx) override;
    void draw(NVGcontext* vg, const UiContext& ctx) override;
    bool hitTest(float x, float y) const override;
    
    bool onMouseDown(float x, float y, const InputState& input) override;
    bool onMouseUp(float x, float y, const InputState& input) override;
    bool onMouseMove(float x, float y, const InputState& input) override;

private:
    void drawKnob(NVGcontext* vg);

    Rect m_bounds;           // Allocated space for knob
    float m_centerX = 0.0f;  // Circle center X
    float m_centerY = 0.0f;  // Circle center Y
    float m_radius = 0.0f;   // Circle radius

    float m_value = 0.5f;            // Current value (0-1)
    bool m_dragging = false;         // Is user dragging?
    float m_dragStartY = 0.0f;       // Y coordinate where drag started
    float m_dragStartValue = 0.0f;   // Value when drag started
    float m_sensitivity = 200.0f;    // Pixels per full range (default: 200px = 0→1)
};

} // namespace gv3::ui2
