#pragma once

#include <nanovg.h>
#include <cmath>
#include <algorithm>

namespace gv3
{
namespace ui
{

/**
 * @brief Professional dBFS meter widget with static scale, momentary peak, and peak hold.
 * 
 * Features:
 * - Static dBFS scale with tick marks (0, -6, -12, -18, -24, -36, -48, -60 dB)
 * - Dynamic momentary peak bar
 * - Peak hold display at bottom (max dBFS reached, updates only upward)
 * - Peak marker line on scale showing maximum level
 * - Click-to-reset peak hold functionality
 * 
 * Real-time safe: no allocations, all state in UI thread only.
 */
class MeterWidget
{
public:
    MeterWidget();

    /**
     * Update meter with new momentary peak value (normalized 0-1 from DSP).
     * @param normalizedPeak Normalized peak value (0-1), will be converted to dBFS
     * @param refLevel Reference level used for normalization (e.g., 0.316 for -10dB ref)
     */
    void update(float normalizedPeak, float refLevel = 0.316f);

    /**
     * Reset peak hold to floor value (-120 dB).
     */
    void resetPeakHold();

    /**
     * Draw meter widget using NanoVG.
     * @param vg NanoVG context
     * @param x Left position
     * @param y Top position
     * @param width Widget width
     * @param height Widget height
     * @param label Optional label text (e.g., "IN L", "OUT R")
     */
    void draw(NVGcontext* vg, float x, float y, float width, float height, const char* label = nullptr);

    /**
     * Check if a point is inside the peak hold clickable region.
     * @param mouseX Mouse X coordinate
     * @param mouseY Mouse Y coordinate
     * @return true if inside clickable bounds
     */
    bool isInsidePeakHoldBounds(float mouseX, float mouseY) const;

    /**
     * Get current momentary dBFS value.
     */
    float getMomentaryDbFS() const { return m_momentaryDbFS; }

    /**
     * Get current peak hold dBFS value.
     */
    float getPeakHoldDbFS() const { return m_peakHoldDbFS; }

private:
    /**
     * Convert normalized 0-1 peak to dBFS.
     * @param normalizedPeak Normalized peak (0-1)
     * @param refLevel Reference level for denormalization
     * @return dBFS value (with floor at -120dB)
     */
    static float normalizedToDbFS(float normalizedPeak, float refLevel);

    /**
     * Map dBFS value to vertical position within meter scale.
     * @param dbfs dBFS value
     * @param scaleTop Top Y of scale
     * @param scaleHeight Height of scale
     * @return Y position (0 = top/0dB, scaleHeight = bottom/-60dB)
     */
    static float dbfsToY(float dbfs, float scaleTop, float scaleHeight);

    // State
    float m_momentaryDbFS;  ///< Current momentary peak in dBFS
    float m_peakHoldDbFS;   ///< Maximum peak hold in dBFS (UI-side storage)
    
    // Layout cache (updated during draw)
    float m_lastX, m_lastY, m_lastWidth, m_lastHeight;
    float m_peakHoldTextY;  ///< Y position of peak hold text (for click detection)
    
    // Constants
    static constexpr float FLOOR_DB = -120.0f;  ///< Minimum dBFS value (silence floor)
    static constexpr float SCALE_MIN_DB = -60.0f;  ///< Bottom of visible scale
    static constexpr float SCALE_MAX_DB = 6.0f;    ///< Top of visible scale (matches ProcessorBase range!)
};

} // namespace ui
} // namespace gv3
