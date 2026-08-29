set(DRS_ENGINE_ADAPTER_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(MAKE_DIRECTORY "${DRS_ENGINE_ADAPTER_GENERATED_DIR}")
file(MAKE_DIRECTORY "${DRS_ENGINE_ADAPTER_GENERATED_DIR}/drs/engine")

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/WorkspacePaths.generated.h.in"
    "${DRS_ENGINE_ADAPTER_GENERATED_DIR}/drs/engine/WorkspacePaths.generated.h"
    @ONLY
)

set(DRS_PACKAGE_PUBLISHER_KEY_ID "" CACHE STRING
    "Public package-recognition signing-key ID embedded in the desktop trust store")
set(DRS_PACKAGE_PUBLISHER_PUBLIC_KEY_HEX "" CACHE STRING
    "Public 32-byte Ed25519 verification key as 64 hexadecimal characters")
set(DRS_PACKAGE_PUBLISHER_KEY_ACTIVATED_UTC "" CACHE STRING
    "Activation timestamp for the embedded public package-recognition key")
set(DRS_PACKAGE_PUBLISHER_TRUST_ENTRIES "" CACHE STRING
    "Semicolon-separated public package-recognition entries: keyId|publicKeyHex|active/retired/revoked|activatedUtc|retiredUtc|revokedUtc")
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/PackageTrustStore.generated.h.in"
    "${DRS_ENGINE_ADAPTER_GENERATED_DIR}/drs/engine/PackageTrustStore.generated.h"
    @ONLY
)

set(DRS_OFFLINE_PACKAGE_PROFILE_HEADER "" CACHE FILEPATH
    "Generated offline package profile header; omit to build without the offline production profile")
if(DRS_OFFLINE_PACKAGE_PROFILE_HEADER)
    if(NOT EXISTS "${DRS_OFFLINE_PACKAGE_PROFILE_HEADER}")
        message(FATAL_ERROR
            "DRS_OFFLINE_PACKAGE_PROFILE_HEADER does not exist: ${DRS_OFFLINE_PACKAGE_PROFILE_HEADER}")
    endif()
    configure_file(
        "${DRS_OFFLINE_PACKAGE_PROFILE_HEADER}"
        "${DRS_ENGINE_ADAPTER_GENERATED_DIR}/drs/engine/PackageOfflineProtection.generated.h"
        COPYONLY
    )
else()
    set(DRS_OFFLINE_PACKAGE_PROFILE_ID "")
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/PackageOfflineProtection.generated.h.in"
        "${DRS_ENGINE_ADAPTER_GENERATED_DIR}/drs/engine/PackageOfflineProtection.generated.h"
        @ONLY
    )
endif()

add_library(drs_sample_import STATIC
    src/SampleImport.cpp
    include/drs/engine/SampleImport.h
)

target_include_directories(drs_sample_import
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_SOURCE_DIR}/third_party/juce/modules
)

target_compile_definitions(drs_sample_import
    PRIVATE
        JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1
        JUCE_MODULE_AVAILABLE_juce_audio_basics=1
        JUCE_MODULE_AVAILABLE_juce_audio_formats=1
        JUCE_MODULE_AVAILABLE_juce_core=1
)

target_compile_features(drs_sample_import PUBLIC cxx_std_17)

add_library(drs_sampler_core STATIC
    src/AuthoringPreviewPreparation.cpp
    src/DeferredPackageSession.cpp
    src/PerformanceLaneState.cpp
    src/SamplerPlaybackContext.cpp
    src/SamplerRenderModel.cpp
    src/SamplerVoice.cpp
    src/SamplerVoicePool.cpp
    include/drs/engine/AuthoringPreviewPreparation.h
    include/drs/engine/DeferredPackageSession.h
    include/drs/engine/PerformanceLaneState.h
    include/drs/engine/SamplerPlaybackContext.h
    include/drs/engine/SamplerRenderModel.h
    include/drs/engine/SamplerVoice.h
    include/drs/engine/SamplerVoicePool.h
)

target_include_directories(drs_sampler_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(drs_sampler_core PUBLIC cxx_std_17)

add_library(drs_engine_adapter STATIC
    src/AuthoringPreviewCommandAdapter.cpp
    src/AuthoringPreviewController.cpp
    src/AuthoringPreviewRecovery.cpp
    src/AuthoringSession.cpp
    src/ControlLaw.cpp
    src/ContinuousDamper.cpp
    src/CuratedDspCatalog.cpp
    src/DraftPlaybackContract.cpp
    src/DspGain.cpp
    src/DspAlgorithmicReverb.cpp
    src/DspCompactEq.cpp
    src/DspChorus.cpp
    src/DspSaturator.cpp
    src/DspStereoDelay.cpp
    src/DspGraphPlan.cpp
    src/DspParameterControl.cpp
    src/DspRenderGeneration.cpp
    src/EngineFacade.cpp
    src/HostSessionState.cpp
    src/HostStatePublicationService.cpp
    src/InstrumentControlContract.cpp
    src/InstrumentControlBinding.cpp
    src/LayerMaterializer.cpp
    src/PlaybackSnapshot.cpp
    src/PlaybackSnapshotWorker.cpp
    src/PerformancePublishController.cpp
    src/PerformancePublishCommandAdapter.cpp
    src/PerformancePublishPreparation.cpp
    src/PerformancePublishPresentation.cpp
    src/PerformanceProgram.cpp
    src/PerformanceRuleContract.cpp
    src/PlayableInstrumentLicense.cpp
    src/PackageCrypto.cpp
    src/PackageKeys.cpp
    src/OfflinePackageKeys.cpp
    src/PackageOfflineRecognitionSigner.cpp
    src/PackageSignature.cpp
    src/PackageV3.cpp
    src/PackageV3FileReader.cpp
    src/PackageV3StreamingExport.cpp
    src/PackageKeyEnvelope.cpp
    src/PackageProtectionDiagnostics.cpp
    src/PackagePublisherTrustStore.cpp
    src/PackageReader.cpp
    src/PackageReaderDispatch.cpp
    src/PackageV2.cpp
    src/SecureBuffer.cpp
    src/PackageWriter.cpp
    src/PublishedMacroBinding.cpp
    src/PreparedPlayback.cpp
    src/SampleDataSource.cpp
    src/Phase1Baseline.cpp
    src/ProjectDocument.cpp
    src/ProjectRestoreCoordinator.cpp
    src/RuntimeCompiler.cpp
    src/RuntimeLoadProfile.cpp
    src/RuntimeLoader.cpp
    src/RuntimePresetState.cpp
    src/SfzImportProjection.cpp
    src/SfzImportReport.cpp
    src/SfzImport.cpp
    src/SfzRegionContract.cpp
    src/WaveformRegionPolicy.cpp
    src/RuntimeStreamingService.cpp
    src/RuntimeStream.cpp
    src/RuntimeVoice.cpp
    src/NativeContent.cpp
    src/VelocityCrossfadeAuthoring.cpp
    include/drs/engine/AuthoringPreviewCommandAdapter.h
    include/drs/engine/AuthoringPreviewContract.h
    include/drs/engine/AuthoringPreviewController.h
    include/drs/engine/AuthoringPreviewRecovery.h
    include/drs/engine/AuthoringSession.h
    include/drs/engine/LayerContract.h
    include/drs/engine/LayerMaterializer.h
    include/drs/engine/VelocityCrossfadeAuthoring.h
    include/drs/engine/PackageSignature.h
    include/drs/engine/SecureBuffer.h
    include/drs/engine/PackageKeys.h
    include/drs/engine/PackageV3.h
    include/drs/engine/PackageV3FileReader.h
    include/drs/engine/PackageV3StreamingExport.h
    include/drs/engine/PackageKeyEnvelope.h
    include/drs/engine/PackageProtectionDiagnostics.h
    include/drs/engine/PackagePublisherTrustStore.h
    include/drs/engine/PackagePublisherSigning.h
    include/drs/engine/PackageOfflineRecognitionSigner.h
    include/drs/engine/ControlLaw.h
    include/drs/engine/ContinuousDamper.h
    include/drs/engine/ContinuousDamperContract.h
    include/drs/engine/CuratedDspCatalog.h
    include/drs/engine/DraftPlaybackContract.h
    include/drs/engine/DspGain.h
    include/drs/engine/DspAlgorithmicReverb.h
    include/drs/engine/DspCompactEq.h
    include/drs/engine/DspChorus.h
    include/drs/engine/DspSaturator.h
    include/drs/engine/DspStereoDelay.h
    include/drs/engine/DspGraphPlan.h
    include/drs/engine/DspParameterControl.h
    include/drs/engine/DspRenderGeneration.h
    include/drs/engine/HostSessionState.h
    include/drs/engine/HostStatePublicationService.h
    include/drs/engine/InstrumentControlContract.h
    include/drs/engine/InstrumentControlBinding.h
    include/drs/engine/PlaybackSnapshot.h
    include/drs/engine/PlaybackSnapshotWorker.h
    include/drs/engine/PerformancePublishContract.h
    include/drs/engine/PerformancePublishCommandAdapter.h
    include/drs/engine/PerformancePublishController.h
    include/drs/engine/PerformancePublishPreparation.h
    include/drs/engine/PerformancePublishPresentation.h
    include/drs/engine/PerformanceProgram.h
    include/drs/engine/PerformanceRuleContract.h
    include/drs/engine/PlayableInstrumentLicense.h
    include/drs/engine/PackageCrypto.h
    include/drs/engine/PackageReader.h
    include/drs/engine/PackageReaderDispatch.h
    include/drs/engine/PackageV2.h
    include/drs/engine/PackageV2StreamingExport.h
    include/drs/engine/PackageWriter.h
    include/drs/engine/PublishedMacroBinding.h
    include/drs/engine/PreparedPlayback.h
    include/drs/engine/SampleDataSource.h
    include/drs/engine/ProjectDocument.h
    include/drs/engine/ProjectRestoreCoordinator.h
    include/drs/engine/RuntimePresetState.h
    include/drs/engine/RuntimeLoadProfile.h
    include/drs/engine/SfzImportProjection.h
    include/drs/engine/SfzImportReport.h
    include/drs/engine/SfzImport.h
    include/drs/engine/SfzImportContract.h
    include/drs/engine/SfzRegionContract.h
    include/drs/engine/WaveformRegionPolicy.h
)

target_include_directories(drs_engine_adapter
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${DRS_ENGINE_ADAPTER_GENERATED_DIR}
        ${CMAKE_SOURCE_DIR}/third_party/nlohmann
)

target_compile_features(drs_engine_adapter PUBLIC cxx_std_17)

target_link_libraries(drs_engine_adapter
    PRIVATE
        drs_libsodium
)

target_compile_definitions(drs_engine_adapter PRIVATE
    SODIUM_STATIC=1
    DRS_PRODUCTION_PACKAGE_READERS_ONLY=1
)

get_target_property(DRS_ENGINE_ADAPTER_PRODUCT_SOURCES drs_engine_adapter SOURCES)
foreach(DRS_LEGACY_WRITER_ONLY_SOURCE src/PackageV2StreamingExport.cpp)
    list(FIND DRS_ENGINE_ADAPTER_PRODUCT_SOURCES
        "${DRS_LEGACY_WRITER_ONLY_SOURCE}" DRS_LEGACY_WRITER_SOURCE_INDEX)
    if(NOT DRS_LEGACY_WRITER_SOURCE_INDEX EQUAL -1)
        message(FATAL_ERROR
            "Legacy package writer-only source is linked into drs_engine_adapter: ${DRS_LEGACY_WRITER_ONLY_SOURCE}")
    endif()
endforeach()

# V1/V2 writers remain available solely to deterministic compatibility-fixture
# tests. Production consumers link drs_engine_adapter, whose legacy writer entry
# points are compiled out.
add_library(drs_legacy_package_writers STATIC
    src/PackageV2.cpp
    src/PackageV2StreamingExport.cpp
    src/PackageWriter.cpp
)

target_include_directories(drs_legacy_package_writers
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${DRS_ENGINE_ADAPTER_GENERATED_DIR}
        ${CMAKE_SOURCE_DIR}/third_party/nlohmann
)

target_compile_features(drs_legacy_package_writers PUBLIC cxx_std_17)
target_compile_definitions(drs_legacy_package_writers PRIVATE
    DRS_ENABLE_LEGACY_PACKAGE_WRITERS=1
    DRS_LEGACY_PACKAGE_WRITERS_ONLY=1
)
target_link_libraries(drs_legacy_package_writers PUBLIC drs_engine_adapter)

target_link_libraries(drs_sampler_core
    PUBLIC
        drs_engine_adapter
)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES
    src/AuthoringPreviewPreparation.cpp
    src/AuthoringPreviewCommandAdapter.cpp
    src/AuthoringPreviewController.cpp
    src/AuthoringPreviewRecovery.cpp
    src/AuthoringSession.cpp
    src/ControlLaw.cpp
    src/DraftPlaybackContract.cpp
    src/EngineFacade.cpp
    src/HostSessionState.cpp
    src/HostStatePublicationService.cpp
    src/PlaybackSnapshot.cpp
    src/PlaybackSnapshotWorker.cpp
    src/PerformancePublishController.cpp
    src/PerformancePublishCommandAdapter.cpp
    src/PerformancePublishPreparation.cpp
    src/PerformancePublishPresentation.cpp
    src/PerformanceRuleContract.cpp
    src/PackageCrypto.cpp
    src/PackageReader.cpp
    src/PackageReaderDispatch.cpp
    src/PackageV2.cpp
    src/PackageV2StreamingExport.cpp
    src/PackageWriter.cpp
    src/PublishedMacroBinding.cpp
    src/PreparedPlayback.cpp
    src/SampleDataSource.cpp
    src/Phase1Baseline.cpp
    src/ProjectDocument.cpp
    src/RuntimeCompiler.cpp
    src/RuntimeLoader.cpp
    src/RuntimePresetState.cpp
    src/RuntimeStreamingService.cpp
    src/RuntimeStream.cpp
    src/RuntimeVoice.cpp
    src/NativeContent.cpp
    src/SfzImportReport.cpp
    src/SfzImport.cpp
    src/SfzRegionContract.cpp
    src/WaveformRegionPolicy.cpp
    src/SamplerPlaybackContext.cpp
    src/SamplerRenderModel.cpp
    src/SamplerVoice.cpp
    src/SamplerVoicePool.cpp
    src/SampleImport.cpp
    include/drs/engine/AuthoringPreviewPreparation.h
    include/drs/engine/AuthoringPreviewCommandAdapter.h
    include/drs/engine/AuthoringPreviewContract.h
    include/drs/engine/AuthoringPreviewController.h
    include/drs/engine/AuthoringPreviewRecovery.h
    include/drs/engine/AuthoringSession.h
    include/drs/engine/ControlLaw.h
    include/drs/engine/DraftPlaybackContract.h
    include/drs/engine/EngineFacade.h
    include/drs/engine/Phase1Baseline.h
    include/drs/engine/HostSessionState.h
    include/drs/engine/HostStatePublicationService.h
    include/drs/engine/PlaybackSnapshot.h
    include/drs/engine/PlaybackSnapshotWorker.h
    include/drs/engine/PerformancePublishContract.h
    include/drs/engine/PerformancePublishController.h
    include/drs/engine/PerformancePublishPreparation.h
    include/drs/engine/PerformanceRuleContract.h
    include/drs/engine/PackageCrypto.h
    include/drs/engine/PackageReader.h
    include/drs/engine/PackageWriter.h
    include/drs/engine/PreparedPlayback.h
    include/drs/engine/SampleDataSource.h
    include/drs/engine/ProjectDocument.h
    include/drs/engine/RuntimeCompiler.h
    include/drs/engine/RuntimeLoadProfile.h
    include/drs/engine/RuntimeLoader.h
    include/drs/engine/RuntimePresetState.h
    include/drs/engine/RuntimeModel.h
    include/drs/engine/SfzImportReport.h
    include/drs/engine/RuntimeStreamingService.h
    include/drs/engine/RuntimeStream.h
    include/drs/engine/RuntimeVoice.h
    include/drs/engine/NativeContent.h
    include/drs/engine/SamplerPlaybackContext.h
    include/drs/engine/SampleImport.h
    include/drs/engine/SfzImport.h
    include/drs/engine/SfzImportContract.h
    include/drs/engine/SfzRegionContract.h
    include/drs/engine/WaveformRegionPolicy.h
    include/drs/engine/SamplerRenderModel.h
    include/drs/engine/SamplerVoice.h
    include/drs/engine/SamplerVoicePool.h
)

set_target_properties(drs_sample_import PROPERTIES
    FOLDER "Product"
)

set_target_properties(drs_sampler_core PROPERTIES
    FOLDER "Product"
)

set_target_properties(drs_engine_adapter PROPERTIES
    FOLDER "Product"
)
