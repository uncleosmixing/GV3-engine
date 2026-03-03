#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace gv3
{
    /// ParameterDefinition — immutable metadata for a parameter
    struct ParameterDefinition
    {
        std::string id;
        std::string name;
        float defaultValue { 0.0f };
        float minValue { 0.0f };
        float maxValue { 1.0f };
    };

    /// ParameterRegistry — immutable, metadata-only
    /// Created during plugin initialization, frozen for the lifetime of the processor.
    /// Safe to read from any thread without locks.
    class ParameterRegistry
    {
    public:
        ParameterRegistry() = default;

        /// Add a parameter definition. Only call during initialization (before process()).
        void define(ParameterDefinition definition);

        /// Total number of parameters
        std::size_t count() const noexcept;

        /// Get definition by index (thread-safe, O(1))
        const ParameterDefinition& definitionAt(std::size_t index) const;

        /// Get index by ID (thread-safe, O(n) — use sparingly)
        bool tryIndexOf(const std::string& id, std::size_t& outIndex) const noexcept;

        /// Get all definitions (for UI/controller setup)
        const std::vector<ParameterDefinition>& all() const noexcept;

    private:
        std::vector<ParameterDefinition> m_definitions;
    };

} // namespace gv3
