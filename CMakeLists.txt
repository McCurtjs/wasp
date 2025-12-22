option(CSPEC_MEMTEST "Enable memory testing defines for spec build" OFF)
option(CSPEC_ASSERT "Enable assert override for spec build" OFF)

if(CMAKE_BUILD_TYPE STREQUAL Spec)
  set(CSPEC_MEMTEST ON)
  set(CSPEC_ASSERT ON)
  set(CMAKE_BUILD_TYPE Debug)
endif()

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  cmake_minimum_required(VERSION 3.6)
  # don't include library paths to linker search paths
  cmake_policy(SET CMP0003 NEW)
  project(Wasp)
endif()

# TODO remove this?
if(!MSVC)
  set(CMAKE_C_STANDARD 23)
  set(CMAKE_C_STANDARD_REQUIRED ON)
endif()

# Function to set up compile options for a target
function(compile_opts target)
  if (MSVC)
    target_compile_options(${target} PRIVATE
      /W4 /WX /std:clatest)
  else()
    target_compile_options(${target} PRIVATE
      -std=c2x -Wall -Wextra -Wpedantic -Werror -Wno-unused-value
    )
  endif()
endfunction()

# Setup for Wasp
add_library(Wasp)
set_property(TARGET Wasp PROPERTY C_STANDARD 23)

target_include_directories(Wasp PUBLIC
  include
  lib/stb
)

target_sources(Wasp PRIVATE
  src/loaders/obj.c
  src/loaders/obj.h
  src/camera.c
  src/draw.c
  src/file.c
  src/game.c
  src/image.c
  src/input.c
  src/material.c
  src/model.c
  src/render_target.c
  src/shader.c
  src/system_events.c
  src/texture.c
  src/wasm.c # rename to bridge.c?
  include/camera.h
  include/draw.h
  include/entity.h
  include/file.h
  include/game.h
  include/gl.h
  include/image.h
  include/input.h
  include/material.h
  include/model.h
  include/render_target.h
  include/shader.h
  include/system_events.h
  include/texture.h
  include/wasm.h
  include/wasp.h
)

compile_opts(Wasp)

# Project source filters for Visual Studio
source_group("Source Files\\Loaders" src/loaders/.*\.c)
source_group("Source Files\\Inline" src/data/.*\.h)
source_group("Header Files\\Loaders" src/loaders/.*\.h)

# Include Galogen library for OpenGL function loading
add_library(Galogen)
set_property(TARGET Galogen PROPERTY C_STANDARD 23)
target_include_directories(Galogen PUBLIC lib/galogen)
target_sources(Galogen PRIVATE
  lib/galogen/galogen.c
  lib/galogen/galogen.h
)

target_link_libraries(Wasp PUBLIC Galogen)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)

  # Add blank C++ project for Natvis support in MSVC
  if(MSVC)
    add_library(Natvis_injector)
    target_sources(Natvis_injector PRIVATE lib/mclib/McLib.natvis)
    set_target_properties(Natvis_injector PROPERTIES LINKER_LANGUAGE CXX)
  endif()

  # Settings for spec build
  if(CSPEC_MEMTEST STREQUAL ON)
    add_executable(Wasp_spec)
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Wasp_spec)

    target_sources(Wasp_spec PRIVATE
      lib/mclib/tst/spec_main.c
      lib/mclib/tst/str_spec.c
      lib/cspec/tst/cspec_spec.c
    )

    add_subdirectory(lib/cspec)

    target_link_libraries(Wasp PUBLIC CSpec)
    target_link_libraries(Wasp_spec PRIVATE Wasp)

  # Settings for demo project
  else()
    add_executable(Wasp_demo)
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Wasp_demo)
    set_target_properties(Wasp_demo PROPERTIES
      VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/web"
    )

    target_sources(Wasp_demo PRIVATE
      src/native/main.c
      demo/demo.c
      demo/level_test.c
      demo/test_behaviors.c
      demo/demo.h
    )

    target_link_libraries(Wasp_demo PRIVATE Wasp Galogen)
    compile_opts(Wasp_demo)
  endif()
endif()

# Bring in McLib
add_subdirectory(lib/mclib)

# Definitions and import for SDL
set(SDL_TESTS OFF)
set(SDL_SHARED OFF)
set(SDL_STATIC ON)
set(ONLY_LIBS ON)

add_subdirectory(lib/sdl)

# Link in libraries
target_link_libraries(Wasp PUBLIC McLib)
target_link_libraries(Wasp PUBLIC SDL3::SDL3)
