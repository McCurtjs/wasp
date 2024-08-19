option(CSPEC_MEMTEST "Enable memory testing defines for spec build" OFF)

if(CMAKE_BUILD_TYPE STREQUAL Spec)
  set(CSPEC_MEMTEST ON)
  set(CMAKE_BUILD_TYPE Debug)
endif()

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  cmake_minimum_required(VERSION 3.6)
  cmake_policy(SET CMP0003 NEW)
  project(Wasp)
endif()

if(!MSVC)
  set(CMAKE_C_STANDARD 17)
  set(CMAKE_C_STANDARD_REQUIRED ON)
endif()

add_library(Wasp)
target_include_directories(Wasp PUBLIC ./include ./lib/galogen)
target_sources(Wasp PRIVATE
  src/camera.c
  src/draw.c
  src/file.c
  src/image.c
  src/model.c
  src/shader.c
  src/texture.c
  src/wasm.c # rename to bridge.c?
)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
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
  else()
    add_executable(Wasp_demo)
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Wasp_demo)

    target_sources(Wasp_demo PRIVATE
      demo/demo.c
    )

    target_link_libraries(Wasp_demo PRIVATE Wasp)
  endif()
endif()

# Bring in McLib
add_subdirectory(lib/mclib)

# Definitions and import for SDL
set(SDL_TEST_LIBRARY OFF)
set(BUILD_SHARED_LIBS OFF)
set(ONLY_LIBS ON)

add_subdirectory(lib/sdl)

# Link in libraries
target_link_libraries(Wasp PUBLIC McLib)
target_link_libraries(Wasp PUBLIC SDL3::SDL3)
