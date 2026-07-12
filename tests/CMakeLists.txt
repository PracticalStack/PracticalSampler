juce_add_console_app(drs_phase0_smoke_tests
    PRODUCT_NAME "Decent Rhapsody Studio Phase 0 Smoke Tests"
)

target_sources(drs_phase0_smoke_tests
    PRIVATE
        src/Phase0SmokeTests.cpp
)

target_link_libraries(drs_phase0_smoke_tests
    PRIVATE
        drs_engine_adapter
        drs_plugin_shell
        drs_standalone_shell
        juce::juce_audio_processors_headless
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_compile_features(drs_phase0_smoke_tests PRIVATE cxx_std_17)

target_compile_definitions(drs_phase0_smoke_tests
    PRIVATE
        JUCE_PLUGINHOST_VST3=1
)

add_test(
    NAME drs.phase0.smoke
    COMMAND drs_phase0_smoke_tests
)

set_target_properties(drs_phase0_smoke_tests PROPERTIES
    FOLDER "Tests"
)
