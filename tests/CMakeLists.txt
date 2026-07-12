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

juce_add_console_app(drs_phase1_runtime_contract_tests
    PRODUCT_NAME "Decent Rhapsody Studio Phase 1 Runtime Contract Tests"
)

target_sources(drs_phase1_runtime_contract_tests
    PRIVATE
        src/Phase1RuntimeContractTests.cpp
)

target_link_libraries(drs_phase1_runtime_contract_tests
    PRIVATE
        drs_engine_adapter
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_compile_features(drs_phase1_runtime_contract_tests PRIVATE cxx_std_17)

target_include_directories(drs_phase1_runtime_contract_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/third_party/hise/hi_tools/hi_neural/RTNeural/modules
)

add_test(
    NAME drs.phase1.runtime_contract
    COMMAND drs_phase1_runtime_contract_tests
)

juce_add_console_app(drs_phase1_runtime_baseline_report
    PRODUCT_NAME "Decent Rhapsody Studio Phase 1 Runtime Baseline Report"
)

target_sources(drs_phase1_runtime_baseline_report
    PRIVATE
        src/Phase1RuntimeBaselineReport.cpp
)

target_link_libraries(drs_phase1_runtime_baseline_report
    PRIVATE
        drs_engine_adapter
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_compile_features(drs_phase1_runtime_baseline_report PRIVATE cxx_std_17)

add_test(
    NAME drs.phase1.runtime_baseline_report
    COMMAND drs_phase1_runtime_baseline_report "${CMAKE_CURRENT_BINARY_DIR}/phase1-runtime-baseline.json"
)

juce_add_console_app(drs_phase1_sample_import_tests
    PRODUCT_NAME "Decent Rhapsody Studio Phase 1 Sample Import Tests"
)

target_sources(drs_phase1_sample_import_tests
    PRIVATE
        src/Phase1SampleImportTests.cpp
        ${CMAKE_SOURCE_DIR}/engine_adapter/src/SampleImport.cpp
)

target_link_libraries(drs_phase1_sample_import_tests
    PRIVATE
        drs_engine_adapter
        juce::juce_audio_formats
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_compile_features(drs_phase1_sample_import_tests PRIVATE cxx_std_17)

target_include_directories(drs_phase1_sample_import_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/third_party/juce/modules
)

add_test(
    NAME drs.phase1.sample_import
    COMMAND drs_phase1_sample_import_tests
)

juce_add_console_app(drs_phase1_compile_path_tests
    PRODUCT_NAME "Decent Rhapsody Studio Phase 1 Compile Path Tests"
)

target_sources(drs_phase1_compile_path_tests
    PRIVATE
        src/Phase1CompilePathTests.cpp
        ${CMAKE_SOURCE_DIR}/engine_adapter/src/SampleImport.cpp
)

target_link_libraries(drs_phase1_compile_path_tests
    PRIVATE
        drs_engine_adapter
        juce::juce_audio_formats
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_compile_features(drs_phase1_compile_path_tests PRIVATE cxx_std_17)

target_include_directories(drs_phase1_compile_path_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/third_party/juce/modules
        ${CMAKE_SOURCE_DIR}/third_party/hise/hi_tools/hi_neural/RTNeural/modules
)

add_test(
    NAME drs.phase1.compile_path
    COMMAND drs_phase1_compile_path_tests
)

juce_add_console_app(drs_phase1_pipeline_report
    PRODUCT_NAME "Decent Rhapsody Studio Phase 1 Pipeline Report"
)

target_sources(drs_phase1_pipeline_report
    PRIVATE
        src/Phase1PipelineReport.cpp
        ${CMAKE_SOURCE_DIR}/engine_adapter/src/SampleImport.cpp
)

target_link_libraries(drs_phase1_pipeline_report
    PRIVATE
        drs_engine_adapter
        juce::juce_audio_formats
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_compile_features(drs_phase1_pipeline_report PRIVATE cxx_std_17)

target_include_directories(drs_phase1_pipeline_report
    PRIVATE
        ${CMAKE_SOURCE_DIR}/third_party/juce/modules
        ${CMAKE_SOURCE_DIR}/third_party/hise/hi_tools/hi_neural/RTNeural/modules
)

add_test(
    NAME drs.phase1.pipeline_report
    COMMAND drs_phase1_pipeline_report "${CMAKE_CURRENT_BINARY_DIR}/phase1-pipeline-report.json"
)

juce_add_console_app(drs_phase1_runtime_baseline_guard
    PRODUCT_NAME "Decent Rhapsody Studio Phase 1 Runtime Baseline Guard"
)

target_sources(drs_phase1_runtime_baseline_guard
    PRIVATE
        src/Phase1RuntimeBaselineGuard.cpp
)

target_link_libraries(drs_phase1_runtime_baseline_guard
    PRIVATE
        drs_engine_adapter
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_compile_features(drs_phase1_runtime_baseline_guard PRIVATE cxx_std_17)

target_include_directories(drs_phase1_runtime_baseline_guard
    PRIVATE
        ${CMAKE_SOURCE_DIR}/third_party/hise/hi_tools/hi_neural/RTNeural/modules
)

add_test(
    NAME drs.phase1.runtime_baseline_guard
    COMMAND drs_phase1_runtime_baseline_guard "${CMAKE_CURRENT_BINARY_DIR}/phase1-runtime-baseline.json"
)

set_tests_properties(drs.phase1.runtime_baseline_guard PROPERTIES
    DEPENDS drs.phase1.runtime_baseline_report
)

juce_add_console_app(drs_phase1_runtime_fixture_tool
    PRODUCT_NAME "Decent Rhapsody Studio Phase 1 Runtime Fixture Tool"
)

target_sources(drs_phase1_runtime_fixture_tool
    PRIVATE
        src/Phase1RuntimeFixtureTool.cpp
)

target_link_libraries(drs_phase1_runtime_fixture_tool
    PRIVATE
        drs_engine_adapter
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_compile_features(drs_phase1_runtime_fixture_tool PRIVATE cxx_std_17)

target_include_directories(drs_phase1_runtime_fixture_tool
    PRIVATE
        ${CMAKE_SOURCE_DIR}/third_party/hise/hi_tools/hi_neural/RTNeural/modules
)

add_test(
    NAME drs.phase1.fixture_tool_verify
    COMMAND drs_phase1_runtime_fixture_tool --verify
)

set_target_properties(drs_phase0_smoke_tests PROPERTIES
    FOLDER "Tests"
)

set_target_properties(drs_phase1_runtime_contract_tests PROPERTIES
    FOLDER "Tests"
)

set_target_properties(drs_phase1_runtime_baseline_report PROPERTIES
    FOLDER "Tests"
)

set_target_properties(drs_phase1_sample_import_tests PROPERTIES
    FOLDER "Tests"
)

set_target_properties(drs_phase1_compile_path_tests PROPERTIES
    FOLDER "Tests"
)

set_target_properties(drs_phase1_pipeline_report PROPERTIES
    FOLDER "Tests"
)

set_target_properties(drs_phase1_runtime_baseline_guard PROPERTIES
    FOLDER "Tests"
)

set_target_properties(drs_phase1_runtime_fixture_tool PROPERTIES
    FOLDER "Tests"
)
