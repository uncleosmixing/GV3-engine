# GV3 Engine — CMake Dependency Manifest

## Overview

The GV3 engine uses a **two-layer CMake manifest** system for managing optional dependencies:

1. **`cmake/Options.cmake`** — Defines all configurable options (binary flags, enums)
2. **`cmake/Dependencies.cmake`** — Wires dependencies based on options

This allows the project to:
- Build with **all options OFF** (no external dependencies required)
- Optionally enable DSP, profiling, audio I/O, and other features
- Maintain a centralized dependency registry
- Support future expansion without major refactoring

---

## Build Options

### UI Backend
- `GV3_UI_BACKEND` — `Current` (NanoVG) | `NanoVGXC` (future)
- `GV3_ENABLE_NANOSVG` — SVG rendering (unused)
- `GV3_ENABLE_NANOVGXC` — NanoVG extended functionality

### DSP & Optimization
- `GV3_FFT_BACKEND` — `KissFFT` | `PFFFT` | `FFTW` | `None` (default)
- `GV3_ENABLE_XSIMD` — SIMD acceleration

### Processors & Filters
- `GV3_ENABLE_HIIR` — High-order IIR filter library
- `GV3_ENABLE_EBUR128` — Loudness metering
- `GV3_ENABLE_SLEEF` — Vectorized math functions

### Development & Profiling
- `GV3_ENABLE_TRACY` — Tracy profiler instrumentation

### Memory & Threading
- `GV3_ALLOCATOR` — `System` | `mimalloc` | `rpmalloc` (default: System)
- `GV3_ENABLE_MOODYCAMEL` — Lock-free concurrent queue

### Resampling & Audio I/O
- `GV3_RESAMPLER` — `libsamplerate` | `r8brain` | `None` (default)
- `GV3_AUDIO_IO` — `RtAudio` | `PortAudio` | `None` (default)

---

## Usage

### Default Build (No External Dependencies)
```sh
cmake --preset=x64-release
cmake --build out/build/x64-release
```

All options are OFF by default — only VST3 SDK, NanoVG, and GLAD (already in repo) are used.

### Enable Specific Features
```sh
cmake --preset=x64-release -DGV3_FFT_BACKEND=KissFFT -DGV3_ENABLE_TRACY=ON
cmake --build out/build/x64-release
```

### List All Available Options
```sh
cd out/build/x64-release
cmake -LAH | grep GV3
```

---

## Adding New Dependencies

When you want to add a new library (e.g., `external/kissfft`):

1. **Add option to `cmake/Options.cmake`**:
   ```cmake
   set(GV3_FFT_BACKEND "None" CACHE STRING "...")
   ```

2. **Add wiring to `cmake/Dependencies.cmake`**:
   ```cmake
   if (GV3_FFT_BACKEND STREQUAL "KissFFT")
       # Check if external/kissfft exists
       # Add to appropriate targets
   endif()
   ```

3. **Ensure placeholder messages**:
   - If feature is OFF → no error
   - If feature is ON but not found → helpful message (or fail if required)

4. **Update main `CMakeLists.txt`** to link targets if needed

5. **Test with options OFF** to ensure default build works

---

## Current Wiring Status

✅ **Integrated** (in repo, wired in main CMakeLists.txt):
- `external/vst3sdk` — Steinberg VST3 SDK
- `external/nanovg` — 2D vector graphics
- `external/glad` — OpenGL loader

⏳ **Placeholder** (options defined, not yet vendored):
- KissFFT, PFFFT, FFTW
- XSIMD, HIIR, EBUR128, SLEEF
- Tracy, mimalloc, rpmalloc, moodycamel
- libsamplerate, r8brain, RtAudio, PortAudio

---

## Architecture Rules

1. **All dependencies live in `external/<name>`** — never `third_party/` or root
2. **Each option has a CMake flag** — no magic hardcoding
3. **Build succeeds with all options OFF** — no required external deps (except SDK already in repo)
4. **Placeholder messages for future features** — clean configure output
5. **Helper functions available** — `gv3_require_path`, `gv3_add_header_only_lib`

---

## See Also

- `.github/copilot-instructions.md` — Full development guidelines
- `CMakeLists.txt` — Main build config
