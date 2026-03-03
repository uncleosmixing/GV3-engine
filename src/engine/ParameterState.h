#pragma once

#include <atomic>
#include <cstddef>
#include <memory>
#include <vector>

namespace gv3
{
    /// ParameterState — holds atomic float values, indexed by size_t
    /// Safe for audio thread access (realtime-safe, lock-free).
    /// No allocations after initialization.
    class ParameterState
    {
    public:
        /// Initialize state with a given count. Call once during prepare().
        void initialize(std::size_t parameterCount);

        /// Set value at index (clipping is responsibility of caller).
        /// Realtime-safe.
        void setAtIndex(std::size_t index, float value) noexcept;

        /// Get value at index.
        /// Realtime-safe.
        float getAtIndex(std::size_t index) const noexcept;

        /// Get the number of tracked parameters.
        std::size_t count() const noexcept;

        /// Set default values (used after initialize).
        void setDefaults(const std::vector<float>& defaultValues) noexcept;

    private:
        std::unique_ptr<std::atomic<float>[]> m_values;
        std::size_t m_count { 0 };
    };

} // namespace gv3
