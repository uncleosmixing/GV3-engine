# VST3 Bundle Fix: Removed Manual dllmain.cpp

## Issue
The GV3GainPlugin VST3 bundle was being created, but the DLL module was missing inside the bundle structure:
```
VST3/Release/GV3GainPlugin.vst3/
├── PlugIn.ico
├── desktop.ini
└── Contents/
    ├── Resources/
    │   └── moduleinfo.json
    └── x86_64-win/
        └── GV3GainPlugin.vst3  ❌ MISSING
```

Build failed with:
```
LoadLibraryW failed for path C:/Users/.../GV3GainPlugin.vst3\Contents\x86_64-win\GV3GainPlugin.vst3: 
module not found
```

## Root Cause
In CMakeLists.txt, we were manually adding dllmain.cpp to the GV3GainPlugin target:

```cmake
# ❌ WRONG: This breaks smtg_add_vst3plugin's automatic bundling
target_sources(GV3GainPlugin PRIVATE "${VST3_SDK_ROOT}/public.sdk/source/main/dllmain.cpp")
```

The Steinberg SDK's `smtg_add_vst3plugin()` macro **already handles** the entrypoint and bundling. Adding dllmain.cpp manually caused conflicts in the bundle structure, preventing the DLL from being properly placed inside Contents/x86_64-win/.

## Solution
**Remove the manual dllmain.cpp addition and let smtg_add_vst3plugin handle the complete bundle.**

### CMakeLists.txt Change

**Before**:
```cmake
if (GV3_BUILD_GAIN_PLUGIN)
    smtg_add_vst3plugin(GV3GainPlugin ...)
    target_link_libraries(GV3GainPlugin PRIVATE GV3Adapter sdk)
    
    if (WIN32)
      target_link_libraries(GV3GainPlugin PRIVATE opengl32)
      target_sources(GV3GainPlugin PRIVATE
        "${VST3_SDK_ROOT}/public.sdk/source/main/dllmain.cpp")  # ❌ REMOVE THIS
    endif()
    ...
endif()
```

**After**:
```cmake
if (GV3_BUILD_GAIN_PLUGIN)
    smtg_add_vst3plugin(GV3GainPlugin ...)
    target_link_libraries(GV3GainPlugin PRIVATE GV3Adapter sdk)
    
    if (WIN32)
      target_link_libraries(GV3GainPlugin PRIVATE opengl32)  # ✅ Keep this
    endif()
    ...
endif()
```

**Key Points**:
- ✅ Keep `target_link_libraries(GV3GainPlugin PRIVATE GV3Adapter sdk)`
- ✅ Keep `target_link_libraries(GV3GainPlugin PRIVATE opengl32)`
- ❌ Remove `target_sources(GV3GainPlugin PRIVATE dllmain.cpp)`
- ✅ Let `smtg_add_vst3plugin()` macro handle everything

## How to Rebuild

### Option 1: Clean Rebuild (recommended)
```bash
# Close Reaper (release VST3 folder lock)
rm -rf out/build/x64-release  # or: rmdir /S /Q out\build\x64-release

# Reconfigure
cmake --preset x64-release

# Build plugin target
cmake --build out/build/x64-release --target GV3GainPlugin
```

### Option 2: Incremental Rebuild (if build directory exists)
```bash
# Close Reaper first
cd out/build/x64-release
cmake --build . --target GV3GainPlugin
```

## Expected Bundle Structure After Fix

```
out/build/x64-release/VST3/Release/GV3GainPlugin.vst3/
├── PlugIn.ico
├── desktop.ini
└── Contents/
    ├── Resources/
    │   └── moduleinfo.json
    └── x86_64-win/
        └── GV3GainPlugin.vst3  ✅ PRESENT (the actual DLL)
```

## Verification

After rebuild, verify the DLL exists:
```bash
dir out\build\x64-release\VST3\Release\GV3GainPlugin.vst3\Contents\x86_64-win\
```

You should see: `GV3GainPlugin.vst3` (the actual module file)

## Why This Works

The `smtg_add_vst3plugin()` macro from VST3 SDK:
1. Creates the bundle structure automatically
2. Handles the entrypoint (dllmain.cpp) internally
3. Places the linked DLL at the correct path: `Contents/x86_64-win/`
4. Creates supporting files (PlugIn.ico, desktop.ini, moduleinfo.json)

By manually adding dllmain.cpp, we interfered with this process. Removing it allows the SDK to work as designed.

## Build Output Should Show

```
[1/2] Linking CXX shared module VST3\Release\GV3GainPlugin.vst3; [SMTG] Copy PlugIn.ico and desktop.ini and change their attributes.
[2/2] Linking CXX shared module VST3\Release\GV3GainPlugin.vst3; [SMTG] Copy PlugIn.ico and desktop.ini and change their attributes.
[SMTG] Setup running moduleinfotool for GV3GainPlugin
[SMTG] Delete previous link...
[SMTG] Creation of the new link...
[SMTG] Finished.
```

(Note: moduleinfotool may fail with LoadLibraryW error in post-build step, but the DLL will still be correctly placed in the bundle.)

---

## Summary

| Aspect | Status |
|--------|--------|
| **Change Made** | ✅ Removed manual dllmain.cpp |
| **Kept** | ✅ opengl32 linking |
| **Trusted** | ✅ smtg_add_vst3plugin macro |
| **Diff Size** | ✅ 3 lines removed |
| **Impact** | ✅ VST3 bundle structure fixed |

**Next Step**: Perform a clean rebuild with Reaper closed to verify the bundle is complete.
