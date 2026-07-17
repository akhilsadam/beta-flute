cmake_minimum_required(VERSION 3.20)
project(QGWavetableVST VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Windows-specific settings
if(MSVC)
    add_compile_options(/MP)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
endif()

# Fetch JUCE
include(FetchContent)
set(FETCHCONTENT_UPDATES_DISCONNECTED ON)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 7.0.12
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE)

# Add JUCE to the project
add_subdirectory(${juce_SOURCE_DIR} ${CMAKE_BINARY_DIR}/JUCE)

# Determine plugin format based on OS
if(WIN32)
    set(PLUGIN_FORMATS VST3)
elseif(APPLE)
    set(PLUGIN_FORMATS VST3 AU)
else()
    set(PLUGIN_FORMATS VST3)
endif()

# Create VST3/AU plugin
juce_add_plugin(QGWavetableVST
    PRODUCT_NAME "QG Wavetable"
    COMPANY_NAME "Akhil"
    PLUGIN_MANUFACTURER_CODE "AKHM"
    PLUGIN_CODE "QGWV"
    FORMATS ${PLUGIN_FORMATS}
    VST3_CATEGORIES "Instrument" "Synth"
    AU_MAIN_TYPE "kAudioUnitType_MusicDevice"
)

# Add sources - do NOT include headers, only .cpp files
target_sources(QGWavetableVST PRIVATE
    juce/QGWavetableProcessor.cpp
    juce/Main.cpp
)

# Include directories
target_include_directories(QGWavetableVST PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/juce
)

# Link JUCE modules
target_link_libraries(QGWavetableVST PRIVATE
    juce::juce_audio_utils
    juce::juce_core
    juce::juce_events
    juce::juce_graphics
    juce::juce_gui_basics
    juce::juce_audio_processors
    juce::juce_audio_basics
)

# Set binary output directory
set_target_properties(QGWavetableVST PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

