#include "src/engine/ParameterState.h"
#include <cstring>

namespace gv3
{

void ParameterState::initialize(std::size_t parameterCount)
{
    // Use unique_ptr with array to avoid copy issues
    m_values = std::make_unique<std::atomic<float>[]>(parameterCount);
    m_count = parameterCount;
    
    // Initialize all to 0.0f
    for (std::size_t i = 0; i < parameterCount; ++i)
    {
        m_values[i].store(0.0f, std::memory_order_relaxed);
    }
}

void ParameterState::setAtIndex(std::size_t index, float value) noexcept
{
    if (index < m_count && m_values)
    {
        m_values[index].store(value, std::memory_order_relaxed);
    }
}

float ParameterState::getAtIndex(std::size_t index) const noexcept
{
    if (index < m_count && m_values)
    {
        return m_values[index].load(std::memory_order_relaxed);
    }
    return 0.0f;
}

std::size_t ParameterState::count() const noexcept
{
    return m_count;
}

void ParameterState::setDefaults(const std::vector<float>& defaultValues) noexcept
{
    for (std::size_t i = 0; i < defaultValues.size() && i < m_count && m_values; ++i)
    {
        m_values[i].store(defaultValues[i], std::memory_order_relaxed);
    }
}

} // namespace gv3
