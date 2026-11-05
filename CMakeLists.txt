cmake_minimum_required(VERSION 3.22)
project(StickBend VERSION 0.1.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 20)

include(FetchContent)
# Pinned to exact commits, not tags: a tag can be moved to different code, a commit hash can't.
FetchContent_Declare(JUCE GIT_REPOSITORY https://github.com/juce-framework/JUCE.git GIT_TAG d60b7a78030661574a615b586d9325ba7a7fa7dd) # 9.0.5
FetchContent_Declare(clap-juce-extensions GIT_REPOSITORY https://github.com/free-audio/clap-juce-extensions.git GIT_TAG 7adee3a1bd4684d4caa5601100e364abccff4b4d) # main: 0.26.0 doesn't build against JUCE 9
FetchContent_MakeAvailable(JUCE clap-juce-extensions)

juce_add_plugin(StickBend
    COMPANY_NAME "Macky Ar"
    PLUGIN_MANUFACTURER_CODE Mhsh
    PLUGIN_CODE Stkb
    PRODUCT_NAME "StickBend"
    BUNDLE_ID com.maheshArora.stickbend
    FORMATS VST3
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    IS_SYNTH FALSE
    VST3_CATEGORIES Fx Tools)

target_sources(StickBend PRIVATE Source/Plugin.cpp Source/Editor.cpp Source/Pad.cpp)

# Outfit (SIL OFL 1.1, Resources/Outfit-OFL.txt), embedded so it works without being installed.
juce_add_binary_data(StickBendFonts SOURCES Resources/Outfit-Regular.ttf Resources/Outfit-SemiBold.ttf Resources/Outfit-Bold.ttf)
target_compile_definitions(StickBend PUBLIC STICKBEND_VERSION="${PROJECT_VERSION}" JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
target_link_libraries(StickBend
    PRIVATE juce::juce_audio_utils xinput StickBendFonts
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)

# Exploit mitigations for the shipped DLLs (PUBLIC so the VST3/CLAP wrapper targets get them too):
# Control Flow Guard, CET shadow stack, ASLR (incl. high-entropy) and DEP. /GS stack cookies are MSVC's default.
if(MSVC)
    target_compile_options(StickBend PUBLIC /guard:cf)
    target_link_options(StickBend PUBLIC /guard:cf /CETCOMPAT /DYNAMICBASE /HIGHENTROPYVA /NXCOMPAT)
endif()

clap_juce_extensions_plugin(TARGET StickBend
    CLAP_ID "com.maheshArora.stickbend"
    CLAP_FEATURES audio-effect note-effect utility)

add_executable(stickmap_test tests/stickmap_test.cpp)

juce_add_console_app(hosttest)
target_sources(hosttest PRIVATE tests/hosttest.cpp)
target_compile_definitions(hosttest PRIVATE JUCE_PLUGINHOST_VST3=1 JUCE_MODAL_LOOPS_PERMITTED=1 JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
target_link_libraries(hosttest PRIVATE juce::juce_audio_processors juce::juce_gui_basics)

# Renders the editor to a PNG with a fake pad pose: snapshot <out.png>
juce_add_console_app(snapshot)
target_sources(snapshot PRIVATE tests/snapshot.cpp Source/Editor.cpp)
target_compile_definitions(snapshot PRIVATE STICKBEND_VERSION="${PROJECT_VERSION}" JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
target_link_libraries(snapshot PRIVATE juce::juce_audio_processors juce::juce_gui_basics StickBendFonts)

# Throws garbage and hostile project data at setStateInformation and checks the plugin survives with sane values.
juce_add_console_app(statetest)
target_sources(statetest PRIVATE tests/statetest.cpp Source/Editor.cpp)
target_compile_definitions(statetest PRIVATE STICKBEND_VERSION="${PROJECT_VERSION}" JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
target_link_libraries(statetest PRIVATE juce::juce_audio_processors juce::juce_gui_basics StickBendFonts)
