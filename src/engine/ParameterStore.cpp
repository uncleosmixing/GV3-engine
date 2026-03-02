#include "GlobalVST3Engine.h"
#include "src/common/GV3Math.h"

#include <stdexcept>
#include <utility>

namespace gv3
{
void ParameterStore::define(ParameterDefinition definition)
{
    if (m_indexById.contains(definition.id))
    {
        throw std::invalid_argument("Parameter id already exists: " + definition.id);
    }

    if (definition.maxValue < definition.minValue)
    {
        std::swap(definition.minValue, definition.maxValue);
    }

    definition.defaultValue = detail::clampValue(definition.defaultValue, definition.minValue, definition.maxValue);

    m_indexById.emplace(definition.id, m_definitions.size());
    m_values.push_back(definition.defaultValue);
    m_definitions.push_back(std::move(definition));
}

bool ParameterStore::set(const std::string& id, float value)
{
    const auto it = m_indexById.find(id);
    if (it == m_indexById.end())
    {
        return false;
    }

    const auto index = it->second;
    const auto& definition = m_definitions[index];
    m_values[index] = detail::clampValue(value, definition.minValue, definition.maxValue);
    return true;
}

float ParameterStore::get(const std::string& id) const
{
    const auto it = m_indexById.find(id);
    if (it == m_indexById.end())
    {
        throw std::out_of_range("Unknown parameter id: " + id);
    }

    return m_values[it->second];
}

const std::vector<ParameterDefinition>& ParameterStore::definitions() const noexcept
{
    return m_definitions;
}

std::size_t ParameterStore::count() const noexcept
{
    return m_definitions.size();
}

const ParameterDefinition& ParameterStore::definitionAt(std::size_t index) const
{
    return m_definitions.at(index);
}

float ParameterStore::getByIndex(std::size_t index) const
{
    return m_values.at(index);
}

bool ParameterStore::setByIndex(std::size_t index, float value)
{
    if (index >= m_definitions.size())
    {
        return false;
    }

    const auto& def = m_definitions[index];
    m_values[index] = detail::clampValue(value, def.minValue, def.maxValue);
    return true;
}

float ParameterStore::getNormalized(std::size_t index) const
{
    if (index >= m_definitions.size())
    {
        return 0.0f;
    }

    const auto& def = m_definitions[index];
    return detail::normalized(m_values[index], def.minValue, def.maxValue);
}

bool ParameterStore::setNormalized(std::size_t index, float normalizedValue)
{
    if (index >= m_definitions.size())
    {
        return false;
    }

    const auto& def = m_definitions[index];
    const float value = def.minValue + detail::clampValue(normalizedValue, 0.0f, 1.0f) * (def.maxValue - def.minValue);
    m_values[index] = value;
    return true;
}
} // namespace gv3
