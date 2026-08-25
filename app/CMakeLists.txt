option(DRS_ENABLE_INSTRUMENT_CONTROLS_UI
       "Expose the imported instrument-controls workbench in application shells"
       ON)

if(DRS_ENABLE_INSTRUMENT_CONTROLS_UI)
    set(DRS_INSTRUMENT_CONTROLS_UI_DEFINE 1)
else()
    set(DRS_INSTRUMENT_CONTROLS_UI_DEFINE 0)
endif()

set(DRS_APP_SHARED_SOURCES
    src/shared/AuthoringPanel.cpp
    src/shared/AuthoringPanel.h
    src/shared/AuthoringPreviewModel.h
    src/shared/InstrumentControlsPanel.cpp
    src/shared/InstrumentControlsPanel.h
    src/shared/HostStateRecoveryBanner.cpp
    src/shared/HostStateRecoveryBanner.h
    src/shared/MessageThreadMetrics.cpp
    src/shared/MessageThreadMetrics.h
    src/shared/midi/DrumPadComponent.cpp
    src/shared/midi/DrumPadComponent.h
    src/shared/PerformanceBankImport.cpp
    src/shared/PerformanceBankImport.h
    src/shared/PerformanceMixer.cpp
    src/shared/PerformanceMixer.h
    src/shared/PerformancePanel.cpp
    src/shared/PerformancePanel.h
    src/shared/PerformancePackageExportService.cpp
    src/shared/PerformancePackageExportService.h
    src/shared/PerformancePackageProjection.cpp
    src/shared/PerformancePackageProjection.h
    src/shared/PlayableInstrumentLicenseViewer.cpp
    src/shared/PlayableInstrumentLicenseViewer.h
    src/shared/ProjectSourceValidationService.cpp
    src/shared/ProjectSourceValidationService.h
    src/shared/ProjectStorage.cpp
    src/shared/ProjectStorage.h
    src/shared/SfzImportReportModel.h
    src/shared/SfzImportReviewService.cpp
    src/shared/SfzImportReviewService.h
    src/shared/SfzImportWorkflow.cpp
    src/shared/SfzImportWorkflow.h
    src/shared/StatusPanel.cpp
    src/shared/StatusPanel.h
    src/shared/WaveformPreviewService.cpp
    src/shared/WaveformPreviewService.h
    src/shared/WaveformSnapService.cpp
    src/shared/WaveformSnapService.h
    src/shared/WavImportService.cpp
    src/shared/WavImportService.h
    src/shared/WavImportWorkflow.cpp
    src/shared/WavImportWorkflow.h
    src/shared/authoring/AuthoringViewModels.h
    src/shared/authoring/AuthoringStructureSelection.cpp
    src/shared/authoring/AuthoringStructureSelection.h
    src/shared/authoring/StructureScope.cpp
    src/shared/authoring/StructureScope.h
    src/shared/authoring/ScopedZoneProjection.cpp
    src/shared/authoring/ScopedZoneProjection.h
    src/shared/authoring/InstrumentStructureBrowser.cpp
    src/shared/authoring/InstrumentStructureBrowser.h
    src/shared/authoring/ZoneMapOverlapIndex.cpp
    src/shared/authoring/ZoneMapOverlapIndex.h
    src/shared/authoring/UnifiedWorkspaceLayout.cpp
    src/shared/authoring/UnifiedWorkspaceLayout.h
    src/shared/authoring/StructureViewModels.cpp
    src/shared/authoring/StructureViewModels.h
    src/shared/authoring/StructureViewState.h
    src/shared/authoring/StructureOverlapPolicy.cpp
    src/shared/authoring/StructureOverlapPolicy.h
    src/shared/authoring/StructureInspector.cpp
    src/shared/authoring/StructureInspector.h
    src/shared/authoring/StructureInspectorHost.h
    src/shared/authoring/StructureZoneList.h
    src/shared/authoring/StructureViewer.cpp
    src/shared/authoring/StructureViewer.h
    src/shared/authoring/AuthoringWorkspaceLayout.h
    src/shared/authoring/OpenWorkbenchVisualSystem.h
     src/shared/authoring/MacroWorkbenchView.cpp
     src/shared/authoring/MacroWorkbenchView.h
    src/shared/authoring/InstrumentControlsWorkbenchView.cpp
    src/shared/authoring/InstrumentControlViewModels.cpp
    src/shared/authoring/InstrumentControlViewModels.h
    src/shared/authoring/MidiLearnCoordinator.cpp
    src/shared/authoring/InstrumentControlsWorkbenchView.h
    src/shared/authoring/MidiLearnCoordinator.h
    src/shared/authoring/RoutingWorkbenchView.cpp
    src/shared/authoring/RoutingWorkbenchView.h
    src/shared/authoring/CompactInspectorPrimitives.cpp
    src/shared/authoring/CompactInspectorPrimitives.h
    src/shared/authoring/RepeatedStructureAdapters.h
    src/shared/authoring/RepeatedStructureList.cpp
    src/shared/authoring/RepeatedStructureList.h
    src/shared/authoring/AuthoringSummaryStrip.cpp
    src/shared/authoring/AuthoringSummaryStrip.h
    src/shared/authoring/WaveformDetailView.cpp
    src/shared/authoring/WaveformDetailView.h
    src/shared/authoring/WorkbenchLayoutState.cpp
    src/shared/authoring/WorkbenchLayoutState.h
    src/shared/authoring/WorkbenchSplitter.cpp
    src/shared/authoring/WorkbenchSplitter.h
    src/shared/authoring/ZoneMappingEditor.cpp
    src/shared/authoring/ZoneMappingEditor.h
    src/shared/authoring/ZoneMapCanvas.cpp
    src/shared/authoring/ZoneMapCanvas.h
    src/shared/authoring/ZoneMapOverview.cpp
    src/shared/authoring/ZoneMapOverview.h
    src/shared/authoring/ZoneMapRenderPolicy.h
    src/shared/authoring/ZoneMapViewState.cpp
    src/shared/authoring/ZoneMapViewState.h
)

set(DRS_STANDALONE_SOURCES
    src/standalone/MainComponent.cpp
    src/standalone/MainComponent.h
)

set(DRS_PLUGIN_SOURCES
    src/plugin/PluginEditor.cpp
    src/plugin/PluginEditor.h
    src/plugin/PluginProcessor.cpp
    src/plugin/PluginProcessor.h
    src/plugin/RealtimeGuard.cpp
    src/plugin/RealtimeGuard.h
)

add_library(drs_app_shared STATIC
    ${DRS_APP_SHARED_SOURCES}
)

target_include_directories(drs_app_shared
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(drs_app_shared
    PUBLIC
        drs_engine_adapter
        drs_sample_import
        juce::juce_audio_utils
        juce::juce_gui_extra
)

target_compile_definitions(drs_app_shared
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        DRS_ENABLE_INSTRUMENT_CONTROLS_UI=${DRS_INSTRUMENT_CONTROLS_UI_DEFINE}
)

target_compile_features(drs_app_shared PUBLIC cxx_std_17)

add_library(drs_standalone_shell STATIC
    ${DRS_STANDALONE_SOURCES}
)

target_include_directories(drs_standalone_shell
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(drs_standalone_shell
    PUBLIC
        drs_app_shared
        drs_plugin_shell
)

target_compile_features(drs_standalone_shell PUBLIC cxx_std_17)

add_library(drs_plugin_shell STATIC
    ${DRS_PLUGIN_SOURCES}
)

target_include_directories(drs_plugin_shell
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(drs_plugin_shell
    PUBLIC
        drs_app_shared
        drs_sample_import
        drs_sampler_core
        juce::juce_audio_formats
        juce::juce_audio_utils
)

target_compile_definitions(drs_plugin_shell
    PUBLIC
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_compile_features(drs_plugin_shell PUBLIC cxx_std_17)

juce_add_gui_app(DecentRhapsodyStudioApp
    PRODUCT_NAME "${DRS_PRODUCT_DISPLAY_NAME}"
    COMPANY_NAME "${DRS_COMPANY_DISPLAY_NAME}"
    BUNDLE_ID "com.decentrhapsody.studio.app"
    NEEDS_WEB_BROWSER FALSE
    NEEDS_CURL FALSE
)

target_sources(DecentRhapsodyStudioApp
    PRIVATE
        src/standalone/Main.cpp
)

target_compile_definitions(DecentRhapsodyStudioApp
    PRIVATE
        JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:DecentRhapsodyStudioApp,JUCE_PRODUCT_NAME>"
        JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:DecentRhapsodyStudioApp,JUCE_VERSION>"
)

target_link_libraries(DecentRhapsodyStudioApp
    PRIVATE
        drs_standalone_shell
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

juce_add_plugin(drs_plugin_bundle
    COMPANY_NAME "${DRS_COMPANY_DISPLAY_NAME}"
    BUNDLE_ID "com.decentrhapsody.studio.plugin"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE Dcrh
    PLUGIN_CODE Drs0
    FORMATS VST3
    PRODUCT_NAME "${DRS_PRODUCT_DISPLAY_NAME}"
    NEEDS_WEB_BROWSER FALSE
    NEEDS_CURL FALSE
)

target_link_libraries(drs_plugin_bundle
    PRIVATE
        drs_plugin_shell
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

get_target_property(DRS_PLUGIN_VST3_ARTEFACT drs_plugin_bundle_VST3 JUCE_PLUGIN_ARTEFACT_FILE)
set(DRS_PLUGIN_LEGACY_VST3_DIR
    "${CMAKE_CURRENT_BINARY_DIR}/DecentRhapsodyStudioPlugin_artefacts/$<CONFIG>/VST3/${DRS_PRODUCT_DISPLAY_NAME}.vst3")

add_custom_command(TARGET drs_plugin_bundle_VST3 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
        "${CMAKE_CURRENT_BINARY_DIR}/DecentRhapsodyStudioPlugin_artefacts/$<CONFIG>/VST3"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${DRS_PLUGIN_VST3_ARTEFACT}"
        "${DRS_PLUGIN_LEGACY_VST3_DIR}"
    COMMENT "Syncing VST3 bundle into legacy DecentRhapsodyStudioPlugin_artefacts path"
)

add_custom_target(DecentRhapsodyStudioPlugin
    DEPENDS drs_plugin_bundle_VST3
)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES
    ${DRS_APP_SHARED_SOURCES}
    ${DRS_STANDALONE_SOURCES}
    src/standalone/Main.cpp
)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES
    src/plugin/PluginEditor.cpp
    src/plugin/PluginEditor.h
    src/plugin/PluginProcessor.cpp
    src/plugin/PluginProcessor.h
    src/plugin/RealtimeGuard.cpp
    src/plugin/RealtimeGuard.h
)

set_target_properties(drs_app_shared PROPERTIES FOLDER "Product")
set_target_properties(drs_standalone_shell PROPERTIES FOLDER "Product")
set_target_properties(drs_plugin_shell PROPERTIES FOLDER "Product")
set_target_properties(DecentRhapsodyStudioApp PROPERTIES FOLDER "Product")
set_target_properties(drs_plugin_bundle PROPERTIES FOLDER "Product")
set_target_properties(drs_plugin_bundle_VST3 PROPERTIES FOLDER "Product")
set_target_properties(DecentRhapsodyStudioPlugin PROPERTIES FOLDER "Product")
