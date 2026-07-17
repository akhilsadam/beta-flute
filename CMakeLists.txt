cmake_minimum_required(VERSION 3.20)
project(QGWavetableVST VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Fetch JUCE
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 7.0.12
)
FetchContent_MakeAvailable(JUCE)

# Create VST3/AU plugin
juce_add_plugin(QGWavetableVST
    PRODUCT_NAME "QG Wavetable"
    COMPANY_NAME "Akhil"
    PLUGIN_MANUFACTURER_CODE "AKHM"
    PLUGIN_CODE "QGWV"
    FORMATS VST3 AU
    VST3_CATEGORIES "Instrument" "Synth"
    AU_MAIN_TYPE "kAudioUnitType_MusicDevice"
)

target_sources(QGWavetableVST PRIVATE
    juce/QGWavetableProcessor.cpp
    juce/Main.cpp
)

target_include_directories(QGWavetableVST PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/juce
)

target_link_libraries(QGWavetableVST PRIVATE
    juce::juce_audio_utils
)

# Set binary output directory
set_target_properties(QGWavetableVST PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
