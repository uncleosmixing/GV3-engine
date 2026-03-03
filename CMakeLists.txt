cmake_minimum_required(VERSION 3.10)

if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT
      "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project("GlobalVST3Engine" VERSION 1.0.0)

# ===========================================================================
# Load CMake Dependency Manifest (Options & Wiring)
# ===========================================================================
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(Options)
include(Dependencies)

# ===========================================================================
# Core engine library (always built)
# ===========================================================================
add_library(GlobalVST3Engine STATIC
  "GlobalVST3Engine.h"
  src/engine/ParameterRegistry.h
  src/engine/ParameterRegistry.cpp
  src/engine/ParameterState.h
  src/engine/ParameterState.cpp
  src/engine/ParameterStore.cpp
  src/engine/AudioBlock.cpp
  src/engine/PluginProcessor.cpp
  src/engine/PluginFactory.cpp
  src/dsp/EqualizerProcessor.cpp
  src/dsp/CompressorProcessor.cpp
  src/dsp/GainProcessor.cpp
  src/ui/NanoVGEditorModel.cpp
  src/dsp/simd/GV3SimdConfig.h
  src/dsp/simd/GV3SimdTypes.h
  src/dsp/simd/GV3SimdMath.h
  src/dsp/simd/GV3SimdGain.h
  
  # --- UI2 LAYER (modular, responsive architecture — ONLY UI PATH) ---
  src/ui2/core/Rect.h
  src/ui2/core/UiContext.h
  src/ui2/core/InputState.h
  src/ui2/core/ScissorGuard.h
  src/ui2/core/Widget.h
  src/ui2/layout/LayoutEngine.h
  src/ui2/layout/LayoutEngine.cpp
  src/ui2/scenes/GainScene.h
  src/ui2/scenes/GainScene.cpp
  src/ui2/widgets/KnobWidget.h
  src/ui2/widgets/KnobWidget.cpp
)

target_include_directories(GlobalVST3Engine PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

# Add NanoVG include path (header-only for UI components)
if (NANOVG_DIR AND EXISTS "${NANOVG_DIR}/src/nanovg.h")
  target_include_directories(GlobalVST3Engine PUBLIC "${NANOVG_DIR}/src")
  target_compile_definitions(GlobalVST3Engine PUBLIC GV3_HAS_NANOVG=1)
  message(STATUS "[GV3] GlobalVST3Engine: NanoVG headers included")
else()
  target_compile_definitions(GlobalVST3Engine PUBLIC GV3_HAS_NANOVG=0)
  message(WARNING "[GV3] GlobalVST3Engine: NanoVG not found, UI components disabled")
endif()

target_compile_features(GlobalVST3Engine PUBLIC cxx_std_20)

# ===========================================================================
# XSIMD integration (if enabled)
# ===========================================================================
if(GV3_ENABLE_XSIMD)
  target_link_libraries(GlobalVST3Engine PUBLIC gv3::xsimd)
  target_compile_definitions(GlobalVST3Engine PUBLIC GV3_SIMD_BACKEND_XSIMD=1)
  message(STATUS "[GV3] GlobalVST3Engine: XSIMD backend enabled (width=${simd_width})")
else()
  target_compile_definitions(GlobalVST3Engine PUBLIC GV3_SIMD_BACKEND_XSIMD=0)
  message(STATUS "[GV3] GlobalVST3Engine: Scalar backend (width=1)")
endif()

# ===========================================================================
# Demo executable (optional)
# ===========================================================================
option(GV3_BUILD_DEMO "Build demo executable for the engine" ON)

if (GV3_BUILD_DEMO)
  add_executable(GlobalVST3EngineDemo "DemoMain.cpp")
  target_link_libraries(GlobalVST3EngineDemo PRIVATE GlobalVST3Engine)
endif()

# ===========================================================================
# SIMD Test executable (optional)
# ===========================================================================
option(GV3_BUILD_SIMD_TEST "Build SIMD backend test" ON)

if (GV3_BUILD_SIMD_TEST)
  add_executable(GV3SimdTest "tests/SimdTest.cpp")
  target_link_libraries(GV3SimdTest PRIVATE GlobalVST3Engine)
  target_include_directories(GV3SimdTest PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
  message(STATUS "[GV3] GV3SimdTest target added")
endif()

# ===========================================================================
# UI Sandbox — standalone GLFW + NanoVG UI development environment
# ===========================================================================

# Set dependency paths (MUST be before UI Sandbox config)
set(VST3_SDK_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/external/vst3sdk" CACHE PATH "Path to Steinberg VST3 SDK root directory" FORCE)
set(NANOVG_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/nanovg" CACHE PATH "Path to NanoVG source directory" FORCE)
set(GLAD_DIR   "${CMAKE_CURRENT_SOURCE_DIR}/external/glfw/glad" CACHE PATH "Path to glad directory (include/ and src/)" FORCE)

if (GV3_BUILD_UI_SANDBOX AND TARGET glfw)
  add_executable(GV3UiSandbox "plugins/ui_sandbox/main.cpp")
  
  # Set C++ standard
  target_compile_features(GV3UiSandbox PRIVATE cxx_std_17)
  
  # Link core engine (for MeterWidget and other UI components)
  target_link_libraries(GV3UiSandbox PRIVATE GlobalVST3Engine)
  
  # Link GLFW
  target_link_libraries(GV3UiSandbox PRIVATE glfw)
  
  # Link OpenGL
  if (WIN32)
    target_link_libraries(GV3UiSandbox PRIVATE opengl32)
  elseif (APPLE)
    target_link_libraries(GV3UiSandbox PRIVATE "-framework OpenGL")
  else()
    target_link_libraries(GV3UiSandbox PRIVATE GL)
  endif()
  
  # Add glad
  if (GLAD_DIR AND EXISTS "${GLAD_DIR}/include/glad/gl.h")
    target_sources(GV3UiSandbox PRIVATE "${GLAD_DIR}/src/gl.c")
    target_include_directories(GV3UiSandbox PRIVATE "${GLAD_DIR}/include")
  endif()
  
  # Add NanoVG
  if (NANOVG_DIR AND EXISTS "${NANOVG_DIR}/src/nanovg.h")
    target_sources(GV3UiSandbox PRIVATE "${NANOVG_DIR}/src/nanovg.c")
    target_include_directories(GV3UiSandbox PRIVATE "${NANOVG_DIR}/src")
    target_compile_definitions(GV3UiSandbox PRIVATE GV3_HAS_NANOVG=1)
  endif()
  
  message(STATUS "[GV3] GV3UiSandbox target added")
elseif(GV3_BUILD_UI_SANDBOX)
  message(STATUS "[GV3] GV3UiSandbox: Skipped (GLFW not available)")
endif()

# ===========================================================================
# GainDemo — standalone Win32 + GDI+ visual demo (no SDK required)
# ===========================================================================
if (WIN32)
  option(GV3_BUILD_GAIN_DEMO "Build standalone Gain knob demo (Win32 + GDI+)" ON)
  if (GV3_BUILD_GAIN_DEMO)
    add_executable(GainDemo WIN32 "GainDemo.cpp")
    target_link_libraries(GainDemo PRIVATE GlobalVST3Engine gdiplus)
  endif()
endif()

# ===========================================================================
# VST3 SDK adapter (optional — set VST3_SDK_ROOT to enable)
# ===========================================================================
if (VST3_SDK_ROOT AND EXISTS "${VST3_SDK_ROOT}/CMakeLists.txt")
  message(STATUS "[GV3] VST3 SDK found at: ${VST3_SDK_ROOT}")

  set(SMTG_ENABLE_VSTGUI_SUPPORT          OFF CACHE BOOL "" FORCE)
  set(SMTG_ENABLE_VST3_PLUGIN_EXAMPLES    OFF CACHE BOOL "" FORCE)
  set(SMTG_ENABLE_VST3_HOSTING_EXAMPLES   OFF CACHE BOOL "" FORCE)
  set(SMTG_ENABLE_WAYLAND_SUPPORT         OFF CACHE BOOL "" FORCE)
  set(SMTG_RUN_VST_VALIDATOR              OFF CACHE BOOL "" FORCE)

  add_subdirectory("${VST3_SDK_ROOT}" "${CMAKE_BINARY_DIR}/vst3sdk")

  add_library(GV3Adapter STATIC
    vst3/GV3ProcessorBase.cpp
    vst3/GV3ProcessorBase.h
    vst3/GV3ControllerBase.cpp
    vst3/GV3ControllerBase.h
    vst3/GV3EditorView.cpp
    vst3/GV3EditorView.h
    vst3/GV3Entry.h
  )

  target_link_libraries(GV3Adapter PUBLIC GlobalVST3Engine sdk)
  target_include_directories(GV3Adapter PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

  if (NANOVG_DIR AND EXISTS "${NANOVG_DIR}/src/nanovg.h")
    message(STATUS "[GV3] NanoVG found at: ${NANOVG_DIR}")
    target_sources(GV3Adapter PRIVATE "${NANOVG_DIR}/src/nanovg.c")
    target_include_directories(GV3Adapter PRIVATE "${NANOVG_DIR}/src")
    target_compile_definitions(GV3Adapter PRIVATE GV3_HAS_NANOVG=1)
  endif()

  if (GLAD_DIR AND EXISTS "${GLAD_DIR}/include/glad/gl.h")
    message(STATUS "[GV3] glad found at: ${GLAD_DIR}")
    target_sources(GV3Adapter PRIVATE "${GLAD_DIR}/src/gl.c")
    target_include_directories(GV3Adapter PRIVATE "${GLAD_DIR}/include")
  endif()

  if (WIN32)
    target_link_libraries(GV3Adapter PRIVATE opengl32)
  endif()

  option(GV3_BUILD_GAIN_PLUGIN "Build the GV3 Gain VST3 plugin" ON)

  if (GV3_BUILD_GAIN_PLUGIN)
    smtg_add_vst3plugin(GV3GainPlugin
      plugins/GainPlugin/GainEntry.cpp
      plugins/GainPlugin/GainUI.h
    )
    target_link_libraries(GV3GainPlugin PRIVATE GV3Adapter sdk)

    if (WIN32)
      target_link_libraries(GV3GainPlugin PRIVATE opengl32)
    endif()

    if (NANOVG_DIR AND EXISTS "${NANOVG_DIR}/src/nanovg.h")
      target_include_directories(GV3GainPlugin PRIVATE "${NANOVG_DIR}/src")
    endif()

    if (GLAD_DIR AND EXISTS "${GLAD_DIR}/include/glad/gl.h")
      target_include_directories(GV3GainPlugin PRIVATE "${GLAD_DIR}/include")
    endif()

    message(STATUS "[GV3] GV3GainPlugin target added")
  endif()
else()
  message(STATUS "[GV3] VST3_SDK_ROOT not set — adapter will not be built")
endif()
