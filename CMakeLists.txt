CMake
cmake_minimum_required(VERSION 3.22)

project(AdvancedLooper VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Scarica automaticamente JUCE da GitHub se non presente
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.0 # Versione di JUCE
)
FetchContent_MakeAvailable(JUCE)

# Configura il plugin JUCE
juce_add_plugin(AdvancedLooper
    COMPANY_NAME "MyAudioDev"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE Loop
    PLUGIN_CODE Aflp
    FORMATS VST3 Standalone # Genera sia VST3 che EXE Standalone
    PRODUCT_NAME "Advanced Dub Looper"
)

# Aggiunge i sorgenti presi dalla cartella Source
target_sources(AdvancedLooper PRIVATE
    Source/PluginProcessor.h
    Source/PluginProcessor.cpp
    Source/PluginEditor.h
    Source/PluginEditor.cpp
)

# Moduli JUCE necessari per l'audio, la GUI e il DSP
target_link_libraries(AdvancedLooper PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_dsp
    juce::juce_gui_extra
    juce::juce_gui_basics
    juce::juce_graphics
    juce::juce_events
    juce::juce_core
)

# Attiva la compilazione dei moduli DSP di JUCE
target_compile_definitions(AdvancedLooper PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)
