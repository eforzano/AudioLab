cmake_minimum_required(VERSION 3.22)

project(AudioLab VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ============================================================
# JUCE — either find an installed copy or fetch it from GitHub
# ============================================================
# Option A (recommended): clone JUCE next to your project and point to it:
#   git clone https://github.com/juce-framework/JUCE.git
# Then this line finds it automatically if it's a sibling folder.
#
# Option B: set JUCE_PATH on the cmake command line:
#   cmake -DJUCE_PATH=/path/to/JUCE ..

if(DEFINED JUCE_PATH)
    add_subdirectory(${JUCE_PATH} JUCE)
elseif(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../JUCE/CMakeLists.txt)
    add_subdirectory(../JUCE JUCE)
else()
    message(STATUS "JUCE not found locally — fetching from GitHub...")
    include(FetchContent)
    FetchContent_Declare(
        JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        8.0.0   # pin to a release tag for reproducible builds
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(JUCE)
endif()

# ============================================================
# The executable target
# ============================================================
juce_add_gui_app(AudioLab
    PRODUCT_NAME            "AudioLab"
    COMPANY_NAME            "TuesdayNightInstruments"
    BUNDLE_ID               "com.TuesdayNightInstruments.AudioLab"
    VERSION                 "1.0.0"

    # macOS only — suppresses the window-state restore warning
    PLIST_TO_MERGE          "<dict>
                                <key>NSMicrophoneUsageDescription</key>
                                <string>AudioLab needs microphone access for audio input.</string>
                                <key>NSQuitAlwaysKeepsWindows</key>
                                <false/>
                             </dict>"
)

# ============================================================
# Source files
# ============================================================
target_sources(AudioLab PRIVATE
    Source/Main.cpp

    # Headers listed here so IDEs show them in the project tree
    Source/AudioLab.h
    Source/AudioDeviceManager.h
    Source/AudioPlayer.h
    Source/Effect.h
    Source/Oscillator.h
    Source/DSPDemos_Common.h
)

# ============================================================
# JUCE modules
# ============================================================
target_link_libraries(AudioLab PRIVATE
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_core
    juce::juce_data_structures
    juce::juce_dsp
    juce::juce_events
    juce::juce_graphics
    juce::juce_gui_basics
    juce::juce_gui_extra

    # Recommended JUCE compile definitions
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)

# ============================================================
# Compile definitions
# ============================================================
target_compile_definitions(AudioLab PRIVATE
    JUCE_STRICT_REFCOUNTEDPOINTER=1
    JUCE_PUSH_NOTIFICATIONS=1
    JUCE_WEB_BROWSER=0          # not needed, avoids extra deps
    JUCE_USE_CURL=0             # not needed, avoids extra deps
    JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:AudioLab,JUCE_PRODUCT_NAME>"
    JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:AudioLab,JUCE_VERSION>"
)

# ============================================================
# Include paths — so #include "AudioLab.h" etc. work from Main.cpp
# ============================================================
target_include_directories(AudioLab PRIVATE
    Source
)

# ============================================================
# Platform-specific tweaks
# ============================================================
if(APPLE)
    # Silence the CoreAudio factory warning in the Xcode console
    set_target_properties(AudioLab PROPERTIES
        XCODE_SCHEME_ENVIRONMENT "OS_ACTIVITY_MODE=disable"
    )
endif()

if(WIN32)
    # Needed on Windows for JUCE to find its own headers cleanly
    target_compile_definitions(AudioLab PRIVATE
        _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
    )
endif()
