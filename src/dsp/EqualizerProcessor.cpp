#include "GlobalVST3Engine.h"
#include "src/common/GV3Math.h"

#include <algorithm>
#include <cmath>

namespace gv3
{
EqualizerProcessor::EqualizerProcessor()
{
    m_parameters.define({ "eq.lowGainDb", "Low", 0.0f, -24.0f, 24.0f });
    m_parameters.define({ "eq.midGainDb", "Mid", 0.0f, -24.0f, 24.0f });
    m_parameters.define({ "eq.highGainDb", "High", 0.0f, -24.0f, 24.0f });
}

std::string EqualizerProcessor::name() const
{
    return "Equalizer";
}

void EqualizerProcessor::prepare(const ProcessSpec& spec)
{
    m_spec = spec;
    initializeParameters();
    m_state.assign(spec.channelCount, FilterState {});
}

void EqualizerProcessor::reset()
{
    for (auto& state : m_state)
    {
        state = {};
    }
}

void EqualizerProcessor::process(AudioBlock block)
{
    if (block.channelCount() == 0)
    {
        return;
    }

    if (m_state.size() < block.channelCount())
    {
        m_state.resize(block.channelCount());
    }

    const float lowGain = detail::dbToLinear(m_parameters.get("eq.lowGainDb"));
    const float midGain = detail::dbToLinear(m_parameters.get("eq.midGainDb"));
    const float highGain = detail::dbToLinear(m_parameters.get("eq.highGainDb"));

    const float sampleRate = static_cast<float>(std::max(1.0, m_spec.sampleRate));
    const float lowCut = std::exp(-2.0f * detail::kPi * 180.0f / sampleRate);
    const float highCut = std::exp(-2.0f * detail::kPi * 3500.0f / sampleRate);

    for (std::size_t ch = 0; ch < block.channelCount(); ++ch)
    {
        auto channelData = block.channel(ch);
        auto& state = m_state[ch];

        for (float& sample : channelData)
        {
            state.low = (1.0f - lowCut) * sample + lowCut * state.low;
            const float highInput = sample - state.low;
            state.high = (1.0f - highCut) * highInput + highCut * state.high;

            const float lowBand = state.low;
            const float highBand = highInput - state.high;
            const float midBand = sample - lowBand - highBand;

            sample = lowBand * lowGain + midBand * midGain + highBand * highGain;
        }
    }
}
} // namespace gv3
