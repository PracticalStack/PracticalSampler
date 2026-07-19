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

# The smoke executable scans the built VST3 bundle; make that runtime fixture a
# build dependency so a narrow smoke-only build is self-contained.
add_dependencies(drs_phase0_smoke_tests DecentRhapsodyStudioPlugin)

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

# Sprint 4.7 deterministic shell-free offline render conformance matrix.
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

add_custom_target(drs_all_tests)

add_dependencies(drs_all_tests
    DecentRhapsodyStudioPlugin
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
    drs_phase2_mapping_workspace_tests
    drs_phase2_waveform_preview_tests
    drs_phase2_macro_routing_tests
    drs_phase2_performance_bank_tests
    drs_phase2_performance_ui_tests
    drs_phase2_authoring_playback_integration_tests
    drs_phase2_authoring_ui_tests
    drs_phase2_repeated_structure_density_tests
    drs_phase1_diagnostics_tests
    drs_phase1_failure_handling_tests
    drs_phase1_macro_bridge_tests
    drs_phase1_runtime_baseline_report
    drs_phase1_sample_import_tests
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

set_target_properties(drs_phase2_authoring_foundation_tests PROPERTIES
    FOLDER "Tests"
)

set_target_properties(drs_phase2_authoring_import_tests PROPERTIES
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
