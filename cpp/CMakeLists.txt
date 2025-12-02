cmake_minimum_required(VERSION 3.20)
project(AutoPatcher VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Windows-specific settings
if(WIN32)
    set(CMAKE_WIN32_EXECUTABLE ON)
    add_definitions(-DUNICODE -D_UNICODE -DWIN32_LEAN_AND_MEAN -DNOMINMAX)
    # Force UTF-8 source and execution character set for MSVC
    add_compile_options(/utf-8)
    
    # Use static runtime library (MT/MTd instead of MD/MDd)
    # This removes dependency on VCRUNTIME*.dll and MSVCP*.dll
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# ==============================================================================
# Third-party libraries
# ==============================================================================

# JSON library (header-only)
include(FetchContent)
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(nlohmann_json)

# zlib for GRF decompression - always build statically
# Don't use find_package to avoid dynamic linking issues
FetchContent_Declare(
    zlib
    GIT_REPOSITORY https://github.com/madler/zlib.git
    GIT_TAG v1.3.1
)
set(ZLIB_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SKIP_INSTALL_ALL ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(zlib)
set(ZLIB_INCLUDE_DIRS ${zlib_SOURCE_DIR} ${zlib_BINARY_DIR})
set(ZLIB_LIBRARIES zlibstatic)

# ==============================================================================
# Core Library
# ==============================================================================

add_library(autopatch_core STATIC
    src/core/config.cpp
    src/core/config.h
    src/core/grf.cpp
    src/core/grf.h
    src/core/thor.cpp
    src/core/thor.h
    src/core/http.cpp
    src/core/http.h
    src/core/patcher.cpp
    src/core/patcher.h
    src/core/resources.cpp
    src/core/resources.h
    src/core/utils.cpp
    src/core/utils.h
)

target_include_directories(autopatch_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${ZLIB_INCLUDE_DIRS}
)

target_link_libraries(autopatch_core PUBLIC
    nlohmann_json::nlohmann_json
    ${ZLIB_LIBRARIES}
    winhttp
    shlwapi
)

# ==============================================================================
# Client Application (Patcher)
# ==============================================================================

# Build with MSHTML (IE WebBrowser control) for HTML mode
# MSHTML is built into Windows and doesn't create extra files
add_executable(AutoPatcher WIN32
    src/client/main.cpp
    src/client/window.cpp
    src/client/window.h
    src/client/ui.cpp
    src/client/ui.h
    src/client/skin.cpp
    src/client/skin.h
    src/client/mshtml_window.cpp
    src/client/mshtml_window.h
    src/client/embedded_browser.cpp
    src/client/embedded_browser.h
    src/client/resources.rc
)

target_link_libraries(AutoPatcher PRIVATE
    autopatch_core
    comctl32
    gdiplus
    uxtheme
    ole32
    oleaut32
)

# Set subsystem to Windows
set_target_properties(AutoPatcher PROPERTIES
    WIN32_EXECUTABLE TRUE
    VS_DPI_AWARE "PerMonitor"
    LINK_FLAGS "/MANIFEST:NO"
)

# ==============================================================================
# Builder Application
# ==============================================================================

add_executable(AutoPatchBuilder WIN32
    src/builder/main.cpp
    src/builder/builder_window.cpp
    src/builder/builder_window.h
    src/builder/modern_ui.cpp
    src/builder/modern_ui.h
    src/builder/embedder.cpp
    src/builder/embedder.h
    src/builder/resources.rc
)

target_link_libraries(AutoPatchBuilder PRIVATE
    autopatch_core
    comctl32
    gdiplus
    comdlg32
    uxtheme
    msimg32
)

set_target_properties(AutoPatchBuilder PROPERTIES
    WIN32_EXECUTABLE TRUE
    LINK_FLAGS "/MANIFEST:NO"
)

# ==============================================================================
# Install
# ==============================================================================

install(TARGETS AutoPatcher AutoPatchBuilder
    RUNTIME DESTINATION bin
)
