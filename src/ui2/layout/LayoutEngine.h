#pragma once

#include "../core/Rect.h"
#include <algorithm>

namespace gv3::ui2
{

/**
 * @brief Layout result for GainPlugin UI.
 * 
 * Contains rectangles for all major UI zones.
 */
struct GainLayout
{
    Rect top;       ///< Top zone (knob + gain readout)
    Rect meterIn;   ///< Input meter zone (left)
    Rect meterOut;  ///< Output meter zone (right)
    Rect bottom;    ///< Bottom zone (waveform/spectrum)

    GainLayout() = default;
};

/**
 * @brief Responsive layout engine for GainPlugin UI.
 * 
 * Computes zone rectangles based on viewport size and DPI scale.
 * All zones adapt smoothly to window resize.
 * 
 * Design principles:
 * - All sizes computed as percentage of viewport + clamping
 * - No hardcoded "magic pixel values"
 * - Graceful degradation on small sizes
 */
class LayoutEngine
{
public:
    /**
     * @brief Compute responsive layout for GainPlugin.
     * 
     * @param view Viewport rectangle (logical pixels)
     * @param uiScale DPI scale factor (e.g., 2.0 for Retina)
     * @return GainLayout with computed zones
     */
    GainLayout computeGainLayout(const Rect& view, float uiScale) const;

private:
    // Layout constraints (in logical pixels, scaled by uiScale)
    static constexpr float MIN_METER_WIDTH = 40.0f;
    static constexpr float MAX_METER_WIDTH = 80.0f;
    static constexpr float MIN_TOP_HEIGHT = 120.0f;
    static constexpr float MAX_TOP_HEIGHT = 240.0f;
    static constexpr float MIN_BOTTOM_HEIGHT = 60.0f;
    static constexpr float MAX_BOTTOM_HEIGHT = 200.0f;
};

} // namespace gv3::ui2
