# MeterWidget — Professional dBFS Metering Component

## Overview

`MeterWidget` is a reusable NanoVG-based UI component that provides professional-grade dBFS metering with static scale, momentary peak display, peak hold, and interactive reset functionality.

## Features

### Visual Elements

1. **Static dBFS Scale** - Tick marks and numbers at standard levels:
   - 0, -6, -12, -18, -24, -36, -48, -60 dB
   - Always visible, provides reference for signal levels

2. **Dynamic Momentary Peak Bar**
   - Real-time peak level display
   - Color-coded zones:
     * **Green**: Below -18 dBFS (safe signal level)
     * **Yellow**: -18 to -6 dBFS (approaching maximum)
     * **Red**: Above -6 dBFS (danger zone, potential clipping)
   - Smooth gradient transitions

3. **Peak Hold Display**
   - Displayed as text at bottom of meter (e.g., "-3.2 dBFS", "+1.0 dBFS")
   - Bright yellow color indicates clickable
   - Updates only upward (captures maximum level)
   - Persists until manually reset
   - Shows "-inf dBFS" when at floor value

4. **Peak Marker Line**
   - Thin horizontal line on scale at peak hold level
   - Bright yellow color for visibility
   - Visual indicator of maximum reached level
   - Helps identify historical peaks during mixing

### Interaction

- **Click-to-Reset**: Click on peak hold text to reset to floor value (-120 dB)
- **Auto-Update**: Peak hold automatically captures new maximum values

## API

### Class: `gv3::ui::MeterWidget`

```cpp
#include "src/ui/MeterWidget.h"

// Constructor
MeterWidget();

// Update with new momentary peak value
void update(float normalizedPeak, float refLevel = 0.316f);

// Reset peak hold to floor
void resetPeakHold();

// Draw meter
void draw(NVGcontext* vg, float x, float y, float width, float height, 
          const char* label = nullptr);

// Check if point is inside clickable peak hold region
bool isInsidePeakHoldBounds(float mouseX, float mouseY) const;

// Accessors
float getMomentaryDbFS() const;
float getPeakHoldDbFS() const;
```

## Usage Example

### Standalone (UI Sandbox)

```cpp
#include "src/ui/MeterWidget.h"

struct MyUIState {
    gv3::ui::MeterWidget meterL;
    gv3::ui::MeterWidget meterR;
};

void updateMeters(MyUIState& state, float signalL, float signalR) {
    // Update with normalized 0-1 peak values from DSP
    state.meterL.update(signalL);
    state.meterR.update(signalR);
}

void renderUI(NVGcontext* vg, MyUIState& state) {
    // Draw meters
    state.meterL.draw(vg, 100, 200, 60, 350, "L");
    state.meterR.draw(vg, 170, 200, 60, 350, "R");
}

void handleMouseClick(float x, float y, MyUIState& state) {
    if (state.meterL.isInsidePeakHoldBounds(x, y)) {
        state.meterL.resetPeakHold();
    }
    if (state.meterR.isInsidePeakHoldBounds(x, y)) {
        state.meterR.resetPeakHold();
    }
}
```

### VST3 Plugin Integration

For VST3 plugin integration, MeterWidget instances should be stored in UI state (not in DSP processor):

```cpp
class MyPluginUI {
private:
    gv3::ui::MeterWidget m_meterInL, m_meterInR;
    gv3::ui::MeterWidget m_meterOutL, m_meterOutR;

public:
    void onTimer() {
        // Read normalized meter values from MeterBridge
        auto snapshot = meterBridge->readSnapshot();
        
        // Update widgets (converts to dBFS internally)
        m_meterInL.update(snapshot.inputL);
        m_meterInR.update(snapshot.inputR);
        m_meterOutL.update(snapshot.outputL);
        m_meterOutR.update(snapshot.outputR);
    }

    void paint(NVGcontext* vg) {
        m_meterInL.draw(vg, x1, y, w, h, "IN L");
        m_meterInR.draw(vg, x2, y, w, h, "IN R");
        m_meterOutL.draw(vg, x3, y, w, h, "OUT L");
        m_meterOutR.draw(vg, x4, y, w, h, "OUT R");
    }

    void onMouseDown(int x, int y) {
        if (m_meterInL.isInsidePeakHoldBounds(x, y)) {
            m_meterInL.resetPeakHold();
        }
        // ... check other meters
    }
};
```

## Data Flow

### DSP → UI Pipeline

1. **DSP Thread** (realtime-safe):
   ```cpp
   float peakL = simd::compute_peak(audioBuffer, numSamples);  // Raw linear peak
   float normPeakL = peakL / refLevel;  // Normalize (e.g., refLevel = 0.316 for -10dB ref)
   meterBridge->writeMeterSample(normPeakL, ...);  // Lock-free write
   ```

2. **UI Thread**:
   ```cpp
   auto snapshot = meterBridge->readSnapshot();  // Lock-free read
   meterWidget.update(snapshot.inputL);  // Widget converts to dBFS internally
   ```

3. **MeterWidget Internal Conversion**:
   ```cpp
   float linearPeak = normalizedPeak * refLevel;  // Denormalize
   float dbfs = 20.0f * log10(linearPeak + epsilon);  // Convert to dBFS
   dbfs = std::max(dbfs, -120.0f);  // Clamp to floor
   ```

### Peak Hold Logic

Peak hold is **UI-side only** (not in DSP):

```cpp
void MeterWidget::update(float normalizedPeak, float refLevel) {
    m_momentaryDbFS = normalizedToDbFS(normalizedPeak, refLevel);
    
    // Update peak hold (only if new peak is higher)
    m_peakHoldDbFS = std::max(m_peakHoldDbFS, m_momentaryDbFS);
}
```

**Why UI-side?**
- DSP side uses attack/release ballistics for visual smoothing
- Peak hold is a UI-only feature for user reference
- Reset is a UI event (mouse click)
- Allows per-channel independent reset without DSP communication

## Performance

### Real-Time Safety
- **NO allocations** in render path
- **NO dynamic memory** (fixed-size stack buffers for formatting)
- **NO locks** (state updated on UI thread only)
- Stack-based string formatting via `snprintf`

### Optimizations
- Layout cache for click detection (no per-frame recalculations)
- Minimal NanoVG state changes
- Efficient gradient rendering

## Constants

```cpp
static constexpr float FLOOR_DB = -120.0f;    // Minimum dBFS (silence floor)
static constexpr float SCALE_MIN_DB = -60.0f;  // Bottom of visible scale
static constexpr float SCALE_MAX_DB = 0.0f;    // Top of visible scale
```

**Note**: Momentary peak can exceed 0 dBFS (e.g., +6 dBFS for clipping signals), but scale only displays 0 to -60 dB range. Peaks above 0 dB will be clamped visually but displayed correctly in text.

## Dependencies

- **NanoVG**: For all rendering operations
- **C++ Standard Library**: `<cmath>`, `<algorithm>`, `<cstdio>` (stack-based formatting)

## Files

- **Header**: `src/ui/MeterWidget.h`
- **Implementation**: `src/ui/MeterWidget.cpp`
- **Demo**: `plugins/ui_sandbox/main.cpp` (full working example)

## Design Rationale

### Why dBFS?

Professional audio software uses dBFS (decibels relative to full scale) for metering because:
- Industry standard (broadcast, mixing, mastering)
- Logarithmic scale matches human hearing
- Clear indication of headroom (0 dBFS = digital maximum)
- Standardized reference points (e.g., -18 dBFS LUFS target for streaming)

### Why UI-Side Peak Hold?

Peak hold stored in UI (not DSP) because:
- **No DSP→UI communication overhead** for reset events
- **Simpler real-time safety** (UI events don't touch audio thread)
- **Per-instance state** (multiple UI views can have independent peak holds)
- **Typical UX pattern** in professional DAWs (e.g., Pro Tools, Reaper, Logic)

### Why Static Scale?

Static scale with tick marks provides:
- **Constant reference** (no mental recalculation during mixing)
- **Faster visual recognition** (trained users know -18 dBFS position instantly)
- **Professional appearance** (matches hardware/software standards)

## Future Enhancements

Potential additions (not currently implemented):

1. **Peak Marker Decay** - Slow fall-off of peak marker (e.g., 1 dB/sec)
2. **Clip Indicators** - Red LED-style indicator when signal exceeds 0 dBFS
3. **RMS Metering** - Average level in addition to peak
4. **Ballistics Options** - User-selectable attack/release (K-20, K-14, BBC PPM, etc.)
5. **Horizontal Layout** - Rotate meter for compact horizontal display
6. **Customizable Scale Range** - Adjustable min/max dB values
7. **Stereo Correlation** - Phase meter for L/R correlation

## License

Part of GV3 Engine (TBD license).
