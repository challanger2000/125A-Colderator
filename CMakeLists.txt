cmake_minimum_required(VERSION 3.20)

project(Colderator VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(SMTG_ENABLE_VSTGUI_SUPPORT ON CACHE BOOL "" FORCE)
set(SMTG_ENABLE_VST3_PLUGIN_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SMTG_ENABLE_VST3_HOSTING_EXAMPLES OFF CACHE BOOL "" FORCE)

include(FetchContent)
FetchContent_Declare(
  vst3sdk
  GIT_REPOSITORY https://github.com/steinbergmedia/vst3sdk.git
  GIT_TAG 3cdf9ca5d1f5b1b21e0a86832aa4abe55607bd96
  GIT_SHALLOW FALSE
)
FetchContent_MakeAvailable(vst3sdk)

smtg_enable_vst3_sdk()

file(READ "${CMAKE_CURRENT_SOURCE_DIR}/resource/125A_Logo_Master_FINAL.svg" COLDERATOR_BRANDING_SVG)
string(REGEX MATCHALL "<path d=\\\"[^\\\"]+\\\" fill=\\\"#[0-9A-Fa-f]+\\\"/>" COLDERATOR_BRANDING_PATH_TAGS "${COLDERATOR_BRANDING_SVG}")
list(LENGTH COLDERATOR_BRANDING_PATH_TAGS COLDERATOR_BRANDING_PATH_COUNT)
if(NOT COLDERATOR_BRANDING_PATH_COUNT EQUAL 30)
 message(FATAL_ERROR "125A branding master must contain exactly 30 vector paths; found ${COLDERATOR_BRANDING_PATH_COUNT}.")
endif()
set(COLDERATOR_BRANDING_PATHS "")
foreach(COLDERATOR_BRANDING_TAG IN LISTS COLDERATOR_BRANDING_PATH_TAGS)
 if(NOT COLDERATOR_BRANDING_TAG MATCHES "^<path d=\\\"([^\\\"]+)\\\" fill=\\\"([^\\\"]+)\\\"/>$")
  message(FATAL_ERROR "Could not parse 125A branding path tag.")
 endif()
 set(COLDERATOR_BRANDING_D "${CMAKE_MATCH_1}")
 set(COLDERATOR_BRANDING_FILL "${CMAKE_MATCH_2}")
 if(COLDERATOR_BRANDING_FILL STREQUAL "#D71920")
  set(COLDERATOR_BRANDING_RED "true")
 elseif(COLDERATOR_BRANDING_FILL STREQUAL "#D9D9D9")
  set(COLDERATOR_BRANDING_RED "false")
 else()
  message(FATAL_ERROR "Unexpected 125A branding fill: ${COLDERATOR_BRANDING_FILL}")
 endif()
 string(APPEND COLDERATOR_BRANDING_PATHS "    {\"${COLDERATOR_BRANDING_D}\", ${COLDERATOR_BRANDING_RED}},\n")
endforeach()
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/source/branding_master.h.in" "${CMAKE_BINARY_DIR}/branding_master.h" @ONLY)

smtg_add_vst3plugin(Colderator
  source/version.h
  source/ids.h
  source/parameters.h
  source/state_format.h
  source/processor.h
  source/processor.cpp
  source/controller.h
  source/controller.cpp
  source/ColderatorControls.h
  source/ColderatorControls.cpp
  source/entry.cpp
  resource/colderator.uidesc
)

target_include_directories(Colderator PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/generated
  ${CMAKE_BINARY_DIR}
)
target_link_libraries(Colderator PRIVATE sdk vstgui_support)

smtg_target_add_plugin_resources(Colderator
  RESOURCES
    resource/colderator.uidesc
    resource/Colderator_Faceplate_800x560.png
    resource/Colderator_Faceplate_1200x840.png
    resource/colderator_icicle_broken.png
    resource/Colderator_Knob_120x120_101f.png
    resource/Colderator_Knob_88x88_101f.png
    resource/Colderator_Knob_72x72_101f.png
)
smtg_target_configure_version_file(Colderator)


add_executable(ColderatorMeasurements
  tests/dsp_core_measurements.cpp
  source/processor.cpp
  ${vst3sdk_SOURCE_DIR}/public.sdk/source/common/memorystream.cpp
  ${vst3sdk_SOURCE_DIR}/public.sdk/source/vst/hosting/parameterchanges.cpp
)

target_compile_definitions(ColderatorMeasurements PRIVATE COLDERATOR_TESTING=1)
target_include_directories(ColderatorMeasurements PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/source
  ${CMAKE_CURRENT_SOURCE_DIR}/generated
)
target_link_libraries(ColderatorMeasurements PRIVATE sdk)

enable_testing()
add_test(NAME ColderatorMeasurements COMMAND ColderatorMeasurements)

add_executable(ColderatorRealFixtureMeasurements
  tests/real_fixture_measurements.cpp
  source/processor.cpp
)
target_compile_definitions(ColderatorRealFixtureMeasurements PRIVATE COLDERATOR_TESTING=1)
target_include_directories(ColderatorRealFixtureMeasurements PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/source
  ${CMAKE_CURRENT_SOURCE_DIR}/generated
)
target_link_libraries(ColderatorRealFixtureMeasurements PRIVATE sdk)
add_test(
  NAME ColderatorRealFixtureMeasurements
  COMMAND ColderatorRealFixtureMeasurements ${CMAKE_BINARY_DIR}/real-fixtures
)

add_executable(ColderatorListeningRender
  tests/listening_render.cpp
  source/processor.cpp
)

target_compile_definitions(ColderatorListeningRender PRIVATE COLDERATOR_TESTING=1)
target_include_directories(ColderatorListeningRender PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/source
  ${CMAKE_CURRENT_SOURCE_DIR}/generated
)
target_link_libraries(ColderatorListeningRender PRIVATE sdk)

if(SMTG_WIN AND MSVC)
  set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Colderator)
endif()
