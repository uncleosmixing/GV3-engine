#include "GlobalVST3Engine.h"

namespace gv3
{
AudioBlock::AudioBlock(std::vector<std::vector<float>>& channels)
{
    m_channelCount = channels.size();
    m_sampleCount = channels.empty() ? 0 : channels[0].size();
    m_pointerStorage.resize(m_channelCount);
    for (std::size_t i = 0; i < m_channelCount; ++i)
    {
        m_pointerStorage[i] = channels[i].data();
    }
    m_channels = m_pointerStorage.data();
}

AudioBlock::AudioBlock(float** channelPointers, std::size_t numChannels, std::size_t numSamples)
    : m_channels(channelPointers)
    , m_channelCount(numChannels)
    , m_sampleCount(numSamples)
{
}

std::size_t AudioBlock::channelCount() const noexcept
{
    return m_channelCount;
}

std::size_t AudioBlock::sampleCount() const noexcept
{
    return m_sampleCount;
}

std::span<float> AudioBlock::channel(std::size_t index)
{
    return { m_channels[index], m_sampleCount };
}
} // namespace gv3
