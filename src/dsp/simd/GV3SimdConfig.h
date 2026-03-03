#pragma once

// ===========================================================================
// GV3 SIMD Backend Configuration
// ===========================================================================
// This header defines which SIMD backend is active and provides compile-time
// feature detection. Supports scalar fallback and XSIMD vectorization.

// Detect backend from CMake definitions
#if defined(GV3_SIMD_BACKEND_XSIMD) && GV3_SIMD_BACKEND_XSIMD
    #define GV3_USE_XSIMD 1
    #define GV3_USE_SCALAR 0
    #include <xsimd/xsimd.hpp>
#else
    #define GV3_USE_XSIMD 0
    #define GV3_USE_SCALAR 1
#endif

namespace gv3::simd
{
    // SIMD configuration constants
#if GV3_USE_XSIMD
    using default_arch = xsimd::default_arch;
    constexpr size_t width = xsimd::batch<float, default_arch>::size;
    constexpr const char* backend_name = "XSIMD";
#else
    constexpr size_t width = 1;
    constexpr const char* backend_name = "Scalar";
#endif

} // namespace gv3::simd
