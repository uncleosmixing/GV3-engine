#pragma once

#include "GV3SimdMath.h"
#include <cstddef>

// ===========================================================================
// GV3 SIMD Gain Application & Peak Detection
// ===========================================================================
// Optimized gain application and peak detection using SIMD operations
// Scalar backend: simple loop
// XSIMD backend: vectorized batch processing with scalar tail

namespace gv3::simd
{

    /// Apply gain to audio buffer (in-place)
    /// @param data Pointer to audio samples (modified in-place)
    /// @param numSamples Number of samples to process
    /// @param gain Linear gain factor to apply
    inline void apply_gain(float* data, std::size_t numSamples, float gain) noexcept
    {
#if GV3_USE_XSIMD
        // Vectorized implementation: process width samples at a time
        const f32 gain_vec = broadcast(gain);
        const std::size_t vec_size = width;
        const std::size_t vec_end = (numSamples / vec_size) * vec_size;

        // Main vectorized loop
        for (std::size_t i = 0; i < vec_end; i += vec_size)
        {
            f32 samples = load_unaligned(&data[i]);
            f32 result = mul(samples, gain_vec);
            store_unaligned(&data[i], result);
        }

        // Scalar tail for remaining samples
        for (std::size_t i = vec_end; i < numSamples; ++i)
        {
            data[i] *= gain;
        }
#else
        // Scalar fallback: simple loop
        for (std::size_t i = 0; i < numSamples; ++i)
        {
            data[i] *= gain;
        }
#endif
    }

    /// Apply gain from source to destination buffer
    /// @param dst Destination buffer
    /// @param src Source buffer
    /// @param numSamples Number of samples to process
    /// @param gain Linear gain factor to apply
    inline void apply_gain(float* dst, const float* src, std::size_t numSamples, float gain) noexcept
    {
#if GV3_USE_XSIMD
        // Vectorized implementation
        const f32 gain_vec = broadcast(gain);
        const std::size_t vec_size = width;
        const std::size_t vec_end = (numSamples / vec_size) * vec_size;

        // Main vectorized loop
        for (std::size_t i = 0; i < vec_end; i += vec_size)
        {
            f32 samples = load_unaligned(&src[i]);
            f32 result = mul(samples, gain_vec);
            store_unaligned(&dst[i], result);
        }

        // Scalar tail
        for (std::size_t i = vec_end; i < numSamples; ++i)
        {
            dst[i] = src[i] * gain;
        }
#else
        // Scalar fallback
        for (std::size_t i = 0; i < numSamples; ++i)
        {
            dst[i] = src[i] * gain;
        }
#endif
    }

    /// Compute peak absolute value in buffer (SIMD-optimized)
    /// @param data Pointer to audio samples
    /// @param numSamples Number of samples to process
    /// @return Maximum absolute value in the buffer
    inline float compute_peak(const float* data, std::size_t numSamples) noexcept
    {
#if GV3_USE_XSIMD
        // Vectorized peak detection
        const std::size_t vec_size = width;
        const std::size_t vec_end = (numSamples / vec_size) * vec_size;

        f32 peak_vec = broadcast(0.0f);

        // Main vectorized loop
        for (std::size_t i = 0; i < vec_end; i += vec_size)
        {
            f32 samples = load_unaligned(&data[i]);
            f32 abs_samples = abs(samples);
            peak_vec = max(peak_vec, abs_samples);
        }

        // Reduce to scalar
        float peak = hmax(peak_vec);

        // Scalar tail
        for (std::size_t i = vec_end; i < numSamples; ++i)
        {
            float abs_sample = std::abs(data[i]);
            peak = std::max(peak, abs_sample);
        }

        return peak;
#else
        // Scalar fallback
        float peak = 0.0f;
        for (std::size_t i = 0; i < numSamples; ++i)
        {
            peak = std::max(peak, std::abs(data[i]));
        }
        return peak;
#endif
    }

} // namespace gv3::simd
