#include "GainScene.h"
#include "../core/ScissorGuard.h"
#include <cstring>
#include <cstdio>

namespace gv3::ui2
{

GainScene::GainScene()
{
}

void GainScene::layout(const Rect& view, const UiContext& ctx)
{
    m_layout = m_layoutEngine.computeGainLayout(view, ctx.uiScale);

    // Layout knob in top zone (centered)
    if (m_layout.top.isValid())
    {
        // Knob gets 80% of top zone height, centered
        float knobSize = std::min(m_layout.top.w * 0.15f, m_layout.top.h * 0.8f);
        knobSize = std::max(knobSize, 40.0f * ctx.uiScale); // Minimum 40px

        Rect knobBounds;
        knobBounds.x = m_layout.top.centerX() - knobSize * 0.5f;
        knobBounds.y = m_layout.top.centerY() - knobSize * 0.5f;
        knobBounds.w = knobSize;
        knobBounds.h = knobSize;

        m_knob.layout(knobBounds, ctx);
    }
}

void GainScene::draw(NVGcontext* vg, const UiContext& ctx)
{
    if (!vg) return;

    // Draw zone outlines (for layout validation — can remove later)
    drawZoneOutline(vg, m_layout.top, "TOP ZONE");
    drawZoneOutline(vg, m_layout.meterIn, "IN METER");
    drawZoneOutline(vg, m_layout.meterOut, "OUT METER");
    drawZoneOutline(vg, m_layout.bottom, "WAVEFORM");

    // Draw knob widget
    m_knob.draw(vg, ctx);

    // Draw knob label + value text
    if (m_layout.top.isValid())
    {
        float knobValue = m_knob.getValue();
        float gainDb = -60.0f + knobValue * 84.0f; // Map 0-1 to -60dB to +24dB

        // Label above knob
        nvgFontSize(vg, 16.0f * ctx.uiScale);
        nvgFontFace(vg, "sans");
        nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_BOTTOM);
        nvgFillColor(vg, nvgRGBA(200, 200, 200, 255));
        nvgText(vg, m_layout.top.centerX(), m_layout.top.y + 30.0f * ctx.uiScale, "Gain", nullptr);

        // Value below knob
        char valueText[32];
        std::snprintf(valueText, sizeof(valueText), "%.1f dB", gainDb);
        nvgFontSize(vg, 18.0f * ctx.uiScale);
        nvgFontFace(vg, "sans-bold");
        nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_TOP);
        nvgFillColor(vg, nvgRGBA(255, 255, 255, 255));
        nvgText(vg, m_layout.top.centerX(), m_layout.top.bottom() - 40.0f * ctx.uiScale, valueText, nullptr);
    }
}

bool GainScene::onMouseDown(float x, float y, const InputState& input)
{
    // Try knob first
    if (m_knob.onMouseDown(x, y, input))
        return true;

    // Try other widgets...
    return false;
}

bool GainScene::onMouseUp(float x, float y, const InputState& input)
{
    if (m_knob.onMouseUp(x, y, input))
        return true;

    return false;
}

bool GainScene::onMouseMove(float x, float y, const InputState& input)
{
    if (m_knob.onMouseMove(x, y, input))
        return true;

    return false;
}

bool GainScene::onMouseWheel(float deltaX, float deltaY, const InputState& input)
{
    // Future: scroll to adjust knob value
    return false;
}

void GainScene::drawZoneOutline(NVGcontext* vg, const Rect& zone, const char* label)
{
    if (!zone.isValid()) return;

    // Draw outline rectangle
    nvgBeginPath(vg);
    nvgRect(vg, zone.x, zone.y, zone.w, zone.h);
    nvgStrokeWidth(vg, 2.0f);
    nvgStrokeColor(vg, nvgRGBA(100, 200, 255, 180));  // Cyan outline
    nvgStroke(vg);

    // Draw semi-transparent fill
    nvgBeginPath(vg);
    nvgRect(vg, zone.x, zone.y, zone.w, zone.h);
    nvgFillColor(vg, nvgRGBA(50, 100, 150, 30));  // Dark blue tint
    nvgFill(vg);

    // Draw label at center
    if (label && std::strlen(label) > 0)
    {
        nvgFontSize(vg, 16.0f);
        nvgFontFace(vg, "sans-bold");
        nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        nvgFillColor(vg, nvgRGBA(255, 255, 255, 200));
        nvgText(vg, zone.centerX(), zone.centerY(), label, nullptr);

        // Draw size info below label
        char sizeInfo[64];
        std::snprintf(sizeInfo, sizeof(sizeInfo), "%.0f x %.0f", zone.w, zone.h);
        nvgFontSize(vg, 12.0f);
        nvgFontFace(vg, "sans");
        nvgFillColor(vg, nvgRGBA(200, 200, 200, 160));
        nvgText(vg, zone.centerX(), zone.centerY() + 20.0f, sizeInfo, nullptr);
    }
}

} // namespace gv3::ui2
