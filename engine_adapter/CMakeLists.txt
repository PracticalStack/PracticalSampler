set(DRS_HISE_VENDOR_ROOT "${CMAKE_SOURCE_DIR}/third_party/hise")
set(DRS_HISE_CURRENT_GIT_HASH_FILE "${DRS_HISE_VENDOR_ROOT}/currentGitHash.txt")
set(DRS_HISE_HI_CORE_HEADER "${DRS_HISE_VENDOR_ROOT}/hi_core/hi_core.h")
set(DRS_HISE_REST_SERVER_HEADER "${DRS_HISE_VENDOR_ROOT}/hi_backend/backend/ai_tools/RestServer.h")
set(DRS_HISE_PROJUCER_DIR "${DRS_HISE_VENDOR_ROOT}/tools/projucer")
set(DRS_HISE_SDK_DIR "${DRS_HISE_VENDOR_ROOT}/tools/SDK")
set(DRS_HISE_JUCE_ROOT "${DRS_HISE_VENDOR_ROOT}/JUCE")

set(DRS_HISE_VENDOR_PRESENT FALSE)
set(DRS_HISE_JUCE_PRESENT FALSE)
set(DRS_HISE_PROJECT_TEMPLATE_PRESENT FALSE)
set(DRS_HISE_WINDOWS_SETUP_DOC_PRESENT FALSE)
set(DRS_HISE_SDK_ZIP_PRESENT FALSE)
set(DRS_HISE_SDK_EXTRACTED FALSE)
set(DRS_HISE_PROJUCER_WINDOWS_PRESENT FALSE)
set(DRS_HISE_PROJUCER_README_PRESENT FALSE)
set(DRS_HISE_CURRENT_GIT_HASH "unknown")
set(DRS_HISE_HI_CORE_VERSION "unknown")
set(DRS_HISE_REST_API_VERSION "unknown")

function(drs_bool_to_int input output)
    if(${input})
        set(${output} 1 PARENT_SCOPE)
    else()
        set(${output} 0 PARENT_SCOPE)
    endif()
endfunction()

if(EXISTS "${DRS_HISE_VENDOR_ROOT}")
    set(DRS_HISE_VENDOR_PRESENT TRUE)
endif()

if(EXISTS "${DRS_HISE_JUCE_ROOT}/modules/juce_core/juce_core.h")
    set(DRS_HISE_JUCE_PRESENT TRUE)
endif()

if(EXISTS "${DRS_HISE_VENDOR_ROOT}/hi_backend/backend/ProjectTemplate.cpp")
    set(DRS_HISE_PROJECT_TEMPLATE_PRESENT TRUE)
endif()

if(EXISTS "${DRS_HISE_VENDOR_ROOT}/hise-setup-windows.md")
    set(DRS_HISE_WINDOWS_SETUP_DOC_PRESENT TRUE)
endif()

if(EXISTS "${DRS_HISE_SDK_DIR}/sdk.zip")
    set(DRS_HISE_SDK_ZIP_PRESENT TRUE)
endif()

if(EXISTS "${DRS_HISE_SDK_DIR}/ASIOSDK2.3" AND EXISTS "${DRS_HISE_SDK_DIR}/VST3 SDK")
    set(DRS_HISE_SDK_EXTRACTED TRUE)
endif()

if(EXISTS "${DRS_HISE_PROJUCER_DIR}/Projucer.exe")
    set(DRS_HISE_PROJUCER_WINDOWS_PRESENT TRUE)
endif()

if(EXISTS "${DRS_HISE_PROJUCER_DIR}/readme.md")
    set(DRS_HISE_PROJUCER_README_PRESENT TRUE)
endif()

if(EXISTS "${DRS_HISE_CURRENT_GIT_HASH_FILE}")
    file(READ "${DRS_HISE_CURRENT_GIT_HASH_FILE}" DRS_HISE_CURRENT_GIT_HASH_RAW)
    string(STRIP "${DRS_HISE_CURRENT_GIT_HASH_RAW}" DRS_HISE_CURRENT_GIT_HASH)
endif()

if(EXISTS "${DRS_HISE_HI_CORE_HEADER}")
    file(READ "${DRS_HISE_HI_CORE_HEADER}" DRS_HISE_HI_CORE_HEADER_RAW)
    string(REGEX MATCH "version:[ \t]+([0-9]+\\.[0-9]+\\.[0-9]+)" DRS_HISE_HI_CORE_VERSION_MATCH "${DRS_HISE_HI_CORE_HEADER_RAW}")
    if(CMAKE_MATCH_1)
        set(DRS_HISE_HI_CORE_VERSION "${CMAKE_MATCH_1}")
    endif()
endif()

if(EXISTS "${DRS_HISE_REST_SERVER_HEADER}")
    file(READ "${DRS_HISE_REST_SERVER_HEADER}" DRS_HISE_REST_SERVER_HEADER_RAW)
    string(REGEX MATCH "#define[ \t]+HISE_REST_API_VERSION[ \t]+\"([^\"]+)\"" DRS_HISE_REST_API_VERSION_MATCH "${DRS_HISE_REST_SERVER_HEADER_RAW}")
    if(CMAKE_MATCH_1)
        set(DRS_HISE_REST_API_VERSION "${CMAKE_MATCH_1}")
    endif()
endif()

drs_bool_to_int(DRS_HISE_VENDOR_PRESENT DRS_HISE_VENDOR_PRESENT_INT)
drs_bool_to_int(DRS_HISE_JUCE_PRESENT DRS_HISE_JUCE_PRESENT_INT)
drs_bool_to_int(DRS_HISE_PROJECT_TEMPLATE_PRESENT DRS_HISE_PROJECT_TEMPLATE_PRESENT_INT)
drs_bool_to_int(DRS_HISE_WINDOWS_SETUP_DOC_PRESENT DRS_HISE_WINDOWS_SETUP_DOC_PRESENT_INT)
drs_bool_to_int(DRS_HISE_SDK_ZIP_PRESENT DRS_HISE_SDK_ZIP_PRESENT_INT)
drs_bool_to_int(DRS_HISE_SDK_EXTRACTED DRS_HISE_SDK_EXTRACTED_INT)
drs_bool_to_int(DRS_HISE_PROJUCER_WINDOWS_PRESENT DRS_HISE_PROJUCER_WINDOWS_PRESENT_INT)
drs_bool_to_int(DRS_HISE_PROJUCER_README_PRESENT DRS_HISE_PROJUCER_README_PRESENT_INT)

set(DRS_ENGINE_ADAPTER_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(MAKE_DIRECTORY "${DRS_ENGINE_ADAPTER_GENERATED_DIR}")
file(MAKE_DIRECTORY "${DRS_ENGINE_ADAPTER_GENERATED_DIR}/drs/engine")
file(MAKE_DIRECTORY "${DRS_ENGINE_ADAPTER_GENERATED_DIR}/hise_plugin_frontend")

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/HiseVendorInfo.generated.h.in"
    "${DRS_ENGINE_ADAPTER_GENERATED_DIR}/drs/engine/HiseVendorInfo.generated.h"
    @ONLY
)

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/WorkspacePaths.generated.h.in"
    "${DRS_ENGINE_ADAPTER_GENERATED_DIR}/drs/engine/WorkspacePaths.generated.h"
    @ONLY
)

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/HisePluginFrontendAppConfig.h.in"
    "${DRS_ENGINE_ADAPTER_GENERATED_DIR}/hise_plugin_frontend/AppConfig.h"
    @ONLY
)

add_library(drs_hise_frontend_bridge STATIC
    src/HiseFrontendBridge.cpp
    include/drs/engine/HiseFrontendBridge.h
)

target_include_directories(drs_hise_frontend_bridge
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${DRS_ENGINE_ADAPTER_GENERATED_DIR}/hise_plugin_frontend
        ${CMAKE_SOURCE_DIR}/third_party/hise
)

target_compile_features(drs_hise_frontend_bridge PUBLIC cxx_std_17)

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
    src/SamplerPlaybackContext.cpp
    src/SamplerRenderModel.cpp
    src/SamplerVoice.cpp
    src/SamplerVoicePool.cpp
    include/drs/engine/AuthoringPreviewPreparation.h
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
    src/HiseProjectContent.cpp
    src/HostSessionState.cpp
    src/PlaybackSnapshot.cpp
    src/PerformancePublishController.cpp
    src/PerformancePublishCommandAdapter.cpp
    src/PerformancePublishPreparation.cpp
    src/PerformancePublishPresentation.cpp
    src/PerformanceProgram.cpp
    src/PerformanceRuleContract.cpp
    src/PublishedMacroBinding.cpp
    src/PreparedPlayback.cpp
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
    src/RuntimeStreamingService.cpp
    src/RuntimeStream.cpp
    src/RuntimeVoice.cpp
    include/drs/engine/AuthoringPreviewCommandAdapter.h
    include/drs/engine/AuthoringPreviewContract.h
    include/drs/engine/AuthoringPreviewController.h
    include/drs/engine/AuthoringPreviewRecovery.h
    include/drs/engine/AuthoringSession.h
    include/drs/engine/ControlLaw.h
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
    include/drs/engine/PlaybackSnapshot.h
    include/drs/engine/PerformancePublishContract.h
    include/drs/engine/PerformancePublishCommandAdapter.h
    include/drs/engine/PerformancePublishController.h
    include/drs/engine/PerformancePublishPreparation.h
    include/drs/engine/PerformancePublishPresentation.h
    include/drs/engine/PerformanceProgram.h
    include/drs/engine/PerformanceRuleContract.h
    include/drs/engine/PublishedMacroBinding.h
    include/drs/engine/PreparedPlayback.h
    include/drs/engine/ProjectDocument.h
    include/drs/engine/ProjectRestoreCoordinator.h
    include/drs/engine/RuntimePresetState.h
    include/drs/engine/RuntimeLoadProfile.h
    include/drs/engine/SfzImportProjection.h
    include/drs/engine/SfzImportReport.h
    include/drs/engine/SfzImport.h
    include/drs/engine/SfzImportContract.h
)

target_include_directories(drs_engine_adapter
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${DRS_ENGINE_ADAPTER_GENERATED_DIR}
    PRIVATE
        ${CMAKE_SOURCE_DIR}/third_party/hise/hi_tools/hi_neural/RTNeural/modules
)

target_compile_features(drs_engine_adapter PUBLIC cxx_std_17)
target_link_libraries(drs_engine_adapter
    PUBLIC
        drs_hise_frontend_bridge
)

target_link_libraries(drs_sampler_core
    PUBLIC
        drs_engine_adapter
)

add_library(drs_hise_frontend_plugin_probe STATIC
    src/HiseFrontendPluginProbe.cpp
)

target_include_directories(drs_hise_frontend_plugin_probe
    PRIVATE
        ${DRS_ENGINE_ADAPTER_GENERATED_DIR}/hise_plugin_frontend
        ${CMAKE_SOURCE_DIR}/third_party/hise
        ${CMAKE_SOURCE_DIR}/third_party/hise/JUCE/modules
)

target_compile_features(drs_hise_frontend_plugin_probe PRIVATE cxx_std_17)

set_target_properties(drs_hise_frontend_plugin_probe PROPERTIES
    FOLDER "Product"
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
    src/HiseFrontendBridge.cpp
    src/HiseFrontendPluginProbe.cpp
    src/HiseProjectContent.cpp
    src/HostSessionState.cpp
    src/PlaybackSnapshot.cpp
    src/PerformancePublishController.cpp
    src/PerformancePublishCommandAdapter.cpp
    src/PerformancePublishPreparation.cpp
    src/PerformancePublishPresentation.cpp
    src/PerformanceRuleContract.cpp
    src/PublishedMacroBinding.cpp
    src/PreparedPlayback.cpp
    src/Phase1Baseline.cpp
    src/ProjectDocument.cpp
    src/RuntimeCompiler.cpp
    src/RuntimeLoader.cpp
    src/RuntimePresetState.cpp
    src/RuntimeStreamingService.cpp
    src/RuntimeStream.cpp
    src/RuntimeVoice.cpp
    src/SfzImportReport.cpp
    src/SfzImport.cpp
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
    include/drs/engine/HiseFrontendBridge.h
    include/drs/engine/HiseProjectContent.h
    include/drs/engine/HostSessionState.h
    include/drs/engine/PlaybackSnapshot.h
    include/drs/engine/PerformancePublishContract.h
    include/drs/engine/PerformancePublishController.h
    include/drs/engine/PerformancePublishPreparation.h
    include/drs/engine/PerformanceRuleContract.h
    include/drs/engine/PreparedPlayback.h
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
    include/drs/engine/SamplerPlaybackContext.h
    include/drs/engine/SampleImport.h
    include/drs/engine/SfzImport.h
    include/drs/engine/SfzImportContract.h
    include/drs/engine/SamplerRenderModel.h
    include/drs/engine/SamplerVoice.h
    include/drs/engine/SamplerVoicePool.h
)

set_target_properties(drs_hise_frontend_bridge PROPERTIES
    FOLDER "Product"
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
