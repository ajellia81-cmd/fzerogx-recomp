if(NOT GCN_HAVE_GAME_ISO)
  message(FATAL_ERROR "The complete iOS game requires a user-supplied disc image")
endif()
set(IOS_DIR "${CMAKE_CURRENT_LIST_DIR}")
set(GCN_ANGLE_DIR "${CMAKE_BINARY_DIR}/angle" CACHE PATH "iPhoneOS ANGLE build")
if(NOT EXISTS "${GCN_ANGLE_DIR}/libANGLE-shared.dylib")
  message(FATAL_ERROR "Build ANGLE for iphoneos before configuring the app")
endif()
# Generate only the iOS variant; upstream desktop/VR behavior is preserved.
find_package(Python3 REQUIRED COMPONENTS Interpreter)
execute_process(COMMAND "${Python3_EXECUTABLE}" "${IOS_DIR}/prepare_runtime.py"
  "${CMAKE_SOURCE_DIR}/gcn-recomp" "${CMAKE_BINARY_DIR}" COMMAND_ERROR_IS_FATAL ANY)
execute_process(COMMAND "${Python3_EXECUTABLE}" "${IOS_DIR}/prepare_renderer.py"
  "${GCN_RT}/gx/render_gl.cpp" "${CMAKE_BINARY_DIR}/render_ios.cpp"
  COMMAND_ERROR_IS_FATAL ANY)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
  "${IOS_DIR}/prepare_renderer.py" "${IOS_DIR}/render_extensions.inc"
  "${IOS_DIR}/prepare_runtime.py")
set(IOS_CORE ${GCN_CORE_SOURCES})
list(REMOVE_ITEM IOS_CORE "${GCN_RT}/audio_stub.cpp")
list(REMOVE_ITEM IOS_CORE "${GCN_RT}/cpu.cpp" "${GCN_RT}/threads.cpp" "${GCN_RT}/platform.cpp")
list(APPEND IOS_CORE "${CMAKE_BINARY_DIR}/cpu_ios.cpp" "${CMAKE_BINARY_DIR}/threads_ios.cpp" "${CMAKE_BINARY_DIR}/platform_ios.cpp")
set(IOS_RENDER_SOURCES ${GCN_RENDER_SOURCES})
list(REMOVE_ITEM IOS_RENDER_SOURCES "${GCN_RT}/gx/render_gl.cpp")
set(IOS_SOURCES ${IOS_CORE} ${GCN_AUDIO_SOURCES} ${IOS_RENDER_SOURCES}
  "${CMAKE_BINARY_DIR}/render_ios.cpp" ${GCN_GLAD_SOURCE})
list(APPEND IOS_SOURCES
  "${CMAKE_SOURCE_DIR}/gcn-recomp/visionos/angle.mm"
  "${CMAKE_BINARY_DIR}/audio_ios.mm"
  "${CMAKE_SOURCE_DIR}/src/race.cpp" "${CMAKE_SOURCE_DIR}/src/vr_fzerogx.cpp"
  "${CMAKE_SOURCE_DIR}/src/motion.cpp" "${CMAKE_BINARY_DIR}/ios_shader_keys.cpp"
  "${IOS_DIR}/rumble.mm" "${IOS_DIR}/touch_controls.mm" "${IOS_DIR}/controller.mm" "${IOS_DIR}/app.mm")
add_custom_command(OUTPUT "${CMAKE_BINARY_DIR}/ios_shader_keys.cpp"
  COMMAND ${CMAKE_COMMAND} "-DIN=${CMAKE_SOURCE_DIR}/src/shader_keys.bin"
    "-DOUT=${CMAKE_BINARY_DIR}/ios_shader_keys.cpp"
    -P "${CMAKE_SOURCE_DIR}/gcn-recomp/tools/embed_shader_keys.cmake"
  DEPENDS "${CMAKE_SOURCE_DIR}/src/shader_keys.bin"
    "${CMAKE_SOURCE_DIR}/gcn-recomp/tools/embed_shader_keys.cmake"
  COMMENT "Embedding iOS shader keys independently of the host benchmark")
if(NOT EXISTS "${CMAKE_BINARY_DIR}/Assets.xcassets/AppIcon.appiconset/Contents.json")
  message(FATAL_ERROR "Prepare the user-approved app icon before building the IPA")
endif()
set(IOS_ASSETS "${CMAKE_BINARY_DIR}/Assets.xcassets")
set_source_files_properties("${IOS_ASSETS}" PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
add_executable(fzerogx_ios MACOSX_BUNDLE ${IOS_SOURCES} "${IOS_ASSETS}")
set_target_properties(fzerogx_ios PROPERTIES
  C_STANDARD 11 CXX_STANDARD 20 CXX_STANDARD_REQUIRED YES
  OUTPUT_NAME FZeroGX
  MACOSX_BUNDLE_INFO_PLIST "${IOS_DIR}/Info.plist.in"
  MACOSX_BUNDLE_GUI_IDENTIFIER com.ajellia81.fzerogx
  XCODE_ATTRIBUTE_ASSETCATALOG_COMPILER_APPICON_NAME AppIcon
  XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER com.ajellia81.fzerogx
  XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2"
  XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET 17.0
  XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED NO
  XCODE_ATTRIBUTE_ENABLE_BITCODE NO
  XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC YES
  XCODE_ATTRIBUTE_LD_RUNPATH_SEARCH_PATHS "@executable_path/Frameworks"
  XCODE_EMBED_FRAMEWORKS "${GCN_ANGLE_DIR}/libANGLE-shared.dylib"
  XCODE_EMBED_FRAMEWORKS_CODE_SIGN_ON_COPY NO)
target_include_directories(fzerogx_ios PRIVATE "${IOS_DIR}" "${GCN_RT}" "${GCN_GEN_DIR}"
  "${GCN_RT}/gx" "${GCN_GLAD_INCLUDE_DIR}" "${GCN_ANGLE_DIR}/include"
  "${CMAKE_SOURCE_DIR}/gcn-recomp/visionos")
target_compile_definitions(fzerogx_ios PRIVATE GCN_GL_ES GCN_GL_ES30 FZGX_IOS_FRONTEND
  GCN_DISC_ID="GFZE01" GCN_GAME_TITLE="F-Zero GX" GCN_APP_LABEL="F-Zero GX Recomp"
  GCN_GAME_FPS=60 GCN_DEFAULT_ISO="")
target_compile_options(fzerogx_ios PRIVATE -Wall -Wno-unused-function)
target_link_libraries(fzerogx_ios PRIVATE game_recomp
  "-framework UIKit" "-framework Foundation" "-framework QuartzCore" "-framework CoreGraphics" "-framework CoreHaptics"
  "-framework Metal" "-framework GameController" "-framework AudioToolbox"
  "-framework AVFAudio" "-framework UniformTypeIdentifiers")
