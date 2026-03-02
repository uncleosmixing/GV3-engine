cmake_minimum_required(VERSION 3.10)

if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT
      "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project("GlobalVST3Engine" VERSION 1.0.0)

# ===========================================================================
# Core engine library (always built)
# ===========================================================================
add_library(GlobalVST3Engine STATIC
  "GlobalVST3Engine.h"
  src/engine/ParameterStore.cpp
  src/engine/AudioBlock.cpp
  src/engine/PluginProcessor.cpp
  src/engine/PluginFactory.cpp
  src/dsp/EqualizerProcessor.cpp
  src/dsp/CompressorProcessor.cpp
  src/dsp/GainProcessor.cpp
  src/ui/NanoVGEditorModel.cpp
)

target_include_directories(GlobalVST3Engine PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_features(GlobalVST3Engine PUBLIC cxx_std_20)

# ===========================================================================
# Demo executable (optional)
# ===========================================================================
option(GV3_BUILD_DEMO "Build demo executable for the engine" ON)

if (GV3_BUILD_DEMO)
  add_executable(GlobalVST3EngineDemo "DemoMain.cpp")
  target_link_libraries(GlobalVST3EngineDemo PRIVATE GlobalVST3Engine)
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
set(VST3_SDK_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/external/vst3sdk" CACHE PATH "Path to Steinberg VST3 SDK root directory")
set(NANOVG_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/nanovg" CACHE PATH "Path to NanoVG source directory")
set(GLAD_DIR   "${CMAKE_CURRENT_SOURCE_DIR}/external/glad" CACHE PATH "Path to glad directory (include/ and src/)")

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
      target_sources(GV3GainPlugin PRIVATE
        "${VST3_SDK_ROOT}/public.sdk/source/main/dllmain.cpp")
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
