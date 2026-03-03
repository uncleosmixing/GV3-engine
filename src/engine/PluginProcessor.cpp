#include "GlobalVST3Engine.h"

namespace gv3
{
ParameterStore& PluginProcessor::parameters() noexcept
{
    return m_parameters;
}

const ParameterStore& PluginProcessor::parameters() const noexcept
{
    return m_parameters;
}

void PluginProcessor::initializeParameters()
{
    m_parameters.initialize(m_parameters.count());
}
} // namespace gv3
