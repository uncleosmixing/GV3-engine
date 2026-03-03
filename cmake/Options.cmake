# ===========================================================================
# GV3 Engine — CMake Dependency Manifest (Options Layer)
# ===========================================================================
# This file defines all configurable options for the GV3 engine.
# Most options are OFF by default (stub implementations provided).
# Users can enable/change these via:
#   cmake -DGV3_ENABLE_NANOVGXC=ON -DGV3_UI_BACKEND=NanoVG ...
# ===========================================================================

# ===========================================================================
# UI Backend Options
# ===========================================================================
option(GV3_ENABLE_NANOSVG
  "Enable NanoSVG (SVG rendering backend, currently unused)" OFF)

option(GV3_ENABLE_NANOVGXC
  "Enable NanoVG with NanoVGXC (extended functionality)" OFF)

set(GV3_UI_BACKEND "Current" CACHE STRING
  "UI rendering backend: Current (NanoVG) | NanoVGXC")
set_property(CACHE GV3_UI_BACKEND PROPERTY STRINGS "Current" "NanoVGXC")

# ===========================================================================
# DSP & Optimization Options
# ===========================================================================
option(GV3_ENABLE_XSIMD
  "Enable XSIMD (SIMD acceleration)" ON)

set(GV3_FFT_BACKEND "None" CACHE STRING
  "FFT backend: KissFFT | PFFFT | FFTW | None")
set_property(CACHE GV3_FFT_BACKEND PROPERTY STRINGS "KissFFT" "PFFFT" "FFTW" "None")

# ===========================================================================
# DSP Processors Options
# ===========================================================================
option(GV3_ENABLE_HIIR
  "Enable HIIR (high-order IIR filter library)" OFF)

option(GV3_ENABLE_EBUR128
  "Enable libebur128 (loudness metering)" OFF)

option(GV3_ENABLE_SLEEF
  "Enable SLEEF (vectorized math)" OFF)

# ===========================================================================
# Development & Profiling Options
# ===========================================================================
option(GV3_ENABLE_TRACY
  "Enable Tracy profiling instrumentation" OFF)

option(GV3_BUILD_UI_SANDBOX
  "Build standalone UI sandbox (GLFW + NanoVG)" ON)

# ===========================================================================
# Memory & Threading Options
# ===========================================================================
set(GV3_ALLOCATOR "System" CACHE STRING
  "Memory allocator: System | mimalloc | rpmalloc")
set_property(CACHE GV3_ALLOCATOR PROPERTY STRINGS "System" "mimalloc" "rpmalloc")

option(GV3_ENABLE_MOODYCAMEL
  "Enable moodycamel::ConcurrentQueue (lock-free MPMC queue)" OFF)

# ===========================================================================
# Resampling & Audio I/O Options
# ===========================================================================
set(GV3_RESAMPLER "None" CACHE STRING
  "Resampler: libsamplerate | r8brain | None")
set_property(CACHE GV3_RESAMPLER PROPERTY STRINGS "libsamplerate" "r8brain" "None")

set(GV3_AUDIO_IO "None" CACHE STRING
  "Audio I/O backend: RtAudio | PortAudio | None")
set_property(CACHE GV3_AUDIO_IO PROPERTY STRINGS "RtAudio" "PortAudio" "None")

# ===========================================================================
# Print summary
# ===========================================================================
message(STATUS "")
message(STATUS "=== GV3 Engine — Build Options ===")
message(STATUS "  UI Backend:       ${GV3_UI_BACKEND}")
message(STATUS "  FFT Backend:      ${GV3_FFT_BACKEND}")
message(STATUS "  Allocator:        ${GV3_ALLOCATOR}")
message(STATUS "  Resampler:        ${GV3_RESAMPLER}")
message(STATUS "  Audio I/O:        ${GV3_AUDIO_IO}")
message(STATUS "  XSIMD:            ${GV3_ENABLE_XSIMD}")
message(STATUS "  HIIR:             ${GV3_ENABLE_HIIR}")
message(STATUS "  EBUR128:          ${GV3_ENABLE_EBUR128}")
message(STATUS "  SLEEF:            ${GV3_ENABLE_SLEEF}")
message(STATUS "  Tracy:            ${GV3_ENABLE_TRACY}")
message(STATUS "  Moodycamel:       ${GV3_ENABLE_MOODYCAMEL}")
message(STATUS "  UI Sandbox:       ${GV3_BUILD_UI_SANDBOX}")
message(STATUS "")
