# GV3 Engine Architecture

## Modules

- `src/engine` — core engine abstractions:
  - `ParameterStore`
  - `AudioBlock`
  - `PluginProcessor` base
  - `PluginFactory`
- `src/dsp` — DSP implementations:
  - `EqualizerProcessor`
  - `CompressorProcessor`
  - `GainProcessor`
- `src/ui` — UI model/bridge:
  - `NanoVGEditorModel`
  - `NanoVGEditorBridge`
- `vst3` — Steinberg VST3 adapter layer:
  - `GV3ProcessorBase`
  - `GV3ControllerBase`
  - `GV3EditorView`
  - `GV3Entry`
- `plugins/GainPlugin` — first plugin implementation and NanoVG UI.

## External dependencies (vendored in repo)

- `external/vst3sdk`
- `external/nanovg`
- `external/glad`

CMake now points to these folders by default.

## Git Initialization

To initialize the git repository and make the initial commit, run the following commands:

```
git init
git add .
git commit -m "Initial commit: GV3 engine + Gain VST3 plugin"
git branch -M main
git remote add origin https://github.com/uncleosmixing/GV3-engine.git
git push -u origin main
