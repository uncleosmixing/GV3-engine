# GV3-engine

Modern C++20 realtime audio plugin framework with VST3 support.

## Features

- **VST3 Plugin Framework** - Steinberg VST3 SDK integration
- **NanoVG UI** - Hardware-accelerated vector graphics (OpenGL)
- **SIMD-Optimized DSP** - XSIMD backend with scalar fallback
- **Realtime-Safe** - Lock-free metering, double-buffer synchronization
- **Modular Architecture** - Separate engine/DSP/UI layers
- **CMake Build System** - Git submodule dependency management
- **UI Sandbox** - Standalone GLFW + NanoVG development environment

## Building

### Prerequisites
- CMake 3.10+
- Visual Studio 2022/2026 (Windows)
- Git (for submodule initialization)

### Quick Start

```bash
# Clone with submodules
git clone --recurse-submodules https://github.com/uncleosmixing/GV3-engine
cd GV3-engine

# Or initialize submodules after cloning
git submodule update --init --recursive

# Configure (Release)
cmake --preset x64-release

# Build plugin
cmake --build out/build/x64-release --config Release --target GV3GainPlugin

# Build UI sandbox
cmake --build out/build/x64-release --config Release --target GV3UiSandbox

# Run UI sandbox
./out/build/x64-release/Release/GV3UiSandbox.exe

# Run SIMD test
cmake --build out/build/x64-release --config Release --target GV3SimdTest
./out/build/x64-release/Release/GV3SimdTest.exe
```

### Build Options

- `GV3_ENABLE_XSIMD=ON/OFF` - SIMD vectorization (default: ON)
- `GV3_BUILD_SIMD_TEST=ON/OFF` - Build SIMD test executable (default: ON)
- `GV3_BUILD_GAIN_PLUGIN=ON/OFF` - Build GV3GainPlugin (default: ON)
- `GV3_BUILD_UI_SANDBOX=ON/OFF` - Build UI sandbox (default: ON)

## UI Sandbox

The UI Sandbox is a standalone application for developing and testing NanoVG UI components without loading the plugin in a DAW:

- **Windowing**: GLFW 3.4 (git submodule)
- **Graphics**: OpenGL 3.3+ Core Profile
- **Rendering**: NanoVG with GL3 backend
- **Features**: 
  - Professional dBFS meters with static scale (0 to -60 dB)
  - Peak hold display with click-to-reset
  - Peak marker line showing maximum level
  - Interactive knobs and controls
  - Resizable window with HiDPI support

**Controls:**
- Drag knob to change gain value
- Scroll to adjust input meter levels
- Click peak hold text (yellow) to reset
- ESC to exit

The sandbox demonstrates the reusable `MeterWidget` component, which provides broadcast-quality metering suitable for professional audio applications. See [`docs/METER_WIDGET.md`](docs/METER_WIDGET.md) for API documentation.

## SIMD Backend

The engine includes a unified SIMD abstraction layer (`gv3::simd`) that transparently switches between scalar and vectorized implementations:

- **XSIMD Backend** (default): 4x speedup via SSE/AVX, integrated as git submodule
- **Scalar Backend** (fallback): Zero-overhead abstraction when XSIMD is disabled

See [`docs/SIMD_INTEGRATION.md`](docs/SIMD_INTEGRATION.md) for details.

## Project Structure

```
GlobalVST3Engine/
├── src/
│   ├── engine/          # Core engine (ParameterRegistry, ParameterState)
│   ├── dsp/             # DSP processors (Gain, EQ, Compressor)
│   │   └── simd/        # SIMD abstraction layer
│   └── ui/              # UI components (MeterBridge, NanoVG bridge)
├── vst3/                # VST3 adapter layer
├── plugins/             # Plugin implementations (GainPlugin)
├── tests/               # Unit tests (SimdTest)
├── external/            # Third-party dependencies
│   ├── vst3sdk/         # Steinberg VST3 SDK
│   ├── nanovg/          # NanoVG vector graphics
│   └── glad/            # OpenGL loader
├── cmake/               # CMake modules (Options, Dependencies)
└── docs/                # Documentation
```

## Architecture Principles

1. **Hard Realtime Rules** - No allocations/locks in audio thread
2. **Lock-Free Synchronization** - Double-buffer patterns for audio→UI
3. **Modular Dependencies** - Optional features via CMake options
4. **Frozen Architecture** - Minimal diffs, incremental changes only

See [`.github/copilot-instructions.md`](.github/copilot-instructions.md) for development guidelines.

## License

TBDTBD