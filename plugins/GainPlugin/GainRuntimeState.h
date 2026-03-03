#pragma once

#include <atomic>
#include <memory>
#include "src/ui/MeterBridge.h"
#include "src/ui/MeterWidget.h"

namespace gv3::plugins
{
// UI state for GainPlugin (persistent, non-realtime)
struct GainUIState
{
    gv3::ui::MeterWidget meterInL;
    gv3::ui::MeterWidget meterInR;
    gv3::ui::MeterWidget meterOutL;
    gv3::ui::MeterWidget meterOutR;
};

struct GainRuntimeState
{
    std::atomic<float> inputDb { -60.0f };
    std::atomic<float> outputDb { -60.0f };
    std::atomic<float> hoverTarget { 0.0f };
    std::atomic<float> hoverAmount { 0.0f };
    
    // Legacy atomic meters (kept for backward compat)
    std::atomic<float> inputMeterL { 0.0f };
    std::atomic<float> inputMeterR { 0.0f };
    std::atomic<float> outputMeterL { 0.0f };
    std::atomic<float> outputMeterR { 0.0f };
    
    std::atomic<bool> valueEditing { false };

    // Meter bridge for realtime-safe metering with ballistics
    // Created on first access, initialized during processor prepare()
    std::shared_ptr<gv3::MeterBridge> meterBridge;

    // UI state (persistent MeterWidget instances for peak hold)
    std::shared_ptr<GainUIState> uiState;

    // Helper to ensure meter bridge is created
    void ensureMeterBridge()
    {
        if (!meterBridge)
        {
            meterBridge = std::make_shared<gv3::MeterBridge>();
        }
    }

    // Helper to ensure UI state is created
    void ensureUIState()
    {
        if (!uiState)
        {
            uiState = std::make_shared<GainUIState>();
        }
    }
};

inline GainRuntimeState& gainRuntimeState()
{
    static GainRuntimeState s;
    return s;
}
} // namespace gv3::plugins

