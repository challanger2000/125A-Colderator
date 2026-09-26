cmake_minimum_required(VERSION 3.20)

project(Colderator VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(SMTG_ENABLE_VSTGUI_SUPPORT OFF CACHE BOOL "" FORCE)
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

smtg_add_vst3plugin(Colderator
  source/version.h
  source/ids.h
  source/parameters.h
  source/state_format.h
  source/processor.h
  source/processor.cpp
  source/controller.h
  source/controller.cpp
  source/entry.cpp
)

target_link_libraries(Colderator PRIVATE sdk)
smtg_target_configure_version_file(Colderator)

if(SMTG_WIN AND MSVC)
  set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Colderator)
endif()
