#pragma once

#include "GV3SimdConfig.h"
#include <cstddef>

// ===========================================================================
// GV3 SIMD Types
// ===========================================================================
// Type definitions and load/store helpers for SIMD operations.
// Scalar backend: f32 is float, SIMD width = 1
// Future: When XSIMD is integrated, f32 will be xsimd::batch<float>

namespace gv3::simd
{

#if GV3_USE_XSIMD
    // XSIMD backend: vectorized batch operations
    using f32 = xsimd::batch<float, default_arch>;

    // Load aligned
    inline f32 load(const float* ptr) noexcept
    {
        return f32::load_aligned(ptr);
    }

    // Store aligned
    inline void store(float* ptr, f32 value) noexcept
    {
        value.store_aligned(ptr);
    }

    // Load unaligned
    inline f32 load_unaligned(const float* ptr) noexcept
    {
        return f32::load_unaligned(ptr);
    }

    // Store unaligned
    inline void store_unaligned(float* ptr, f32 value) noexcept
    {
        value.store_unaligned(ptr);
    }

    // Broadcast scalar to SIMD register
    inline f32 broadcast(float value) noexcept
    {
        return f32(value);
    }

    // Horizontal maximum (reduce batch to single float)
    inline float hmax(f32 value) noexcept
    {
        return xsimd::reduce_max(value);
    }

#else
    // Scalar backend: SIMD type is just a single float
    using f32 = float;

    // Load a single float from memory
    inline f32 load(const float* ptr) noexcept
    {
        return *ptr;
    }

    // Store a single float to memory
    inline void store(float* ptr, f32 value) noexcept
    {
        *ptr = value;
    }

    // Load unaligned (same as load for scalar)
    inline f32 load_unaligned(const float* ptr) noexcept
    {
        return *ptr;
    }

    // Store unaligned (same as store for scalar)
    inline void store_unaligned(float* ptr, f32 value) noexcept
    {
        *ptr = value;
    }

    // Broadcast a scalar value to SIMD type (no-op for scalar)
    inline f32 broadcast(float value) noexcept
    {
        return value;
    }

    // Horizontal maximum (identity for scalar)
    inline float hmax(f32 value) noexcept
    {
        return value;
    }

#endif

} // namespace gv3::simd
