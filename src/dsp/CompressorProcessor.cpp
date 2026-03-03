#include "GlobalVST3Engine.h"
#include "src/common/GV3Math.h"

#include <algorithm>
#include <cmath>

namespace gv3
{
CompressorProcessor::CompressorProcessor()
{
    m_parameters.define({ "comp.thresholdDb", "Threshold", -18.0f, -60.0f, 0.0f });
    m_parameters.define({ "comp.ratio", "Ratio", 4.0f, 1.0f, 20.0f });
    m_parameters.define({ "comp.attackMs", "Attack", 10.0f, 0.1f, 200.0f });
    m_parameters.define({ "comp.releaseMs", "Release", 120.0f, 5.0f, 1200.0f });
    m_parameters.define({ "comp.makeupDb", "MakeUp", 0.0f, 0.0f, 24.0f });
}

std::string CompressorProcessor::name() const
{
    return "Compressor";
}

void CompressorProcessor::prepare(const ProcessSpec& spec)
{
    m_spec = spec;
    initializeParameters();
    m_envelope.assign(spec.channelCount, 0.0f);
}

void CompressorProcessor::reset()
{
    for (auto& envelope : m_envelope)
    {
        envelope = 0.0f;
    }
}

void CompressorProcessor::process(AudioBlock block)
{
    if (block.channelCount() == 0)
    {
        return;
    }

    if (m_envelope.size() < block.channelCount())
    {
        m_envelope.resize(block.channelCount(), 0.0f);
    }

    const float thresholdDb = m_parameters.get("comp.thresholdDb");
    const float ratio = m_parameters.get("comp.ratio");
    const float attackMs = m_parameters.get("comp.attackMs");
    const float releaseMs = m_parameters.get("comp.releaseMs");
    const float makeup = detail::dbToLinear(m_parameters.get("comp.makeupDb"));

    const float sampleRate = static_cast<float>(std::max(1.0, m_spec.sampleRate));
    const float attackCoeff = std::exp(-1.0f / (0.001f * attackMs * sampleRate));
    const float releaseCoeff = std::exp(-1.0f / (0.001f * releaseMs * sampleRate));

    for (std::size_t ch = 0; ch < block.channelCount(); ++ch)
    {
        auto channelData = block.channel(ch);
        auto& envelope = m_envelope[ch];

        for (float& sample : channelData)
        {
            const float inputAbs = std::max(std::abs(sample), 1.0e-9f);
            if (inputAbs > envelope)
            {
                envelope = attackCoeff * envelope + (1.0f - attackCoeff) * inputAbs;
            }
            else
            {
                envelope = releaseCoeff * envelope + (1.0f - releaseCoeff) * inputAbs;
            }

            const float levelDb = 20.0f * std::log10(std::max(envelope, 1.0e-9f));
            const float overDb = levelDb - thresholdDb;
            const float gainReductionDb = overDb > 0.0f ? overDb * (1.0f - 1.0f / ratio) : 0.0f;
            const float gain = detail::dbToLinear(-gainReductionDb) * makeup;
            sample *= gain;
        }
    }
}
} // namespace gv3
