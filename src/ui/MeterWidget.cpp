#include "MeterWidget.h"
#include <cstdio>
#include <cstring>

namespace gv3
{
namespace ui
{

MeterWidget::MeterWidget()
    : m_momentaryDbFS(FLOOR_DB)
    , m_peakHoldDbFS(FLOOR_DB)
    , m_lastX(0.0f)
    , m_lastY(0.0f)
    , m_lastWidth(0.0f)
    , m_lastHeight(0.0f)
    , m_peakHoldTextY(0.0f)
{
}

void MeterWidget::update(float normalizedPeak, float refLevel)
{
    // normalizedPeak is already in dB range (0-1 for SCALE_MIN_DB to SCALE_MAX_DB from ProcessorBase::toNorm)
    // Convert from normalized [0, 1] to dBFS [SCALE_MIN_DB, SCALE_MAX_DB]
    m_momentaryDbFS = SCALE_MIN_DB + normalizedPeak * (SCALE_MAX_DB - SCALE_MIN_DB);
    
    // Clamp to floor
    m_momentaryDbFS = std::max(m_momentaryDbFS, FLOOR_DB);
    
    // Update peak hold (only if new peak is higher)
    m_peakHoldDbFS = std::max(m_peakHoldDbFS, m_momentaryDbFS);
}

void MeterWidget::resetPeakHold()
{
    m_peakHoldDbFS = FLOOR_DB;
}

float MeterWidget::normalizedToDbFS(float normalizedPeak, float refLevel)
{
    // Denormalize: linearPeak = normalizedPeak * refLevel
    float linearPeak = normalizedPeak * refLevel;
    
    // Convert to dBFS: 20 * log10(linearPeak)
    // Add epsilon to avoid log10(0) = -inf
    constexpr float epsilon = 1e-30f;
    float dbfs = 20.0f * std::log10(linearPeak + epsilon);
    
    // Clamp to floor
    return std::max(dbfs, FLOOR_DB);
}

float MeterWidget::dbfsToY(float dbfs, float scaleTop, float scaleHeight)
{
    // Clamp dBFS to scale range [SCALE_MIN_DB, SCALE_MAX_DB]
    dbfs = std::clamp(dbfs, SCALE_MIN_DB, SCALE_MAX_DB);
    
    // Map: 0 dB -> scaleTop, -60 dB -> scaleTop + scaleHeight
    // Normalize to [0, 1]: (dbfs - SCALE_MIN_DB) / (SCALE_MAX_DB - SCALE_MIN_DB)
    float norm = (dbfs - SCALE_MIN_DB) / (SCALE_MAX_DB - SCALE_MIN_DB);
    
    // Invert: 0 dB at top (y=0), -60 dB at bottom (y=scaleHeight)
    return scaleTop + scaleHeight * (1.0f - norm);
}

void MeterWidget::draw(NVGcontext* vg, float x, float y, float width, float height, const char* label)
{
    // Cache layout for click detection
    m_lastX = x;
    m_lastY = y;
    m_lastWidth = width;
    m_lastHeight = height;
    
    // Layout constants
    const float padding = 3.0f;
    
    // SIMPLIFIED: Always use LARGE mode for maximum visibility
    // (removed adaptive small/medium modes that made things invisible)
    const float scaleWidth = 30.0f;  // Always show full scale with numbers
    const float trackWidth = width - scaleWidth - padding * 2;
    const float barWidth = trackWidth - 4.0f;  // Bar narrower than track for depth
    const float labelHeight = label ? 16.0f : 0.0f;
    const float peakTextHeight = 18.0f;  // Always show peak hold text
    const float scaleHeight = height - labelHeight - peakTextHeight - padding * 3;
    
    const float scaleX = x + padding;
    const float scaleY = y + labelHeight + padding;
    const float trackX = scaleX + scaleWidth + padding;
    const float trackY = scaleY;
    const float barX = trackX + 2.0f;  // Centered in track
    const float barY = trackY;
    
    // Draw label at top (always visible)
    if (label)
    {
        nvgFontSize(vg, 11.0f);
        nvgFontFace(vg, "sans-bold");
        nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_TOP);
        nvgFillColor(vg, nvgRGBA(200, 200, 200, 255));
        nvgText(vg, x + width * 0.5f, y + 1.0f, label, nullptr);
    }
    
    // Draw static dBFS scale with tick marks (always visible)
    {
        // Tick marks covering full range [-60dB to +6dB]
        const float tickMarks[] = { 6.0f, 0.0f, -6.0f, -12.0f, -18.0f, -24.0f, -36.0f, -48.0f, -60.0f };
        const int numTicks = sizeof(tickMarks) / sizeof(tickMarks[0]);
        
        // Always show numbers for clarity
        nvgFontSize(vg, 9.0f);
        nvgFontFace(vg, "sans");
        nvgTextAlign(vg, NVG_ALIGN_RIGHT | NVG_ALIGN_MIDDLE);
        
        for (int i = 0; i < numTicks; ++i)
        {
            float db = tickMarks[i];
            float tickY = dbfsToY(db, scaleY, scaleHeight);
            
            // Tick mark line
            nvgBeginPath(vg);
            nvgMoveTo(vg, scaleX + scaleWidth - 6.0f, tickY);
            nvgLineTo(vg, scaleX + scaleWidth, tickY);
            nvgStrokeColor(vg, nvgRGBA(255, 255, 255, 60));  // Slightly brighter
            nvgStrokeWidth(vg, 1.0f);
            nvgStroke(vg);
            
            // Tick label (always show)
            char buf[8];
            if (db >= 0.0f)
                std::snprintf(buf, sizeof(buf), "+%.0f", db);
            else
                std::snprintf(buf, sizeof(buf), "%.0f", db);
            nvgFillColor(vg, nvgRGBA(255, 255, 255, 180));  // Brighter text
            nvgText(vg, scaleX + scaleWidth - 8.0f, tickY, buf, nullptr);
        }
    }
    
    // Draw track background (classic elegant style)
    {
        // Dark base: #0C101A (deep blue-gray)
        nvgBeginPath(vg);
        nvgRoundedRect(vg, trackX, trackY, trackWidth, scaleHeight, 2.5f);
        nvgFillColor(vg, nvgRGBA(12, 16, 26, 255));
        nvgFill(vg);
        
        // Subtle inner gradient for depth
        NVGpaint trackGrad = nvgLinearGradient(vg, trackX, trackY, trackX, trackY + scaleHeight * 0.3f,
            nvgRGBA(20, 24, 34, 255), nvgRGBA(12, 16, 26, 255));
        nvgBeginPath(vg);
        nvgRoundedRect(vg, trackX + 0.5f, trackY + 0.5f, trackWidth - 1.0f, scaleHeight * 0.3f, 2.0f);
        nvgFillPaint(vg, trackGrad);
        nvgFill(vg);
        
        // Inner border (darker)
        nvgBeginPath(vg);
        nvgRoundedRect(vg, trackX, trackY, trackWidth, scaleHeight, 2.5f);
        nvgStrokeWidth(vg, 1.0f);
        nvgStrokeColor(vg, nvgRGBA(0, 0, 0, 100));
        nvgStroke(vg);
    }
    
    // Draw momentary peak bar (LED SEGMENTED STYLE - premium look)
    if (m_momentaryDbFS > FLOOR_DB)
    {
        float barTop = dbfsToY(m_momentaryDbFS, barY, scaleHeight);
        float barBottom = barY + scaleHeight;
        float barHeight = barBottom - barTop;
        
        if (barHeight > 1.0f)
        {
            // LED segmented meter (like hardware gear)
            // Segments: 32 LEDs covering full range [-60dB to +6dB]
            constexpr int kNumSegments = 32;
            const float segmentHeight = scaleHeight / static_cast<float>(kNumSegments);
            const float segmentGap = 1.5f;  // Gap between LEDs
            
            // Calculate how many segments to light
            float levelNorm = (m_momentaryDbFS - SCALE_MIN_DB) / (SCALE_MAX_DB - SCALE_MIN_DB);
            int litSegments = static_cast<int>(levelNorm * kNumSegments);
            litSegments = std::clamp(litSegments, 0, kNumSegments);
            
            // Color thresholds (dBFS):
            // -60 to -18: Green (safe zone)
            // -18 to -3:  Yellow (caution zone)
            // -3 to 0:    Orange (warning zone)
            // 0 to +6:    Red (clipping zone)
            
            for (int i = 0; i < litSegments; ++i)
            {
                // Y position (bottom to top)
                float segY = barBottom - ((i + 1) * segmentHeight);
                
                // Convert segment index to dBFS
                float segLevelNorm = static_cast<float>(i + 1) / static_cast<float>(kNumSegments);
                float segDbFS = SCALE_MIN_DB + segLevelNorm * (SCALE_MAX_DB - SCALE_MIN_DB);
                
                // Determine LED color based on dBFS level
                NVGcolor segColor;
                if (segDbFS > 0.0f)
                {
                    // Over 0dB: RED
                    segColor = nvgRGBA(255, 40, 40, 255);
                }
                else if (segDbFS > -3.0f)
                {
                    // -3 to 0dB: ORANGE (narrow red zone!)
                    segColor = nvgRGBA(255, 140, 30, 255);
                }
                else if (segDbFS > -18.0f)
                {
                    // -18 to -3dB: YELLOW
                    segColor = nvgRGBA(240, 240, 50, 255);
                }
                else
                {
                    // Below -18dB: GREEN
                    segColor = nvgRGBA(50, 220, 100, 255);
                }
                
                // Draw LED segment (rounded rect with gap)
                nvgBeginPath(vg);
                nvgRoundedRect(vg, barX, segY + segmentGap * 0.5f, 
                              barWidth, segmentHeight - segmentGap, 1.0f);
                nvgFillColor(vg, segColor);
                nvgFill(vg);
            }
        }
    }
    
    // Draw peak hold marker (thin line with subtle glow)
    if (m_peakHoldDbFS > FLOOR_DB)
    {
        float markerY = dbfsToY(m_peakHoldDbFS, barY, scaleHeight);
        
        // Subtle glow (very subtle, not excessive)
        nvgBeginPath(vg);
        nvgRect(vg, trackX - 1.0f, markerY - 1.5f, trackWidth + 2.0f, 3.0f);
        NVGpaint glow = nvgBoxGradient(vg, trackX, markerY, trackWidth, 1.0f, 1.5f, 2.5f,
            nvgRGBA(255, 255, 100, 50), nvgRGBA(255, 255, 100, 0));
        nvgFillPaint(vg, glow);
        nvgFill(vg);
        
        // Main marker line (2px, brighter than bar)
        nvgBeginPath(vg);
        nvgRect(vg, trackX, markerY - 1.0f, trackWidth, 2.0f);
        nvgFillColor(vg, nvgRGBA(255, 255, 120, 255));  // Bright yellow-white
        nvgFill(vg);
    }
    
    // Draw peak hold text at bottom (clickable) with background for visibility
    m_peakHoldTextY = barY + scaleHeight + padding;
    
    char peakBuf[24];
    // Always use full format (not compact)
    if (m_peakHoldDbFS <= FLOOR_DB)
    {
        std::snprintf(peakBuf, sizeof(peakBuf), "-inf");
    }
    else if (m_peakHoldDbFS >= 0.0f)
    {
        std::snprintf(peakBuf, sizeof(peakBuf), "+%.1f dB", m_peakHoldDbFS);
    }
    else
    {
        std::snprintf(peakBuf, sizeof(peakBuf), "%.1f dB", m_peakHoldDbFS);
    }
    
    // Semi-transparent background for peak hold text (improves visibility)
    float peakTextX = x + width * 0.5f;
    float textBgWidth = 60.0f;
    float textBgHeight = 18.0f;
    nvgBeginPath(vg);
    nvgRoundedRect(vg, peakTextX - textBgWidth * 0.5f, m_peakHoldTextY - 2.0f, textBgWidth, textBgHeight, 3.0f);
    nvgFillColor(vg, nvgRGBA(0, 0, 0, 140));  // Darker background
    nvgFill(vg);
    
    // Main peak hold number (ALWAYS visible, full format)
    nvgFontSize(vg, 11.0f);
    nvgFontFace(vg, "sans-bold");
    nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_TOP);
    nvgFillColor(vg, nvgRGBA(255, 255, 255, 255));  // 100% white
    nvgText(vg, peakTextX, m_peakHoldTextY, peakBuf, nullptr);
}

bool MeterWidget::isInsidePeakHoldBounds(float mouseX, float mouseY) const
{
    // Check if mouse is inside the bottom text region
    const float textRegionHeight = 18.0f;
    const float textRegionTop = m_peakHoldTextY;
    const float textRegionBottom = m_lastY + m_lastHeight;
    
    return mouseX >= m_lastX && mouseX <= m_lastX + m_lastWidth &&
           mouseY >= textRegionTop && mouseY <= textRegionBottom;
}

} // namespace ui
} // namespace gv3
