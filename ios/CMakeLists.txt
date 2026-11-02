cmake_minimum_required(VERSION 3.20)
project(fzerogx_ios_port_check LANGUAGES C CXX OBJCXX)

# Deliberately compile-only. No guest functions, disc data, or fake guest stubs.
if(NOT CMAKE_SYSTEM_NAME STREQUAL "iOS")
  message(FATAL_ERROR "Configure with -DCMAKE_SYSTEM_NAME=iOS and the iPhoneOS SDK")
endif()
set(RT "${CMAKE_CURRENT_LIST_DIR}/../gcn-recomp/runtime")
if(NOT EXISTS "${RT}/runtime.h")
  message(FATAL_ERROR "Initialize the pinned gcn-recomp submodule first")
endif()
file(GLOB_RECURSE RUNTIME CONFIGURE_DEPENDS "${RT}/*.cpp")
list(REMOVE_ITEM RUNTIME
  "${RT}/main.cpp" "${RT}/bench_main.cpp" "${RT}/input_map.cpp"
  "${RT}/ffb.cpp" "${RT}/audio_sdl.cpp" "${RT}/audio_stub.cpp"
  "${RT}/record.cpp" "${RT}/gx/render_stub.cpp")
list(REMOVE_ITEM RUNTIME "${RT}/gx/render_gl.cpp")
find_package(Python3 REQUIRED COMPONENTS Interpreter)
execute_process(COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_LIST_DIR}/prepare_renderer.py"
  "${RT}/gx/render_gl.cpp" "${CMAKE_CURRENT_BINARY_DIR}/render_ios.cpp"
  COMMAND_ERROR_IS_FATAL ANY)
add_library(ios_runtime_check OBJECT ${RUNTIME} "${CMAKE_CURRENT_BINARY_DIR}/render_ios.cpp"
  "${RT}/gx/glad_es/src/gles2.c"
  "${CMAKE_CURRENT_LIST_DIR}/../gcn-recomp/visionos/audio_coreaudio.mm"
  "${CMAKE_CURRENT_LIST_DIR}/../src/race.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/../src/vr_fzerogx.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/../src/motion.cpp"
  controller.mm touch_controls.mm)
set_target_properties(ios_runtime_check PROPERTIES
  C_STANDARD 11 CXX_STANDARD 20 CXX_STANDARD_REQUIRED YES
  XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC YES)
target_include_directories(ios_runtime_check PRIVATE
  "${CMAKE_CURRENT_LIST_DIR}" "${RT}" "${RT}/gx" "${RT}/gx/glad_es/include")
target_compile_definitions(ios_runtime_check PRIVATE
  GCN_GL_ES GCN_GL_ES30 GCN_DISC_ID="GFZE01"
  GCN_GAME_TITLE="F-Zero GX" GCN_APP_LABEL="F-Zero GX Recomp"
  GCN_GAME_FPS=60 GCN_DEFAULT_ISO="")
target_compile_options(ios_runtime_check PRIVATE
  -Wall -Wno-unused-function "$<$<COMPILE_LANGUAGE:OBJCXX>:-fobjc-arc>")

# The UIKit launcher is compiled when the cached public ANGLE headers exist.
if(GCN_ANGLE_DIR AND EXISTS "${GCN_ANGLE_DIR}/include/EGL/egl.h")
  target_sources(ios_runtime_check PRIVATE app.mm)
  target_include_directories(ios_runtime_check PRIVATE "${GCN_ANGLE_DIR}/include" "${CMAKE_CURRENT_LIST_DIR}/../gcn-recomp/visionos")
else()
  message(FATAL_ERROR "Public ANGLE headers required to compile-check the launcher")
endif()
