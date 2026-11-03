cmake_minimum_required(VERSION 3.20)
project(fzerogx C CXX)
if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
  enable_language(OBJCXX)
  # Reuse the portable core; the desktop SDL frontend is not part of this app.
  set(GCN_BENCH_ONLY ON CACHE BOOL "" FORCE)
  set(GCN_GL_ES ON CACHE BOOL "" FORCE)
endif()
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

# Everything that is not specific to F-Zero GX lives in gcn-recomp (a submodule): the
# recompiler, the GameCube runtime, the renderer and the frontends. See its README.
add_subdirectory(gcn-recomp)
gcn_enable_languages()

option(FZGX_IOS_LOGO_PROBE "Run the iOS logo timing policy in the private host harness" OFF)
set(FZGX_TABLES "${CMAKE_SOURCE_DIR}/recomp")
if(CMAKE_SYSTEM_NAME STREQUAL "iOS" OR FZGX_IOS_LOGO_PROBE)
  find_package(Python3 REQUIRED COMPONENTS Interpreter)
  set(FZGX_TABLES "${CMAKE_BINARY_DIR}/ios-recomp")
  execute_process(COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/ios/prepare_tables.py"
    "${CMAKE_SOURCE_DIR}/recomp" "${FZGX_TABLES}" COMMAND_ERROR_IS_FATAL ANY)
  set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/ios/prepare_tables.py" "${CMAKE_SOURCE_DIR}/recomp/patches.txt")
endif()

gcn_add_game(
  NAME fzerogx
  TITLE "F-Zero GX"
  LABEL "F-Zero GX Recomp"
  DISC_ID GFZE01
  DOL_SHA1 421c88106697d3275a3fc26fb7a01bf6d816b271
  SYMBOLS ${CMAKE_SOURCE_DIR}/analysis/symbols.txt
  TABLES ${FZGX_TABLES}
  FPS 60
  # The shader keys a play-through uses, compiled on a fresh install before the game starts:
  # made by gcn-recomp/tools/shader_keys from the Vision Pro's cache (docs/dev/vr.md).
  SHADER_KEYS ${CMAKE_SOURCE_DIR}/src/shader_keys.bin
  # Most of the game is in relocatable modules: the mode modules on the disc, and the main
  # one, fze.main, which the game assembles in memory at boot; the first build takes it
  # from the running game (gcn-recomp/docs/rels.md).
  RELS fze.car_colchg.rel fze.customize.rel fze.interview.rel fze.movie.rel fze.movie_module.rel
       fze.option.rel fze.pilotpoint.rel fze.profile.rel fze.replay.rel fze.sample.rel fze.sel.rel
       fze.story.rel fze.title.rel fze.winning.rel
  REL_RUNTIME fze.main
  REL_SYMBOLS ${CMAKE_SOURCE_DIR}/analysis/rels
  # The game's half of the headset and the motion rig: when to be in stereo, its own
  # first-person camera, and the machine's motion (docs/dev/vr.md).
  SOURCES ${CMAKE_SOURCE_DIR}/src/race.cpp ${CMAKE_SOURCE_DIR}/src/vr_fzerogx.cpp ${CMAKE_SOURCE_DIR}/src/motion.cpp
  # The headset app and its harness, on an Android build.
  ANDROID_PACKAGE com.ethannicholas.fzerogx
  # The Vision Pro app, on a visionOS build: see build-visionos.sh.
  VISIONOS_BUNDLE_ID com.ethannicholas.fzerogx)

if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
  include(ios/game.cmake)
endif()

option(FZGX_IOS_INPUT_PROBE "Validate iOS scene/motion inputs in the host harness" OFF)
if(FZGX_IOS_INPUT_PROBE AND NOT CMAKE_SYSTEM_NAME STREQUAL "iOS")
  find_package(Python3 REQUIRED COMPONENTS Interpreter)
  execute_process(COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/ios/prepare_probe.py"
    "${CMAKE_SOURCE_DIR}/gcn-recomp/runtime/bench_main.cpp" "${CMAKE_BINARY_DIR}/ios_probe_main.cpp"
    COMMAND_ERROR_IS_FATAL ANY)
  get_target_property(_sources fzerogx_bench SOURCES)
  list(REMOVE_ITEM _sources "${CMAKE_SOURCE_DIR}/gcn-recomp/runtime/bench_main.cpp")
  set_property(TARGET fzerogx_bench PROPERTY SOURCES "${_sources}")
  target_sources(fzerogx_bench PRIVATE "${CMAKE_BINARY_DIR}/ios_probe_main.cpp")
  target_include_directories(fzerogx_bench PRIVATE "${CMAKE_SOURCE_DIR}/ios")
  target_compile_definitions(fzerogx_bench PRIVATE FZGX_IOS_INPUT_PROBE)
endif()

if(FZGX_IOS_LOGO_PROBE AND NOT CMAKE_SYSTEM_NAME STREQUAL "iOS")
  find_package(Python3 REQUIRED COMPONENTS Interpreter)
  execute_process(COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/ios/prepare_runtime.py"
    "${CMAKE_SOURCE_DIR}/gcn-recomp" "${CMAKE_BINARY_DIR}" COMMAND_ERROR_IS_FATAL ANY)
  get_target_property(_logo_sources gcn_core SOURCES)
  list(REMOVE_ITEM _logo_sources "${GCN_RT}/cpu.cpp")
  set_property(TARGET gcn_core PROPERTY SOURCES "${_logo_sources}")
  target_sources(gcn_core PRIVATE "${CMAKE_BINARY_DIR}/cpu_ios.cpp")
  target_include_directories(gcn_core PRIVATE "${CMAKE_SOURCE_DIR}/ios")
endif()
