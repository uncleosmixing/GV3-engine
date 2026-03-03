# ===========================================================================
# GV3 Engine — CMake Dependency Manifest (Wiring Layer)
# ===========================================================================
# This file wires dependencies based on options defined in Options.cmake.
# - Only actual dependencies found in external/ are linked.
# - All other options have safe placeholders (no target failures).
# ===========================================================================

# ===========================================================================
# Helper Functions
# ===========================================================================

# gv3_require_path(VAR_NAME DESCRIPTION)
# Ensure a path exists, fail if not found.
function(gv3_require_path VAR_NAME DESCRIPTION)
  if (NOT EXISTS "${${VAR_NAME}}")
    message(FATAL_ERROR
      "[GV3] Required ${DESCRIPTION} not found at: ${${VAR_NAME}}\n"
      "Set the CMake variable to the correct path and reconfigure.")
  endif()
endfunction()

# gv3_add_header_only_lib(LIBNAME PUBLIC_HEADERS)
# Create an INTERFACE target for a header-only library.
function(gv3_add_header_only_lib LIBNAME)
  if (NOT TARGET ${LIBNAME})
    add_library(${LIBNAME} INTERFACE)
    message(STATUS "[GV3] Created header-only interface: ${LIBNAME}")
  endif()
endfunction()

# ===========================================================================
# VST3 SDK
# ===========================================================================
set(VST3_SDK_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/external/vst3sdk" CACHE PATH
  "Path to Steinberg VST3 SDK root directory")

# ===========================================================================
# NanoVG (Immediate dependency — present in repo)
# ===========================================================================
set(NANOVG_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/nanovg" CACHE PATH
  "Path to NanoVG source directory")

# ===========================================================================
# GLAD (Immediate dependency — present in repo)
# ===========================================================================
set(GLAD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/glad" CACHE PATH
  "Path to glad directory (include/ and src/)")

# ===========================================================================
# GLFW (git submodule — used by UI sandbox)
# ===========================================================================
if (GV3_BUILD_UI_SANDBOX)
  set(GLFW_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/glfw" CACHE PATH
    "Path to GLFW submodule directory")
  
  if (EXISTS "${GLFW_DIR}/CMakeLists.txt")
    message(STATUS "[GV3] GLFW: Found at ${GLFW_DIR}")
    
    # Disable GLFW extras to keep build lean
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
    
    add_subdirectory("${GLFW_DIR}" "${CMAKE_BINARY_DIR}/glfw")
    message(STATUS "[GV3] GLFW: Successfully integrated")
  else()
    message(WARNING "[GV3] GLFW: Not found at ${GLFW_DIR}")
    message(WARNING "[GV3] GLFW: Run 'git submodule update --init external/glfw'")
    message(STATUS "[GV3] UI Sandbox will be disabled")
    set(GV3_BUILD_UI_SANDBOX OFF PARENT_SCOPE)
  endif()
endif()

# ===========================================================================
# UI Backend Wiring
# ===========================================================================
if (GV3_UI_BACKEND STREQUAL "Current")
  message(STATUS "[GV3] UI Backend: Current (NanoVG)")
  # Current backend uses NanoVG directly; wiring done in main CMakeLists.txt
elseif (GV3_UI_BACKEND STREQUAL "NanoVGXC")
  message(STATUS "[GV3] UI Backend: NanoVGXC (placeholder — not yet integrated)")
else()
  message(FATAL_ERROR "[GV3] Unknown UI backend: ${GV3_UI_BACKEND}")
endif()

# ===========================================================================
# FFT Backend Wiring
# ===========================================================================
if (GV3_FFT_BACKEND STREQUAL "KissFFT")
  message(STATUS "[GV3] FFT Backend: KissFFT (placeholder — not yet integrated)")
  # Placeholder: external/kissfft would be checked here when vendored
elseif (GV3_FFT_BACKEND STREQUAL "PFFFT")
  message(STATUS "[GV3] FFT Backend: PFFFT (placeholder — not yet integrated)")
  # Placeholder: external/pffft would be checked here when vendored
elseif (GV3_FFT_BACKEND STREQUAL "FFTW")
  message(STATUS "[GV3] FFT Backend: FFTW (placeholder — assumes system install)")
  # Placeholder: find_package(FFTW3) would go here
elseif (GV3_FFT_BACKEND STREQUAL "None")
  message(STATUS "[GV3] FFT Backend: None (no FFT support)")
else()
  message(FATAL_ERROR "[GV3] Unknown FFT backend: ${GV3_FFT_BACKEND}")
endif()

# ===========================================================================
# XSIMD Wiring (git submodule integration)
# ===========================================================================
if (GV3_ENABLE_XSIMD)
  # Check if xsimd submodule exists
  set(XSIMD_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/external/xsimd/include")
  
  if (EXISTS "${XSIMD_INCLUDE_DIR}/xsimd/xsimd.hpp")
    message(STATUS "[GV3] XSIMD: Enabled (submodule at external/xsimd)")
    
    # Create INTERFACE target for header-only library
    add_library(gv3_dep_xsimd INTERFACE)
    target_include_directories(gv3_dep_xsimd INTERFACE "${XSIMD_INCLUDE_DIR}")
    target_compile_definitions(gv3_dep_xsimd INTERFACE GV3_SIMD_BACKEND_XSIMD=1)
    
    # Create alias for backwards compatibility
    add_library(gv3::xsimd ALIAS gv3_dep_xsimd)
    
    message(STATUS "[GV3] XSIMD: Successfully integrated (header-only)")
  else()
    message(WARNING "[GV3] XSIMD: Enabled but submodule not found at external/xsimd")
    message(WARNING "[GV3] XSIMD: Run 'git submodule update --init external/xsimd'")
    message(STATUS "[GV3] XSIMD: Falling back to scalar mode")
    
    # Fallback to scalar
    add_library(gv3_dep_xsimd INTERFACE)
    target_compile_definitions(gv3_dep_xsimd INTERFACE GV3_SIMD_BACKEND_XSIMD=0)
    add_library(gv3::xsimd ALIAS gv3_dep_xsimd)
  endif()
else()
  message(STATUS "[GV3] XSIMD: Disabled (scalar fallback)")
  
  # Create empty INTERFACE target with scalar definition
  add_library(gv3_dep_xsimd INTERFACE)
  target_compile_definitions(gv3_dep_xsimd INTERFACE GV3_SIMD_BACKEND_XSIMD=0)
  add_library(gv3::xsimd ALIAS gv3_dep_xsimd)
endif()

# ===========================================================================
# HIIR Wiring
# ===========================================================================
if (GV3_ENABLE_HIIR)
  message(STATUS "[GV3] HIIR: Enabled (placeholder — not yet integrated)")
  # Placeholder: external/hiir would be checked here when vendored
else()
  message(STATUS "[GV3] HIIR: Disabled")
endif()

# ===========================================================================
# EBUR128 Wiring
# ===========================================================================
if (GV3_ENABLE_EBUR128)
  message(STATUS "[GV3] EBUR128: Enabled (placeholder — not yet integrated)")
  # Placeholder: external/libebur128 would be checked here when vendored
else()
  message(STATUS "[GV3] EBUR128: Disabled")
endif()

# ===========================================================================
# SLEEF Wiring
# ===========================================================================
if (GV3_ENABLE_SLEEF)
  message(STATUS "[GV3] SLEEF: Enabled (placeholder — not yet integrated)")
  # Placeholder: external/sleef would be checked here when vendored
else()
  message(STATUS "[GV3] SLEEF: Disabled")
endif()

# ===========================================================================
# Tracy Profiling Wiring
# ===========================================================================
if (GV3_ENABLE_TRACY)
  message(STATUS "[GV3] Tracy: Enabled (placeholder — not yet integrated)")
  # Placeholder: external/tracy would be checked here when vendored
else()
  message(STATUS "[GV3] Tracy: Disabled")
endif()

# ===========================================================================
# Allocator Wiring
# ===========================================================================
if (GV3_ALLOCATOR STREQUAL "System")
  message(STATUS "[GV3] Allocator: System (malloc/free)")
elseif (GV3_ALLOCATOR STREQUAL "mimalloc")
  message(STATUS "[GV3] Allocator: mimalloc (placeholder — not yet integrated)")
  # Placeholder: external/mimalloc would be checked here when vendored
elseif (GV3_ALLOCATOR STREQUAL "rpmalloc")
  message(STATUS "[GV3] Allocator: rpmalloc (placeholder — not yet integrated)")
  # Placeholder: external/rpmalloc would be checked here when vendored
else()
  message(FATAL_ERROR "[GV3] Unknown allocator: ${GV3_ALLOCATOR}")
endif()

# ===========================================================================
# Moodycamel Queue Wiring
# ===========================================================================
if (GV3_ENABLE_MOODYCAMEL)
  message(STATUS "[GV3] Moodycamel: Enabled (placeholder — not yet integrated)")
  # Placeholder: external/concurrentqueue would be checked here when vendored
else()
  message(STATUS "[GV3] Moodycamel: Disabled")
endif()

# ===========================================================================
# Resampler Wiring
# ===========================================================================
if (GV3_RESAMPLER STREQUAL "libsamplerate")
  message(STATUS "[GV3] Resampler: libsamplerate (placeholder — not yet integrated)")
  # Placeholder: external/libsamplerate would be checked here when vendored
elseif (GV3_RESAMPLER STREQUAL "r8brain")
  message(STATUS "[GV3] Resampler: r8brain (placeholder — not yet integrated)")
  # Placeholder: external/r8brain would be checked here when vendored
elseif (GV3_RESAMPLER STREQUAL "None")
  message(STATUS "[GV3] Resampler: None (no resampling support)")
else()
  message(FATAL_ERROR "[GV3] Unknown resampler: ${GV3_RESAMPLER}")
endif()

# ===========================================================================
# Audio I/O Wiring
# ===========================================================================
if (GV3_AUDIO_IO STREQUAL "RtAudio")
  message(STATUS "[GV3] Audio I/O: RtAudio (placeholder — not yet integrated)")
  # Placeholder: external/rtaudio would be checked here when vendored
elseif (GV3_AUDIO_IO STREQUAL "PortAudio")
  message(STATUS "[GV3] Audio I/O: PortAudio (placeholder — not yet integrated)")
  # Placeholder: external/portaudio would be checked here when vendored
elseif (GV3_AUDIO_IO STREQUAL "None")
  message(STATUS "[GV3] Audio I/O: None (VST3 host only)")
else()
  message(FATAL_ERROR "[GV3] Unknown audio I/O backend: ${GV3_AUDIO_IO}")
endif()

message(STATUS "[GV3] Dependency manifest loaded successfully")
