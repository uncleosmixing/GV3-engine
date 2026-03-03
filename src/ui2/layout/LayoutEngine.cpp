#include "LayoutEngine.h"
#include <algorithm>

namespace gv3::ui2
{

GainLayout LayoutEngine::computeGainLayout(const Rect& view, float uiScale) const
{
    GainLayout result;

    if (!view.isValid() || view.w < 200.0f || view.h < 200.0f)
    {
        // Viewport too small - return empty layout
        return result;
    }

    // Scale constraints by DPI
    const float minMeterW = MIN_METER_WIDTH * uiScale;
    const float maxMeterW = MAX_METER_WIDTH * uiScale;
    const float minTopH = MIN_TOP_HEIGHT * uiScale;
    const float maxTopH = MAX_TOP_HEIGHT * uiScale;
    const float minBottomH = MIN_BOTTOM_HEIGHT * uiScale;
    const float maxBottomH = MAX_BOTTOM_HEIGHT * uiScale;

    const float padding = 8.0f * uiScale;

    // --- Vertical split: Top / Middle (meters) / Bottom ---

    // Top: 30-40% of height, clamped
    float topHeight = std::clamp(view.h * 0.35f, minTopH, maxTopH);

    // Bottom: 20-30% of height, clamped
    float bottomHeight = std::clamp(view.h * 0.25f, minBottomH, maxBottomH);

    // Middle (meters): remaining space
    float middleHeight = view.h - topHeight - bottomHeight - padding * 2;
    if (middleHeight < 100.0f * uiScale)
    {
        // Not enough space - shrink bottom
        bottomHeight = std::max(minBottomH, view.h - topHeight - 100.0f * uiScale - padding * 2);
        middleHeight = view.h - topHeight - bottomHeight - padding * 2;
    }

    // --- Top zone ---
    result.top = Rect(view.x, view.y, view.w, topHeight);

    // --- Meter zones (left/right split) ---
    float metersY = view.y + topHeight + padding;

    // Each meter: 15-20% of width, clamped
    float meterWidth = std::clamp(view.w * 0.15f, minMeterW, maxMeterW);

    // Center meters with gap
    float totalMetersWidth = meterWidth * 2 + padding;
    float metersStartX = view.x + (view.w - totalMetersWidth) * 0.5f;

    result.meterIn = Rect(metersStartX, metersY, meterWidth, middleHeight);
    result.meterOut = Rect(metersStartX + meterWidth + padding, metersY, meterWidth, middleHeight);

    // --- Bottom zone ---
    result.bottom = Rect(view.x, view.y + view.h - bottomHeight, view.w, bottomHeight);

    return result;
}

} // namespace gv3::ui2
