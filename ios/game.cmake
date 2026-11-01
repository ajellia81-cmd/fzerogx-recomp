if(NOT GCN_HAVE_GAME_ISO)
  message(FATAL_ERROR "The complete iOS game requires a user-supplied disc image")
endif()
set(IOS_DIR "${CMAKE_CURRENT_LIST_DIR}")
set(GCN_ANGLE_DIR "${CMAKE_BINARY_DIR}/angle" CACHE PATH "iPhoneOS ANGLE build")
if(NOT EXISTS "${GCN_ANGLE_DIR}/libANGLE-shared.dylib")
  message(FATAL_ERROR "Build ANGLE for iphoneos before configuring the app")
endif()
# A local renderer variant stretches the final image to the native landscape
# drawable. The upstream desktop renderer keeps its original 4:3 behavior.
file(READ "${GCN_RT}/gx/render_gl.cpp" IOS_RENDER)
set(IOS_LETTERBOX "int vw = w, vh = w * 3 / 4;\n    if (vh > h) { vh = h; vw = h * 4 / 3; }")
string(FIND "${IOS_RENDER}" "${IOS_LETTERBOX}" IOS_BOX_AT)
if(IOS_BOX_AT EQUAL -1)
  message(FATAL_ERROR "Renderer presentation layout changed; review the fullscreen patch")
endif()
string(REPLACE "${IOS_LETTERBOX}" "int vw = w, vh = h;" IOS_RENDER "${IOS_RENDER}")
file(WRITE "${CMAKE_BINARY_DIR}/render_ios.cpp" "${IOS_RENDER}")
set(IOS_CORE ${GCN_CORE_SOURCES})
list(REMOVE_ITEM IOS_CORE "${GCN_RT}/audio_stub.cpp")
set(IOS_RENDER_SOURCES ${GCN_RENDER_SOURCES})
list(REMOVE_ITEM IOS_RENDER_SOURCES "${GCN_RT}/gx/render_gl.cpp")
set(IOS_SOURCES ${IOS_CORE} ${GCN_AUDIO_SOURCES} ${IOS_RENDER_SOURCES}
  "${CMAKE_BINARY_DIR}/render_ios.cpp" ${GCN_GLAD_SOURCE})
list(APPEND IOS_SOURCES
  "${CMAKE_SOURCE_DIR}/gcn-recomp/visionos/angle.mm"
  "${CMAKE_SOURCE_DIR}/gcn-recomp/visionos/audio_coreaudio.mm"
  "${CMAKE_SOURCE_DIR}/src/race.cpp" "${CMAKE_SOURCE_DIR}/src/vr_fzerogx.cpp"
  "${CMAKE_SOURCE_DIR}/src/motion.cpp" "${CMAKE_BINARY_DIR}/shader_keys.cpp"
  "${IOS_DIR}/controller.mm" "${IOS_DIR}/app.mm")
add_executable(fzerogx_ios MACOSX_BUNDLE ${IOS_SOURCES})
set_target_properties(fzerogx_ios PROPERTIES
  C_STANDARD 11 CXX_STANDARD 20 CXX_STANDARD_REQUIRED YES
  OUTPUT_NAME FZeroGX
  MACOSX_BUNDLE_INFO_PLIST "${IOS_DIR}/Info.plist.in"
  MACOSX_BUNDLE_GUI_IDENTIFIER com.ajellia81.fzerogx
  XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER com.ajellia81.fzerogx
  XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2"
  XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET 17.0
  XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED NO
  XCODE_ATTRIBUTE_ENABLE_BITCODE NO
  XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC YES
  XCODE_ATTRIBUTE_LD_RUNPATH_SEARCH_PATHS "@executable_path/Frameworks"
  XCODE_EMBED_FRAMEWORKS "${GCN_ANGLE_DIR}/libANGLE-shared.dylib"
  XCODE_EMBED_FRAMEWORKS_CODE_SIGN_ON_COPY NO)
target_include_directories(fzerogx_ios PRIVATE "${GCN_RT}" "${GCN_GEN_DIR}"
  "${GCN_RT}/gx" "${GCN_GLAD_INCLUDE_DIR}" "${GCN_ANGLE_DIR}/include"
  "${CMAKE_SOURCE_DIR}/gcn-recomp/visionos")
target_compile_definitions(fzerogx_ios PRIVATE GCN_GL_ES GCN_GL_ES30
  GCN_DISC_ID="GFZE01" GCN_GAME_TITLE="F-Zero GX" GCN_APP_LABEL="F-Zero GX Recomp"
  GCN_GAME_FPS=60 GCN_DEFAULT_ISO="")
target_compile_options(fzerogx_ios PRIVATE -Wall -Wno-unused-function)
target_link_libraries(fzerogx_ios PRIVATE game_recomp
  "-framework UIKit" "-framework Foundation" "-framework QuartzCore"
  "-framework Metal" "-framework GameController" "-framework AudioToolbox"
  "-framework AVFAudio" "-framework UniformTypeIdentifiers")
