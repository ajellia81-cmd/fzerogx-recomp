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
add_library(ios_runtime_check OBJECT ${RUNTIME}
  "${RT}/gx/glad_es/src/gles2.c"
  "${CMAKE_CURRENT_LIST_DIR}/../gcn-recomp/visionos/audio_coreaudio.mm"
  controller.mm)
set_target_properties(ios_runtime_check PROPERTIES
  C_STANDARD 11 CXX_STANDARD 20 CXX_STANDARD_REQUIRED YES)
target_include_directories(ios_runtime_check PRIVATE
  "${RT}" "${RT}/gx/glad_es/include")
target_compile_definitions(ios_runtime_check PRIVATE
  GCN_GL_ES GCN_GL_ES30 GCN_DISC_ID="GFZE01"
  GCN_GAME_TITLE="F-Zero GX" GCN_APP_LABEL="F-Zero GX Recomp"
  GCN_GAME_FPS=60 GCN_DEFAULT_ISO="")
target_compile_options(ios_runtime_check PRIVATE
  -Wall -Wno-unused-function "$<$<COMPILE_LANGUAGE:OBJCXX>:-fobjc-arc>")
