#pragma once

#include "../core/UiContext.h"
#include "../core/Rect.h"
#include "../layout/LayoutEngine.h"
#include "../widgets/KnobWidget.h"
#include <nanovg.h>

namespace gv3::ui2
{

/**
 * @brief Main scene container for GainPlugin UI.
 * 
 * Manages layout and rendering of all UI zones:
 * - Top: Knob + gain readout
 * - Meters: Input/Output level meters
 * - Bottom: Waveform/spectrum display
 * 
 * Phase 1: Zone outlines (DONE)
 * Phase 2: KnobWidget integration (CURRENT)
 * Phase 3: MeterWidget2 + WaveformWidget
 */
class GainScene
{
public:
    GainScene();

    // Layout and rendering
    void layout(const Rect& view, const UiContext& ctx);
    void draw(NVGcontext* vg, const UiContext& ctx);

    // Input handling (forward to widgets)
    bool onMouseDown(float x, float y, const InputState& input);
    bool onMouseUp(float x, float y, const InputState& input);
    bool onMouseMove(float x, float y, const InputState& input);
    bool onMouseWheel(float deltaX, float deltaY, const InputState& input);

    // Widget accessors
    KnobWidget& getKnob() { return m_knob; }

private:
    void drawZoneOutline(NVGcontext* vg, const Rect& zone, const char* label);

    LayoutEngine m_layoutEngine;
    GainLayout m_layout;

    // Widgets
    KnobWidget m_knob;
};

} // namespace gv3::ui2
