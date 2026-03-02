#pragma once

#include <atomic>

namespace gv3::plugins
{
struct GainRuntimeState
{
    std::atomic<float> inputDb { -60.0f };
    std::atomic<float> outputDb { -60.0f };
    std::atomic<float> hoverTarget { 0.0f };
    std::atomic<float> hoverAmount { 0.0f };
    std::atomic<float> inputMeterL { 0.0f };
    std::atomic<float> inputMeterR { 0.0f };
    std::atomic<float> outputMeterL { 0.0f };
    std::atomic<float> outputMeterR { 0.0f };
    std::atomic<bool> valueEditing { false };
};

inline GainRuntimeState& gainRuntimeState()
{
    static GainRuntimeState s;
    return s;
}
} // namespace gv3::plugins
