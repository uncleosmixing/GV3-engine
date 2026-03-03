# GV3 Engine — Copilot Instructions

You are working inside a realtime audio plugin framework:
- VST3 SDK (Steinberg) adapter layer in `vst3/`
- Engine + DSP in `src/`
- UI uses NanoVG (currently in `external/nanovg`)
NO JUCE. Do not introduce JUCE.

## 1) Hard realtime rules (audio thread)
In any audio processing callback / `process()`-like code:
- NO allocations (new/delete, malloc/free, std::vector growth, push_back, std::string creation)
- NO mutex/locks (std::mutex, std::lock_guard, condition_variable)
- NO exceptions (throw/catch), NO RTTI tricks
- NO logging/printf/file I/O
- NO unordered_map/map lookups or inserts
- Use: atomics, fixed-size arrays, preallocated buffers, lock-free patterns only

If you need data from UI -> audio: use atomics or lock-free messaging (double-buffer/queue).
If you need heavy work (FFT, loudness, analysis): do it on background thread and publish results lock-free.

## 2) Frozen architecture (do not rewrite)
Do NOT rewrite the framework from scratch.
Do NOT rename/relocate major modules without explicit request.
Prefer minimal diffs and incremental changes.

Repo structure (must keep):
- external/      (ALL third party dependencies live here)
- src/           (engine, dsp, ui core)
- vst3/          (VST3 adapter/controller/processor/editor)
- plugins/       (example plugins, build outputs)
- docs/          (documentation)
- CMakeLists.txt / CMakePresets.json

## 3) Dependency policy (CMake)
Goal: centralized dependency wiring with options.
- Dependencies must remain in `external/<name>` (NOT `third_party/`)
- Each dependency must have a CMake option (GV3_ENABLE_* or enum)
- All wiring must go into:
  - `cmake/Options.cmake`
  - `cmake/Dependencies.cmake`
- The project MUST build when all dependency options are OFF (provide stubs/fallbacks)

Do not vendor or download new libraries unless explicitly asked.
First implement the manifest and the options; wiring to actual external libs can be incremental.

## 4) Parameters & state (framework rule)
- Separate Parameter Registry (descriptions/metadata) from Parameter State (atomic values).
- Audio thread accesses parameters by index/ID that is NOT a string.
- UI/Controller may use string IDs for display only.
- Automation must use VST3 beginEdit/performEdit/endEdit pattern.

## 5) UI rules
- UI widgets must not depend on backend implementation details.
- If multiple backends are added, introduce `IRenderer2D` abstraction.
- Support DPI scaling and resizing.
- Prefer parametric drawing (paths/gradients) over bitmap assets.

## 6) Git policy (IMPORTANT)
- Do NOT run git commands automatically.
- Do NOT push to remotes unless explicitly asked.
- When working on a big change: propose a small step plan and implement in small chunks.

## 7) Work style
- First respond with a short plan (5–8 steps) before editing.
- Keep changes local and focused.
- Build after each step and fix errors before continuing.
- Prefer readable modern C++ and avoid overengineering.