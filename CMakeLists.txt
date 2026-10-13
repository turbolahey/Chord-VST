cmake_minimum_required(VERSION 3.22)

project(Chordwright VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Apple Silicon and Intel Macs in one plugin file, macOS 11 or newer.
if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version")
    if(NOT CMAKE_OSX_ARCHITECTURES)
        set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Mac architectures" FORCE)
    endif()
endif()

# JUCE is downloaded automatically. Point JUCE_DIR at a local copy to skip the download.
if(DEFINED JUCE_DIR)
    add_subdirectory(${JUCE_DIR} JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 9.0.4
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

if(APPLE)
    set(CHORDWRIGHT_FORMATS VST3 AU)
else()
    set(CHORDWRIGHT_FORMATS VST3)
endif()

juce_add_plugin(Chordwright
    PRODUCT_NAME "Chordwright"
    COMPANY_NAME "Kristian"
    BUNDLE_ID "com.kristian.chordwright"
    PLUGIN_MANUFACTURER_CODE Krsn
    PLUGIN_CODE Cwrt
    FORMATS ${CHORDWRIGHT_FORMATS}
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES Instrument Generator
    AU_MAIN_TYPE kAudioUnitType_MusicDevice
    DESCRIPTION "Chord progressions and humanized chord playing")

target_sources(Chordwright PRIVATE
    Source/Theory.cpp
    Source/ProgressionLibrary.cpp
    Source/Engine.cpp
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_compile_definitions(Chordwright PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(Chordwright
    PRIVATE
        juce::juce_audio_processors
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# Unit tests for the chord engine: cmake -DCHORDWRIGHT_TESTS=ON
option(CHORDWRIGHT_TESTS "Build the engine unit tests" OFF)
if(CHORDWRIGHT_TESTS)
    add_executable(engine_tests
        tests/EngineTests.cpp
        Source/Theory.cpp
        Source/ProgressionLibrary.cpp
        Source/Engine.cpp)
    enable_testing()
    add_test(NAME engine_tests COMMAND engine_tests)
endif()
