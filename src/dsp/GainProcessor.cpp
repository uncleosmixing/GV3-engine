#include "GlobalVST3Engine.h"
#include "src/common/GV3Math.h"

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
}

void GainProcessor::reset()
{
}

void GainProcessor::process(AudioBlock block)
{
    const float gain = detail::dbToLinear(m_parameters.get("gain.db"));

    for (std::size_t ch = 0; ch < block.channelCount(); ++ch)
    {
        auto data = block.channel(ch);
        for (float& sample : data)
        {
            sample *= gain;
        }
    }
}
} // namespace gv3
