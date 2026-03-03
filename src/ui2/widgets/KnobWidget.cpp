#include "KnobWidget.h"
#include <algorithm>
#include <cmath>

namespace gv3::ui2
{

KnobWidget::KnobWidget()
{
}

void KnobWidget::setValue(float value)
{
    m_value = std::clamp(value, 0.0f, 1.0f);
}

void KnobWidget::layout(const Rect& bounds, const UiContext& ctx)
{
    m_bounds = bounds;

    // Calculate circle parameters (fit inside bounds with padding)
    float padding = 10.0f * ctx.uiScale;
    float availableWidth = bounds.w - padding * 2.0f;
    float availableHeight = bounds.h - padding * 2.0f;

    // Radius: fit to smallest dimension
    m_radius = std::min(availableWidth, availableHeight) * 0.5f;
    m_radius = std::max(m_radius, 10.0f * ctx.uiScale); // Minimum 10px

    // Center in bounds
    m_centerX = bounds.centerX();
    m_centerY = bounds.centerY();
}

void KnobWidget::draw(NVGcontext* vg, const UiContext& ctx)
{
    if (!vg || !m_bounds.isValid()) return;

    drawKnob(vg);
}

bool KnobWidget::hitTest(float x, float y) const
{
    // Hit test: inside circle
    float dx = x - m_centerX;
    float dy = y - m_centerY;
    float distSq = dx * dx + dy * dy;
    return distSq <= (m_radius * m_radius);
}

bool KnobWidget::onMouseDown(float x, float y, const InputState& input)
{
    if (!input.leftButtonDown) return false;

    if (hitTest(x, y))
    {
        m_dragging = true;
        m_dragStartY = y;
        m_dragStartValue = m_value;
        return true; // Event handled
    }

    return false;
}

bool KnobWidget::onMouseUp(float x, float y, const InputState& input)
{
    if (m_dragging)
    {
        m_dragging = false;
        return true;
    }
    return false;
}

bool KnobWidget::onMouseMove(float x, float y, const InputState& input)
{
    if (!m_dragging) return false;

    // Vertical drag: negative Y = increase value (knob turns clockwise)
    float deltaY = m_dragStartY - y;
    float deltaValue = deltaY / m_sensitivity;

    m_value = std::clamp(m_dragStartValue + deltaValue, 0.0f, 1.0f);

    return true; // Event handled
}

void KnobWidget::drawKnob(NVGcontext* vg)
{
    // Outer ring
    nvgBeginPath(vg);
    nvgCircle(vg, m_centerX, m_centerY, m_radius);
    nvgStrokeWidth(vg, 3.0f);
    nvgStrokeColor(vg, nvgRGBA(60, 60, 70, 255));
    nvgStroke(vg);

    // Fill with radial gradient
    NVGpaint bg = nvgRadialGradient(vg,
        m_centerX, m_centerY - m_radius * 0.3f,
        m_radius * 0.3f,
        m_radius * 1.2f,
        nvgRGBA(50, 50, 60, 255),
        nvgRGBA(30, 30, 35, 255));
    nvgBeginPath(vg);
    nvgCircle(vg, m_centerX, m_centerY, m_radius);
    nvgFillPaint(vg, bg);
    nvgFill(vg);

    // Value indicator (line from center)
    // Map value to angle: -135° to +135° (270° range)
    float minAngle = -2.35619f; // -135 deg in radians
    float maxAngle = 2.35619f;  // +135 deg in radians
    float angle = minAngle + m_value * (maxAngle - minAngle);

    float indicatorLen = m_radius * 0.7f;
    float endX = m_centerX + std::cos(angle) * indicatorLen;
    float endY = m_centerY + std::sin(angle) * indicatorLen;

    nvgBeginPath(vg);
    nvgMoveTo(vg, m_centerX, m_centerY);
    nvgLineTo(vg, endX, endY);
    nvgStrokeWidth(vg, 4.0f);
    nvgStrokeColor(vg, nvgRGBA(100, 180, 255, 255)); // Cyan indicator
    nvgStroke(vg);

    // Center dot
    nvgBeginPath(vg);
    nvgCircle(vg, m_centerX, m_centerY, 5.0f);
    nvgFillColor(vg, nvgRGBA(80, 150, 220, 255));
    nvgFill(vg);

    // Highlight if dragging
    if (m_dragging)
    {
        nvgBeginPath(vg);
        nvgCircle(vg, m_centerX, m_centerY, m_radius + 2.0f);
        nvgStrokeWidth(vg, 2.0f);
        nvgStrokeColor(vg, nvgRGBA(100, 200, 255, 150)); // Glow effect
        nvgStroke(vg);
    }
}

} // namespace gv3::ui2
