#pragma once

#include <atomic>
#include <cstddef>
#include <array>

namespace gv3
{
    /// MeterSnapshot — a stable, immutable snapshot of meter values
    /// Safe to read from UI thread without locks
    struct MeterSnapshot
    {
        float inputL { 0.0f };      // Normalized 0-1 (peak or RMS)
        float inputR { 0.0f };
        float outputL { 0.0f };
        float outputR { 0.0f };
        float peakL { 0.0f };       // Optional peak hold
        float peakR { 0.0f };
        std::uint64_t sequenceNumber { 0 };  // For change detection
    };

    /// MeterBridge — lock-free double-buffer for meter data
    /// Audio thread writes raw meter values → processed with ballistics → swapped atomically
    /// UI thread reads latest stable snapshot
    class MeterBridge
    {
    public:
        /// Initialize the meter bridge (call once during processor setup)
        void initialize(float sampleRate, float attackMs = 25.0f, float releaseMs = 400.0f);

        /// Write raw meter sample (audio thread, realtime-safe, O(1))
        /// Inputs: left/right levels in range [0.0, 1.0]
        void writeMeterSample(float inputL, float inputR, float outputL, float outputR) noexcept;

        /// Read latest stable snapshot (UI thread, lock-free, O(1))
        /// Returns current ballistics-processed values + peak hold
        MeterSnapshot readSnapshot() const noexcept;

        /// Reset peak hold values (UI thread)
        void resetPeakHold() noexcept;

    private:
        struct Buffer
        {
            float inputL { 0.0f };
            float inputR { 0.0f };
            float outputL { 0.0f };
            float outputR { 0.0f };
            float peakL { 0.0f };
            float peakR { 0.0f };
        };

        // Double-buffer for meter values
        std::array<Buffer, 2> m_buffers;
        std::atomic<int> m_activeBuffer { 0 };  // 0 or 1
        std::atomic<std::uint64_t> m_sequenceNumber { 0 };

        // Ballistics parameters
        float m_attackCoeff { 0.25f };      // 1.0 - fast, 0.0 - slow (fast attack)
        float m_releaseCoeff { 0.08f };     // 1.0 - fast, 0.0 - slow (slow release)
        
        // Internally smoothed values (for ballistics)
        float m_smoothInputL { 0.0f };
        float m_smoothInputR { 0.0f };
        float m_smoothOutputL { 0.0f };
        float m_smoothOutputR { 0.0f };
        float m_peakL { 0.0f };
        float m_peakR { 0.0f };
        float m_peakDecay { 0.9999f };      // How quickly peak decays
    };

} // namespace gv3
