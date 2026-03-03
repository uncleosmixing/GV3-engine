#include "GlobalVST3Engine.h"
#include "src/common/GV3Math.h"
#include "plugins/GainPlugin/GainRuntimeState.h"
#include "src/dsp/simd/GV3SimdGain.h"

#include <cmath>
#include <algorithm>

namespace gv3
{
GainProcessor::GainProcessor()
{
    m_parameters.define({ "gain.db", "Gain", 0.0f, -60.0f, 24.0f });
}

std::string GainProcessor::name() const
{
    return "Gain";
}

void GainProcessor::prepare(const ProcessSpec& spec)
{
    m_spec = spec;
    initializeParameters();

    // Initialize meter bridge for realtime metering with ballistics
    auto& rt = plugins::gainRuntimeState();
    rt.ensureMeterBridge();
    // Verify meterBridge was created
    if (rt.meterBridge)
    {
        rt.meterBridge->initialize(spec.sampleRate, 25.0f, 400.0f);
    }
}

void GainProcessor::reset()
{
}

void GainProcessor::process(AudioBlock block)
{
    const float gain = detail::dbToLinear(m_parameters.get("gain.db"));

    // Measure input peak (before processing) - SIMD optimized
    const std::size_t chCount = block.channelCount();
    float peakInL = 0.0f, peakInR = 0.0f;
    
    if (chCount > 0)
    {
        auto inL = block.channel(0);
        peakInL = simd::compute_peak(inL.data(), inL.size());
    }
    if (chCount > 1)
    {
        auto inR = block.channel(1);
        peakInR = simd::compute_peak(inR.data(), inR.size());
    }

    // Apply gain using SIMD abstraction layer
    for (std::size_t ch = 0; ch < chCount; ++ch)
    {
        auto data = block.channel(ch);
        simd::apply_gain(data.data(), data.size(), gain);
    }

    // Measure output peak (after processing) - SIMD optimized
    float peakOutL = 0.0f, peakOutR = 0.0f;
    
    if (chCount > 0)
    {
        auto outL = block.channel(0);
        peakOutL = simd::compute_peak(outL.data(), outL.size());
    }
    if (chCount > 1)
    {
        auto outR = block.channel(1);
        peakOutR = simd::compute_peak(outR.data(), outR.size());
    }

    // Normalize peaks to 0-1 range (-60dB to +6dB = reference is ~0.316 linear)
    // Use simplified scaling: 0 dB = 0.5, soft-clip above
    constexpr float refLevel = 0.316f;  // ~-10dB in linear
    float normInL = std::min(1.0f, peakInL / refLevel);
    float normInR = std::min(1.0f, peakInR / refLevel);
    float normOutL = std::min(1.0f, peakOutL / refLevel);
    float normOutR = std::min(1.0f, peakOutR / refLevel);

    // Write to meter bridge (realtime-safe, lock-free)
    auto& rt = plugins::gainRuntimeState();
    if (rt.meterBridge)
    {
        rt.meterBridge->writeMeterSample(normInL, normInR, normOutL, normOutR);
    }

    // Also update legacy atomic meters for backward compat with existing UI
    rt.inputMeterL.store(normInL, std::memory_order_relaxed);
    rt.inputMeterR.store(normInR, std::memory_order_relaxed);
    rt.outputMeterL.store(normOutL, std::memory_order_relaxed);
    rt.outputMeterR.store(normOutR, std::memory_order_relaxed);
}
} // namespace gv3
