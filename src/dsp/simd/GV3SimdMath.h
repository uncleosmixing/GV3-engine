#pragma once

#include "GV3SimdTypes.h"
#include <algorithm>
#include <cmath>

// ===========================================================================
// GV3 SIMD Math Operations
// ===========================================================================
// Basic SIMD math operations: add, mul, min, max, clamp, abs
// Scalar backend: direct operations on float
// XSIMD backend: vectorized batch operations

namespace gv3::simd
{

#if GV3_USE_XSIMD

    // XSIMD vectorized operations
    inline f32 add(f32 a, f32 b) noexcept
    {
        return a + b;
    }

    inline f32 mul(f32 a, f32 b) noexcept
    {
        return a * b;
    }

    inline f32 sub(f32 a, f32 b) noexcept
    {
        return a - b;
    }

    inline f32 div(f32 a, f32 b) noexcept
    {
        return a / b;
    }

    inline f32 min(f32 a, f32 b) noexcept
    {
        return xsimd::min(a, b);
    }

    inline f32 max(f32 a, f32 b) noexcept
    {
        return xsimd::max(a, b);
    }

    inline f32 abs(f32 a) noexcept
    {
        return xsimd::abs(a);
    }

    inline f32 clamp(f32 value, f32 min_val, f32 max_val) noexcept
    {
        return xsimd::clip(value, min_val, max_val);
    }

    inline f32 fmadd(f32 a, f32 b, f32 c) noexcept
    {
        return xsimd::fma(a, b, c);
    }

    inline f32 sqrt(f32 a) noexcept
    {
        return xsimd::sqrt(a);
    }

#else
    // Scalar fallback operations
    inline f32 add(f32 a, f32 b) noexcept
    {
        return a + b;
    }

    inline f32 mul(f32 a, f32 b) noexcept
    {
        return a * b;
    }

    inline f32 sub(f32 a, f32 b) noexcept
    {
        return a - b;
    }

    inline f32 div(f32 a, f32 b) noexcept
    {
        return a / b;
    }

    inline f32 min(f32 a, f32 b) noexcept
    {
        return std::min(a, b);
    }

    inline f32 max(f32 a, f32 b) noexcept
    {
        return std::max(a, b);
    }

    inline f32 abs(f32 a) noexcept
    {
        return std::abs(a);
    }

    inline f32 clamp(f32 value, f32 min_val, f32 max_val) noexcept
    {
        return std::clamp(value, min_val, max_val);
    }

    inline f32 fmadd(f32 a, f32 b, f32 c) noexcept
    {
        return (a * b) + c;
    }

    inline f32 sqrt(f32 a) noexcept
    {
        return std::sqrt(a);
    }

#endif

} // namespace gv3::simd
