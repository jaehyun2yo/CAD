cmake_minimum_required(VERSION 3.21)
project(YJ-CAD VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# vcpkg toolchain (if available)
if(DEFINED ENV{VCPKG_ROOT})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "Vcpkg toolchain file")
endif()

# Find Qt6
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets OpenGL OpenGLWidgets)

# Find Boost (headers only for Geometry)
find_package(Boost REQUIRED)

# Find Google Test
find_package(GTest CONFIG QUIET)

# OpenGL
find_package(OpenGL REQUIRED)

# pybind11
find_package(pybind11 CONFIG QUIET)

# Clipper2
find_package(Clipper2 CONFIG QUIET)

# libdxfrw
find_package(libdxfrw CONFIG QUIET)
if(NOT libdxfrw_FOUND)
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(LIBDXFRW IMPORTED_TARGET libdxfrw)
    endif()
endif()

# ── Core Library ──────────────────────────────────────────
add_library(yjcad-core STATIC
    # Document Model
    src/core/document/entity.cpp
    src/core/document/layer.cpp
    src/core/document/document.cpp
    src/core/document/command.cpp
    src/core/document/selection.cpp
    # File I/O
    src/core/fileio/dxf_reader.cpp
    src/core/fileio/dxf_writer.cpp
    # Renderer
    src/core/renderer/camera.cpp
    src/core/renderer/gl_renderer.cpp
    # Geometry
    src/core/geometry/operations.cpp
    # UI
    src/ui/cad_canvas.cpp
    src/ui/tool_manager.cpp
    src/ui/domusong_panel.cpp
    src/ui/tools/draw_line_tool.cpp
)

target_include_directories(yjcad-core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(yjcad-core PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::OpenGL
    Qt6::OpenGLWidgets
    OpenGL::GL
    Boost::boost
)

if(libdxfrw_FOUND)
    target_link_libraries(yjcad-core PUBLIC libdxfrw::libdxfrw)
elseif(LIBDXFRW_FOUND)
    target_link_libraries(yjcad-core PUBLIC PkgConfig::LIBDXFRW)
endif()

if(Clipper2_FOUND)
    target_link_libraries(yjcad-core PUBLIC Clipper2::Clipper2)
endif()

# ── Main Application ─────────────────────────────────────
add_executable(yjcad
    src/app/main.cpp
)

target_link_libraries(yjcad PRIVATE
    yjcad-core
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::OpenGL
    Qt6::OpenGLWidgets
    OpenGL::GL
)

# ── Tests ─────────────────────────────────────────────────
if(GTest_FOUND)
    enable_testing()

    add_executable(yjcad-tests
        tests/core/document/test_entity.cpp
        tests/core/document/test_document.cpp
        tests/core/document/test_command.cpp
        tests/core/fileio/test_dxf.cpp
        tests/core/geometry/test_operations.cpp
    )

    target_link_libraries(yjcad-tests PRIVATE
        yjcad-core
        GTest::gtest
        GTest::gtest_main
    )

    include(GoogleTest)
    gtest_discover_tests(yjcad-tests)
endif()

# ── Python Module (pybind11) ────────────────────────────
if(pybind11_FOUND)
    pybind11_add_module(yjcad_core
        src/core/plugin/plugin_api.cpp
    )
    target_link_libraries(yjcad_core PRIVATE yjcad-core)
    target_include_directories(yjcad_core PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    message(STATUS "Python plugin module: yjcad_core ENABLED")
else()
    message(STATUS "Python plugin module: DISABLED (pybind11 not found)")
endif()
