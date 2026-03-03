#include "GlobalVST3Engine.h"
#include "src/common/GV3Math.h"

#include <stdexcept>

namespace gv3
{

void ParameterStore::define(ParameterDefinition definition)
{
    m_registry.define(std::move(definition));
}

void ParameterStore::initialize(std::size_t count)
{
    // Pre-allocate state with defaults from registry
    m_state.initialize(count);

    std::vector<float> defaults;
    for (std::size_t i = 0; i < m_registry.count(); ++i)
    {
        defaults.push_back(m_registry.definitionAt(i).defaultValue);
    }
    m_state.setDefaults(defaults);
}

bool ParameterStore::set(const std::string& id, float value)
{
    std::size_t index = 0;
    if (!m_registry.tryIndexOf(id, index))
    {
        return false;
    }

    const auto& definition = m_registry.definitionAt(index);
    float clampedValue = detail::clampValue(value, definition.minValue, definition.maxValue);
    m_state.setAtIndex(index, clampedValue);
    return true;
}

float ParameterStore::get(const std::string& id) const
{
    std::size_t index = 0;
    if (!m_registry.tryIndexOf(id, index))
    {
        throw std::out_of_range("Unknown parameter id: " + id);
    }
    return m_state.getAtIndex(index);
}

const std::vector<ParameterDefinition>& ParameterStore::definitions() const noexcept
{
    return m_registry.all();
}

std::size_t ParameterStore::count() const noexcept
{
    return m_registry.count();
}

const ParameterDefinition& ParameterStore::definitionAt(std::size_t index) const
{
    return m_registry.definitionAt(index);
}

float ParameterStore::getByIndex(std::size_t index) const
{
    return m_state.getAtIndex(index);
}

bool ParameterStore::setByIndex(std::size_t index, float value)
{
    if (index >= m_registry.count())
    {
        return false;
    }

    const auto& def = m_registry.definitionAt(index);
    float clampedValue = detail::clampValue(value, def.minValue, def.maxValue);
    m_state.setAtIndex(index, clampedValue);
    return true;
}

float ParameterStore::getNormalized(std::size_t index) const
{
    if (index >= m_registry.count())
    {
        return 0.0f;
    }

    const auto& def = m_registry.definitionAt(index);
    float rawValue = m_state.getAtIndex(index);
    return detail::normalized(rawValue, def.minValue, def.maxValue);
}

bool ParameterStore::setNormalized(std::size_t index, float normalizedValue)
{
    if (index >= m_registry.count())
    {
        return false;
    }

    const auto& def = m_registry.definitionAt(index);
    float clampedNorm = detail::clampValue(normalizedValue, 0.0f, 1.0f);
    float rawValue = def.minValue + clampedNorm * (def.maxValue - def.minValue);
    m_state.setAtIndex(index, rawValue);
    return true;
}

const ParameterRegistry& ParameterStore::registry() const noexcept
{
    return m_registry;
}

const ParameterState& ParameterStore::state() const noexcept
{
    return m_state;
}

} // namespace gv3
