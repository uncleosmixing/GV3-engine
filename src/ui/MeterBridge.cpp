#include "src/ui/MeterBridge.h"
#include <algorithm>
#include <cmath>

namespace gv3
{

void MeterBridge::initialize(float sampleRate, float attackMs, float releaseMs)
{
    // Convert ms to smoothing coefficients
    // coeff = exp(-2.0 / (time_in_samples))
    // Lower = slower response, higher = faster response
    
    float attackSamples = std::max(1.0f, sampleRate * attackMs / 1000.0f);
    float releaseSamples = std::max(1.0f, sampleRate * releaseMs / 1000.0f);

    // attack: fast rise (high coeff means we blend more of the new value)
    m_attackCoeff = 2.0f / (1.0f + attackSamples);
    
    // release: slow fall (low coeff means we keep more of the old value)
    m_releaseCoeff = 2.0f / (1.0f + releaseSamples);

    // Peak decay: slight decay per sample (peak hold slowly fades)
    // Over 5 seconds, decay to ~10% at 44.1 kHz
    float peakHoldSamples = 5.0f * sampleRate;
    m_peakDecay = std::exp(-1.0f / peakHoldSamples);

    // Clear buffers
    m_buffers[0] = Buffer{};
    m_buffers[1] = Buffer{};
    m_smoothInputL = 0.0f;
    m_smoothInputR = 0.0f;
    m_smoothOutputL = 0.0f;
    m_smoothOutputR = 0.0f;
    m_peakL = 0.0f;
    m_peakR = 0.0f;
}

void MeterBridge::writeMeterSample(float inputL, float inputR, float outputL, float outputR) noexcept
{
    // Clamp inputs to valid range
    inputL = std::max(0.0f, std::min(1.0f, inputL));
    inputR = std::max(0.0f, std::min(1.0f, inputR));
    outputL = std::max(0.0f, std::min(1.0f, outputL));
    outputR = std::max(0.0f, std::min(1.0f, outputR));

    // Apply ballistics: attack (fast rise) / release (slow fall)
    // If new value > smoothed, use attack (fast); otherwise use release (slow)
    if (inputL > m_smoothInputL)
        m_smoothInputL = m_attackCoeff * inputL + (1.0f - m_attackCoeff) * m_smoothInputL;
    else
        m_smoothInputL = m_releaseCoeff * inputL + (1.0f - m_releaseCoeff) * m_smoothInputL;

    if (inputR > m_smoothInputR)
        m_smoothInputR = m_attackCoeff * inputR + (1.0f - m_attackCoeff) * m_smoothInputR;
    else
        m_smoothInputR = m_releaseCoeff * inputR + (1.0f - m_releaseCoeff) * m_smoothInputR;

    if (outputL > m_smoothOutputL)
        m_smoothOutputL = m_attackCoeff * outputL + (1.0f - m_attackCoeff) * m_smoothOutputL;
    else
        m_smoothOutputL = m_releaseCoeff * outputL + (1.0f - m_releaseCoeff) * m_smoothOutputL;

    if (outputR > m_smoothOutputR)
        m_smoothOutputR = m_attackCoeff * outputR + (1.0f - m_attackCoeff) * m_smoothOutputR;
    else
        m_smoothOutputR = m_releaseCoeff * outputR + (1.0f - m_releaseCoeff) * m_smoothOutputR;

    // Track peak hold (maximum value seen, with slow decay)
    m_peakL = std::max(m_smoothInputL, m_peakL * m_peakDecay);
    m_peakR = std::max(m_smoothInputR, m_peakR * m_peakDecay);

    // Get current write buffer
    int writeBuffer = m_activeBuffer.load(std::memory_order_relaxed);
    int readBuffer = 1 - writeBuffer;  // Other buffer (UI reads this)

    // Write to the read buffer (safe: no contention)
    auto& buffer = m_buffers[readBuffer];
    buffer.inputL = m_smoothInputL;
    buffer.inputR = m_smoothInputR;
    buffer.outputL = m_smoothOutputL;
    buffer.outputR = m_smoothOutputR;
    buffer.peakL = m_peakL;
    buffer.peakR = m_peakR;

    // Atomically increment sequence number to signal new data
    // Use release semantics: ensures all buffer writes above are visible to UI thread
    std::uint64_t currentSeq = m_sequenceNumber.load(std::memory_order_relaxed);
    m_sequenceNumber.store(currentSeq + 1, std::memory_order_release);
}

MeterSnapshot MeterBridge::readSnapshot() const noexcept
{
    // Read sequence number with acquire semantics
    // This ensures we see all writes from the audio thread that released before incrementing seq
    std::uint64_t seqBefore = m_sequenceNumber.load(std::memory_order_acquire);

    // Read from the current non-active buffer (UI's read buffer)
    // Use relaxed load here; acquire above ensures visibility
    int readBuffer = 1 - m_activeBuffer.load(std::memory_order_relaxed);
    const auto& buffer = m_buffers[readBuffer];

    MeterSnapshot snapshot;
    snapshot.inputL = buffer.inputL;
    snapshot.inputR = buffer.inputR;
    snapshot.outputL = buffer.outputL;
    snapshot.outputR = buffer.outputR;
    snapshot.peakL = buffer.peakL;
    snapshot.peakR = buffer.peakR;
    snapshot.sequenceNumber = seqBefore;

    return snapshot;
}

void MeterBridge::resetPeakHold() noexcept
{
    m_peakL = 0.0f;
    m_peakR = 0.0f;
}

} // namespace gv3
