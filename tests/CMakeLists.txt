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

if(MSVC)
    target_link_options(drs_phase0_smoke_tests PRIVATE /STACK:8388608)
endif()

target_compile_definitions(drs_phase0_smoke_tests
    PRIVATE
        JUCE_PLUGINHOST_VST3=1
)

# The smoke executable scans the built VST3 bundle; make that runtime fixture a
# build dependency so a narrow smoke-only build is self-contained.
add_dependencies(drs_phase0_smoke_tests DecentRhapsodyStudioPlugin)

add_test(
    NAME drs.phase0.smoke
    COMMAND drs_phase0_smoke_tests
)

juce_add_console_app(drs_vst3_host_state_qualification_tests
    PRODUCT_NAME "Decent Rhapsody Studio VST3 Host State Qualification Tests"
)

target_sources(drs_vst3_host_state_qualification_tests
    PRIVATE
        src/Vst3HostStateQualificationTests.cpp
)

target_link_libraries(drs_vst3_host_state_qualification_tests
    PRIVATE
        drs_engine_adapter
        drs_plugin_shell
        juce::juce_audio_processors_headless
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_compile_features(drs_vst3_host_state_qualification_tests PRIVATE cxx_std_17)

target_compile_definitions(drs_vst3_host_state_qualification_tests
    PRIVATE
        JUCE_PLUGINHOST_VST3=1
        JUCE_MODAL_LOOPS_PERMITTED=1
)

add_dependencies(drs_vst3_host_state_qualification_tests DecentRhapsodyStudioPlugin)

add_test(
    NAME drs.host_state.vst3_qualification
    COMMAND drs_vst3_host_state_qualification_tests
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

juce_add_console_app(drs_phase1_draft_playback_contract_tests
    PRODUCT_NAME "Decent Rhapsody Studio Phase 1 Draft Playback Contract Tests"
)

target_sources(drs_phase1_draft_playback_contract_tests
    PRIVATE
        src/Phase1DraftPlaybackContractTests.cpp
)

target_link_libraries(drs_phase1_draft_playback_contract_tests
    PRIVATE
        drs_engine_adapter
        drs_sample_import
        juce::juce_audio_formats
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_compile_features(drs_phase1_draft_playback_contract_tests PRIVATE cxx_std_17)

add_test(
    NAME drs.phase1.draft_playback_contract
    COMMAND drs_phase1_draft_playback_contract_tests
)

juce_add_console_app(drs_phase1_draft_playback_facade_tests
    PRODUCT_NAME "Decent Rhapsody Studio Phase 1 Draft Playback Facade Tests"
)

target_sources(drs_phase1_draft_playback_facade_tests
    PRIVATE
        src/Phase1DraftPlaybackFacadeTests.cpp
)

target_link_libraries(drs_phase1_draft_playback_facade_tests
    PRIVATE
        drs_engine_adapter
        drs_sampler_core
        drs_sample_import
        juce::juce_audio_formats
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_compile_features(drs_phase1_draft_playback_facade_tests PRIVATE cxx_std_17)

add_test(
    NAME drs.phase1.draft_playback_facade
    COMMAND drs_phase1_draft_playback_facade_tests
)

juce_add_console_app(drs_phase1_playback_snapshot_tests
    PRODUCT_NAME "Decent Rhapsody Studio Phase 1 Playback Snapshot Tests"
)

target_sources(drs_phase1_playback_snapshot_tests
    PRIVATE
        src/Phase1PlaybackSnapshotTests.cpp
)

target_link_libraries(drs_phase1_playback_snapshot_tests
    PRIVATE
        drs_engine_adapter
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_compile_features(drs_phase1_playback_snapshot_tests PRIVATE cxx_std_17)

add_test(
    NAME drs.phase1.playback_snapshot
    COMMAND drs_phase1_playback_snapshot_tests
)

juce_add_console_app(drs_phase1_prepared_playback_tests
    PRODUCT_NAME "Decent Rhapsody Studio Phase 1 Prepared Playback Tests"
)

target_sources(drs_phase1_prepared_playback_tests
    PRIVATE
        src/Phase1PreparedPlaybackTests.cpp
)

target_link_libraries(drs_phase1_prepared_playback_tests
    PRIVATE
        drs_engine_adapter
        drs_sample_import
        juce::juce_audio_formats
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_compile_features(drs_phase1_prepared_playback_tests PRIVATE cxx_std_17)

add_test(
    NAME drs.phase1.prepared_playback
    COMMAND drs_phase1_prepared_playback_tests
)

juce_add_console_app(drs_phase1_prepared_playback_worker_tests
    PRODUCT_NAME "Decent Rhapsody Studio Phase 1 Prepared Playback Worker Tests"
)

target_sources(drs_phase1_prepared_playback_worker_tests
    PRIVATE
        src/Phase1PreparedPlaybackWorkerTests.cpp
)

target_link_libraries(drs_phase1_prepared_playback_worker_tests
    PRIVATE
        drs_engine_adapter
        drs_sampler_core
        drs_sample_import
        juce::juce_audio_formats
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_compile_features(drs_phase1_prepared_playback_worker_tests PRIVATE cxx_std_17)

add_test(
    NAME drs.phase1.prepared_playback_worker
    COMMAND drs_phase1_prepared_playback_worker_tests
)

add_executable(drs_package_v2_tests
    src/PackageV2Tests.cpp
)

target_link_libraries(drs_package_v2_tests
    PRIVATE
        drs_sampler_core
        drs_sample_import
        juce::juce_audio_formats
)
target_compile_features(drs_package_v2_tests PRIVATE cxx_std_17)

add_test(
    NAME drs.package_v2.records
    COMMAND drs_package_v2_tests
)

add_executable(drs_large_instrument_qualification
    src/LargeInstrumentQualification.cpp
)

target_link_libraries(drs_large_instrument_qualification
    PRIVATE
        drs_engine_adapter
        drs_sampler_core
        drs_sample_import
        drs_app_shared
        juce::juce_audio_formats
)
if(WIN32)
    target_link_libraries(drs_large_instrument_qualification PRIVATE Psapi)
endif()
target_compile_features(drs_large_instrument_qualification PRIVATE cxx_std_17)
target_compile_definitions(drs_large_instrument_qualification PRIVATE NOMINMAX)
target_include_directories(drs_large_instrument_qualification PRIVATE ${CMAKE_SOURCE_DIR}/app/src)

# Sprint 4 Entry Gate EG1 focused regression matrix.
juce_add_console_app(drs_sprint4_entry_authored_input_tests
    PRODUCT_NAME "Decent Rhapsody Studio Sprint 4 Entry Authored Input Tests"
)

target_sources(drs_sprint4_entry_authored_input_tests
    PRIVATE
        src/Phase1Sprint4EntryAuthoredInputTests.cpp
)

target_link_libraries(drs_sprint4_entry_authored_input_tests
    PRIVATE
        drs_engine_adapter
        drs_sample_import
        juce::juce_audio_formats
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_compile_features(drs_sprint4_entry_authored_input_tests PRIVATE cxx_std_17)

add_test(
    NAME drs.sprint4_entry.authored_input
    COMMAND drs_sprint4_entry_authored_input_tests
)

# Sprint 4 Entry Gate EG2 focused lifetime and handoff matrix.
juce_add_console_app(drs_sprint4_entry_activation_payload_tests
    PRODUCT_NAME "Decent Rhapsody Studio Sprint 4 Entry Activation Payload Tests"
)

target_sources(drs_sprint4_entry_activation_payload_tests
    PRIVATE
        src/Phase1Sprint4EntryActivationPayloadTests.cpp
)

target_link_libraries(drs_sprint4_entry_activation_payload_tests
    PRIVATE
        drs_engine_adapter
        drs_sample_import
        drs_plugin_shell
        juce::juce_audio_processors_headless
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_compile_features(drs_sprint4_entry_activation_payload_tests PRIVATE cxx_std_17)

add_test(
    NAME drs.sprint4_entry.activation_payload
    COMMAND drs_sprint4_entry_activation_payload_tests
)

# Sprint 4 Entry Gate EG3 deterministic concurrency regression.
juce_add_console_app(drs_sprint4_entry_diagnostics_concurrency_tests
    PRODUCT_NAME "Decent Rhapsody Studio Sprint 4 Entry Diagnostics Concurrency Tests"
)

target_sources(drs_sprint4_entry_diagnostics_concurrency_tests
    PRIVATE
        src/Phase1Sprint4EntryDiagnosticsConcurrencyTests.cpp
)

target_link_libraries(drs_sprint4_entry_diagnostics_concurrency_tests
    PRIVATE
        drs_plugin_shell
        juce::juce_audio_processors_headless
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_compile_features(drs_sprint4_entry_diagnostics_concurrency_tests PRIVATE cxx_std_17)

add_test(
    NAME drs.sprint4_entry.diagnostics_concurrency
    COMMAND drs_sprint4_entry_diagnostics_concurrency_tests
)

# Sprint 4 closure soak: two live contexts, activation churn, retirement, and UI polling.
juce_add_console_app(drs_sprint4_concurrency_soak_tests
    PRODUCT_NAME "Decent Rhapsody Studio Sprint 4 Concurrency Soak Tests"
)

target_sources(drs_sprint4_concurrency_soak_tests
    PRIVATE
        src/Sprint4ConcurrencySoakTests.cpp
)

target_link_libraries(drs_sprint4_concurrency_soak_tests
    PRIVATE
        drs_plugin_shell
        juce::juce_audio_processors_headless
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_compile_features(drs_sprint4_concurrency_soak_tests PRIVATE cxx_std_17)

add_test(
    NAME drs.sprint4.concurrency_soak
    COMMAND drs_sprint4_concurrency_soak_tests
)

# Sprint 4 Entry Gate EG4 focused real-time guard and negative-injection matrix.
juce_add_console_app(drs_sprint4_entry_realtime_guard_tests
    PRODUCT_NAME "Decent Rhapsody Studio Sprint 4 Entry Realtime Guard Tests"
)

target_sources(drs_sprint4_entry_realtime_guard_tests
    PRIVATE
        src/Phase1Sprint4EntryRealtimeGuardTests.cpp
)

target_link_libraries(drs_sprint4_entry_realtime_guard_tests
    PRIVATE
        drs_plugin_shell
        juce::juce_audio_processors_headless
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_compile_features(drs_sprint4_entry_realtime_guard_tests PRIVATE cxx_std_17)

add_test(
    NAME drs.sprint4_entry.realtime_guard
    COMMAND drs_sprint4_entry_realtime_guard_tests
)

# Sprint 4 Entry Gate EG5 Preview/Publish parity through standalone and editor-closed plugin shells.
juce_add_console_app(drs_sprint4_entry_shell_parity_tests
    PRODUCT_NAME "Decent Rhapsody Studio Sprint 4 Entry Shell Parity Tests"
)

target_sources(drs_sprint4_entry_shell_parity_tests
    PRIVATE
        src/Phase1Sprint4EntryShellParityTests.cpp
)

target_link_libraries(drs_sprint4_entry_shell_parity_tests
    PRIVATE
        drs_plugin_shell
        drs_standalone_shell
        juce::juce_audio_processors_headless
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_compile_features(drs_sprint4_entry_shell_parity_tests PRIVATE cxx_std_17)

add_test(
    NAME drs.sprint4_entry.shell_parity
    COMMAND drs_sprint4_entry_shell_parity_tests
)

# Sprint 4.1 renderer boundary and immutable render-model matrix.
add_executable(drs_sprint4_render_model_tests
    src/Sprint4RenderModelTests.cpp
)

target_link_libraries(drs_sprint4_render_model_tests
    PRIVATE
        drs_sampler_core
)

target_compile_features(drs_sprint4_render_model_tests PRIVATE cxx_std_17)

add_test(
    NAME drs.sprint4.render_model
    COMMAND drs_sprint4_render_model_tests
)

# Sprint 4.2 deterministic single-voice DSP and compatibility-vector matrix.
add_executable(drs_sprint4_voice_kernel_tests
    src/Sprint4VoiceKernelTests.cpp
)

target_link_libraries(drs_sprint4_voice_kernel_tests
    PRIVATE
        drs_sampler_core
)

target_compile_features(drs_sprint4_voice_kernel_tests PRIVATE cxx_std_17)

add_test(
    NAME drs.sprint4.voice_kernel
    COMMAND drs_sprint4_voice_kernel_tests
)

# Sprint 4.3 fixed voice-pool, bounded event scratch, and sample-accurate scheduling matrix.
add_executable(drs_sprint4_scheduler_tests
    src/Sprint4VoicePoolTests.cpp
)

target_link_libraries(drs_sprint4_scheduler_tests
    PRIVATE
        drs_sampler_core
)

target_compile_features(drs_sprint4_scheduler_tests PRIVATE cxx_std_17)

add_test(
    NAME drs.sprint4.scheduler
    COMMAND drs_sprint4_scheduler_tests
)

# Sprint 4.4 loop traversal, release envelope, and voice-lifecycle matrix.
add_executable(drs_sprint4_voice_lifecycle_tests
    src/Sprint4VoiceLifecycleTests.cpp
)

target_link_libraries(drs_sprint4_voice_lifecycle_tests
    PRIVATE
        drs_sampler_core
)

target_compile_features(drs_sprint4_voice_lifecycle_tests PRIVATE cxx_std_17)

add_test(
    NAME drs.sprint4.voice_lifecycle
    COMMAND drs_sprint4_voice_lifecycle_tests
)

# Sprint 4.5 isolated Preview/Performance contexts and activation-lifetime matrix.
add_executable(drs_sprint4_playback_context_tests
    src/Sprint4PlaybackContextTests.cpp
)

target_link_libraries(drs_sprint4_playback_context_tests
    PRIVATE
        drs_sampler_core
)

target_compile_features(drs_sprint4_playback_context_tests PRIVATE cxx_std_17)

add_test(
    NAME drs.sprint4.playback_context
    COMMAND drs_sprint4_playback_context_tests
)

# Sprint 4.6 normalized shell routes, host MIDI cutover, and shell parity matrix.
juce_add_console_app(drs_sprint4_shell_cutover_tests
    PRODUCT_NAME "Decent Rhapsody Studio Sprint 4 Shell Cutover Tests"
)

target_sources(drs_sprint4_shell_cutover_tests
    PRIVATE
        src/Sprint4ShellCutoverTests.cpp
)

target_link_libraries(drs_sprint4_shell_cutover_tests
    PRIVATE
        drs_plugin_shell
        drs_standalone_shell
        juce::juce_audio_processors_headless
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_compile_features(drs_sprint4_shell_cutover_tests PRIVATE cxx_std_17)

add_test(
    NAME drs.sprint4.shell_cutover
    COMMAND drs_sprint4_shell_cutover_tests
)

# Declarative performance-engine Sprint 0 contract, fixture, and direct-red baseline.
add_executable(drs_performance_engine_s0_red_tests
    src/PerformanceEngineSprint0RedTests.cpp
)

target_compile_features(drs_performance_engine_s0_red_tests PRIVATE cxx_std_17)

# The declarative-event Sprint 0 seams are intentionally direct-only expected-red
# checks. Later sprints replace each named seam with a registered green test.
set_target_properties(drs_performance_engine_s0_red_tests PROPERTIES
    FOLDER "Tests"
)

add_executable(drs_performance_engine_s0_fixture_tests
    src/PerformanceEngineSprint0FixtureTests.cpp
)

target_compile_definitions(drs_performance_engine_s0_fixture_tests
    PRIVATE
        DRS_PERFORMANCE_ENGINE_S0_FIXTURE_ROOT="${CMAKE_SOURCE_DIR}/content/runtime/performance-engine/sprint0"
)

target_compile_features(drs_performance_engine_s0_fixture_tests PRIVATE cxx_std_17)

add_test(
    NAME drs.performance_engine.s0.fixtures
    COMMAND drs_performance_engine_s0_fixture_tests
)

set_target_properties(drs_performance_engine_s0_fixture_tests PROPERTIES
    FOLDER "Tests"
)

add_executable(drs_performance_engine_s1_articulation_tests
    src/PerformanceEngineSprint1ArticulationTests.cpp
)

target_link_libraries(drs_performance_engine_s1_articulation_tests
    PRIVATE
        drs_engine_adapter
)

target_compile_definitions(drs_performance_engine_s1_articulation_tests
    PRIVATE
        DRS_PERFORMANCE_ENGINE_S1_LEGACY_FIXTURE_PATH="${CMAKE_SOURCE_DIR}/content/runtime/phase2/control-law/three-group-linear-gain.drsproj"
)

target_compile_features(drs_performance_engine_s1_articulation_tests PRIVATE cxx_std_17)

add_test(
    NAME drs.performance_engine.s1.articulations
    COMMAND drs_performance_engine_s1_articulation_tests
)

set_target_properties(drs_performance_engine_s1_articulation_tests PROPERTIES
    FOLDER "Tests"
)

add_executable(drs_performance_engine_s2_rule_tests
    src/PerformanceEngineSprint2RuleTests.cpp
)

target_link_libraries(drs_performance_engine_s2_rule_tests
    PRIVATE
        drs_engine_adapter
)

target_compile_definitions(drs_performance_engine_s2_rule_tests
    PRIVATE
        DRS_PERFORMANCE_ENGINE_S2_LEGACY_FIXTURE_PATH="${CMAKE_SOURCE_DIR}/content/runtime/phase2/control-law/three-group-linear-gain.drsproj"
)

target_compile_features(drs_performance_engine_s2_rule_tests PRIVATE cxx_std_17)

add_test(
    NAME drs.performance_engine.s2.rules
    COMMAND drs_performance_engine_s2_rule_tests
)

set_target_properties(drs_performance_engine_s2_rule_tests PROPERTIES
    FOLDER "Tests"
)

add_executable(drs_performance_engine_s3_program_tests
    src/PerformanceEngineSprint3ProgramTests.cpp
)

target_link_libraries(drs_performance_engine_s3_program_tests
    PRIVATE
        drs_engine_adapter
        drs_sample_import
        drs_sampler_core
        juce::juce_audio_formats
)

target_compile_definitions(drs_performance_engine_s3_program_tests
    PRIVATE
        DRS_PERFORMANCE_ENGINE_S3_LEGACY_FIXTURE_PATH="${CMAKE_SOURCE_DIR}/content/runtime/phase2/control-law/three-group-linear-gain.drsproj"
)

target_compile_features(drs_performance_engine_s3_program_tests PRIVATE cxx_std_17)

add_test(
    NAME drs.performance_engine.s3.program
    COMMAND drs_performance_engine_s3_program_tests
)

set_target_properties(drs_performance_engine_s3_program_tests PROPERTIES
    FOLDER "Tests"
)

add_executable(drs_performance_engine_s4_state_tests
    src/PerformanceEngineSprint4StateTests.cpp
)

target_link_libraries(drs_performance_engine_s4_state_tests
    PRIVATE
        drs_sampler_core
)

target_compile_features(drs_performance_engine_s4_state_tests PRIVATE cxx_std_17)

add_test(
    NAME drs.performance_engine.s4.state
    COMMAND drs_performance_engine_s4_state_tests
)

set_target_properties(drs_performance_engine_s4_state_tests PROPERTIES
    FOLDER "Tests"
)

add_executable(drs_performance_engine_s5_keyswitch_tests
    src/PerformanceEngineSprint5KeySwitchTests.cpp
)

target_link_libraries(drs_performance_engine_s5_keyswitch_tests
    PRIVATE
        drs_sampler_core
)

target_compile_features(drs_performance_engine_s5_keyswitch_tests PRIVATE cxx_std_17)

add_test(
    NAME drs.performance_engine.s5.keyswitch
    COMMAND drs_performance_engine_s5_keyswitch_tests
)

set_target_properties(drs_performance_engine_s5_keyswitch_tests PROPERTIES
    FOLDER "Tests"
)

add_executable(drs_performance_engine_s6_release_tests
    src/PerformanceEngineSprint6ReleaseTests.cpp
)

target_link_libraries(drs_performance_engine_s6_release_tests
    PRIVATE
        drs_sampler_core
)

target_compile_features(drs_performance_engine_s6_release_tests PRIVATE cxx_std_17)

add_test(
    NAME drs.performance_engine.s6.release
    COMMAND drs_performance_engine_s6_release_tests
)

set_target_properties(drs_performance_engine_s6_release_tests PROPERTIES
    FOLDER "Tests"
)

add_executable(drs_performance_engine_s7_pedal_tests
    src/PerformanceEngineSprint7PedalTests.cpp
)

target_link_libraries(drs_performance_engine_s7_pedal_tests
    PRIVATE
        drs_sampler_core
)

target_compile_features(drs_performance_engine_s7_pedal_tests PRIVATE cxx_std_17)

add_test(
    NAME drs.performance_engine.s7.pedal
    COMMAND drs_performance_engine_s7_pedal_tests
)

set_target_properties(drs_performance_engine_s7_pedal_tests PROPERTIES
    FOLDER "Tests"
)

add_executable(drs_performance_engine_s8_choke_tests
    src/PerformanceEngineSprint8ChokeTests.cpp
)

target_link_libraries(drs_performance_engine_s8_choke_tests
    PRIVATE
        drs_sampler_core
)

target_compile_features(drs_performance_engine_s8_choke_tests PRIVATE cxx_std_17)

add_test(
    NAME drs.performance_engine.s8.choke
    COMMAND drs_performance_engine_s8_choke_tests
)

set_target_properties(drs_performance_engine_s8_choke_tests PROPERTIES
    FOLDER "Tests"
)

add_executable(drs_performance_engine_s9_round_robin_tests
    src/PerformanceEngineSprint9RoundRobinTests.cpp
)

target_link_libraries(drs_performance_engine_s9_round_robin_tests
    PRIVATE
        drs_sampler_core
)

target_compile_features(drs_performance_engine_s9_round_robin_tests PRIVATE cxx_std_17)

add_test(
    NAME drs.performance_engine.s9.round_robin
    COMMAND drs_performance_engine_s9_round_robin_tests
)

set_target_properties(drs_performance_engine_s9_round_robin_tests PROPERTIES
    FOLDER "Tests"
)

add_executable(drs_curated_dsp_contract_red_tests
    src/CuratedDspContractRedTests.cpp
)

target_link_libraries(drs_curated_dsp_contract_red_tests
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_compile_features(drs_curated_dsp_contract_red_tests PRIVATE cxx_std_17)

# S0 expected-red checks are intentionally not CTest tests and not drs_all_tests
# dependencies. Each case is invoked directly by its named missing seam.
set_target_properties(drs_curated_dsp_contract_red_tests PROPERTIES
    FOLDER "Tests"
)

# Sprint 1 converts the Sprint 0 direct-only red seams into the shared-core
# deterministic and randomized property suite.
juce_add_console_app(drs_control_law_tests
    PRODUCT_NAME "Decent Rhapsody Studio Control Law Tests"
)

target_sources(drs_control_law_tests
    PRIVATE
        src/ControlLawTests.cpp
)

target_link_libraries(drs_control_law_tests
    PRIVATE
        drs_engine_adapter
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_compile_features(drs_control_law_tests PRIVATE cxx_std_17)

target_compile_definitions(drs_control_law_tests
    PRIVATE
        DRS_CONTROL_LAW_S0_FIXTURE_PATH="${CMAKE_SOURCE_DIR}/content/runtime/phase2/control-law/three-group-linear-gain.drsproj"
)

add_test(
    NAME drs.control_law
    COMMAND drs_control_law_tests
)

set_target_properties(drs_control_law_tests PROPERTIES
    FOLDER "Tests"
)

add_executable(drs_control_law_project_schema_tests
    src/ControlLawProjectSchemaTests.cpp
)
target_link_libraries(drs_control_law_project_schema_tests PRIVATE drs_engine_adapter)
target_compile_features(drs_control_law_project_schema_tests PRIVATE cxx_std_17)
target_compile_definitions(drs_control_law_project_schema_tests
    PRIVATE
        DRS_CONTROL_LAW_S0_FIXTURE_PATH="${CMAKE_SOURCE_DIR}/content/runtime/phase2/control-law/three-group-linear-gain.drsproj"
)
add_test(NAME drs.control_law.project_schema COMMAND drs_control_law_project_schema_tests)
set_target_properties(drs_control_law_project_schema_tests PROPERTIES FOLDER "Tests")

juce_add_console_app(drs_control_law_release_proof_tests
    PRODUCT_NAME "Decent Rhapsody Studio Control Law Release Proof Tests"
)
target_sources(drs_control_law_release_proof_tests
    PRIVATE
        src/ControlLawReleaseProofTests.cpp
)
target_link_libraries(drs_control_law_release_proof_tests
    PRIVATE
        drs_engine_adapter
        drs_app_shared
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
target_compile_features(drs_control_law_release_proof_tests PRIVATE cxx_std_17)
add_test(NAME drs.control_law.release_proof COMMAND drs_control_law_release_proof_tests)
set_target_properties(drs_control_law_release_proof_tests PROPERTIES FOLDER "Tests")

juce_add_console_app(drs_curated_dsp_contract_tests
    PRODUCT_NAME "Decent Rhapsody Studio Curated DSP Contract Tests"
)

target_sources(drs_curated_dsp_contract_tests
    PRIVATE
        src/CuratedDspContractTests.cpp
)

target_link_libraries(drs_curated_dsp_contract_tests
    PRIVATE
        drs_engine_adapter
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_compile_features(drs_curated_dsp_contract_tests PRIVATE cxx_std_17)

target_compile_definitions(drs_curated_dsp_contract_tests
    PRIVATE
        DRS_CURATED_DSP_FIXTURE_ROOT="${CMAKE_SOURCE_DIR}/content/runtime/phase2/curated-dsp"
)

target_include_directories(drs_curated_dsp_contract_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/third_party/hise/hi_tools/hi_neural/RTNeural/modules
)

add_test(
    NAME drs.curated_dsp.contract
    COMMAND drs_curated_dsp_contract_tests
)

set_target_properties(drs_curated_dsp_contract_tests PROPERTIES
    FOLDER "Tests"
)

add_executable(drs_curated_dsp_control_law_tests
    src/CuratedDspControlLawTests.cpp
)
target_link_libraries(drs_curated_dsp_control_law_tests PRIVATE drs_engine_adapter)
target_compile_features(drs_curated_dsp_control_law_tests PRIVATE cxx_std_17)
add_test(NAME drs.curated_dsp.control_law COMMAND drs_curated_dsp_control_law_tests)
set_target_properties(drs_curated_dsp_control_law_tests PROPERTIES FOLDER "Tests")

add_executable(drs_curated_dsp_graph_plan_tests
    src/CuratedDspGraphPlanTests.cpp
)
target_link_libraries(drs_curated_dsp_graph_plan_tests PRIVATE drs_engine_adapter)
target_compile_features(drs_curated_dsp_graph_plan_tests PRIVATE cxx_std_17)
add_test(NAME drs.curated_dsp.graph_plan COMMAND drs_curated_dsp_graph_plan_tests)
set_target_properties(drs_curated_dsp_graph_plan_tests PROPERTIES FOLDER "Tests")

add_executable(drs_curated_dsp_gain_tests
    src/CuratedDspGainTests.cpp
)
target_link_libraries(drs_curated_dsp_gain_tests PRIVATE drs_engine_adapter)
target_compile_features(drs_curated_dsp_gain_tests PRIVATE cxx_std_17)
add_test(NAME drs.curated_dsp.gain COMMAND drs_curated_dsp_gain_tests)
set_target_properties(drs_curated_dsp_gain_tests PROPERTIES FOLDER "Tests")

add_executable(drs_curated_dsp_saturator_tests
    src/CuratedDspSaturatorTests.cpp
)
target_link_libraries(drs_curated_dsp_saturator_tests PRIVATE drs_engine_adapter)
target_compile_features(drs_curated_dsp_saturator_tests PRIVATE cxx_std_17)
add_test(NAME drs.curated_dsp.saturator COMMAND drs_curated_dsp_saturator_tests)
set_target_properties(drs_curated_dsp_saturator_tests PROPERTIES FOLDER "Tests")

add_executable(drs_curated_dsp_stereo_delay_tests src/CuratedDspStereoDelayTests.cpp)
target_link_libraries(drs_curated_dsp_stereo_delay_tests PRIVATE drs_engine_adapter)
target_compile_features(drs_curated_dsp_stereo_delay_tests PRIVATE cxx_std_17)
add_test(NAME drs.curated_dsp.stereo_delay COMMAND drs_curated_dsp_stereo_delay_tests)
set_target_properties(drs_curated_dsp_stereo_delay_tests PROPERTIES FOLDER "Tests")

add_executable(drs_curated_dsp_reverb_tests src/CuratedDspReverbTests.cpp)
target_link_libraries(drs_curated_dsp_reverb_tests PRIVATE drs_engine_adapter)
target_compile_features(drs_curated_dsp_reverb_tests PRIVATE cxx_std_17)
add_test(NAME drs.curated_dsp.reverb COMMAND drs_curated_dsp_reverb_tests)
set_target_properties(drs_curated_dsp_reverb_tests PROPERTIES FOLDER "Tests")

add_executable(drs_curated_dsp_compact_eq_tests src/CuratedDspCompactEqTests.cpp)
target_link_libraries(drs_curated_dsp_compact_eq_tests PRIVATE drs_engine_adapter)
target_compile_features(drs_curated_dsp_compact_eq_tests PRIVATE cxx_std_17)
add_test(NAME drs.curated_dsp.compact_eq COMMAND drs_curated_dsp_compact_eq_tests)
set_target_properties(drs_curated_dsp_compact_eq_tests PROPERTIES FOLDER "Tests")

add_executable(drs_curated_dsp_chorus_tests src/CuratedDspChorusTests.cpp)
target_link_libraries(drs_curated_dsp_chorus_tests PRIVATE drs_engine_adapter)
target_compile_features(drs_curated_dsp_chorus_tests PRIVATE cxx_std_17)
add_test(NAME drs.curated_dsp.chorus COMMAND drs_curated_dsp_chorus_tests)
set_target_properties(drs_curated_dsp_chorus_tests PROPERTIES FOLDER "Tests")

add_executable(drs_curated_dsp_scoped_routing_tests
    src/CuratedDspScopedRoutingTests.cpp
)
target_link_libraries(drs_curated_dsp_scoped_routing_tests PRIVATE drs_engine_adapter drs_sampler_core)
target_compile_features(drs_curated_dsp_scoped_routing_tests PRIVATE cxx_std_17)
add_test(NAME drs.curated_dsp.scoped_routing COMMAND drs_curated_dsp_scoped_routing_tests)
set_target_properties(drs_curated_dsp_scoped_routing_tests PROPERTIES FOLDER "Tests")

juce_add_console_app(drs_curated_dsp_s0_baseline_report
    PRODUCT_NAME "Decent Rhapsody Studio Curated DSP S0 Baseline Report"
)

target_sources(drs_curated_dsp_s0_baseline_report
    PRIVATE
        src/CuratedDspS0BaselineReport.cpp
)

target_link_libraries(drs_curated_dsp_s0_baseline_report
    PRIVATE
        drs_plugin_shell
        juce::juce_audio_processors_headless
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_compile_features(drs_curated_dsp_s0_baseline_report PRIVATE cxx_std_17)

add_test(
    NAME drs.curated_dsp.s0_baseline
    COMMAND drs_curated_dsp_s0_baseline_report "${CMAKE_CURRENT_BINARY_DIR}/curated-dsp-s0-baseline.json"
)

set_target_properties(drs_curated_dsp_s0_baseline_report PROPERTIES
    FOLDER "Tests"
)

add_executable(drs_sprint4_offline_renderer_tests
    src/Sprint4OfflineRendererTests.cpp
    support/Sprint4OfflineRenderHarness.cpp
    support/Sprint4OfflineRenderHarness.h
)

target_link_libraries(drs_sprint4_offline_renderer_tests
    PRIVATE
        drs_sampler_core
)

target_include_directories(drs_sprint4_offline_renderer_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/support
)

target_compile_definitions(drs_sprint4_offline_renderer_tests
    PRIVATE
        DRS_SPRINT4_OFFLINE_BASELINE_PATH="${CMAKE_CURRENT_SOURCE_DIR}/baselines/sprint4-offline-render-baselines.txt"
)

target_compile_features(drs_sprint4_offline_renderer_tests PRIVATE cxx_std_17)

add_test(
    NAME drs.sprint4.offline_renderer
    COMMAND drs_sprint4_offline_renderer_tests
)

# Sprint 4.8 crossfade route selection, diagnostics, and RR overlap matrix.
add_executable(drs_sprint4_crossfade_mixing_tests
    src/Sprint4CrossfadeMixingTests.cpp
)

target_link_libraries(drs_sprint4_crossfade_mixing_tests
    PRIVATE
        drs_sampler_core
)

target_compile_features(drs_sprint4_crossfade_mixing_tests PRIVATE cxx_std_17)

add_test(
    NAME drs.sprint4.crossfade_mixing
    COMMAND drs_sprint4_crossfade_mixing_tests
)

# Sprint 5.1 frozen Preview scope/lifecycle/note-policy contract.
add_executable(drs_sprint5_preview_contract_tests
    src/Sprint5PreviewContractTests.cpp
)

target_link_libraries(drs_sprint5_preview_contract_tests
    PRIVATE
        drs_engine_adapter
)

target_compile_features(drs_sprint5_preview_contract_tests PRIVATE cxx_std_17)

add_test(
    NAME drs.sprint5.preview_contract
    COMMAND drs_sprint5_preview_contract_tests
)

add_executable(drs_sprint5_preview_controller_tests
    src/Sprint5PreviewControllerTests.cpp
)

target_link_libraries(drs_sprint5_preview_controller_tests
    PRIVATE
        drs_engine_adapter
)

target_compile_features(drs_sprint5_preview_controller_tests PRIVATE cxx_std_17)

add_test(
    NAME drs.sprint5.preview_controller
    COMMAND drs_sprint5_preview_controller_tests
)

juce_add_console_app(drs_sprint5_preview_controller_integration_tests
    PRODUCT_NAME "Decent Rhapsody Studio Sprint 5 Preview Controller Integration Tests"
)

target_sources(drs_sprint5_preview_controller_integration_tests
    PRIVATE
        src/Sprint5PreviewControllerIntegrationTests.cpp
)

target_link_libraries(drs_sprint5_preview_controller_integration_tests
    PRIVATE
        drs_plugin_shell
        juce::juce_audio_processors_headless
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_compile_features(drs_sprint5_preview_controller_integration_tests PRIVATE cxx_std_17)

add_test(
    NAME drs.sprint5.preview_controller_integration
    COMMAND drs_sprint5_preview_controller_integration_tests
)

juce_add_console_app(drs_sprint5_preview_coalescing_tests
    PRODUCT_NAME "Decent Rhapsody Studio Sprint 5 Preview Coalescing Tests"
)

target_sources(drs_sprint5_preview_coalescing_tests
    PRIVATE
        src/Sprint5PreviewCoalescingTests.cpp
)

target_link_libraries(drs_sprint5_preview_coalescing_tests
    PRIVATE
        drs_sampler_core
        drs_sample_import
        juce::juce_audio_formats
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_compile_features(drs_sprint5_preview_coalescing_tests PRIVATE cxx_std_17)

add_test(
    NAME drs.sprint5.preview_coalescing
    COMMAND drs_sprint5_preview_coalescing_tests
)

juce_add_console_app(drs_sprint5_preview_preparation_tests
    PRODUCT_NAME "Decent Rhapsody Studio Sprint 5 Preview Preparation Tests"
)

target_sources(drs_sprint5_preview_preparation_tests
    PRIVATE
        src/Sprint5PreviewPreparationTests.cpp
)

target_link_libraries(drs_sprint5_preview_preparation_tests
    PRIVATE
        drs_sampler_core
        drs_sample_import
        juce::juce_audio_formats
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_compile_features(drs_sprint5_preview_preparation_tests PRIVATE cxx_std_17)

add_test(
    NAME drs.sprint5.preview_preparation
    COMMAND drs_sprint5_preview_preparation_tests
)

juce_add_console_app(drs_sprint5_preview_audition_tests
    PRODUCT_NAME "Decent Rhapsody Studio Sprint 5 Preview Audition Tests"
)

target_sources(drs_sprint5_preview_audition_tests
    PRIVATE
        src/Sprint5PreviewAuditionTests.cpp
)

target_link_libraries(drs_sprint5_preview_audition_tests
    PRIVATE
        drs_plugin_shell
        juce::juce_audio_processors_headless
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_compile_features(drs_sprint5_preview_audition_tests PRIVATE cxx_std_17)

add_test(
    NAME drs.sprint5.preview_audition
    COMMAND drs_sprint5_preview_audition_tests
)

juce_add_console_app(drs_sprint5_preview_recovery_tests
    PRODUCT_NAME "Decent Rhapsody Studio Sprint 5 Preview Recovery Tests"
)

target_sources(drs_sprint5_preview_recovery_tests
    PRIVATE
        src/Sprint5PreviewRecoveryTests.cpp
)

target_link_libraries(drs_sprint5_preview_recovery_tests
    PRIVATE
        drs_plugin_shell
        juce::juce_audio_processors_headless
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_compile_features(drs_sprint5_preview_recovery_tests PRIVATE cxx_std_17)

add_test(
    NAME drs.sprint5.preview_recovery
    COMMAND drs_sprint5_preview_recovery_tests
)

juce_add_console_app(drs_sprint5_preview_shell_parity_tests
    PRODUCT_NAME "Decent Rhapsody Studio Sprint 5 Preview Shell Parity Tests"
)

target_sources(drs_sprint5_preview_shell_parity_tests
    PRIVATE
        src/Sprint5PreviewShellParityTests.cpp
)

target_link_libraries(drs_sprint5_preview_shell_parity_tests
    PRIVATE
        drs_plugin_shell
        drs_standalone_shell
        juce::juce_audio_processors_headless
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_compile_features(drs_sprint5_preview_shell_parity_tests PRIVATE cxx_std_17)

add_test(
    NAME drs.sprint5.preview_shell_parity
    COMMAND drs_sprint5_preview_shell_parity_tests
)

# Permanent source-level regression audit for Preview replacement seams retired by Sprint 5.
add_executable(drs_sprint5_preview_contract_red_tests
    src/Sprint5PreviewContractRedTests.cpp
)

target_compile_definitions(drs_sprint5_preview_contract_red_tests
    PRIVATE
        DRS_SOURCE_ROOT="${CMAKE_SOURCE_DIR}"
)

target_compile_features(drs_sprint5_preview_contract_red_tests PRIVATE cxx_std_17)

add_test(
    NAME drs.sprint5.preview_contract_seams
    COMMAND drs_sprint5_preview_contract_red_tests
)

juce_add_console_app(drs_sprint5_integration_hardening_tests
    PRODUCT_NAME "Decent Rhapsody Studio Sprint 5 Integration Hardening Tests"
)

target_sources(drs_sprint5_integration_hardening_tests
    PRIVATE
        src/Sprint5IntegrationHardeningTests.cpp
        support/Sprint4OfflineRenderHarness.cpp
        support/Sprint4OfflineRenderHarness.h
)

target_compile_definitions(drs_sprint5_integration_hardening_tests
    PRIVATE
        DRS_SOURCE_ROOT="${CMAKE_SOURCE_DIR}"
)

target_include_directories(drs_sprint5_integration_hardening_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/support
)

target_link_libraries(drs_sprint5_integration_hardening_tests
    PRIVATE
        drs_plugin_shell
        juce::juce_audio_processors_headless
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_compile_features(drs_sprint5_integration_hardening_tests PRIVATE cxx_std_17)

add_test(
    NAME drs.sprint5.integration_hardening
    COMMAND drs_sprint5_integration_hardening_tests
)

# Sprint 6.1 frozen Publish command/identity/lifecycle/voice/macro contract.
add_executable(drs_sprint6_publish_contract_tests
    src/Sprint6PublishContractTests.cpp
)

target_link_libraries(drs_sprint6_publish_contract_tests
    PRIVATE
        drs_engine_adapter
)

target_compile_features(drs_sprint6_publish_contract_tests PRIVATE cxx_std_17)

add_test(
    NAME drs.sprint6.publish_contract
    COMMAND drs_sprint6_publish_contract_tests
)

# Sprint 3.1.1 frozen SFZ analyze/review/commit contract.
add_executable(drs_sprint31_sfz_contract_tests
    src/Sprint31SfzContractTests.cpp
)

target_link_libraries(drs_sprint31_sfz_contract_tests
    PRIVATE
        drs_engine_adapter
)

target_compile_features(drs_sprint31_sfz_contract_tests PRIVATE cxx_std_17)

add_test(
    NAME drs.sprint31.sfz_contract
    COMMAND drs_sprint31_sfz_contract_tests
)

add_executable(drs_sprint31_sfz_fixture_characterization_tests
    src/Sprint31SfzFixtureCharacterizationTests.cpp
)

target_link_libraries(drs_sprint31_sfz_fixture_characterization_tests
    PRIVATE
        drs_engine_adapter
)

target_compile_definitions(drs_sprint31_sfz_fixture_characterization_tests
    PRIVATE
        DRS_SOURCE_ROOT="${CMAKE_SOURCE_DIR}"
)

target_compile_features(drs_sprint31_sfz_fixture_characterization_tests PRIVATE cxx_std_17)

add_test(
    NAME drs.sprint31.sfz_fixture_profile
    COMMAND drs_sprint31_sfz_fixture_characterization_tests
)

add_executable(drs_sprint31_sfz_parser_tests
    src/Sprint31SfzParserTests.cpp
)

target_link_libraries(drs_sprint31_sfz_parser_tests
    PRIVATE
        drs_engine_adapter
)

target_compile_definitions(drs_sprint31_sfz_parser_tests
    PRIVATE
        DRS_SOURCE_ROOT="${CMAKE_SOURCE_DIR}"
)

target_compile_features(drs_sprint31_sfz_parser_tests PRIVATE cxx_std_17)

add_test(
    NAME drs.sprint31.sfz_parser
    COMMAND drs_sprint31_sfz_parser_tests
)

add_executable(drs_sprint31_sfz_cancellation_tests
    src/SfzImportCancellationTests.cpp
)

target_link_libraries(drs_sprint31_sfz_cancellation_tests
    PRIVATE
        drs_engine_adapter
)

target_compile_definitions(drs_sprint31_sfz_cancellation_tests
    PRIVATE
        DRS_SOURCE_ROOT="${CMAKE_SOURCE_DIR}"
)

target_compile_features(drs_sprint31_sfz_cancellation_tests PRIVATE cxx_std_17)

add_test(
    NAME drs.sprint31.sfz_cancellation
    COMMAND drs_sprint31_sfz_cancellation_tests
)

add_executable(drs_sprint31_sfz_normalization_tests
    src/Sprint31SfzNormalizationTests.cpp
)

target_link_libraries(drs_sprint31_sfz_normalization_tests
    PRIVATE
        drs_engine_adapter
)

target_compile_features(drs_sprint31_sfz_normalization_tests PRIVATE cxx_std_17)

add_test(
    NAME drs.sprint31.sfz_normalization
    COMMAND drs_sprint31_sfz_normalization_tests
)

add_executable(drs_sprint31_sfz_compatibility_tests
    src/Sprint31SfzCompatibilityTests.cpp
)

target_link_libraries(drs_sprint31_sfz_compatibility_tests
    PRIVATE
        drs_engine_adapter
)

target_compile_definitions(drs_sprint31_sfz_compatibility_tests
    PRIVATE
        DRS_SOURCE_ROOT="${CMAKE_SOURCE_DIR}"
)

target_compile_features(drs_sprint31_sfz_compatibility_tests PRIVATE cxx_std_17)

add_test(
    NAME drs.sprint31.sfz_compatibility
    COMMAND drs_sprint31_sfz_compatibility_tests
)

add_executable(drs_sprint31_sfz_report_model_tests
    src/Sprint31SfzReportModelTests.cpp
)

target_link_libraries(drs_sprint31_sfz_report_model_tests
    PRIVATE
        drs_app_shared
        drs_sample_import
        drs_sampler_core
)

target_compile_definitions(drs_sprint31_sfz_report_model_tests
    PRIVATE
        DRS_SOURCE_ROOT="${CMAKE_SOURCE_DIR}"
)

target_compile_features(drs_sprint31_sfz_report_model_tests PRIVATE cxx_std_17)

add_test(
    NAME drs.sprint31.sfz_report_model
    COMMAND drs_sprint31_sfz_report_model_tests
)

add_executable(drs_sprint31_sfz_projection_tests
    src/Sprint31SfzProjectionTests.cpp
)

target_link_libraries(drs_sprint31_sfz_projection_tests
    PRIVATE
        drs_app_shared
        drs_engine_adapter
)

target_compile_definitions(drs_sprint31_sfz_projection_tests
    PRIVATE
        DRS_SOURCE_ROOT="${CMAKE_SOURCE_DIR}"
)

target_compile_features(drs_sprint31_sfz_projection_tests PRIVATE cxx_std_17)

add_test(
    NAME drs.sprint31.sfz_projection
    COMMAND drs_sprint31_sfz_projection_tests
)

juce_add_console_app(drs_sprint31_sfz_runtime_playback_tests
    PRODUCT_NAME "Decent Rhapsody Studio Sprint 3.1 SFZ Runtime Playback Tests"
)

target_sources(drs_sprint31_sfz_runtime_playback_tests
    PRIVATE
        src/Sprint31SfzRuntimePlaybackTests.cpp
)

target_link_libraries(drs_sprint31_sfz_runtime_playback_tests
    PRIVATE
        drs_app_shared
        drs_plugin_shell
        juce::juce_audio_processors_headless
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_compile_definitions(drs_sprint31_sfz_runtime_playback_tests
    PRIVATE
        DRS_SOURCE_ROOT="${CMAKE_SOURCE_DIR}"
)

target_compile_features(drs_sprint31_sfz_runtime_playback_tests PRIVATE cxx_std_17)

add_test(
    NAME drs.sprint31.sfz_runtime_playback
    COMMAND drs_sprint31_sfz_runtime_playback_tests
)

add_executable(drs_sprint31_sfz_review_ui_tests
    src/Sprint31SfzReviewUiTests.cpp
)

target_link_libraries(drs_sprint31_sfz_review_ui_tests
    PRIVATE
        drs_app_shared
        drs_engine_adapter
)

target_compile_definitions(drs_sprint31_sfz_review_ui_tests
    PRIVATE
        DRS_SOURCE_ROOT="${CMAKE_SOURCE_DIR}"
)

target_compile_features(drs_sprint31_sfz_review_ui_tests PRIVATE cxx_std_17)

add_test(
    NAME drs.sprint31.sfz_review_ui
    COMMAND drs_sprint31_sfz_review_ui_tests
)

add_executable(drs_sprint31_sfz_corpus_hardening_tests
    src/Sprint31SfzCorpusHardeningTests.cpp
)

target_link_libraries(drs_sprint31_sfz_corpus_hardening_tests
    PRIVATE
        drs_app_shared
        drs_engine_adapter
)

target_compile_definitions(drs_sprint31_sfz_corpus_hardening_tests
    PRIVATE
        DRS_SOURCE_ROOT="${CMAKE_SOURCE_DIR}"
)

target_compile_features(drs_sprint31_sfz_corpus_hardening_tests PRIVATE cxx_std_17)

add_test(
    NAME drs.sprint31.sfz_corpus_hardening
    COMMAND drs_sprint31_sfz_corpus_hardening_tests
)

add_executable(drs_sprint31_sfz_determinism_tests
    src/Sprint31SfzDeterminismTests.cpp
)

target_link_libraries(drs_sprint31_sfz_determinism_tests
    PRIVATE
        drs_app_shared
        drs_engine_adapter
)

target_compile_definitions(drs_sprint31_sfz_determinism_tests
    PRIVATE
        DRS_SOURCE_ROOT="${CMAKE_SOURCE_DIR}"
)

target_compile_features(drs_sprint31_sfz_determinism_tests PRIVATE cxx_std_17)

add_test(
    NAME drs.sprint31.sfz_determinism
    COMMAND drs_sprint31_sfz_determinism_tests
)

add_executable(drs_sprint0_import_storage_hardening_tests
    src/Sprint0ImportAndStorageHardeningTests.cpp
)

target_link_libraries(drs_sprint0_import_storage_hardening_tests
    PRIVATE
        drs_app_shared
        drs_engine_adapter
)

target_compile_definitions(drs_sprint0_import_storage_hardening_tests
    PRIVATE
        DRS_SOURCE_ROOT="${CMAKE_SOURCE_DIR}"
)

target_compile_features(drs_sprint0_import_storage_hardening_tests PRIVATE cxx_std_17)

add_test(
    NAME drs.sprint0.import_storage_hardening
    COMMAND drs_sprint0_import_storage_hardening_tests
)

add_executable(drs_sfz_import_lifecycle_tests
    src/SfzImportLifecycleTests.cpp
)

target_link_libraries(drs_sfz_import_lifecycle_tests
    PRIVATE
        drs_app_shared
        drs_engine_adapter
)

target_compile_definitions(drs_sfz_import_lifecycle_tests
    PRIVATE
        DRS_SOURCE_ROOT="${CMAKE_SOURCE_DIR}"
)

target_compile_features(drs_sfz_import_lifecycle_tests PRIVATE cxx_std_17)

add_test(
    NAME drs.sfz_import.lifecycle
    COMMAND drs_sfz_import_lifecycle_tests
)

add_executable(drs_sfz_import_shell_lifecycle_tests
    src/SfzImportShellLifecycleTests.cpp
)
target_compile_definitions(drs_sfz_import_shell_lifecycle_tests PRIVATE DRS_SOURCE_ROOT="${CMAKE_SOURCE_DIR}")
target_compile_features(drs_sfz_import_shell_lifecycle_tests PRIVATE cxx_std_17)
add_test(NAME drs.sfz_import.shell_lifecycle COMMAND drs_sfz_import_shell_lifecycle_tests)

add_executable(drs_wav_import_service_contract_tests
    src/WavImportServiceContractTests.cpp
)

target_link_libraries(drs_wav_import_service_contract_tests
    PRIVATE
        drs_app_shared
        drs_engine_adapter
)

target_compile_features(drs_wav_import_service_contract_tests PRIVATE cxx_std_17)

add_test(
    NAME drs.wav_import.service_contract
    COMMAND drs_wav_import_service_contract_tests
)

add_executable(drs_wav_import_lifecycle_tests
    src/WavImportServiceLifecycleTests.cpp
)

target_link_libraries(drs_wav_import_lifecycle_tests
    PRIVATE
        drs_app_shared
)

target_compile_features(drs_wav_import_lifecycle_tests PRIVATE cxx_std_17)
target_sources(drs_wav_import_lifecycle_tests
    PRIVATE
        support/WavImportTestSupport.cpp
        support/WavImportTestSupport.h
)

add_test(
    NAME drs.wav_import.lifecycle
    COMMAND drs_wav_import_lifecycle_tests
)

add_executable(drs_wav_import_staging_tests
    src/WavImportServiceStagingTests.cpp
)

target_link_libraries(drs_wav_import_staging_tests
    PRIVATE
        drs_app_shared
        drs_engine_adapter
)

target_compile_features(drs_wav_import_staging_tests PRIVATE cxx_std_17)
target_sources(drs_wav_import_staging_tests
    PRIVATE
        support/WavImportTestSupport.cpp
        support/WavImportTestSupport.h
)

add_test(
    NAME drs.wav_import.staging
    COMMAND drs_wav_import_staging_tests
)

add_executable(drs_wav_import_analysis_tests
    src/WavImportServiceAnalysisTests.cpp
)

target_link_libraries(drs_wav_import_analysis_tests
    PRIVATE
        drs_app_shared
        drs_engine_adapter
)

target_compile_features(drs_wav_import_analysis_tests PRIVATE cxx_std_17)
target_sources(drs_wav_import_analysis_tests
    PRIVATE
        support/WavImportTestSupport.cpp
        support/WavImportTestSupport.h
)

add_test(
    NAME drs.wav_import.analysis
    COMMAND drs_wav_import_analysis_tests
)

add_executable(drs_wav_import_workflow_tests
    src/WavImportWorkflowTests.cpp
)

target_link_libraries(drs_wav_import_workflow_tests
    PRIVATE
        drs_app_shared
        drs_engine_adapter
)

target_compile_features(drs_wav_import_workflow_tests PRIVATE cxx_std_17)
target_sources(drs_wav_import_workflow_tests
    PRIVATE
        support/WavImportTestSupport.cpp
        support/WavImportTestSupport.h
)

add_test(
    NAME drs.wav_import.workflow
    COMMAND drs_wav_import_workflow_tests
)

juce_add_console_app(drs_wav_import_progress_tests
    PRODUCT_NAME "Decent Rhapsody Studio WAV Import Progress Tests"
)

target_sources(drs_wav_import_progress_tests
    PRIVATE
        src/WavImportProgressTests.cpp
)

target_link_libraries(drs_wav_import_progress_tests
    PRIVATE
        drs_app_shared
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_compile_features(drs_wav_import_progress_tests PRIVATE cxx_std_17)

add_test(
    NAME drs.wav_import.progress
    COMMAND drs_wav_import_progress_tests
)

juce_add_console_app(drs_wav_import_processor_responsiveness_tests
    PRODUCT_NAME "Decent Rhapsody Studio WAV Import Processor Responsiveness Tests"
)

target_sources(drs_wav_import_processor_responsiveness_tests
    PRIVATE
        src/WavImportProcessorResponsivenessTests.cpp
        support/WavImportTestSupport.cpp
)

target_link_libraries(drs_wav_import_processor_responsiveness_tests
    PRIVATE
        drs_plugin_shell
        drs_sample_import
        juce::juce_audio_processors_headless
        juce::juce_audio_formats
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_compile_features(drs_wav_import_processor_responsiveness_tests PRIVATE cxx_std_17)

target_include_directories(drs_wav_import_processor_responsiveness_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/support
        ${CMAKE_SOURCE_DIR}/third_party/juce/modules
)

add_test(
    NAME drs.wav_import.processor_responsiveness
    COMMAND drs_wav_import_processor_responsiveness_tests
)

juce_add_console_app(drs_wav_import_lifecycle_io_audit_tests
    PRODUCT_NAME "Decent Rhapsody Studio WAV Import Lifecycle IO Audit Tests"
)

target_sources(drs_wav_import_lifecycle_io_audit_tests
    PRIVATE
        src/WavImportLifecycleIoAuditTests.cpp
)

target_link_libraries(drs_wav_import_lifecycle_io_audit_tests
    PRIVATE
        drs_plugin_shell
        drs_standalone_shell
        juce::juce_audio_processors_headless
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_compile_features(drs_wav_import_lifecycle_io_audit_tests PRIVATE cxx_std_17)

add_test(
    NAME drs.wav_import.lifecycle_io_audit
    COMMAND drs_wav_import_lifecycle_io_audit_tests
)

juce_add_console_app(drs_wav_import_lifecycle_stress_tests
    PRODUCT_NAME "Decent Rhapsody Studio WAV Import Lifecycle Stress Tests"
)

target_sources(drs_wav_import_lifecycle_stress_tests
    PRIVATE
        src/WavImportLifecycleStressTests.cpp
        support/WavImportTestSupport.cpp
)

target_link_libraries(drs_wav_import_lifecycle_stress_tests
    PRIVATE
        drs_plugin_shell
        drs_sample_import
        juce::juce_audio_processors_headless
        juce::juce_audio_formats
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_compile_features(drs_wav_import_lifecycle_stress_tests PRIVATE cxx_std_17)

target_include_directories(drs_wav_import_lifecycle_stress_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/support
        ${CMAKE_SOURCE_DIR}/third_party/juce/modules
)

add_test(
    NAME drs.wav_import.lifecycle_stress
    COMMAND drs_wav_import_lifecycle_stress_tests
)

juce_add_console_app(drs_wav_import_ci_budget_tests
    PRODUCT_NAME "Decent Rhapsody Studio WAV Import CI Budget Tests"
)

target_sources(drs_wav_import_ci_budget_tests
    PRIVATE
        src/WavImportCiBudgetTests.cpp
        support/WavImportTestSupport.cpp
)

target_link_libraries(drs_wav_import_ci_budget_tests
    PRIVATE
        drs_plugin_shell
        drs_sample_import
        juce::juce_audio_processors_headless
        juce::juce_audio_formats
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_compile_features(drs_wav_import_ci_budget_tests PRIVATE cxx_std_17)

target_include_directories(drs_wav_import_ci_budget_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/support
        ${CMAKE_SOURCE_DIR}/third_party/juce/modules
)

add_test(
    NAME drs.wav_import.ci_budgets
    COMMAND drs_wav_import_ci_budget_tests
)

juce_add_console_app(drs_wav_import_host_validation_tests
    PRODUCT_NAME "Decent Rhapsody Studio WAV Import Host Validation Tests"
)

target_sources(drs_wav_import_host_validation_tests
    PRIVATE
        src/WavImportHostValidationTests.cpp
)

target_link_libraries(drs_wav_import_host_validation_tests
    PRIVATE
        drs_plugin_shell
        drs_standalone_shell
        juce::juce_audio_processors_headless
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_compile_features(drs_wav_import_host_validation_tests PRIVATE cxx_std_17)

add_test(
    NAME drs.wav_import.host_validation
    COMMAND drs_wav_import_host_validation_tests
)

juce_add_console_app(drs_project_source_validation_service_tests
    PRODUCT_NAME "Decent Rhapsody Studio Project Source Validation Service Tests"
)

target_sources(drs_project_source_validation_service_tests
    PRIVATE
        src/ProjectSourceValidationServiceTests.cpp
        support/WavImportTestSupport.cpp
)

target_link_libraries(drs_project_source_validation_service_tests
    PRIVATE
        drs_app_shared
        drs_sample_import
        juce::juce_audio_formats
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_compile_features(drs_project_source_validation_service_tests PRIVATE cxx_std_17)

target_include_directories(drs_project_source_validation_service_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/support
        ${CMAKE_SOURCE_DIR}/third_party/juce/modules
)

add_test(
    NAME drs.wav_import.source_validation_service
    COMMAND drs_project_source_validation_service_tests
)

juce_add_console_app(drs_waveform_peak_builder_tests
    PRODUCT_NAME "Decent Rhapsody Studio Waveform Peak Builder Tests"
)

target_sources(drs_waveform_peak_builder_tests
    PRIVATE
        src/WaveformPeakBuilderTests.cpp
        support/WavImportTestSupport.cpp
)

target_link_libraries(drs_waveform_peak_builder_tests
    PRIVATE
        drs_engine_adapter
        drs_sample_import
        juce::juce_audio_formats
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_compile_features(drs_waveform_peak_builder_tests PRIVATE cxx_std_17)

target_include_directories(drs_waveform_peak_builder_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/support
        ${CMAKE_SOURCE_DIR}/third_party/juce/modules
)

add_test(
    NAME drs.wav_import.waveform_peak_builder
    COMMAND drs_waveform_peak_builder_tests
)

juce_add_console_app(drs_waveform_preview_service_tests
    PRODUCT_NAME "Decent Rhapsody Studio Waveform Preview Service Tests"
)

target_sources(drs_waveform_preview_service_tests
    PRIVATE
        src/WaveformPreviewServiceTests.cpp
        support/WavImportTestSupport.cpp
)

target_link_libraries(drs_waveform_preview_service_tests
    PRIVATE
        drs_app_shared
        drs_sample_import
        juce::juce_audio_formats
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_compile_features(drs_waveform_preview_service_tests PRIVATE cxx_std_17)

target_include_directories(drs_waveform_preview_service_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/support
        ${CMAKE_SOURCE_DIR}/third_party/juce/modules
)

add_test(
    NAME drs.wav_import.waveform_preview_service
    COMMAND drs_waveform_preview_service_tests
)

# Phase 3 crossfade Sprint 1 contract and fixture baseline.
add_executable(drs_phase3_crossfade_contract_tests
    src/Phase3CrossfadeContractTests.cpp
)

target_link_libraries(drs_phase3_crossfade_contract_tests
    PRIVATE
        drs_engine_adapter
)

target_compile_features(drs_phase3_crossfade_contract_tests PRIVATE cxx_std_17)

add_test(
    NAME drs.phase3.crossfade_contract
    COMMAND drs_phase3_crossfade_contract_tests
)

add_executable(drs_velocity_crossfade_authoring_tests
    src/VelocityCrossfadeAuthoringTests.cpp
)

target_link_libraries(drs_velocity_crossfade_authoring_tests
    PRIVATE
        drs_engine_adapter
)

target_compile_features(drs_velocity_crossfade_authoring_tests PRIVATE cxx_std_17)

add_test(
    NAME drs.velocity_crossfade.authoring
    COMMAND drs_velocity_crossfade_authoring_tests
)

add_executable(drs_velocity_crossfade_stack_authoring_tests
    src/VelocityCrossfadeStackAuthoringTests.cpp
)

target_link_libraries(drs_velocity_crossfade_stack_authoring_tests
    PRIVATE
        drs_engine_adapter
)

target_compile_features(drs_velocity_crossfade_stack_authoring_tests PRIVATE cxx_std_17)

add_test(
    NAME drs.velocity_crossfade.stack_authoring
    COMMAND drs_velocity_crossfade_stack_authoring_tests
)

add_executable(drs_velocity_crossfade_inspector_ui_tests
    src/VelocityCrossfadeInspectorUiTests.cpp
)

target_link_libraries(drs_velocity_crossfade_inspector_ui_tests
    PRIVATE
        drs_app_shared
        drs_engine_adapter
)

target_compile_features(drs_velocity_crossfade_inspector_ui_tests PRIVATE cxx_std_17)

add_test(
    NAME drs.velocity_crossfade.inspector_ui
    COMMAND drs_velocity_crossfade_inspector_ui_tests
)

add_executable(drs_velocity_crossfade_zone_map_tests
    src/VelocityCrossfadeZoneMapTests.cpp
)

target_link_libraries(drs_velocity_crossfade_zone_map_tests
    PRIVATE
        drs_app_shared
        drs_engine_adapter
)

target_compile_features(drs_velocity_crossfade_zone_map_tests PRIVATE cxx_std_17)

add_test(
    NAME drs.velocity_crossfade.zone_map
    COMMAND drs_velocity_crossfade_zone_map_tests
)

add_executable(drs_phase3_crossfade_fixture_characterization_tests
    src/Phase3CrossfadeFixtureCharacterizationTests.cpp
)

target_link_libraries(drs_phase3_crossfade_fixture_characterization_tests
    PRIVATE
        drs_engine_adapter
)

target_compile_definitions(drs_phase3_crossfade_fixture_characterization_tests
    PRIVATE
        DRS_SOURCE_ROOT="${CMAKE_SOURCE_DIR}"
)

target_compile_features(drs_phase3_crossfade_fixture_characterization_tests PRIVATE cxx_std_17)

add_test(
    NAME drs.phase3.crossfade_fixture_profile
    COMMAND drs_phase3_crossfade_fixture_characterization_tests
)

add_executable(drs_phase3_crossfade_persistence_tests
    src/Phase3CrossfadePersistenceTests.cpp
)

target_link_libraries(drs_phase3_crossfade_persistence_tests
    PRIVATE
        drs_app_shared
        drs_engine_adapter
)

target_compile_features(drs_phase3_crossfade_persistence_tests PRIVATE cxx_std_17)

add_test(
    NAME drs.phase3.crossfade_persistence
    COMMAND drs_phase3_crossfade_persistence_tests
)

add_executable(drs_phase3_crossfade_prepared_playback_tests
    src/Phase3CrossfadePreparedPlaybackTests.cpp
)

target_link_libraries(drs_phase3_crossfade_prepared_playback_tests
    PRIVATE
        drs_engine_adapter
        drs_sample_import
        juce::juce_audio_formats
)

target_compile_features(drs_phase3_crossfade_prepared_playback_tests PRIVATE cxx_std_17)

target_include_directories(drs_phase3_crossfade_prepared_playback_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/third_party/juce/modules
)

add_test(
    NAME drs.phase3.crossfade_prepared_playback
    COMMAND drs_phase3_crossfade_prepared_playback_tests
)

# Phase 3 Round Robin Sprint 1 contract and fixture baseline.
add_executable(drs_phase3_round_robin_contract_tests
    src/Phase3RoundRobinContractTests.cpp
)

target_link_libraries(drs_phase3_round_robin_contract_tests
    PRIVATE
        drs_engine_adapter
)

target_compile_features(drs_phase3_round_robin_contract_tests PRIVATE cxx_std_17)

add_test(
    NAME drs.phase3.round_robin_contract
    COMMAND drs_phase3_round_robin_contract_tests
)

# Phase 1 Zone Groups Sprint 1 contract and migration baseline.
add_executable(drs_phase1_zone_groups_contract_tests
    src/Phase1ZoneGroupsContractTests.cpp
)

target_link_libraries(drs_phase1_zone_groups_contract_tests
    PRIVATE
        drs_engine_adapter
)

target_compile_features(drs_phase1_zone_groups_contract_tests PRIVATE cxx_std_17)

add_test(
    NAME drs.phase1.zone_groups_contract
    COMMAND drs_phase1_zone_groups_contract_tests
)

add_executable(drs_phase1_zone_groups_schema_persistence_tests
    src/Phase1ZoneGroupsSchemaPersistenceTests.cpp
)

target_link_libraries(drs_phase1_zone_groups_schema_persistence_tests
    PRIVATE
        drs_app_shared
        drs_engine_adapter
)

target_compile_features(drs_phase1_zone_groups_schema_persistence_tests PRIVATE cxx_std_17)

add_test(
    NAME drs.phase1.zone_groups_schema_persistence
    COMMAND drs_phase1_zone_groups_schema_persistence_tests
)

add_executable(drs_phase3_round_robin_fixture_characterization_tests
    src/Phase3RoundRobinFixtureCharacterizationTests.cpp
)

target_compile_definitions(drs_phase3_round_robin_fixture_characterization_tests
    PRIVATE
        DRS_SOURCE_ROOT="${CMAKE_SOURCE_DIR}"
)

target_compile_features(drs_phase3_round_robin_fixture_characterization_tests PRIVATE cxx_std_17)

add_test(
    NAME drs.phase3.round_robin_fixture_profile
    COMMAND drs_phase3_round_robin_fixture_characterization_tests
)

add_executable(drs_phase3_round_robin_schema_persistence_tests
    src/Phase3RoundRobinSchemaPersistenceTests.cpp
)

target_link_libraries(drs_phase3_round_robin_schema_persistence_tests
    PRIVATE
        drs_app_shared
        drs_engine_adapter
)

target_compile_features(drs_phase3_round_robin_schema_persistence_tests PRIVATE cxx_std_17)

add_test(
    NAME drs.phase3.round_robin_schema_persistence
    COMMAND drs_phase3_round_robin_schema_persistence_tests
)

add_executable(drs_phase3_round_robin_prepared_playback_tests
    src/Phase3RoundRobinPreparedPlaybackTests.cpp
)

target_link_libraries(drs_phase3_round_robin_prepared_playback_tests
    PRIVATE
        drs_engine_adapter
        drs_sampler_core
        drs_sample_import
        juce::juce_audio_formats
)

target_compile_features(drs_phase3_round_robin_prepared_playback_tests PRIVATE cxx_std_17)

add_test(
    NAME drs.phase3.round_robin_prepared_playback
    COMMAND drs_phase3_round_robin_prepared_playback_tests
)

juce_add_console_app(drs_phase3_round_robin_hardening_tests
    PRODUCT_NAME "Decent Rhapsody Studio Phase 3 Round Robin Sprint 8 Hardening Tests"
)

target_sources(drs_phase3_round_robin_hardening_tests
    PRIVATE
        src/Phase3RoundRobinHardeningTests.cpp
)

target_compile_definitions(drs_phase3_round_robin_hardening_tests
    PRIVATE
        DRS_SOURCE_ROOT="${CMAKE_SOURCE_DIR}"
)

target_link_libraries(drs_phase3_round_robin_hardening_tests
    PRIVATE
        drs_app_shared
        drs_engine_adapter
        drs_sampler_core
        drs_sample_import
        juce::juce_audio_formats
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_compile_features(drs_phase3_round_robin_hardening_tests PRIVATE cxx_std_17)

add_test(
    NAME drs.phase3.round_robin_hardening
    COMMAND drs_phase3_round_robin_hardening_tests
)

add_executable(drs_phase3_round_robin_contract_red_tests
    src/Phase3RoundRobinContractRedTests.cpp
)

target_compile_definitions(drs_phase3_round_robin_contract_red_tests
    PRIVATE
        DRS_SOURCE_ROOT="${CMAKE_SOURCE_DIR}"
)

target_compile_features(drs_phase3_round_robin_contract_red_tests PRIVATE cxx_std_17)

add_executable(drs_phase1_zone_groups_contract_red_tests
    src/Phase1ZoneGroupsContractRedTests.cpp
)

target_compile_definitions(drs_phase1_zone_groups_contract_red_tests
    PRIVATE
        DRS_SOURCE_ROOT="${CMAKE_SOURCE_DIR}"
)

target_compile_features(drs_phase1_zone_groups_contract_red_tests PRIVATE cxx_std_17)

# Direct-only expected-red audit for the parser/report/UI seams intentionally left open by Sprint 3.1.1.
add_executable(drs_sprint31_sfz_contract_red_tests
    src/Sprint31SfzContractRedTests.cpp
)

target_compile_definitions(drs_sprint31_sfz_contract_red_tests
    PRIVATE
        DRS_SOURCE_ROOT="${CMAKE_SOURCE_DIR}"
)

target_compile_features(drs_sprint31_sfz_contract_red_tests PRIVATE cxx_std_17)

# Permanent source-level regression audit for Publish replacement seams retired by Sprint 6.
add_executable(drs_sprint6_publish_contract_seam_tests
    src/Sprint6PublishContractSeamTests.cpp
)

target_compile_definitions(drs_sprint6_publish_contract_seam_tests
    PRIVATE
        DRS_SOURCE_ROOT="${CMAKE_SOURCE_DIR}"
)

target_compile_features(drs_sprint6_publish_contract_seam_tests PRIVATE cxx_std_17)

add_test(
    NAME drs.sprint6.publish_contract_seams
    COMMAND drs_sprint6_publish_contract_seam_tests
)

add_executable(drs_sprint6_publish_controller_tests
    src/Sprint6PublishControllerTests.cpp
)

target_link_libraries(drs_sprint6_publish_controller_tests
    PRIVATE
        drs_engine_adapter
)

target_compile_features(drs_sprint6_publish_controller_tests PRIVATE cxx_std_17)

add_test(
    NAME drs.sprint6.publish_controller
    COMMAND drs_sprint6_publish_controller_tests
)

juce_add_console_app(drs_sprint6_publish_controller_integration_tests
    PRODUCT_NAME "Decent Rhapsody Studio Sprint 6 Publish Controller Integration Tests"
)

target_sources(drs_sprint6_publish_controller_integration_tests
    PRIVATE
        src/Sprint6PublishControllerIntegrationTests.cpp
)

target_link_libraries(drs_sprint6_publish_controller_integration_tests
    PRIVATE
        drs_plugin_shell
        juce::juce_audio_processors_headless
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_compile_features(drs_sprint6_publish_controller_integration_tests PRIVATE cxx_std_17)

add_test(
    NAME drs.sprint6.publish_controller_integration
    COMMAND drs_sprint6_publish_controller_integration_tests
)

# Mini Sprint 6.3 all-or-nothing full-project immutable preparation conformance.
add_executable(drs_sprint6_performance_preparation_tests
    src/Sprint6PerformancePreparationTests.cpp
)

target_link_libraries(drs_sprint6_performance_preparation_tests
    PRIVATE
        drs_engine_adapter
        drs_sample_import
        juce::juce_audio_formats
)

target_compile_features(drs_sprint6_performance_preparation_tests PRIVATE cxx_std_17)

add_test(
    NAME drs.sprint6.performance_preparation
    COMMAND drs_sprint6_performance_preparation_tests
)

juce_add_console_app(drs_sprint6_publish_scheduling_tests
    PRODUCT_NAME "Decent Rhapsody Studio Sprint 6 Publish Scheduling Tests"
)

target_sources(drs_sprint6_publish_scheduling_tests
    PRIVATE
        src/Sprint6PublishSchedulingTests.cpp
)

target_link_libraries(drs_sprint6_publish_scheduling_tests
    PRIVATE
        drs_engine_adapter
        drs_sample_import
        juce::juce_audio_formats
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_compile_features(drs_sprint6_publish_scheduling_tests PRIVATE cxx_std_17)

add_test(
    NAME drs.sprint6.publish_scheduling
    COMMAND drs_sprint6_publish_scheduling_tests
)

juce_add_console_app(drs_sprint6_performance_activation_recovery_tests
    PRODUCT_NAME "Decent Rhapsody Studio Sprint 6 Performance Activation Recovery Tests"
)

target_sources(drs_sprint6_performance_activation_recovery_tests
    PRIVATE
        src/Sprint6PerformanceActivationRecoveryTests.cpp
)

target_link_libraries(drs_sprint6_performance_activation_recovery_tests
    PRIVATE
        drs_plugin_shell
        juce::juce_audio_processors_headless
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_compile_features(drs_sprint6_performance_activation_recovery_tests PRIVATE cxx_std_17)

add_test(
    NAME drs.sprint6.performance_activation_recovery
    COMMAND drs_sprint6_performance_activation_recovery_tests
)

juce_add_console_app(drs_sprint6_voice_generation_cutover_tests
    PRODUCT_NAME "Decent Rhapsody Studio Sprint 6 Voice Generation Cutover Tests"
)

target_sources(drs_sprint6_voice_generation_cutover_tests
    PRIVATE
        src/Sprint6VoiceGenerationCutoverTests.cpp
)

target_link_libraries(drs_sprint6_voice_generation_cutover_tests
    PRIVATE
        drs_plugin_shell
        juce::juce_audio_processors_headless
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_compile_features(drs_sprint6_voice_generation_cutover_tests PRIVATE cxx_std_17)

add_test(
    NAME drs.sprint6.voice_generation_cutover
    COMMAND drs_sprint6_voice_generation_cutover_tests
)

juce_add_console_app(drs_sprint6_published_macro_binding_tests
    PRODUCT_NAME "Decent Rhapsody Studio Sprint 6 Published Macro Binding Tests"
)

target_sources(drs_sprint6_published_macro_binding_tests
    PRIVATE
        src/Sprint6PublishedMacroBindingTests.cpp
)

target_link_libraries(drs_sprint6_published_macro_binding_tests
    PRIVATE
        drs_engine_adapter
        drs_sampler_core
        drs_plugin_shell
        juce::juce_audio_processors_headless
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_compile_features(drs_sprint6_published_macro_binding_tests PRIVATE cxx_std_17)

if(MSVC)
    target_link_options(drs_sprint6_published_macro_binding_tests PRIVATE /STACK:8388608)
endif()

add_test(
    NAME drs.sprint6.published_macro_binding
    COMMAND drs_sprint6_published_macro_binding_tests
)

# Performance mixer Sprint 0 introduced direct expected-red characterization.
# Sprints 1 through 3 promote the implemented topology, preflight, recovery,
# and immutable presentation seams into the regular test matrix.
juce_add_console_app(drs_performance_mixer_s0_red_tests
    PRODUCT_NAME "Decent Rhapsody Studio Performance Mixer Sprint 0 Red Tests"
)

target_sources(drs_performance_mixer_s0_red_tests
    PRIVATE
        src/PerformanceMixerSprint0RedTests.cpp
)

target_link_libraries(drs_performance_mixer_s0_red_tests
    PRIVATE
        drs_engine_adapter
        drs_sampler_core
        drs_plugin_shell
        drs_standalone_shell
        juce::juce_audio_processors_headless
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_compile_features(drs_performance_mixer_s0_red_tests PRIVATE cxx_std_17)

target_compile_definitions(drs_performance_mixer_s0_red_tests
    PRIVATE
        DRS_PERFORMANCE_MIXER_S0_FIXTURE_PATH="${CMAKE_SOURCE_DIR}/content/runtime/phase2/performance-mixer/three-layer-gain-mixer.drsproj"
)

foreach(DRS_PERFORMANCE_MIXER_S1_SEAM IN ITEMS
        binding-3-exposed
        binding-12-exposed
        binding-16-authored
        processor-topology-lifecycle)
    string(REPLACE "-" "_" DRS_PERFORMANCE_MIXER_S1_TEST_SUFFIX
           "${DRS_PERFORMANCE_MIXER_S1_SEAM}")
    add_test(
        NAME "drs.performance_mixer.s1.${DRS_PERFORMANCE_MIXER_S1_TEST_SUFFIX}"
        COMMAND drs_performance_mixer_s0_red_tests "${DRS_PERFORMANCE_MIXER_S1_SEAM}"
    )
endforeach()

foreach(DRS_PERFORMANCE_MIXER_S3_SEAM IN ITEMS
        published-presentation-model
        published-presentation-rename)
    string(REPLACE "-" "_" DRS_PERFORMANCE_MIXER_S3_TEST_SUFFIX
           "${DRS_PERFORMANCE_MIXER_S3_SEAM}")
    add_test(
        NAME "drs.performance_mixer.s3.${DRS_PERFORMANCE_MIXER_S3_TEST_SUFFIX}"
        COMMAND drs_performance_mixer_s0_red_tests "${DRS_PERFORMANCE_MIXER_S3_SEAM}"
    )
endforeach()

foreach(DRS_PERFORMANCE_MIXER_S5_SEAM IN ITEMS
        host-state-roundtrip
        automation-boundary-16-slots
        republish-churn-realtime)
    string(REPLACE "-" "_" DRS_PERFORMANCE_MIXER_S5_TEST_SUFFIX
           "${DRS_PERFORMANCE_MIXER_S5_SEAM}")
    add_test(
        NAME "drs.performance_mixer.s5.${DRS_PERFORMANCE_MIXER_S5_TEST_SUFFIX}"
        COMMAND drs_performance_mixer_s0_red_tests "${DRS_PERFORMANCE_MIXER_S5_SEAM}"
    )
endforeach()

foreach(DRS_PERFORMANCE_MIXER_S2_SEAM IN ITEMS
        overflow-13-exposed
        overflow-17-authored
        invalid-dsp-target
        failed-then-successful-recovery
        shell-diagnostic-parity)
    string(REPLACE "-" "_" DRS_PERFORMANCE_MIXER_S2_TEST_SUFFIX
           "${DRS_PERFORMANCE_MIXER_S2_SEAM}")
    add_test(
        NAME "drs.performance_mixer.s2.${DRS_PERFORMANCE_MIXER_S2_TEST_SUFFIX}"
        COMMAND drs_performance_mixer_s0_red_tests "${DRS_PERFORMANCE_MIXER_S2_SEAM}"
    )
endforeach()

juce_add_console_app(drs_sprint6_publish_shell_parity_tests
    PRODUCT_NAME "Decent Rhapsody Studio Sprint 6 Publish Shell Parity Tests"
)

target_sources(drs_sprint6_publish_shell_parity_tests
    PRIVATE
        src/Sprint6PublishShellParityTests.cpp
)

target_compile_definitions(drs_sprint6_publish_shell_parity_tests
    PRIVATE
        DRS_SOURCE_ROOT="${CMAKE_SOURCE_DIR}"
)

target_link_libraries(drs_sprint6_publish_shell_parity_tests
    PRIVATE
        drs_plugin_shell
        drs_standalone_shell
        juce::juce_audio_processors_headless
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_compile_features(drs_sprint6_publish_shell_parity_tests PRIVATE cxx_std_17)

add_test(
    NAME drs.sprint6.publish_shell_parity
    COMMAND drs_sprint6_publish_shell_parity_tests
)

juce_add_console_app(drs_sprint6_integration_hardening_tests
    PRODUCT_NAME "Decent Rhapsody Studio Sprint 6 Integration Hardening Tests"
)

target_sources(drs_sprint6_integration_hardening_tests
    PRIVATE
        src/Sprint6IntegrationHardeningTests.cpp
)

target_link_libraries(drs_sprint6_integration_hardening_tests
    PRIVATE
        drs_plugin_shell
        juce::juce_audio_processors_headless
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_compile_features(drs_sprint6_integration_hardening_tests PRIVATE cxx_std_17)

add_test(
    NAME drs.sprint6.integration_hardening
    COMMAND drs_sprint6_integration_hardening_tests
)

juce_add_console_app(drs_phase1_realtime_safety_tests
    PRODUCT_NAME "Decent Rhapsody Studio Phase 1 Realtime Safety Tests"
)

target_sources(drs_phase1_realtime_safety_tests
    PRIVATE
        src/Phase1RealtimeSafetyTests.cpp
)

target_link_libraries(drs_phase1_realtime_safety_tests
    PRIVATE
        drs_plugin_shell
        juce::juce_audio_processors_headless
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_compile_features(drs_phase1_realtime_safety_tests PRIVATE cxx_std_17)
if(MSVC)
    target_link_options(drs_phase1_realtime_safety_tests PRIVATE /STACK:8388608)
endif()

add_test(
    NAME drs.phase1.realtime_safety
    COMMAND drs_phase1_realtime_safety_tests
)

juce_add_console_app(drs_phase1_preset_state_tests
    PRODUCT_NAME "Decent Rhapsody Studio Phase 1 Preset State Tests"
)

target_sources(drs_phase1_preset_state_tests
    PRIVATE
        src/Phase1PresetStateTests.cpp
)

target_link_libraries(drs_phase1_preset_state_tests
    PRIVATE
        drs_engine_adapter
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_compile_features(drs_phase1_preset_state_tests PRIVATE cxx_std_17)

target_include_directories(drs_phase1_preset_state_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/third_party/hise/hi_tools/hi_neural/RTNeural/modules
)

add_test(
    NAME drs.phase1.preset_state
    COMMAND drs_phase1_preset_state_tests
)

juce_add_console_app(drs_phase1_state_recall_tests
    PRODUCT_NAME "Decent Rhapsody Studio Phase 1 State Recall Tests"
)

target_sources(drs_phase1_state_recall_tests
    PRIVATE
        src/Phase1StateRecallTests.cpp
)

target_link_libraries(drs_phase1_state_recall_tests
    PRIVATE
        drs_plugin_shell
        drs_standalone_shell
        juce::juce_audio_processors_headless
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_compile_features(drs_phase1_state_recall_tests PRIVATE cxx_std_17)

add_test(
    NAME drs.phase1.state_recall
    COMMAND drs_phase1_state_recall_tests
)

# Registered green contract target for the project-aware host-state codec.
add_executable(drs_host_session_state_contract_tests
    src/HostSessionStateContractTests.cpp
)

target_compile_definitions(drs_host_session_state_contract_tests
    PRIVATE
        DRS_SOURCE_ROOT="${CMAKE_SOURCE_DIR}"
        DRS_CURATED_DSP_FIXTURE_ROOT="${CMAKE_SOURCE_DIR}/content/runtime/phase2/curated-dsp"
)

target_compile_features(drs_host_session_state_contract_tests PRIVATE cxx_std_17)

target_link_libraries(drs_host_session_state_contract_tests
    PRIVATE
        drs_engine_adapter
)

add_test(
    NAME drs.host_state.contract
    COMMAND drs_host_session_state_contract_tests
)

add_executable(drs_project_restore_coordinator_tests
    src/ProjectRestoreCoordinatorTests.cpp
)

target_compile_definitions(drs_project_restore_coordinator_tests
    PRIVATE
        DRS_SOURCE_ROOT="${CMAKE_SOURCE_DIR}"
)

target_compile_features(drs_project_restore_coordinator_tests PRIVATE cxx_std_17)

target_link_libraries(drs_project_restore_coordinator_tests
    PRIVATE
        drs_engine_adapter
        drs_sampler_core
        drs_sample_import
        juce::juce_audio_formats
)

add_test(
    NAME drs.host_state.restore_coordinator
    COMMAND drs_project_restore_coordinator_tests
)

juce_add_console_app(drs_host_state_recovery_ui_tests
    PRODUCT_NAME "Decent Rhapsody Studio Host State Recovery UI Tests"
)

target_sources(drs_host_state_recovery_ui_tests
    PRIVATE
        src/HostStateRecoveryUiTests.cpp
)

target_link_libraries(drs_host_state_recovery_ui_tests
    PRIVATE
        drs_app_shared
        drs_sampler_core
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_compile_features(drs_host_state_recovery_ui_tests PRIVATE cxx_std_17)

add_test(
    NAME drs.host_state.recovery_ui
    COMMAND drs_host_state_recovery_ui_tests
)

add_executable(drs_host_state_restore_stress_tests
    src/HostStateRestoreStressTests.cpp
)

target_compile_definitions(drs_host_state_restore_stress_tests
    PRIVATE
        DRS_SOURCE_ROOT="${CMAKE_SOURCE_DIR}"
)

target_compile_features(drs_host_state_restore_stress_tests PRIVATE cxx_std_17)

target_link_libraries(drs_host_state_restore_stress_tests
    PRIVATE
        drs_sample_import
        drs_sampler_core
        drs_engine_adapter
        juce::juce_audio_formats
)

add_test(
    NAME drs.host_state.restore_stress
    COMMAND drs_host_state_restore_stress_tests
)

# Permanent project-aware processor capture/restore integration target.
juce_add_console_app(drs_host_project_recall_tests
    PRODUCT_NAME "Decent Rhapsody Studio Host Project Recall Tests"
)

target_sources(drs_host_project_recall_tests
    PRIVATE
        src/HostProjectRecallTests.cpp
)

target_link_libraries(drs_host_project_recall_tests
    PRIVATE
        drs_plugin_shell
        juce::juce_audio_processors_headless
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_compile_features(drs_host_project_recall_tests PRIVATE cxx_std_17)

add_test(
    NAME drs.host_state.project_recall
    COMMAND drs_host_project_recall_tests
)

juce_add_console_app(drs_phase2_authoring_foundation_tests
    PRODUCT_NAME "Decent Rhapsody Studio Phase 2 Authoring Foundation Tests"
)

target_sources(drs_phase2_authoring_foundation_tests
    PRIVATE
        src/Phase2AuthoringFoundationTests.cpp
)

target_link_libraries(drs_phase2_authoring_foundation_tests
    PRIVATE
        drs_engine_adapter
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_compile_features(drs_phase2_authoring_foundation_tests PRIVATE cxx_std_17)

add_test(
    NAME drs.phase2.authoring_foundation
    COMMAND drs_phase2_authoring_foundation_tests
)

juce_add_console_app(drs_phase2_authoring_import_tests
    PRODUCT_NAME "Decent Rhapsody Studio Phase 2 Authoring Import Tests"
)

target_sources(drs_phase2_authoring_import_tests
    PRIVATE
        src/Phase2AuthoringImportTests.cpp
)

target_link_libraries(drs_phase2_authoring_import_tests
    PRIVATE
        drs_engine_adapter
        drs_sample_import
        juce::juce_audio_formats
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_compile_features(drs_phase2_authoring_import_tests PRIVATE cxx_std_17)

target_include_directories(drs_phase2_authoring_import_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/third_party/juce/modules
)

add_test(
    NAME drs.phase2.authoring_import
    COMMAND drs_phase2_authoring_import_tests
)

add_executable(drs_wav_import_shell_characterization_tests
    src/WavImportShellCharacterizationTests.cpp
)
target_compile_definitions(drs_wav_import_shell_characterization_tests
    PRIVATE
        DRS_SOURCE_ROOT="${CMAKE_SOURCE_DIR}"
)
target_compile_features(drs_wav_import_shell_characterization_tests PRIVATE cxx_std_17)
add_test(
    NAME drs.wav_import.shell_characterization
    COMMAND drs_wav_import_shell_characterization_tests
)

juce_add_console_app(drs_phase2_mapping_workspace_tests
    PRODUCT_NAME "Decent Rhapsody Studio Phase 2 Mapping Workspace Tests"
)

target_sources(drs_phase2_mapping_workspace_tests
    PRIVATE
        src/Phase2MappingWorkspaceTests.cpp
)

target_link_libraries(drs_phase2_mapping_workspace_tests
    PRIVATE
        drs_engine_adapter
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_compile_features(drs_phase2_mapping_workspace_tests PRIVATE cxx_std_17)

add_test(
    NAME drs.phase2.mapping_workspace
    COMMAND drs_phase2_mapping_workspace_tests
)

add_executable(drs_phase2_zone_group_transaction_tests
    src/Phase2ZoneGroupTransactionsTests.cpp
)

target_link_libraries(drs_phase2_zone_group_transaction_tests
    PRIVATE
        drs_engine_adapter
)

target_compile_features(drs_phase2_zone_group_transaction_tests PRIVATE cxx_std_17)

add_test(
    NAME drs.phase2.zone_group_transactions
    COMMAND drs_phase2_zone_group_transaction_tests
)

add_executable(drs_sprint8_zone_group_hardening_tests
    src/Sprint8ZoneGroupHardeningTests.cpp
)

target_link_libraries(drs_sprint8_zone_group_hardening_tests
    PRIVATE
        drs_engine_adapter
)

target_compile_features(drs_sprint8_zone_group_hardening_tests PRIVATE cxx_std_17)

add_test(
    NAME drs.sprint8.zone_group_hardening
    COMMAND drs_sprint8_zone_group_hardening_tests
)

juce_add_console_app(drs_phase2_waveform_preview_tests
    PRODUCT_NAME "Decent Rhapsody Studio Phase 2 Waveform Preview Tests"
)

target_sources(drs_phase2_waveform_preview_tests
    PRIVATE
        src/Phase2WaveformPreviewTests.cpp
)

target_link_libraries(drs_phase2_waveform_preview_tests
    PRIVATE
        drs_plugin_shell
        juce::juce_audio_processors_headless
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_compile_features(drs_phase2_waveform_preview_tests PRIVATE cxx_std_17)

if(MSVC)
    target_link_options(drs_phase2_waveform_preview_tests PRIVATE /STACK:8388608)
endif()

add_test(
    NAME drs.phase2.waveform_preview
    COMMAND drs_phase2_waveform_preview_tests
)

juce_add_console_app(drs_phase2_macro_routing_tests
    PRODUCT_NAME "Decent Rhapsody Studio Phase 2 Macro Routing Tests"
)

target_sources(drs_phase2_macro_routing_tests
    PRIVATE
        src/Phase2MacroRoutingTests.cpp
)

target_link_libraries(drs_phase2_macro_routing_tests
    PRIVATE
        drs_plugin_shell
        drs_standalone_shell
        juce::juce_audio_processors_headless
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_compile_features(drs_phase2_macro_routing_tests PRIVATE cxx_std_17)

if(MSVC)
    target_link_options(drs_phase2_macro_routing_tests PRIVATE /STACK:8388608)
endif()

add_test(
    NAME drs.phase2.macro_routing
    COMMAND drs_phase2_macro_routing_tests
)

juce_add_console_app(drs_phase2_performance_bank_tests
    PRODUCT_NAME "Decent Rhapsody Studio Phase 2 Performance Bank Tests"
)

target_sources(drs_phase2_performance_bank_tests
    PRIVATE
        src/Phase2PerformanceBankTests.cpp
)

target_link_libraries(drs_phase2_performance_bank_tests
    PRIVATE
        drs_plugin_shell
        drs_standalone_shell
        juce::juce_audio_processors_headless
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_compile_features(drs_phase2_performance_bank_tests PRIVATE cxx_std_17)

add_test(
    NAME drs.phase2.performance_bank
    COMMAND drs_phase2_performance_bank_tests
)

juce_add_console_app(drs_phase2_performance_ui_tests
    PRODUCT_NAME "Decent Rhapsody Studio Phase 2 Performance UI Tests"
)

target_sources(drs_phase2_performance_ui_tests
    PRIVATE
        src/Phase2PerformanceUiTests.cpp
)

target_link_libraries(drs_phase2_performance_ui_tests
    PRIVATE
        drs_app_shared
        drs_engine_adapter
        drs_sampler_core
        drs_sample_import
        juce::juce_audio_formats
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_compile_features(drs_phase2_performance_ui_tests PRIVATE cxx_std_17)

add_test(
    NAME drs.phase2.performance_ui
    COMMAND drs_phase2_performance_ui_tests
)

juce_add_console_app(drs_performance_mixer_ui_tests
    PRODUCT_NAME "Decent Rhapsody Studio Performance Mixer UI Tests"
)

target_sources(drs_performance_mixer_ui_tests
    PRIVATE
        src/PerformanceMixerTests.cpp
)

target_link_libraries(drs_performance_mixer_ui_tests
    PRIVATE
        drs_app_shared
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_compile_features(drs_performance_mixer_ui_tests PRIVATE cxx_std_17)

add_test(
    NAME drs.performance_mixer.ui
    COMMAND drs_performance_mixer_ui_tests
)

juce_add_console_app(drs_phase2_authoring_playback_integration_tests
    PRODUCT_NAME "Decent Rhapsody Studio Phase 2 Authoring Playback Integration Tests"
)

target_sources(drs_phase2_authoring_playback_integration_tests
    PRIVATE
        src/Phase2AuthoringPlaybackIntegrationTests.cpp
)

target_link_libraries(drs_phase2_authoring_playback_integration_tests
    PRIVATE
        drs_plugin_shell
        drs_standalone_shell
        juce::juce_audio_processors_headless
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_compile_features(drs_phase2_authoring_playback_integration_tests PRIVATE cxx_std_17)

add_test(
    NAME drs.phase2.authoring_playback_integration
    COMMAND drs_phase2_authoring_playback_integration_tests
)

juce_add_console_app(drs_phase2_authoring_ui_tests
    PRODUCT_NAME "Decent Rhapsody Studio Phase 2 Authoring UI Tests"
)

target_sources(drs_phase2_authoring_ui_tests
    PRIVATE
        src/Phase2AuthoringUiTests.cpp
)

target_link_libraries(drs_phase2_authoring_ui_tests
    PRIVATE
        drs_app_shared
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_compile_features(drs_phase2_authoring_ui_tests PRIVATE cxx_std_17)

add_test(
    NAME drs.phase2.authoring_ui
    COMMAND drs_phase2_authoring_ui_tests
)

juce_add_console_app(drs_open_workbench_phase0_tests
    PRODUCT_NAME "Decent Rhapsody Studio Open Workbench Phase 0 Tests"
)

target_sources(drs_open_workbench_phase0_tests
    PRIVATE
        src/OpenWorkbenchPhase0Tests.cpp
)

target_link_libraries(drs_open_workbench_phase0_tests
    PRIVATE
        drs_app_shared
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_compile_features(drs_open_workbench_phase0_tests PRIVATE cxx_std_17)

add_test(
    NAME drs.open_workbench.phase0
    COMMAND drs_open_workbench_phase0_tests
)

set_target_properties(drs_open_workbench_phase0_tests PROPERTIES FOLDER "Tests")

juce_add_console_app(drs_open_workbench_phase1_tests
    PRODUCT_NAME "Decent Rhapsody Studio Open Workbench Phase 1 Tests"
)

target_sources(drs_open_workbench_phase1_tests
    PRIVATE
        src/OpenWorkbenchPhase1Tests.cpp
)

target_link_libraries(drs_open_workbench_phase1_tests
    PRIVATE
        drs_app_shared
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_compile_features(drs_open_workbench_phase1_tests PRIVATE cxx_std_17)

add_test(
    NAME drs.open_workbench.phase1
    COMMAND drs_open_workbench_phase1_tests
)

set_target_properties(drs_open_workbench_phase1_tests PROPERTIES FOLDER "Tests")

juce_add_console_app(drs_open_workbench_phase2_tests
    PRODUCT_NAME "Decent Rhapsody Studio Open Workbench Phase 2 Tests"
)

target_sources(drs_open_workbench_phase2_tests
    PRIVATE
        src/OpenWorkbenchPhase2Tests.cpp
)

target_link_libraries(drs_open_workbench_phase2_tests
    PRIVATE
        drs_app_shared
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_compile_features(drs_open_workbench_phase2_tests PRIVATE cxx_std_17)

add_test(
    NAME drs.open_workbench.phase2
    COMMAND drs_open_workbench_phase2_tests
)

set_target_properties(drs_open_workbench_phase2_tests PROPERTIES FOLDER "Tests")

juce_add_console_app(drs_open_workbench_phase3_tests
    PRODUCT_NAME "Decent Rhapsody Studio Open Workbench Phase 3 Tests"
)

target_sources(drs_open_workbench_phase3_tests
    PRIVATE
        src/OpenWorkbenchPhase3Tests.cpp
)

target_link_libraries(drs_open_workbench_phase3_tests
    PRIVATE
        drs_app_shared
        drs_engine_adapter
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_compile_features(drs_open_workbench_phase3_tests PRIVATE cxx_std_17)

add_test(
    NAME drs.open_workbench.phase3
    COMMAND drs_open_workbench_phase3_tests
)

set_target_properties(drs_open_workbench_phase3_tests PROPERTIES FOLDER "Tests")

juce_add_console_app(drs_open_workbench_phase4_tests
    PRODUCT_NAME "Decent Rhapsody Studio Open Workbench Phase 4 Tests"
)

target_sources(drs_open_workbench_phase4_tests
    PRIVATE
        src/OpenWorkbenchPhase4Tests.cpp
)

target_link_libraries(drs_open_workbench_phase4_tests
    PRIVATE
        drs_app_shared
        drs_engine_adapter
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_compile_features(drs_open_workbench_phase4_tests PRIVATE cxx_std_17)

add_test(
    NAME drs.open_workbench.phase4
    COMMAND drs_open_workbench_phase4_tests
)

set_target_properties(drs_open_workbench_phase4_tests PROPERTIES FOLDER "Tests")

juce_add_console_app(drs_open_workbench_phase5_tests
    PRODUCT_NAME "Decent Rhapsody Studio Open Workbench Phase 5 Qualification"
)

target_sources(drs_open_workbench_phase5_tests
    PRIVATE
        src/OpenWorkbenchPhase5Tests.cpp
)

target_link_libraries(drs_open_workbench_phase5_tests
    PRIVATE
        drs_app_shared
        drs_engine_adapter
        drs_plugin_shell
        drs_standalone_shell
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_compile_features(drs_open_workbench_phase5_tests PRIVATE cxx_std_17)

if(MSVC)
    target_link_options(drs_open_workbench_phase5_tests PRIVATE /STACK:8388608)
endif()

add_test(
    NAME drs.open_workbench.phase5
    COMMAND drs_open_workbench_phase5_tests
)

set_tests_properties(drs.open_workbench.phase5 PROPERTIES TIMEOUT 120)
set_target_properties(drs_open_workbench_phase5_tests PROPERTIES FOLDER "Tests")

juce_add_console_app(drs_performance_engine_s10_articulation_ui_tests
    PRODUCT_NAME "Decent Rhapsody Studio Sprint 10 Articulation UI Tests"
)

target_sources(drs_performance_engine_s10_articulation_ui_tests
    PRIVATE
        src/PerformanceEngineSprint10ArticulationUiTests.cpp
)

target_link_libraries(drs_performance_engine_s10_articulation_ui_tests
    PRIVATE
        drs_app_shared
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_compile_features(drs_performance_engine_s10_articulation_ui_tests PRIVATE cxx_std_17)

add_test(
    NAME drs.performance_engine.s10.articulation_ui
    COMMAND drs_performance_engine_s10_articulation_ui_tests
)

juce_add_console_app(drs_phase2_repeated_structure_density_tests
    PRODUCT_NAME "Decent Rhapsody Studio Phase 2 Repeated Structure Density Tests"
)

target_sources(drs_phase2_repeated_structure_density_tests
    PRIVATE
        src/Phase2RepeatedStructureDensityTests.cpp
)

target_link_libraries(drs_phase2_repeated_structure_density_tests
    PRIVATE
        drs_app_shared
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_compile_features(drs_phase2_repeated_structure_density_tests PRIVATE cxx_std_17)

add_test(
    NAME drs.phase2.repeated_structure_density
    COMMAND drs_phase2_repeated_structure_density_tests
)

juce_add_console_app(drs_phase1_diagnostics_tests
    PRODUCT_NAME "Decent Rhapsody Studio Phase 1 Diagnostics Tests"
)

target_sources(drs_phase1_diagnostics_tests
    PRIVATE
        src/Phase1DiagnosticsTests.cpp
)

target_link_libraries(drs_phase1_diagnostics_tests
    PRIVATE
        drs_engine_adapter
        drs_sampler_core
        drs_sample_import
        juce::juce_audio_formats
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_compile_features(drs_phase1_diagnostics_tests PRIVATE cxx_std_17)

add_test(
    NAME drs.phase1.diagnostics
    COMMAND drs_phase1_diagnostics_tests
)

juce_add_console_app(drs_phase1_failure_handling_tests
    PRODUCT_NAME "Decent Rhapsody Studio Phase 1 Failure Handling Tests"
)

target_sources(drs_phase1_failure_handling_tests
    PRIVATE
        src/Phase1FailureHandlingTests.cpp
)

target_link_libraries(drs_phase1_failure_handling_tests
    PRIVATE
        drs_engine_adapter
        drs_sampler_core
        drs_sample_import
        juce::juce_audio_formats
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_compile_features(drs_phase1_failure_handling_tests PRIVATE cxx_std_17)

add_test(
    NAME drs.phase1.failure_handling
    COMMAND drs_phase1_failure_handling_tests
)

juce_add_console_app(drs_phase1_macro_bridge_tests
    PRODUCT_NAME "Decent Rhapsody Studio Phase 1 Macro Bridge Tests"
)

target_sources(drs_phase1_macro_bridge_tests
    PRIVATE
        src/Phase1MacroBridgeTests.cpp
)

target_link_libraries(drs_phase1_macro_bridge_tests
    PRIVATE
        drs_plugin_shell
        drs_standalone_shell
        juce::juce_audio_processors_headless
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_compile_features(drs_phase1_macro_bridge_tests PRIVATE cxx_std_17)

add_test(
    NAME drs.phase1.macro_bridge
    COMMAND drs_phase1_macro_bridge_tests
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
)

target_link_libraries(drs_phase1_sample_import_tests
    PRIVATE
        drs_engine_adapter
        drs_sample_import
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

juce_add_console_app(drs_wav_import_fixture_support_tests
    PRODUCT_NAME "Decent Rhapsody Studio WAV Import Fixture Support Tests"
)

target_sources(drs_wav_import_fixture_support_tests
    PRIVATE
        src/WavImportFixtureSupportTests.cpp
        support/WavImportTestSupport.cpp
)

target_link_libraries(drs_wav_import_fixture_support_tests
    PRIVATE
        drs_engine_adapter
        drs_sample_import
        juce::juce_audio_formats
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_compile_features(drs_wav_import_fixture_support_tests PRIVATE cxx_std_17)

target_include_directories(drs_wav_import_fixture_support_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/support
        ${CMAKE_SOURCE_DIR}/third_party/juce/modules
)

add_test(
    NAME drs.wav_import.fixture_support
    COMMAND drs_wav_import_fixture_support_tests
)

juce_add_console_app(drs_wav_import_baseline_report
    PRODUCT_NAME "Decent Rhapsody Studio WAV Import Baseline Report"
)

target_sources(drs_wav_import_baseline_report
    PRIVATE
        src/WavImportBaselineReport.cpp
        support/WavImportTestSupport.cpp
)

target_link_libraries(drs_wav_import_baseline_report
    PRIVATE
        drs_plugin_shell
        drs_sample_import
        juce::juce_audio_processors_headless
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_compile_features(drs_wav_import_baseline_report PRIVATE cxx_std_17)

target_include_directories(drs_wav_import_baseline_report
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/support
        ${CMAKE_SOURCE_DIR}/third_party/juce/modules
)

add_test(
    NAME drs.wav_import.baseline_report
    COMMAND drs_wav_import_baseline_report "${CMAKE_CURRENT_BINARY_DIR}/wav-import-baseline-report.json"
)

juce_add_console_app(drs_phase1_compile_path_tests
    PRODUCT_NAME "Decent Rhapsody Studio Phase 1 Compile Path Tests"
)

target_sources(drs_phase1_compile_path_tests
    PRIVATE
        src/Phase1CompilePathTests.cpp
)

target_link_libraries(drs_phase1_compile_path_tests
    PRIVATE
        drs_engine_adapter
        drs_sample_import
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

add_test(
    NAME drs.packaged_instrument_fx_routing.compile
    COMMAND drs_phase1_compile_path_tests --px02-fx-routing
)

juce_add_console_app(drs_phase1_performance_package_tests
    PRODUCT_NAME "Decent Rhapsody Studio Phase 1 Performance Package Tests"
)

target_sources(drs_phase1_performance_package_tests
    PRIVATE
        src/Phase1PerformancePackageTests.cpp
)

target_link_libraries(drs_phase1_performance_package_tests
    PRIVATE
        drs_engine_adapter
        drs_sampler_core
        drs_sample_import
        juce::juce_audio_formats
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_compile_features(drs_phase1_performance_package_tests PRIVATE cxx_std_17)

target_include_directories(drs_phase1_performance_package_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/third_party/juce/modules
        ${CMAKE_SOURCE_DIR}/third_party/hise/hi_tools/hi_neural/RTNeural/modules
)

add_test(
    NAME drs.phase1.performance_package
    COMMAND drs_phase1_performance_package_tests
)

add_executable(drs_packaged_instrument_fx_routing_contract_tests
    src/PackagedInstrumentFxRoutingContractTests.cpp
)

target_link_libraries(drs_packaged_instrument_fx_routing_contract_tests
    PRIVATE
        drs_engine_adapter
)

target_compile_features(drs_packaged_instrument_fx_routing_contract_tests PRIVATE cxx_std_17)

target_compile_definitions(drs_packaged_instrument_fx_routing_contract_tests
    PRIVATE
        DRS_PACKAGED_INSTRUMENT_FX_ROUTING_FIXTURE_ROOT="${CMAKE_SOURCE_DIR}/content/runtime/phase2/packaged-instrument-fx-routing"
)

target_include_directories(drs_packaged_instrument_fx_routing_contract_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/third_party/hise/hi_tools/hi_neural/RTNeural/modules
)

add_test(
    NAME drs.packaged_instrument_fx_routing.contract
    COMMAND drs_packaged_instrument_fx_routing_contract_tests
)

set_target_properties(drs_packaged_instrument_fx_routing_contract_tests PROPERTIES
    FOLDER "Tests"
)

juce_add_console_app(drs_packaged_instrument_fx_routing_activation_tests
    PRODUCT_NAME "Decent Rhapsody Studio Packaged Instrument FX Routing Activation Tests"
)

target_sources(drs_packaged_instrument_fx_routing_activation_tests
    PRIVATE
        src/PackagedInstrumentFxRoutingContractRedTests.cpp
)

target_link_libraries(drs_packaged_instrument_fx_routing_activation_tests
    PRIVATE
        drs_engine_adapter
        drs_sampler_core
        drs_sample_import
        juce::juce_audio_formats
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_compile_features(drs_packaged_instrument_fx_routing_activation_tests PRIVATE cxx_std_17)

target_compile_definitions(drs_packaged_instrument_fx_routing_activation_tests
    PRIVATE
        DRS_PACKAGED_INSTRUMENT_FX_ROUTING_FIXTURE_ROOT="${CMAKE_SOURCE_DIR}/content/runtime/phase2/packaged-instrument-fx-routing"
)

target_include_directories(drs_packaged_instrument_fx_routing_activation_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/third_party/juce/modules
        ${CMAKE_SOURCE_DIR}/third_party/hise/hi_tools/hi_neural/RTNeural/modules
)

add_test(
    NAME drs.packaged_instrument_fx_routing.activation
    COMMAND drs_packaged_instrument_fx_routing_activation_tests
)

set_target_properties(drs_packaged_instrument_fx_routing_activation_tests PROPERTIES
    FOLDER "Tests"
)

juce_add_console_app(drs_phase1_performance_package_loader_tests
    PRODUCT_NAME "Decent Rhapsody Studio Phase 1 Performance Package Loader Tests"
)

target_sources(drs_phase1_performance_package_loader_tests
    PRIVATE
        src/Phase1PerformancePackageLoaderTests.cpp
)

target_link_libraries(drs_phase1_performance_package_loader_tests
    PRIVATE
        drs_engine_adapter
        drs_sampler_core
        drs_sample_import
        juce::juce_audio_formats
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_compile_features(drs_phase1_performance_package_loader_tests PRIVATE cxx_std_17)

target_include_directories(drs_phase1_performance_package_loader_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/third_party/juce/modules
        ${CMAKE_SOURCE_DIR}/third_party/hise/hi_tools/hi_neural/RTNeural/modules
)

add_test(
    NAME drs.phase1.performance_package_loader
    COMMAND drs_phase1_performance_package_loader_tests
)

juce_add_console_app(drs_phase1_performance_package_session_tests
    PRODUCT_NAME "Decent Rhapsody Studio Phase 1 Performance Package Session Tests"
)

target_sources(drs_phase1_performance_package_session_tests
    PRIVATE
        src/Phase1PerformancePackageSessionTests.cpp
)

target_link_libraries(drs_phase1_performance_package_session_tests
    PRIVATE
        drs_plugin_shell
        drs_standalone_shell
        drs_engine_adapter
        drs_sample_import
        juce::juce_audio_formats
        juce::juce_audio_processors_headless
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_compile_features(drs_phase1_performance_package_session_tests PRIVATE cxx_std_17)

if(MSVC)
    target_link_options(drs_phase1_performance_package_session_tests PRIVATE /STACK:8388608)
endif()

target_include_directories(drs_phase1_performance_package_session_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/third_party/juce/modules
        ${CMAKE_SOURCE_DIR}/third_party/hise/hi_tools/hi_neural/RTNeural/modules
)

add_test(
    NAME drs.phase1.performance_package_session
    COMMAND drs_phase1_performance_package_session_tests
)

juce_add_console_app(drs_performance_package_export_lifecycle_tests
    PRODUCT_NAME "Decent Rhapsody Studio Playable Package Export Lifecycle Tests"
)

target_sources(drs_performance_package_export_lifecycle_tests
    PRIVATE
        src/PerformancePackageExportLifecycleTests.cpp
)

target_link_libraries(drs_performance_package_export_lifecycle_tests
    PRIVATE
        drs_app_shared
        drs_engine_adapter
        drs_sampler_core
        drs_sample_import
        juce::juce_audio_formats
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_compile_features(drs_performance_package_export_lifecycle_tests PRIVATE cxx_std_17)

target_include_directories(drs_performance_package_export_lifecycle_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/third_party/hise/hi_tools/hi_neural/RTNeural/modules
        ${CMAKE_SOURCE_DIR}/third_party/juce/modules
)

add_test(
    NAME drs.performance_package.export_lifecycle
    COMMAND drs_performance_package_export_lifecycle_tests
)

juce_add_console_app(drs_phase1_performance_package_host_validation_tests
    PRODUCT_NAME "Decent Rhapsody Studio Phase 1 Performance Package Host Validation Tests"
)

target_sources(drs_phase1_performance_package_host_validation_tests
    PRIVATE
        src/Phase1PerformancePackageHostValidationTests.cpp
)

target_link_libraries(drs_phase1_performance_package_host_validation_tests
    PRIVATE
        drs_plugin_shell
        drs_standalone_shell
        drs_engine_adapter
        drs_sample_import
        juce::juce_audio_formats
        juce::juce_audio_processors_headless
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_compile_features(drs_phase1_performance_package_host_validation_tests PRIVATE cxx_std_17)

target_include_directories(drs_phase1_performance_package_host_validation_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/third_party/juce/modules
        ${CMAKE_SOURCE_DIR}/third_party/hise/hi_tools/hi_neural/RTNeural/modules
)

add_test(
    NAME drs.phase1.performance_package_host_validation
    COMMAND drs_phase1_performance_package_host_validation_tests
)
set_tests_properties(drs.phase1.performance_package_host_validation PROPERTIES RUN_SERIAL TRUE)

juce_add_console_app(drs_playable_instrument_license_contract_tests
    PRODUCT_NAME "Decent Rhapsody Studio Playable Instrument License Contract Tests"
)

target_sources(drs_playable_instrument_license_contract_tests
    PRIVATE
        src/PlayableInstrumentLicenseContractTests.cpp
)

target_link_libraries(drs_playable_instrument_license_contract_tests
    PRIVATE
        juce::juce_core
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_include_directories(drs_playable_instrument_license_contract_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/app/src
        ${CMAKE_SOURCE_DIR}/engine_adapter/include
)

target_compile_features(drs_playable_instrument_license_contract_tests PRIVATE cxx_std_17)

add_test(
    NAME drs.playable_instrument_license.contract
    COMMAND drs_playable_instrument_license_contract_tests
)

set_target_properties(drs_playable_instrument_license_contract_tests PROPERTIES
    FOLDER "Tests"
)

juce_add_console_app(drs_playable_instrument_license_project_import_tests
    PRODUCT_NAME "Decent Rhapsody Studio Playable Instrument License Project Import Tests"
)

target_sources(drs_playable_instrument_license_project_import_tests
    PRIVATE
        src/PlayableInstrumentLicenseProjectImportTests.cpp
)

target_link_libraries(drs_playable_instrument_license_project_import_tests
    PRIVATE
        drs_app_shared
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_compile_features(drs_playable_instrument_license_project_import_tests PRIVATE cxx_std_17)

add_test(
    NAME drs.playable_instrument_license.project_import
    COMMAND drs_playable_instrument_license_project_import_tests
)

set_target_properties(drs_playable_instrument_license_project_import_tests PROPERTIES
    FOLDER "Tests"
)

juce_add_console_app(drs_playable_instrument_license_viewer_tests
    PRODUCT_NAME "Decent Rhapsody Studio Playable Instrument License Viewer Tests"
)

target_sources(drs_playable_instrument_license_viewer_tests
    PRIVATE
        src/PlayableInstrumentLicenseViewerTests.cpp
)

target_link_libraries(drs_playable_instrument_license_viewer_tests
    PRIVATE
        drs_app_shared
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_compile_features(drs_playable_instrument_license_viewer_tests PRIVATE cxx_std_17)

add_test(
    NAME drs.playable_instrument_license.viewer
    COMMAND drs_playable_instrument_license_viewer_tests
)

set_target_properties(drs_playable_instrument_license_viewer_tests PROPERTIES
    FOLDER "Tests"
)

add_executable(drs_playable_instrument_license_contract_red_tests
    src/PlayableInstrumentLicenseContractRedTests.cpp
)

target_compile_features(drs_playable_instrument_license_contract_red_tests PRIVATE cxx_std_17)
target_compile_definitions(drs_playable_instrument_license_contract_red_tests
    PRIVATE
        DRS_SOURCE_ROOT="${CMAKE_SOURCE_DIR}"
)

# LI-01 expected-red seams are invoked directly and intentionally excluded from
# CTest and drs_all_tests. LI-02 through LI-04 promote each seam to green coverage.
set_target_properties(drs_playable_instrument_license_contract_red_tests PROPERTIES
    FOLDER "Tests"
)

juce_add_console_app(drs_phase1_performance_package_release_gate
    PRODUCT_NAME "Decent Rhapsody Studio Phase 1 Performance Package Release Gate"
)

target_sources(drs_phase1_performance_package_release_gate
    PRIVATE
        src/Phase1PerformancePackageReleaseGate.cpp
        support/Sprint4OfflineRenderHarness.cpp
        support/Sprint4OfflineRenderHarness.h
)

target_link_libraries(drs_phase1_performance_package_release_gate
    PRIVATE
        drs_plugin_shell
        drs_standalone_shell
        drs_app_shared
        drs_engine_adapter
        drs_sampler_core
        drs_sample_import
        juce::juce_audio_formats
        juce::juce_audio_processors_headless
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_compile_features(drs_phase1_performance_package_release_gate PRIVATE cxx_std_17)

if(MSVC)
    target_link_options(drs_phase1_performance_package_release_gate PRIVATE /STACK:8388608)
endif()

target_include_directories(drs_phase1_performance_package_release_gate
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/support
        ${CMAKE_SOURCE_DIR}/third_party/juce/modules
        ${CMAKE_SOURCE_DIR}/third_party/hise/hi_tools/hi_neural/RTNeural/modules
)

add_test(
    NAME drs.phase1.performance_package_release_gate
    COMMAND drs_phase1_performance_package_release_gate
        "${CMAKE_CURRENT_BINARY_DIR}/phase1-performance-package-release-gate.json"
)

juce_add_console_app(drs_phase1_pipeline_report
    PRODUCT_NAME "Decent Rhapsody Studio Phase 1 Pipeline Report"
)

target_sources(drs_phase1_pipeline_report
    PRIVATE
        src/Phase1PipelineReport.cpp
)

target_link_libraries(drs_phase1_pipeline_report
    PRIVATE
        drs_engine_adapter
        drs_sample_import
        drs_plugin_shell
        drs_standalone_shell
        juce::juce_audio_formats
        juce::juce_audio_processors_headless
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

juce_add_console_app(drs_phase1_benchmark_scene
    PRODUCT_NAME "Decent Rhapsody Studio Phase 1 Benchmark Scene"
)

target_sources(drs_phase1_benchmark_scene
    PRIVATE
        src/Phase1BenchmarkScene.cpp
)

target_link_libraries(drs_phase1_benchmark_scene
    PRIVATE
        drs_engine_adapter
        drs_standalone_shell
        juce::juce_audio_processors_headless
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_compile_features(drs_phase1_benchmark_scene PRIVATE cxx_std_17)

target_include_directories(drs_phase1_benchmark_scene
    PRIVATE
        ${CMAKE_SOURCE_DIR}/third_party/juce/modules
        ${CMAKE_SOURCE_DIR}/third_party/hise/hi_tools/hi_neural/RTNeural/modules
)

add_test(
    NAME drs.phase1.benchmark_scene
    COMMAND drs_phase1_benchmark_scene
        "${CMAKE_SOURCE_DIR}/content/runtime/phase1/benchmark-scenes/reference-playback-scene.json"
        "${CMAKE_CURRENT_BINARY_DIR}/phase1-benchmark-scene.json"
)

juce_add_console_app(drs_phase1_stream_reader_tests
    PRODUCT_NAME "Decent Rhapsody Studio Phase 1 Stream Reader Tests"
)

target_sources(drs_phase1_stream_reader_tests
    PRIVATE
        src/Phase1StreamReaderTests.cpp
)

target_link_libraries(drs_phase1_stream_reader_tests
    PRIVATE
        drs_engine_adapter
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_compile_features(drs_phase1_stream_reader_tests PRIVATE cxx_std_17)

target_include_directories(drs_phase1_stream_reader_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/third_party/hise/hi_tools/hi_neural/RTNeural/modules
)

add_test(
    NAME drs.phase1.stream_reader
    COMMAND drs_phase1_stream_reader_tests
)

juce_add_console_app(drs_phase1_streaming_service_tests
    PRODUCT_NAME "Decent Rhapsody Studio Phase 1 Streaming Service Tests"
)

target_sources(drs_phase1_streaming_service_tests
    PRIVATE
        src/Phase1StreamingServiceTests.cpp
)

target_link_libraries(drs_phase1_streaming_service_tests
    PRIVATE
        drs_engine_adapter
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_compile_features(drs_phase1_streaming_service_tests PRIVATE cxx_std_17)

target_include_directories(drs_phase1_streaming_service_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/third_party/hise/hi_tools/hi_neural/RTNeural/modules
)

add_test(
    NAME drs.phase1.streaming_service
    COMMAND drs_phase1_streaming_service_tests
)

juce_add_console_app(drs_phase1_voice_runtime_tests
    PRODUCT_NAME "Decent Rhapsody Studio Phase 1 Voice Runtime Tests"
)

target_sources(drs_phase1_voice_runtime_tests
    PRIVATE
        src/Phase1VoiceRuntimeTests.cpp
)

target_link_libraries(drs_phase1_voice_runtime_tests
    PRIVATE
        drs_engine_adapter
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_compile_features(drs_phase1_voice_runtime_tests PRIVATE cxx_std_17)

target_include_directories(drs_phase1_voice_runtime_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/third_party/hise/hi_tools/hi_neural/RTNeural/modules
)

add_test(
    NAME drs.phase1.voice_runtime
    COMMAND drs_phase1_voice_runtime_tests
)

juce_add_console_app(drs_phase1_note_routing_tests
    PRODUCT_NAME "Decent Rhapsody Studio Phase 1 Note Routing Tests"
)

target_sources(drs_phase1_note_routing_tests
    PRIVATE
        src/Phase1NoteRoutingTests.cpp
)

target_link_libraries(drs_phase1_note_routing_tests
    PRIVATE
        drs_engine_adapter
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_compile_features(drs_phase1_note_routing_tests PRIVATE cxx_std_17)

target_include_directories(drs_phase1_note_routing_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/third_party/hise/hi_tools/hi_neural/RTNeural/modules
)

add_test(
    NAME drs.phase1.note_routing
    COMMAND drs_phase1_note_routing_tests
)

juce_add_console_app(drs_phase1_load_profile_tests
    PRODUCT_NAME "Decent Rhapsody Studio Phase 1 Load Profile Tests"
)

target_sources(drs_phase1_load_profile_tests
    PRIVATE
        src/Phase1LoadProfileTests.cpp
)

target_link_libraries(drs_phase1_load_profile_tests
    PRIVATE
        drs_engine_adapter
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_compile_features(drs_phase1_load_profile_tests PRIVATE cxx_std_17)

target_include_directories(drs_phase1_load_profile_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/third_party/hise/hi_tools/hi_neural/RTNeural/modules
)

add_test(
    NAME drs.phase1.load_profile
    COMMAND drs_phase1_load_profile_tests
)

juce_add_console_app(drs_phase1_runtime_counters_tests
    PRODUCT_NAME "Decent Rhapsody Studio Phase 1 Runtime Counter Tests"
)

target_sources(drs_phase1_runtime_counters_tests
    PRIVATE
        src/Phase1RuntimeCountersTests.cpp
)

target_link_libraries(drs_phase1_runtime_counters_tests
    PRIVATE
        drs_engine_adapter
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_compile_features(drs_phase1_runtime_counters_tests PRIVATE cxx_std_17)

target_include_directories(drs_phase1_runtime_counters_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/third_party/hise/hi_tools/hi_neural/RTNeural/modules
)

add_test(
    NAME drs.phase1.runtime_counters
    COMMAND drs_phase1_runtime_counters_tests
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
        drs_sample_import
        juce::juce_audio_formats
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_compile_features(drs_phase1_runtime_fixture_tool PRIVATE cxx_std_17)

target_include_directories(drs_phase1_runtime_fixture_tool
    PRIVATE
        ${CMAKE_SOURCE_DIR}/third_party/juce/modules
        ${CMAKE_SOURCE_DIR}/third_party/hise/hi_tools/hi_neural/RTNeural/modules
)

add_test(
    NAME drs.phase1.fixture_tool_verify
    COMMAND drs_phase1_runtime_fixture_tool --verify
)

add_custom_target(drs_all_tests)

add_dependencies(drs_all_tests
    DecentRhapsodyStudioPlugin
    drs_package_v2_tests
    drs_host_session_state_contract_tests
    drs_host_project_recall_tests
    drs_host_state_recovery_ui_tests
    drs_host_state_restore_stress_tests
    drs_curated_dsp_contract_tests
    drs_performance_engine_s0_fixture_tests
    drs_performance_engine_s1_articulation_tests
    drs_performance_engine_s2_rule_tests
    drs_curated_dsp_control_law_tests
    drs_control_law_tests
    drs_control_law_project_schema_tests
    drs_control_law_release_proof_tests
    drs_curated_dsp_graph_plan_tests
    drs_curated_dsp_gain_tests
    drs_curated_dsp_reverb_tests
    drs_curated_dsp_compact_eq_tests
    drs_curated_dsp_chorus_tests
    drs_curated_dsp_scoped_routing_tests
    drs_curated_dsp_s0_baseline_report
    drs_project_restore_coordinator_tests
    drs_sprint4_entry_authored_input_tests
    drs_sprint4_entry_activation_payload_tests
    drs_sprint4_entry_diagnostics_concurrency_tests
    drs_sprint4_concurrency_soak_tests
    drs_sprint4_entry_realtime_guard_tests
    drs_sprint4_entry_shell_parity_tests
    drs_sprint4_render_model_tests
    drs_sprint4_voice_kernel_tests
    drs_sprint4_scheduler_tests
    drs_sprint4_voice_lifecycle_tests
    drs_sprint4_playback_context_tests
    drs_sprint4_shell_cutover_tests
    drs_sprint4_offline_renderer_tests
    drs_sprint4_crossfade_mixing_tests
    drs_sprint5_preview_contract_tests
    drs_sprint5_preview_controller_tests
    drs_sprint5_preview_controller_integration_tests
    drs_sprint5_preview_coalescing_tests
    drs_sprint5_preview_preparation_tests
    drs_sprint5_preview_audition_tests
    drs_sprint5_preview_recovery_tests
    drs_sprint5_preview_shell_parity_tests
    drs_sprint5_preview_contract_red_tests
    drs_sprint5_integration_hardening_tests
    drs_sprint31_sfz_contract_tests
    drs_sprint31_sfz_fixture_characterization_tests
    drs_sprint31_sfz_parser_tests
    drs_sprint31_sfz_normalization_tests
    drs_sprint31_sfz_compatibility_tests
    drs_sprint31_sfz_report_model_tests
    drs_sprint31_sfz_projection_tests
    drs_sprint31_sfz_review_ui_tests
    drs_sprint31_sfz_corpus_hardening_tests
    drs_sfz_import_lifecycle_tests
    drs_sfz_import_shell_lifecycle_tests
    drs_wav_import_service_contract_tests
    drs_wav_import_lifecycle_tests
    drs_wav_import_staging_tests
    drs_wav_import_analysis_tests
    drs_sprint31_sfz_contract_red_tests
    drs_phase3_crossfade_contract_tests
    drs_phase3_crossfade_fixture_characterization_tests
    drs_phase3_crossfade_persistence_tests
    drs_phase3_crossfade_prepared_playback_tests
    drs_phase1_zone_groups_contract_tests
    drs_phase1_zone_groups_schema_persistence_tests
    drs_phase3_round_robin_contract_tests
    drs_phase3_round_robin_fixture_characterization_tests
    drs_phase3_round_robin_schema_persistence_tests
    drs_phase3_round_robin_prepared_playback_tests
    drs_phase3_round_robin_hardening_tests
    drs_phase1_zone_groups_contract_red_tests
    drs_phase3_round_robin_contract_red_tests
    drs_sprint6_publish_contract_tests
    drs_sprint6_publish_contract_seam_tests
    drs_sprint6_publish_controller_tests
    drs_sprint6_publish_controller_integration_tests
    drs_sprint6_performance_preparation_tests
    drs_sprint6_publish_scheduling_tests
    drs_sprint6_performance_activation_recovery_tests
    drs_sprint6_voice_generation_cutover_tests
    drs_sprint6_published_macro_binding_tests
    drs_performance_mixer_s0_red_tests
    drs_sprint6_publish_shell_parity_tests
    drs_sprint6_integration_hardening_tests
    drs_phase0_smoke_tests
    drs_phase1_runtime_contract_tests
    drs_phase1_draft_playback_contract_tests
    drs_phase1_draft_playback_facade_tests
    drs_phase1_playback_snapshot_tests
    drs_phase1_prepared_playback_tests
    drs_phase1_prepared_playback_worker_tests
    drs_phase1_realtime_safety_tests
    drs_phase1_preset_state_tests
    drs_phase1_state_recall_tests
    drs_phase2_authoring_foundation_tests
    drs_phase2_authoring_import_tests
    drs_wav_import_workflow_tests
    drs_wav_import_progress_tests
    drs_wav_import_processor_responsiveness_tests
    drs_wav_import_lifecycle_io_audit_tests
    drs_wav_import_ci_budget_tests
    drs_wav_import_host_validation_tests
    drs_wav_import_shell_characterization_tests
    drs_phase2_mapping_workspace_tests
    drs_sprint8_zone_group_hardening_tests
    drs_phase2_waveform_preview_tests
    drs_phase2_macro_routing_tests
    drs_phase2_performance_bank_tests
    drs_phase2_performance_ui_tests
    drs_performance_mixer_ui_tests
    drs_phase2_authoring_playback_integration_tests
    drs_phase2_authoring_ui_tests
    drs_phase2_repeated_structure_density_tests
    drs_phase1_diagnostics_tests
    drs_phase1_failure_handling_tests
    drs_phase1_macro_bridge_tests
    drs_phase1_runtime_baseline_report
    drs_phase1_sample_import_tests
    drs_wav_import_fixture_support_tests
    drs_wav_import_baseline_report
    drs_phase1_compile_path_tests
    drs_phase1_pipeline_report
    drs_phase1_benchmark_scene
    drs_phase1_stream_reader_tests
    drs_phase1_streaming_service_tests
    drs_phase1_voice_runtime_tests
    drs_phase1_note_routing_tests
    drs_phase1_load_profile_tests
    drs_phase1_runtime_counters_tests
    drs_phase1_runtime_baseline_guard
    drs_phase1_runtime_fixture_tool
    drs_phase1_performance_package_tests
    drs_packaged_instrument_fx_routing_contract_tests
    drs_phase1_performance_package_loader_tests
    drs_phase1_performance_package_session_tests
    drs_performance_package_export_lifecycle_tests
    drs_phase1_performance_package_host_validation_tests
    drs_playable_instrument_license_contract_tests
    drs_playable_instrument_license_project_import_tests
    drs_playable_instrument_license_viewer_tests
    drs_phase1_performance_package_release_gate
)

set_target_properties(drs_all_tests PROPERTIES
    FOLDER "Tests"
)

set_target_properties(drs_phase0_smoke_tests PROPERTIES
    FOLDER "Tests"
)

set_target_properties(drs_phase1_runtime_contract_tests PROPERTIES
    FOLDER "Tests"
)

set_target_properties(drs_phase1_preset_state_tests PROPERTIES
    FOLDER "Tests"
)

set_target_properties(drs_phase1_state_recall_tests PROPERTIES
    FOLDER "Tests"
)

set_target_properties(drs_project_restore_coordinator_tests PROPERTIES
    FOLDER "Tests"
)

set_target_properties(drs_host_project_recall_tests PROPERTIES
    FOLDER "Tests"
)

set_target_properties(drs_host_state_recovery_ui_tests PROPERTIES
    FOLDER "Tests"
)

set_target_properties(drs_host_state_restore_stress_tests PROPERTIES
    FOLDER "Tests"
)

set_target_properties(drs_phase2_authoring_foundation_tests PROPERTIES
    FOLDER "Tests"
)

set_target_properties(drs_phase2_authoring_import_tests PROPERTIES
    FOLDER "Tests"
)

set_target_properties(drs_wav_import_workflow_tests PROPERTIES
    FOLDER "Tests"
)

set_target_properties(drs_wav_import_progress_tests PROPERTIES
    FOLDER "Tests"
)

set_target_properties(drs_wav_import_processor_responsiveness_tests PROPERTIES
    FOLDER "Tests"
)

set_target_properties(drs_wav_import_lifecycle_io_audit_tests PROPERTIES
    FOLDER "Tests"
)

set_target_properties(drs_wav_import_ci_budget_tests PROPERTIES
    FOLDER "Tests"
)

set_target_properties(drs_wav_import_host_validation_tests PROPERTIES
    FOLDER "Tests"
)

set_target_properties(drs_wav_import_shell_characterization_tests PROPERTIES
    FOLDER "Tests"
)

set_target_properties(drs_phase2_mapping_workspace_tests PROPERTIES
    FOLDER "Tests"
)

set_target_properties(drs_phase2_waveform_preview_tests PROPERTIES
    FOLDER "Tests"
)

set_target_properties(drs_phase2_macro_routing_tests PROPERTIES
    FOLDER "Tests"
)

set_target_properties(drs_phase2_performance_bank_tests PROPERTIES
    FOLDER "Tests"
)

set_target_properties(drs_phase2_performance_ui_tests PROPERTIES
    FOLDER "Tests"
)

set_target_properties(drs_phase2_authoring_playback_integration_tests PROPERTIES
    FOLDER "Tests"
)

set_target_properties(drs_phase2_authoring_ui_tests PROPERTIES
    FOLDER "Tests"
)

set_target_properties(drs_phase2_repeated_structure_density_tests PROPERTIES
    FOLDER "Tests"
)

set_target_properties(drs_phase1_diagnostics_tests PROPERTIES
    FOLDER "Tests"
)

set_target_properties(drs_phase1_failure_handling_tests PROPERTIES
    FOLDER "Tests"
)

set_target_properties(drs_phase1_macro_bridge_tests PROPERTIES
    FOLDER "Tests"
)

set_target_properties(drs_phase1_runtime_baseline_report PROPERTIES
    FOLDER "Tests"
)

set_target_properties(drs_phase1_sample_import_tests PROPERTIES
    FOLDER "Tests"
)

set_target_properties(drs_wav_import_fixture_support_tests PROPERTIES
    FOLDER "Tests"
)

set_target_properties(drs_wav_import_baseline_report PROPERTIES
    FOLDER "Tests"
)

set_target_properties(drs_wav_import_service_contract_tests PROPERTIES
    FOLDER "Tests"
)

set_target_properties(drs_wav_import_lifecycle_tests PROPERTIES
    FOLDER "Tests"
)

set_target_properties(drs_wav_import_staging_tests PROPERTIES
    FOLDER "Tests"
)

set_target_properties(drs_wav_import_analysis_tests PROPERTIES
    FOLDER "Tests"
)

set_target_properties(drs_phase1_compile_path_tests PROPERTIES
    FOLDER "Tests"
)

set_target_properties(drs_phase1_pipeline_report PROPERTIES
    FOLDER "Tests"
)

set_target_properties(drs_phase1_benchmark_scene PROPERTIES
    FOLDER "Tests"
)

set_target_properties(drs_phase1_stream_reader_tests PROPERTIES
    FOLDER "Tests"
)

set_target_properties(drs_phase1_streaming_service_tests PROPERTIES
    FOLDER "Tests"
)

set_target_properties(drs_phase1_voice_runtime_tests PROPERTIES
    FOLDER "Tests"
)

set_target_properties(drs_phase1_note_routing_tests PROPERTIES
    FOLDER "Tests"
)

set_target_properties(drs_phase1_load_profile_tests PROPERTIES
    FOLDER "Tests"
)

set_target_properties(drs_phase1_runtime_counters_tests PROPERTIES
    FOLDER "Tests"
)

set_target_properties(drs_phase1_runtime_baseline_guard PROPERTIES
    FOLDER "Tests"
)

set_target_properties(drs_phase1_runtime_fixture_tool PROPERTIES
    FOLDER "Tests"
)

juce_add_console_app(drs_ui_message_thread_metrics_tests
    PRODUCT_NAME "Decent Rhapsody Studio UI Message Thread Metrics Tests"
)

target_sources(drs_ui_message_thread_metrics_tests
    PRIVATE
        src/MessageThreadMetricsTests.cpp
)

target_link_libraries(drs_ui_message_thread_metrics_tests
    PRIVATE
        drs_app_shared
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_compile_features(drs_ui_message_thread_metrics_tests PRIVATE cxx_std_17)

add_test(
    NAME drs.ui.message_thread_metrics
    COMMAND drs_ui_message_thread_metrics_tests
)

set_target_properties(drs_ui_message_thread_metrics_tests PROPERTIES
    FOLDER "Tests"
)

add_executable(drs_host_state_publication_service_tests
    src/HostStatePublicationServiceTests.cpp
)

target_link_libraries(drs_host_state_publication_service_tests
    PRIVATE
        drs_engine_adapter
)

target_compile_features(drs_host_state_publication_service_tests PRIVATE cxx_std_17)

add_test(
    NAME drs.host_state.background_publication
    COMMAND drs_host_state_publication_service_tests
)

if(MSVC)
    # These integration mains intentionally keep several large Processor instances alive
    # at once. Processor-owned background workers make the default 1 MiB test stack too
    # small even though production plug-in processors are host/heap-owned.
    target_link_options(drs_phase1_state_recall_tests PRIVATE /STACK:8388608)
    target_link_options(drs_phase1_macro_bridge_tests PRIVATE /STACK:8388608)
    target_link_options(drs_performance_mixer_s0_red_tests PRIVATE /STACK:8388608)
    target_link_options(drs_wav_import_lifecycle_io_audit_tests PRIVATE /STACK:8388608)
endif()

set_tests_properties(drs.host_state.background_publication PROPERTIES
    TIMEOUT 45
)

set_target_properties(drs_host_state_publication_service_tests PROPERTIES
    FOLDER "Tests"
)

juce_add_console_app(drs_ui_responsiveness_baseline
    PRODUCT_NAME "Decent Rhapsody Studio UI Responsiveness Baseline"
)

target_sources(drs_ui_responsiveness_baseline
    PRIVATE
        src/UiResponsivenessBaseline.cpp
)

target_link_libraries(drs_ui_responsiveness_baseline
    PRIVATE
        drs_app_shared
        drs_plugin_shell
        drs_sample_import
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_compile_features(drs_ui_responsiveness_baseline PRIVATE cxx_std_17)
target_compile_definitions(drs_ui_responsiveness_baseline PRIVATE JUCE_MODAL_LOOPS_PERMITTED=1)

add_test(
    NAME drs.ui.responsiveness_baseline
    COMMAND drs_ui_responsiveness_baseline
        "${CMAKE_SOURCE_DIR}/../DemoSFVInstruments/AccurateSalamanderGrandPianoV6.2beta2_48khz24bit/sfz_daw/Accurate-SalamanderGrandPiano_flat.Recommended.sfz"
        "${CMAKE_SOURCE_DIR}/content/runtime/phase1/reference-corpus/tiny-open-instrument/performance-package-corpus/valid.drpkg"
        "${CMAKE_CURRENT_BINARY_DIR}/ui-responsiveness-baseline.json"
)

set_tests_properties(drs.ui.responsiveness_baseline PROPERTIES
    RUN_SERIAL TRUE
    TIMEOUT 300
)

set_target_properties(drs_ui_responsiveness_baseline PROPERTIES
    FOLDER "Tests"
)

juce_add_console_app(drs_performance_responsiveness_tests
    PRODUCT_NAME "Decent Rhapsody Studio Performance Responsiveness Tests"
)

target_sources(drs_performance_responsiveness_tests
    PRIVATE
        src/PerformanceResponsivenessTests.cpp
)

target_link_libraries(drs_performance_responsiveness_tests
    PRIVATE
        drs_app_shared
        drs_sampler_core
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_compile_features(drs_performance_responsiveness_tests PRIVATE cxx_std_17)

add_test(
    NAME drs.ui.performance_responsiveness
    COMMAND drs_performance_responsiveness_tests
)

set_target_properties(drs_performance_responsiveness_tests PROPERTIES
    FOLDER "Tests"
)

add_custom_target(drs_open_workbench_qualification)
add_dependencies(drs_open_workbench_qualification
    drs_open_workbench_phase0_tests
    drs_open_workbench_phase1_tests
    drs_open_workbench_phase2_tests
    drs_open_workbench_phase3_tests
    drs_open_workbench_phase4_tests
    drs_open_workbench_phase5_tests
    drs_phase2_authoring_ui_tests
    drs_phase2_mapping_workspace_tests
    drs_phase2_repeated_structure_density_tests
    drs_phase2_waveform_preview_tests
    drs_phase2_macro_routing_tests
    drs_phase2_performance_ui_tests
    drs_phase2_performance_bank_tests
    drs_performance_engine_s10_articulation_ui_tests
)
set_target_properties(drs_open_workbench_qualification PROPERTIES FOLDER "Tests")
