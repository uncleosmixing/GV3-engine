#include "src/engine/ParameterRegistry.h"
#include "src/common/GV3Math.h"

#include <stdexcept>
#include <utility>
#include <algorithm>

namespace gv3
{

void ParameterRegistry::define(ParameterDefinition definition)
{
    // Check for duplicate IDs
    for (const auto& existing : m_definitions)
    {
        if (existing.id == definition.id)
        {
            throw std::invalid_argument("Parameter id already exists: " + definition.id);
        }
    }

    // Normalize min/max
    if (definition.maxValue < definition.minValue)
    {
        std::swap(definition.minValue, definition.maxValue);
    }

    // Clamp default
    definition.defaultValue = detail::clampValue(definition.defaultValue, definition.minValue, definition.maxValue);

    m_definitions.push_back(std::move(definition));
}

std::size_t ParameterRegistry::count() const noexcept
{
    return m_definitions.size();
}

const ParameterDefinition& ParameterRegistry::definitionAt(std::size_t index) const
{
    return m_definitions.at(index);
}

bool ParameterRegistry::tryIndexOf(const std::string& id, std::size_t& outIndex) const noexcept
{
    for (std::size_t i = 0; i < m_definitions.size(); ++i)
    {
        if (m_definitions[i].id == id)
        {
            outIndex = i;
            return true;
        }
    }
    return false;
}

const std::vector<ParameterDefinition>& ParameterRegistry::all() const noexcept
{
    return m_definitions;
}

} // namespace gv3
