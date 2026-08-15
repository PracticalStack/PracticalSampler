#include "drs/engine/AuthoringSession.h"
#include "drs/engine/DraftPlaybackContract.h"
#include "drs/engine/EngineFacade.h"
#include "drs/engine/PlaybackSnapshot.h"
#include "drs/engine/PreparedPlayback.h"
#include "drs/engine/PackageV2StreamingExport.h"
#include "drs/engine/DeferredPackageSession.h"
#include "drs/engine/SamplerPlaybackContext.h"
#include "drs/engine/SamplerRenderModel.h"
#include "drs/engine/SamplerVoicePool.h"
#include "drs/engine/SfzImportProjection.h"
#include "drs/engine/PackageReaderDispatch.h"
#include "shared/PerformancePackageExportService.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#if defined(_WIN32)
#include <Windows.h>
#include <Psapi.h>
#endif

namespace
{
namespace fs = std::filesystem;
using Clock = std::chrono::steady_clock;

void require(const bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

std::uint64_t elapsedMicros(const Clock::time_point start)
{
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(Clock::now() - start).count());
}

std::uint64_t peakWorkingSetBytes()
{
#if defined(_WIN32)
    PROCESS_MEMORY_COUNTERS counters {};
    counters.cb = sizeof(counters);
    if (GetProcessMemoryInfo(GetCurrentProcess(), &counters, sizeof(counters)) != FALSE)
        return static_cast<std::uint64_t>(counters.PeakWorkingSetSize);
#endif
    return 0;
}

std::string joinIssues(const std::vector<std::string>& issues)
{
    std::ostringstream stream;
    for (std::size_t index = 0; index < issues.size(); ++index)
    {
        if (index != 0)
            stream << " | ";
        stream << issues[index];
    }
    return stream.str();
}

std::string joinFindings(const std::vector<drs::engine::PlaybackSnapshotFinding>& findings)
{
    std::ostringstream stream;
    for (std::size_t index = 0; index < findings.size(); ++index)
    {
        if (index != 0)
            stream << " | ";
        stream << findings[index].code << ": " << findings[index].message;
    }
    return stream.str();
}

drs::engine::RuntimeProjectModel makeBlankProject(const fs::path& sfzPath)
{
    drs::engine::RuntimeProjectModel project;
    project.schemaName = "drs.project";
    project.schemaVersion = 6;
    project.projectId = "qualification.accurate-salamander";
    project.displayName = "Accurate Salamander Qualification";
    project.contentRootPath = sfzPath.parent_path().generic_string();
    project.defaultInstrumentManifestPath = (sfzPath.parent_path() / "qualification.drinst").generic_string();
    project.authoring.schemaName = "drs.authoring";
    project.authoring.schemaVersion = 5;
    return project;
}

drs::engine::PlaybackSnapshotBuildResult buildSnapshot(
    drs::engine::PlaybackSnapshotBuilder& builder,
    const drs::engine::RuntimeProjectModel& project,
    const std::size_t revision)
{
    const auto request = builder.requestBuild(revision, false);
    require(request.accepted, "Snapshot request was rejected.");
    const auto result = builder.buildSnapshot(request, project);
    require(result.built && result.activationEligible,
            "Snapshot build failed: " + result.state);
    return result;
}

struct PreparedRun
{
    drs::engine::PreparedPlaybackWorkerStepResult step;
    std::uint64_t elapsed = 0;
};

PreparedRun prepareSynchronously(const drs::engine::PlaybackSnapshotBuildResult& snapshot)
{
    drs::engine::PreparedPlaybackSchedulerBudgets budgets;
    // Force the qualification through the streaming seam even for a one-zone scope.
    budgets.maximumRetainedPreparedBytes = 1;
    drs::engine::PreparedPlaybackService service("large-instrument-qualification", 2, false, budgets);
    const drs::engine::RuntimeStreamLoadResult noCompiledStream;
    const auto start = Clock::now();
    require(service.enqueuePreviewBuild(snapshot).accepted,
            "Preparation request was rejected before worker execution.");
    auto step = service.processNextQueuedBuild(noCompiledStream);
    const auto elapsed = elapsedMicros(start);
    require(step.processed && step.result.built && step.result.activationEligible,
            "Preparation failed: " + step.result.state + " :: "
                + joinFindings(step.result.findings));
    require(step.result.metrics.decodedBytes == 0,
            "Streaming preparation decoded full sample PCM.");
    return { std::move(step), elapsed };
}

drs::engine::SamplerRenderModelPtr buildQualificationRenderModel(
    const drs::engine::PlaybackSnapshotBuildResult& snapshot,
    const drs::engine::PreparedPlaybackBuildResult& prepared)
{
    const auto payload = drs::engine::buildPlaybackActivationPayload(
        drs::engine::PlaybackActivationLane::performance,
        snapshot.requestedDraftRevision,
        &snapshot,
        &prepared);
    drs::engine::SamplerRenderModelBuildOptions options;
    options.selectedArticulationId = "sustain";
    const auto build = drs::engine::buildSamplerRenderModel(payload, options);
    require(build.built && build.model != nullptr,
            "Semantic qualification could not build the full render model.");
    return build.model;
}

drs::engine::SamplerRouteEligibilityQuery makeDefaultEligibilityQuery(
    const drs::engine::SamplerRenderModel& model,
    const drs::engine::PerformanceEventKind event,
    const int note,
    const int velocity)
{
    drs::engine::SamplerRouteEligibilityQuery query;
    query.performanceEvent = event;
    query.midiNote = note;
    query.velocity = velocity;
    const auto& program = model.getPerformanceProgram();
    query.articulationIndex = program.defaultArticulationIndex;
    for (std::size_t controller = 0; controller < query.controllerValues.size(); ++controller)
        if (program.hasControllerDefault[controller])
            query.controllerValues[controller] = program.controllerDefaults[controller];
    query.sustainPedalDown = query.controllerValues[64] >= 64;
    return query;
}

bool routeHasControllerCondition(const drs::engine::SamplerRenderRoute& route,
                                 const int controller)
{
    return std::any_of(route.controllerConditions.begin(), route.controllerConditions.end(),
                       [&](const drs::engine::RuntimeControllerCondition& condition)
                       { return condition.controllerNumber == controller; });
}

struct SemanticQualificationResult
{
    std::size_t defaultMiddleCRoutes = 0;
    std::size_t enabledResonanceRoutes = 0;
    std::size_t sampledReleaseRoutes = 0;
    std::size_t hammerRoutes = 0;
    std::size_t pedalTransitionRoutes = 0;
};

SemanticQualificationResult qualifySalamanderSemantics(
    const drs::engine::SamplerRenderModel& model)
{
    const auto evaluate = [&](const drs::engine::SamplerRouteEligibilityQuery& query)
    {
        const auto result = drs::engine::evaluateSamplerRouteEligibility(model, query);
        require(result.evaluated, "A semantic route-state query was rejected.");
        return result.eligibleRouteIndices;
    };

    auto defaultNote = makeDefaultEligibilityQuery(
        model, drs::engine::PerformanceEventKind::noteOn, 60, 64);
    const auto defaultRoutes = evaluate(defaultNote);
    require(defaultRoutes.size() == 1,
            "Default-controller middle C must start exactly one ordinary piano route.");

    auto resonanceWithoutPedal = defaultNote;
    resonanceWithoutPedal.controllerValues[23] = 1;
    require(evaluate(resonanceWithoutPedal).size() == 1,
            "CC23 alone must not enable pseudo pedal resonance while sustain is up.");

    auto pedalWithoutResonance = defaultNote;
    pedalWithoutResonance.controllerValues[64] = 127;
    pedalWithoutResonance.sustainPedalDown = true;
    require(evaluate(pedalWithoutResonance).size() == 1,
            "Sustain alone must not enable pseudo pedal resonance while CC23 is disabled.");

    auto enabledResonance = pedalWithoutResonance;
    enabledResonance.controllerValues[23] = 1;
    const auto resonanceRoutes = evaluate(enabledResonance);
    require(resonanceRoutes.size() == 3,
            "Enabled middle-C resonance must produce one piano route plus two sympathetic routes; actual="
                + std::to_string(resonanceRoutes.size()));
    std::vector<std::size_t> auxiliaryResonanceRoutes;
    for (const auto routeIndex : resonanceRoutes)
        if (routeHasControllerCondition(model.getRoutes().at(routeIndex), 23))
            auxiliaryResonanceRoutes.push_back(routeIndex);
    require(auxiliaryResonanceRoutes.size() == 2
                && std::all_of(auxiliaryResonanceRoutes.begin(), auxiliaryResonanceRoutes.end(),
                               [&](const std::size_t routeIndex)
                               { return std::abs(model.getRoutes().at(routeIndex).gainDb + 6.0) < 0.000001; }),
            "Middle-C resonance must retain exactly two -6 dB auxiliary routes.");

    auto tunedResonance = enabledResonance;
    tunedResonance.midiNote = 61;
    const auto tunedRoutes = evaluate(tunedResonance);
    require(std::any_of(tunedRoutes.begin(), tunedRoutes.end(),
                        [&](const std::size_t routeIndex)
                        {
                            const auto& route = model.getRoutes().at(routeIndex);
                            return routeHasControllerCondition(route, 23)
                                && std::abs(route.fineTuneCents - 1.0) < 0.000001;
                        }),
            "The enabled resonance matrix must retain its authored +1-cent C#4 auxiliary route.");

    auto defaultRelease = makeDefaultEligibilityQuery(
        model, drs::engine::PerformanceEventKind::release, 60, 64);
    require(evaluate(defaultRelease).empty(),
            "Release and hammer routes must remain silent at default controller values.");
    auto sampledRelease = defaultRelease;
    sampledRelease.controllerValues[20] = 1;
    const auto sampledReleaseRoutes = evaluate(sampledRelease);
    require(sampledReleaseRoutes.size() == 2,
            "CC20-enabled middle-C release must select the intended two sampled-release layers.");
    auto hammerRelease = defaultRelease;
    hammerRelease.controllerValues[21] = 1;
    const auto hammerRoutes = evaluate(hammerRelease);
    require(hammerRoutes.size() == 1,
            "CC21-enabled middle-C release must select exactly one hammer-noise route.");
    const auto& hammerRoute = model.getRoutes().at(hammerRoutes.front());
    require(hammerRoute.performancePitchSource
                == drs::engine::PerformancePitchSource::eventKeyFixedPitch
                && std::abs(hammerRoute.gainDb + 37.0) < 0.000001,
            "The middle-C hammer route must remain untransposed at its intended -37 dB gain.");

    auto pedalDown = makeDefaultEligibilityQuery(
        model, drs::engine::PerformanceEventKind::pedalDown, 60, 127);
    pedalDown.controllerValues[22] = 1;
    pedalDown.controllerValues[64] = 127;
    pedalDown.sustainPedalDown = true;
    auto pedalUp = makeDefaultEligibilityQuery(
        model, drs::engine::PerformanceEventKind::pedalUp, 60, 127);
    pedalUp.controllerValues[22] = 1;
    pedalUp.controllerValues[64] = 0;
    const auto pedalTransitionRouteCount = evaluate(pedalDown).size() + evaluate(pedalUp).size();
    require(pedalTransitionRouteCount == 0,
            "The explicit sound-safe random policy must keep all random pedal-action routes silent.");

    return { defaultRoutes.size(),
             auxiliaryResonanceRoutes.size(),
             sampledReleaseRoutes.size(),
             hammerRoutes.size(),
             pedalTransitionRouteCount };
}

PreparedRun prepareResidentSynchronously(
    const drs::engine::PlaybackSnapshotBuildResult& snapshot)
{
    drs::engine::PreparedPlaybackService service(
        "sfz-semantic-parity-resident-reference", 1, false);
    const drs::engine::RuntimeStreamLoadResult noCompiledStream;
    const auto start = Clock::now();
    require(service.enqueuePreviewBuild(snapshot).accepted,
            "Resident reference preparation request was rejected.");
    auto step = service.processNextQueuedBuild(noCompiledStream);
    const auto elapsed = elapsedMicros(start);
    require(step.processed && step.result.built && step.result.activationEligible,
            "Resident reference preparation failed: " + step.result.state + " :: "
                + joinFindings(step.result.findings));
    require(step.result.metrics.decodedBytes > 0,
            "Selected-zone audio reference was not decoded into deterministic resident PCM.");
    return { std::move(step), elapsed };
}

std::string findDefaultMiddleCZoneId(const drs::engine::SamplerRenderModel& model)
{
    const auto query = makeDefaultEligibilityQuery(
        model, drs::engine::PerformanceEventKind::noteOn, 60, 64);
    const auto routes = drs::engine::evaluateSamplerRouteEligibility(model, query);
    require(routes.evaluated && routes.eligibleRouteIndices.size() == 1,
            "Default middle-C reference selection must resolve exactly one route.");
    return model.getRoutes().at(routes.eligibleRouteIndices.front()).zoneId;
}

struct DeterministicAudioRender
{
    std::vector<float> mono;
    double peak = 0.0;
    double rms = 0.0;
    std::int64_t lastNonZeroFrame = -1;
    std::array<double, 8> spectralProfile {};
};

DeterministicAudioRender renderDeterministicReference(
    const drs::engine::SamplerRenderModelPtr& model)
{
    require(model != nullptr, "Deterministic reference render requires a model.");
    drs::engine::SamplerPlaybackContext context(drs::engine::PlaybackActivationLane::performance);
    require(context.prepare(48000.0) && context.stageActivation(model),
            "Deterministic reference context could not prepare and stage its model.");

    constexpr std::uint32_t framesPerBlock = 256;
    constexpr std::size_t blockCount = 375;
    constexpr std::size_t noteOffBlock = 94;
    DeterministicAudioRender result;
    result.mono.reserve(framesPerBlock * blockCount);
    std::array<std::vector<float>, 2> storage {
        std::vector<float>(framesPerBlock), std::vector<float>(framesPerBlock)
    };
    std::array<float*, 2> channels { storage[0].data(), storage[1].data() };
    for (std::size_t block = 0; block < blockCount; ++block)
    {
        std::fill(storage[0].begin(), storage[0].end(), 0.0f);
        std::fill(storage[1].begin(), storage[1].end(), 0.0f);
        std::array<drs::engine::SamplerRenderEvent, 1> events {};
        std::size_t eventCount = 0;
        if (block == 0 || block == noteOffBlock)
        {
            events[0].type = block == 0
                ? drs::engine::SamplerRenderEventType::noteOn
                : drs::engine::SamplerRenderEventType::noteOff;
            events[0].midiNote = 60;
            events[0].velocity = 64.0f / 127.0f;
            events[0].noteOffVelocity = 64.0f / 127.0f;
            eventCount = 1;
        }
        drs::engine::SamplerAudioBufferView output { channels.data(), 2, framesPerBlock };
        const auto rendered = context.renderBlock(output, { events.data(), eventCount });
        require(rendered.accepted && rendered.voicePool.render.pageMissCount == 0
                    && rendered.voicePool.render.underrunFrameCount == 0,
                "Resident deterministic reference render encountered a page miss or underrun.");
        for (std::size_t frame = 0; frame < framesPerBlock; ++frame)
            result.mono.push_back((storage[0][frame] + storage[1][frame]) * 0.5f);
    }

    long double squared = 0.0;
    for (std::size_t frame = 0; frame < result.mono.size(); ++frame)
    {
        const auto value = static_cast<double>(result.mono[frame]);
        result.peak = std::max(result.peak, std::abs(value));
        squared += static_cast<long double>(value) * static_cast<long double>(value);
        if (std::abs(value) > 1.0e-8)
            result.lastNonZeroFrame = static_cast<std::int64_t>(frame);
    }
    result.rms = std::sqrt(static_cast<double>(squared / result.mono.size()));

    constexpr std::array<std::size_t, 8> bins { 1, 2, 4, 8, 16, 32, 64, 128 };
    const auto spectrumFrames = std::min<std::size_t>(4096, result.mono.size());
    constexpr auto twoPi = 6.283185307179586476925286766559;
    for (std::size_t band = 0; band < bins.size(); ++band)
    {
        long double real = 0.0;
        long double imaginary = 0.0;
        for (std::size_t frame = 0; frame < spectrumFrames; ++frame)
        {
            const auto phase = twoPi * static_cast<double>(bins[band] * frame)
                / static_cast<double>(spectrumFrames);
            const auto window = 0.5 - 0.5 * std::cos(twoPi * static_cast<double>(frame)
                / static_cast<double>(spectrumFrames - 1));
            const auto sample = static_cast<double>(result.mono[frame]) * window;
            real += sample * std::cos(phase);
            imaginary -= sample * std::sin(phase);
        }
        result.spectralProfile[band] = std::sqrt(
            static_cast<double>(real * real + imaginary * imaginary))
            / static_cast<double>(spectrumFrames);
    }
    return result;
}

struct AudioParityQualificationResult
{
    double standardPeak = 0.0;
    double referencePeak = 0.0;
    double standardRms = 0.0;
    double referenceRms = 0.0;
    std::int64_t durationFrames = 0;
    double maximumSampleError = 0.0;
    double rmsSampleError = 0.0;
    double maximumSpectralError = 0.0;
    bool strictParityEnforced = true;
    bool withinStrictTolerance = false;
};

AudioParityQualificationResult compareApprovedAudioReference(
    const DeterministicAudioRender& standard,
    const DeterministicAudioRender& reference,
    const bool enforceStrictParity = true)
{
    require(standard.mono.size() == reference.mono.size(),
            "Standard and approved minimum-reference renders have different frame counts.");
    long double squaredError = 0.0;
    double maximumError = 0.0;
    for (std::size_t frame = 0; frame < standard.mono.size(); ++frame)
    {
        const auto error = std::abs(static_cast<double>(standard.mono[frame])
                                    - static_cast<double>(reference.mono[frame]));
        maximumError = std::max(maximumError, error);
        squaredError += static_cast<long double>(error) * static_cast<long double>(error);
    }
    const auto rmsError = std::sqrt(static_cast<double>(squaredError / standard.mono.size()));
    double maximumSpectralError = 0.0;
    for (std::size_t band = 0; band < standard.spectralProfile.size(); ++band)
        maximumSpectralError = std::max(
            maximumSpectralError,
            std::abs(standard.spectralProfile[band] - reference.spectralProfile[band]));

    const auto withinStrictTolerance
        = std::abs(standard.peak - reference.peak) <= 1.0e-6
        && std::abs(standard.rms - reference.rms) <= 1.0e-7
        && standard.lastNonZeroFrame == reference.lastNonZeroFrame
        && maximumSpectralError <= 1.0e-8
        && maximumError <= 1.0e-6 && rmsError <= 1.0e-7;
    if (enforceStrictParity)
        require(withinStrictTolerance,
                "Standard and approved minimum-reference audio diverged beyond tolerance.");
    else
        require(std::isfinite(standard.peak) && std::isfinite(standard.rms)
                    && standard.peak > 0.0 && standard.rms > 0.0,
                "Live-preset deterministic audio must remain finite and audible.");
    return { standard.peak, reference.peak, standard.rms, reference.rms,
             standard.lastNonZeroFrame + 1, maximumError, rmsError, maximumSpectralError,
             enforceStrictParity, withinStrictTolerance };
}

struct SourceMetrics
{
    std::uint64_t headBytes = 0;
    std::uint64_t pageBytes = 0;
    std::uint64_t rangeReads = 0;
    std::uint64_t rangeBytes = 0;
    std::uint64_t maximumReadMicros = 0;
    std::uint64_t pageMisses = 0;
    std::uint64_t pageCacheBudgetBytes = 0;
    std::uint64_t maximumAllocatedPageBytes = 0;
    std::uint64_t leasedPageBytes = 0;
    std::uint64_t retiredPageBytes = 0;
    std::uint64_t intentPublished = 0;
    std::uint64_t intentConsumed = 0;
    std::uint64_t intentDropped = 0;
    std::size_t maximumIntentDepth = 0;
};

SourceMetrics collectSourceMetrics(const drs::engine::ImmutablePreparedPlayback& prepared)
{
    SourceMetrics total;
    for (const auto& sample : prepared.samples)
    {
        const auto source = std::dynamic_pointer_cast<const drs::engine::WavPagedSampleDataSource>(
            sample.dataSource);
        if (source == nullptr)
            continue;
        const auto metrics = source->metrics();
        total.headBytes += metrics.residentHeadBytes;
        total.pageBytes += metrics.residentPageBytes;
        total.rangeReads += metrics.rangeReadCount;
        total.rangeBytes += metrics.bytesRead;
        total.maximumReadMicros = std::max(total.maximumReadMicros,
                                           metrics.maximumReadLatencyMicros);
        total.pageMisses += metrics.pageMissCount;
        total.pageCacheBudgetBytes += metrics.pageCacheBudgetBytes;
        total.maximumAllocatedPageBytes += metrics.maximumAllocatedPageBytes;
        total.leasedPageBytes += metrics.leasedPageBytes;
        total.retiredPageBytes += metrics.retiredPageBytes;
        const auto intents = source->intentMetrics();
        total.intentPublished += intents.publishedCount;
        total.intentConsumed += intents.consumedCount;
        total.intentDropped += intents.droppedCount;
        total.maximumIntentDepth = std::max(total.maximumIntentDepth, intents.maximumDepth);
    }
    return total;
}

struct SustainedPlaybackResult
{
    float peak = 0.0f;
    std::uint64_t pageMisses = 0;
    std::uint64_t underrunFrames = 0;
    std::uint64_t recoveries = 0;
    std::uint64_t elapsed = 0;
    std::uint64_t maximumRenderMicros = 0;
    drs::engine::PreparedPlaybackWorkerStatus worker;
    SourceMetrics sources;
};

struct PackageQualificationResult
{
    std::uint64_t packageBytes = 0;
    std::uint64_t recordCount = 0;
    std::uint64_t totalMicros = 0;
    std::uint64_t peakPlaintextBytes = 0;
    std::uint64_t peakSealedBytes = 0;
    std::uint64_t verificationBytes = 0;
    double throughput = 0.0;
    std::uint64_t headBytes = 0;
    std::uint64_t metadataOpenMicros = 0;
    std::uint64_t warmMetadataOpenMicros = 0;
    std::uint64_t playableMicros = 0;
    float firstNotePeak = 0.0f;
    std::uint64_t cancellationMicros = 0;
    std::uint64_t cancellationPolls = 0;
    std::uint64_t cancellationBytesProcessed = 0;
};

PackageQualificationResult exportAndActivatePackage(
    const fs::path& packagePath,
    const drs::engine::RuntimeProjectModel& project,
    const drs::engine::SfzImportProjectionResult& projection,
    const drs::engine::PlaybackSnapshotBuildResult& fullSnapshot,
    const drs::engine::PreparedPlaybackBuildResult& fullPrepared,
    const int note,
    const int velocity)
{
    drs::app::PerformancePackageExportRequest request;
    request.project = project;
    request.projectId = project.projectId;
    request.baseRevision = 1;
    request.packagePath = packagePath.generic_string();
    request.sessionState.loadProfileId = "balanced";
    const auto operation = drs::app::executePerformancePackageExport(
        request,
        { [](const drs::app::PerformancePackageExportProgress& progress)
    {
        if (progress.stage == drs::app::PerformancePackageExportStage::sealingPackage
            && progress.bytesProcessed != 0
            && progress.bytesProcessed % (256ull * 1024ull * 1024ull) < 65536)
            std::cout << "Package export progress bytes: " << progress.bytesProcessed << "/"
                      << progress.totalBytes << std::endl;
    }, {} });
    require(operation.exported,
            "Production actual-corpus v2 export failed: " + operation.state + " :: "
                + joinIssues(operation.issues));

    PackageQualificationResult result;
    result.packageBytes = operation.packageBytes;
    result.recordCount = operation.payloadCount;
    result.totalMicros = operation.totalDurationMicros;
    result.peakPlaintextBytes = operation.peakPlaintextBufferBytes;
    result.peakSealedBytes = operation.peakSealedBufferBytes;
    result.verificationBytes = operation.verificationBytesRead;
    result.throughput = operation.plaintextThroughputBytesPerSecond;

    const auto metadataStart = Clock::now();
    const auto packageMetadata = drs::engine::loadPerformancePackageV2Metadata(
        packagePath.generic_string());
    require(packageMetadata.loaded && packageMetadata.package != nullptr,
            "Production package v2 metadata loader failed: "
                + joinIssues(packageMetadata.issues));
    auto opened = packageMetadata.package;
    result.metadataOpenMicros = elapsedMicros(metadataStart);
    require(opened->opened && opened->records.size() == result.recordCount,
            "Exported actual-corpus package failed structural reopen.");
    {
        auto productionActivation = drs::engine::preparePerformancePackageV2Activation(
            packageMetadata.metadata,
            opened,
            packageMetadata.sampleDescriptors);
        require(productionActivation.prepared
                    && productionActivation.activationPayload != nullptr
                    && productionActivation.renderModel != nullptr,
                "Exported actual-corpus package failed production package-v2 activation preparation: "
                    + joinIssues(productionActivation.issues));
        static_cast<void>(qualifySalamanderSemantics(*productionActivation.renderModel));
    }
    const auto warmMetadataStart = Clock::now();
    const auto warmOpened = drs::engine::openPackageV2(packagePath.generic_string());
    result.warmMetadataOpenMicros = elapsedMicros(warmMetadataStart);
    require(warmOpened.opened, "Warm package metadata reopen failed.");
    const auto& packageDescriptors = packageMetadata.sampleDescriptors;

    auto packagePrepared = fullPrepared;
    std::vector<std::shared_ptr<drs::engine::PackagePagedSampleDataSource>> sources;
    sources.reserve(packagePrepared.prepared.samples.size());
    for (auto& preparedSample : packagePrepared.prepared.samples)
    {
        const auto descriptor = std::find_if(packageDescriptors.begin(), packageDescriptors.end(),
            [&](const auto& candidate) { return candidate.sourceId == preparedSample.sampleSourceId; });
        require(descriptor != packageDescriptors.end(),
                "Exported package descriptor is missing a prepared sample source.");
        auto source = std::make_shared<drs::engine::PackagePagedSampleDataSource>(*descriptor, opened);
        preparedSample.dataSource = source;
        preparedSample.decodedSampleData.reset();
        sources.push_back(std::move(source));
    }
    const auto payload = drs::engine::buildPlaybackActivationPayload(
        drs::engine::PlaybackActivationLane::performance,
        fullSnapshot.requestedDraftRevision,
        &fullSnapshot,
        &packagePrepared);
    drs::engine::SamplerRenderModelBuildOptions packageModelOptions;
    packageModelOptions.selectedArticulationId = "sustain";
    const auto model = drs::engine::buildSamplerRenderModel(payload, packageModelOptions);
    require(model.built && model.model != nullptr,
            "Exported actual-corpus package did not build the common render model.");
    static_cast<void>(qualifySalamanderSemantics(*model.model));

    drs::engine::DeferredPackageSession deferred;
    drs::engine::DeferredPackageSessionPlan plan;
    plan.packagePath = packagePath.generic_string();
    plan.package = opened;
    plan.sources = sources;
    plan.buildRenderModel = [retained = model.model] { return retained; };
    const auto playableStart = Clock::now();
    require(deferred.begin(std::move(plan)), "Deferred actual-corpus package begin failed.");
    while (deferred.snapshot().stage != drs::engine::DeferredPackageSessionStage::playable)
        require(deferred.serviceNextWorkerStep(),
                "Deferred actual-corpus package stopped before playable readiness.");
    result.playableMicros = elapsedMicros(playableStart);
    for (const auto& source : sources)
        result.headBytes += source->metrics().publishedHeadBytes;

    drs::engine::SamplerPlaybackContext context(drs::engine::PlaybackActivationLane::performance);
    require(context.prepare(48000.0) && deferred.stagePlayableActivation(context),
            "Deferred actual-corpus package activation staging failed.");
    constexpr std::uint32_t framesPerBlock = 256;
    std::array<std::vector<float>, 2> outputStorage {
        std::vector<float>(framesPerBlock), std::vector<float>(framesPerBlock)
    };
    std::array<float*, 2> channels { outputStorage[0].data(), outputStorage[1].data() };
    for (std::size_t block = 0; block < 8; ++block)
    {
        std::fill(outputStorage[0].begin(), outputStorage[0].end(), 0.0f);
        std::fill(outputStorage[1].begin(), outputStorage[1].end(), 0.0f);
        std::array<drs::engine::SamplerRenderEvent, 1> events {};
        std::size_t eventCount = 0;
        if (block == 0)
        {
            events[0].type = drs::engine::SamplerRenderEventType::noteOn;
            events[0].midiNote = static_cast<std::uint8_t>(note);
            events[0].velocity = static_cast<float>(velocity) / 127.0f;
            eventCount = 1;
        }
        const auto rendered = context.renderBlock(
            { channels.data(), 2, framesPerBlock }, { events.data(), eventCount });
        require(rendered.accepted, "Package first-note render block failed.");
        if (block == 0)
            require(deferred.observeAudioCutover(context),
                    "Package callback cutover was not observed.");
        for (const auto& channel : outputStorage)
            for (const auto value : channel)
                result.firstNotePeak = std::max(result.firstNotePeak, std::abs(value));
    }
    require(result.firstNotePeak > 1.0e-5f,
            "Exported actual-corpus package first note was inaudible.");

    const auto canceledPath = packagePath.parent_path()
        / (packagePath.stem().generic_string() + "-cancelled.drpkg");
    std::error_code cleanupError;
    fs::remove(canceledPath, cleanupError);
    fs::remove(fs::path(canceledPath.generic_string() + ".stage"), cleanupError);
    auto cancellationRequest = request;
    cancellationRequest.packagePath = canceledPath.generic_string();
    const auto cancellationStart = Clock::now();
    const auto canceled = drs::app::executePerformancePackageExport(
        cancellationRequest,
        { [&](const drs::app::PerformancePackageExportProgress& progress)
        {
            if (progress.stage == drs::app::PerformancePackageExportStage::sealingPackage)
            {
                result.cancellationBytesProcessed = std::max(
                    result.cancellationBytesProcessed, progress.bytesProcessed);
            }
        }, [&]
        {
            ++result.cancellationPolls;
            return result.cancellationBytesProcessed >= 64ull * 1024ull * 1024ull;
        } });
    result.cancellationMicros = elapsedMicros(cancellationStart);
    require(canceled.canceled && !canceled.exported,
            "Actual-corpus production export cancellation was not honored.");
    require(!fs::exists(canceledPath)
                && !fs::exists(fs::path(canceledPath.generic_string() + ".stage")),
            "Actual-corpus canceled export left a publishable or staging package.");
    return result;
}

SustainedPlaybackResult runSustainedPlayback(
    const drs::engine::PlaybackSnapshotBuildResult& snapshot,
    const int note,
    const int velocity,
    const std::uint64_t pageServicePollMilliseconds = 5)
{
    drs::engine::PreparedPlaybackSchedulerBudgets budgets;
    budgets.maximumRetainedPreparedBytes = 1;
    budgets.pageServicePollMilliseconds = pageServicePollMilliseconds;
    drs::engine::PreparedPlaybackService service("large-instrument-qualification-live", 2, true, budgets);
    const drs::engine::RuntimeStreamLoadResult noCompiledStream;
    service.setBackgroundWorkerStream(noCompiledStream);
    require(service.enqueuePreviewBuild(snapshot).accepted,
            "Live preparation request was rejected.");
    require(service.waitForWorkerIdle(30000),
            "Live preparation worker did not reach idle within 30 seconds.");
    auto completed = service.drainCompletedBuilds();
    require(completed.size() == 1 && completed.front().result.built,
            "Live preparation did not publish one completed result.");
    auto& prepared = completed.front().result;

    const auto payload = drs::engine::buildPlaybackActivationPayload(
        drs::engine::PlaybackActivationLane::preview,
        snapshot.requestedDraftRevision,
        &snapshot,
        &prepared);
    const auto renderBuild = drs::engine::buildSamplerRenderModel(payload);
    require(renderBuild.built && renderBuild.model != nullptr,
            "Streaming preparation did not build a render model.");

    drs::engine::SamplerPlaybackContext context(drs::engine::PlaybackActivationLane::preview);
    require(context.prepare(48000.0), "Playback context preparation failed.");
    require(context.stageActivation(renderBuild.model), "Playback activation staging failed.");

    SustainedPlaybackResult result;
    const auto start = Clock::now();
    constexpr std::uint32_t framesPerBlock = 256;
    constexpr std::size_t blockCount = 375; // Two seconds at 48 kHz.
    std::array<std::vector<float>, 2> outputStorage {
        std::vector<float>(framesPerBlock), std::vector<float>(framesPerBlock)
    };
    std::array<float*, 2> channels {
        outputStorage[0].data(), outputStorage[1].data()
    };

    for (std::size_t block = 0; block < blockCount; ++block)
    {
        std::fill(outputStorage[0].begin(), outputStorage[0].end(), 0.0f);
        std::fill(outputStorage[1].begin(), outputStorage[1].end(), 0.0f);
        drs::engine::SamplerAudioBufferView output { channels.data(), 2, framesPerBlock };
        std::array<drs::engine::SamplerRenderEvent, 8> events {};
        std::size_t eventCount = 0;
        if (block == 0)
        {
            for (std::size_t voice = 0; voice < 8; ++voice)
            {
                events[voice].type = drs::engine::SamplerRenderEventType::noteOn;
                events[voice].midiNote = static_cast<std::uint8_t>(note);
                events[voice].velocity = static_cast<float>(velocity) / 127.0f;
                events[voice].inputSequence = static_cast<std::uint32_t>(voice + 1);
            }
            eventCount = events.size();
        }
        const auto renderStarted = Clock::now();
        const auto render = context.renderBlock(output, { events.data(), eventCount });
        result.maximumRenderMicros = std::max(
            result.maximumRenderMicros, elapsedMicros(renderStarted));
        require(render.accepted, "Sustained streaming render block was rejected.");
        result.pageMisses += render.voicePool.render.pageMissCount;
        result.underrunFrames += render.voicePool.render.underrunFrameCount;
        result.recoveries += render.voicePool.render.pageRecoveryCount;
        for (const auto& channel : outputStorage)
        {
            for (const auto value : channel)
                result.peak = std::max(result.peak, std::abs(value));
        }
        // Pace the synthetic callback loop no faster than a 48 kHz device so the
        // asynchronous page worker is measured under a realistic service window.
        std::this_thread::sleep_for(std::chrono::milliseconds(6));
    }
    result.elapsed = elapsedMicros(start);
    result.worker = service.getWorkerStatus();
    result.sources = collectSourceMetrics(prepared.prepared);
    require(result.peak > 1.0e-5f, "Sustained streamed playback was inaudible.");
    require(result.worker.pagePrepareCount > 0,
            "Sustained playback published no worker-prepared pages beyond the resident head.");
    require(result.worker.pagePrepareFailureCount == 0,
            "Sustained playback encountered page preparation failures.");
    require(result.maximumRenderMicros < 5333,
            "Sustained playback exceeded one 256-frame callback period at 48 kHz.");
    return result;
}

std::uint64_t fileTreeBytes(const fs::path& root, std::size_t& wavCount)
{
    std::uint64_t bytes = 0;
    wavCount = 0;
    for (const auto& entry : fs::recursive_directory_iterator(root))
    {
        if (!entry.is_regular_file())
            continue;
        if (entry.path().extension() == ".wav" || entry.path().extension() == ".WAV")
        {
            ++wavCount;
            bytes += entry.file_size();
        }
    }
    return bytes;
}
} // namespace

int main(int argc, char** argv)
{
    try
    {
        require(argc >= 4,
                "Usage: drs_large_instrument_qualification <instrument.sfz> <report.md> <package.drpkg>");
        const auto sfzPath = fs::absolute(fs::path(argv[1]));
        const auto reportPath = fs::absolute(fs::path(argv[2]));
        const auto packagePath = fs::absolute(fs::path(argv[3]));
        require(fs::is_regular_file(sfzPath), "SFZ qualification input does not exist.");

        std::size_t wavCount = 0;
        const auto corpusBytes = fileTreeBytes(sfzPath.parent_path().parent_path(), wavCount);
        const auto blankProject = makeBlankProject(sfzPath);

        const auto importStart = Clock::now();
        const auto projection = drs::engine::projectSfzImportDocument(
            blankProject, sfzPath.generic_string());
        const auto importMicros = elapsedMicros(importStart);
        require(projection.projected && projection.playable && !projection.blocking,
                "Salamander import projection failed: " + projection.state + " :: "
                    + joinIssues(projection.issues));
        require(projection.sampleSources.size() == 637,
                "Salamander Phase 3 projection did not retain exactly 637 referenced sample sources.");
        require(projection.zones.size() == 1700,
                "Salamander Phase 3 projection did not retain exactly 1,700 supported regions.");
        require(projection.semanticAnalyzedRegionCount == 1704,
                "Salamander semantic analysis did not classify all 1,704 regions.");
        require(projection.unsafeUnconditionalRegionCount == 4,
                "Salamander semantic analysis did not isolate the four unsupported random regions.");
        require(projection.omittedUnsafeRegionCount == 4,
                "Salamander projection did not omit exactly the four unsupported random regions.");

        drs::engine::AuthoringSession session(blankProject);
        const auto applyStart = Clock::now();
        const auto apply = drs::engine::applySfzImportProjection(
            session, projection, "Import Accurate Salamander qualification corpus");
        const auto applyMicros = elapsedMicros(applyStart);
        require(apply.applied, "Salamander import projection did not apply.");

        drs::engine::PlaybackSnapshotBuilder snapshotBuilder;
        const auto snapshotStart = Clock::now();
        const auto fullSnapshot = buildSnapshot(snapshotBuilder, session.getProject(), 1);
        const auto snapshotMicros = elapsedMicros(snapshotStart);
        require(fullSnapshot.snapshot.sampleIdentities.size() == 637
                    && fullSnapshot.snapshot.zones.size() == 1700,
                "Full-draft snapshot lost Salamander dependencies.");

        const auto& anchorZone = fullSnapshot.snapshot.zones.front();
        const auto scopedZone = drs::engine::scopePlaybackSnapshotForPreparation(
            fullSnapshot,
            { drs::engine::PlaybackPreparationScope::selectedZone, anchorZone.id, {} });
        const auto scopedGroup = drs::engine::scopePlaybackSnapshotForPreparation(
            fullSnapshot,
            { drs::engine::PlaybackPreparationScope::selectedGroup, {}, anchorZone.groupId });
        require(scopedZone.built && scopedZone.retainedZoneCount < fullSnapshot.snapshot.zones.size(),
                "Selected-zone scope did not reduce the full draft.");
        require(scopedGroup.built && scopedGroup.retainedZoneCount < fullSnapshot.snapshot.zones.size(),
                "Selected-group scope did not reduce the full draft.");

        const auto zonePrepared = prepareSynchronously(scopedZone);
        const auto groupPrepared = prepareSynchronously(scopedGroup);
        const auto fullPrepared = prepareSynchronously(fullSnapshot);
        require(fullPrepared.step.result.admission.estimatedDecodedBytes
                    > fullPrepared.step.result.admission.residentBudgetBytes,
                "Actual Salamander metadata did not exceed resident admission.");
        require(fullPrepared.step.result.admission.readiness
                    == drs::engine::PreparedPlaybackReadinessState::playable,
                "Actual Salamander full draft did not reach streaming playable readiness.");
        const auto fullSources = collectSourceMetrics(fullPrepared.step.result.prepared);
        require(fullSources.headBytes <= 16ull * 1024ull * projection.sampleSources.size(),
                "Prepared Salamander heads exceeded the configured 16 KiB-per-source ceiling.");
        const auto fullRenderModel = buildQualificationRenderModel(
            fullSnapshot, fullPrepared.step.result);
        const auto semanticQualification = qualifySalamanderSemantics(*fullRenderModel);

        const auto minimumSfzPath = sfzPath.parent_path().parent_path()
            / "sfz_minimum" / sfzPath.filename();
        require(fs::is_regular_file(minimumSfzPath),
                "Approved minimum Salamander audio reference does not exist.");
        const auto minimumBlankProject = makeBlankProject(minimumSfzPath);
        const auto minimumProjection = drs::engine::projectSfzImportDocument(
            minimumBlankProject, minimumSfzPath.generic_string());
        require(minimumProjection.projected && minimumProjection.playable
                    && minimumProjection.semanticAnalyzedRegionCount == 1408
                    && minimumProjection.unsafeUnconditionalRegionCount == 0
                    && minimumProjection.omittedUnsafeRegionCount == 0,
                "Approved minimum Salamander reference must remain a sound-safe 1,408-region piano-only import.");
        drs::engine::AuthoringSession minimumSession(minimumBlankProject);
        require(drs::engine::applySfzImportProjection(
                    minimumSession, minimumProjection,
                    "Import minimum Salamander audio reference").applied,
                "Approved minimum Salamander reference projection did not apply.");
        drs::engine::PlaybackSnapshotBuilder minimumSnapshotBuilder;
        const auto minimumSnapshot = buildSnapshot(
            minimumSnapshotBuilder, minimumSession.getProject(), 2);
        const auto minimumPrepared = prepareSynchronously(minimumSnapshot);
        const auto minimumRenderModel = buildQualificationRenderModel(
            minimumSnapshot, minimumPrepared.step.result);
        const auto minimumDefaultQuery = makeDefaultEligibilityQuery(
            *minimumRenderModel, drs::engine::PerformanceEventKind::noteOn, 60, 64);
        require(drs::engine::evaluateSamplerRouteEligibility(
                    *minimumRenderModel, minimumDefaultQuery).eligibleRouteIndices.size() == 1,
                "Approved minimum Salamander reference must start exactly one default middle-C route.");

        const auto standardReferenceZoneId = findDefaultMiddleCZoneId(*fullRenderModel);
        const auto minimumReferenceZoneId = findDefaultMiddleCZoneId(*minimumRenderModel);
        const auto standardReferenceSnapshot = drs::engine::scopePlaybackSnapshotForPreparation(
            fullSnapshot,
            { drs::engine::PlaybackPreparationScope::selectedZone,
              standardReferenceZoneId, {} });
        const auto minimumReferenceSnapshot = drs::engine::scopePlaybackSnapshotForPreparation(
            minimumSnapshot,
            { drs::engine::PlaybackPreparationScope::selectedZone,
              minimumReferenceZoneId, {} });
        require(standardReferenceSnapshot.built && minimumReferenceSnapshot.built,
                "Standard/minimum middle-C reference scoping failed.");
        const auto standardResident = prepareResidentSynchronously(standardReferenceSnapshot);
        const auto minimumResident = prepareResidentSynchronously(minimumReferenceSnapshot);
        const auto standardReferenceModel = buildQualificationRenderModel(
            standardReferenceSnapshot, standardResident.step.result);
        const auto minimumReferenceModel = buildQualificationRenderModel(
            minimumReferenceSnapshot, minimumResident.step.result);
        const auto standardReferenceAudio = renderDeterministicReference(standardReferenceModel);
        const auto minimumReferenceAudio = renderDeterministicReference(minimumReferenceModel);
        const auto isLivePreset = sfzPath.parent_path().filename() == "sfz_live";
        const auto audioParity = compareApprovedAudioReference(
            standardReferenceAudio, minimumReferenceAudio, !isLivePreset);

        const auto zonePlayback = runSustainedPlayback(
            scopedZone,
            std::clamp(anchorZone.rootKey, 0, 127),
            std::clamp((anchorZone.velocityLow + anchorZone.velocityHigh) / 2, 1, 127));
        const auto groupPlayback = runSustainedPlayback(
            scopedGroup,
            std::clamp(anchorZone.rootKey, 0, 127),
            std::clamp((anchorZone.velocityLow + anchorZone.velocityHigh) / 2, 1, 127));
        const auto constrainedPlayback = runSustainedPlayback(
            scopedZone,
            std::clamp(anchorZone.rootKey, 0, 127),
            std::clamp((anchorZone.velocityLow + anchorZone.velocityHigh) / 2, 1, 127),
            75);
        require(zonePlayback.pageMisses == 0 && zonePlayback.underrunFrames == 0
                    && groupPlayback.pageMisses == 0 && groupPlayback.underrunFrames == 0,
                "Normal-storage Salamander playback reported a page miss or underrun.");
        const auto package = exportAndActivatePackage(
            packagePath, session.getProject(), projection, fullSnapshot,
            fullPrepared.step.result,
            std::clamp(anchorZone.rootKey, 0, 127),
            std::clamp((anchorZone.velocityLow + anchorZone.velocityHigh) / 2, 1, 127));

        fs::create_directories(reportPath.parent_path());
        std::ofstream report(reportPath, std::ios::binary | std::ios::trunc);
        require(report.good(), "Could not create the qualification report.");
        report << "# Accurate Salamander large-instrument qualification\n\n"
               << "Result: PASS\n\n"
               << "Signed by: DRS automated large-instrument qualification (Release)\n\n"
               << "Corpus: `" << sfzPath.generic_string() << "`\n\n"
               << "- WAV files: " << wavCount << "\n"
               << "- Corpus WAV bytes: " << corpusBytes << "\n"
               << "- Projected sources: " << projection.sampleSources.size() << "\n"
               << "- Projected zones/routes: " << projection.zones.size() << "\n"
               << "- Semantically analyzed regions: " << projection.semanticAnalyzedRegionCount << "\n"
               << "- Unsafe unconditional regions: " << projection.unsafeUnconditionalRegionCount << "\n"
               << "- Import analysis/projection: " << importMicros << " us\n"
               << "- Atomic authoring apply: " << applyMicros << " us\n"
               << "- Full snapshot: " << snapshotMicros << " us\n"
               << "- Selected-zone retained zones/sources: " << scopedZone.retainedZoneCount
               << "/" << scopedZone.retainedSampleCount << "\n"
               << "- Selected-group retained zones/sources: " << scopedGroup.retainedZoneCount
               << "/" << scopedGroup.retainedSampleCount << "\n"
               << "- Selected-zone preparation: " << zonePrepared.elapsed << " us\n"
               << "- Selected-group preparation: " << groupPrepared.elapsed << " us\n"
               << "- Full-draft preparation: " << fullPrepared.elapsed << " us\n"
               << "- Estimated resident decoded bytes: "
               << fullPrepared.step.result.admission.estimatedDecodedBytes << "\n"
               << "- Full-draft decoded bytes: " << fullPrepared.step.result.metrics.decodedBytes << "\n"
               << "- Full-draft resident-head bytes: " << fullSources.headBytes << "\n"
               << "- Default middle-C eligible routes: "
               << semanticQualification.defaultMiddleCRoutes << "\n"
               << "- Enabled middle-C auxiliary resonance routes: "
               << semanticQualification.enabledResonanceRoutes << "\n"
               << "- CC20 middle-C sampled-release routes: "
               << semanticQualification.sampledReleaseRoutes << "\n"
               << "- CC21 middle-C hammer routes: "
               << semanticQualification.hammerRoutes << "\n"
               << "- Retained random pedal-transition routes: "
               << semanticQualification.pedalTransitionRoutes << "\n"
               << "- Approved minimum-reference regions: "
               << minimumProjection.semanticAnalyzedRegionCount << "\n"
               << "- Standard/minimum reference peak: " << audioParity.standardPeak
               << "/" << audioParity.referencePeak << "\n"
               << "- Standard/minimum reference RMS: " << audioParity.standardRms
               << "/" << audioParity.referenceRms << "\n"
               << "- Standard/minimum reference duration frames: "
               << audioParity.durationFrames << "\n"
               << "- Standard/minimum maximum sample-aligned error: "
               << audioParity.maximumSampleError << "\n"
               << "- Standard/minimum RMS sample-aligned error: "
               << audioParity.rmsSampleError << "\n"
               << "- Standard/minimum maximum spectral-profile error: "
               << audioParity.maximumSpectralError << "\n"
               << "- Standard/minimum strict parity enforced: "
               << (audioParity.strictParityEnforced ? "yes" : "no (live-preset qualification)")
               << "\n"
               << "- Standard/minimum within strict tolerance: "
               << (audioParity.withinStrictTolerance ? "yes" : "no") << "\n"
               << "- Selected-zone sustained peak/elapsed: " << std::setprecision(9)
               << zonePlayback.peak << "/" << zonePlayback.elapsed << " us\n"
               << "- Selected-group sustained peak/elapsed: " << groupPlayback.peak
               << "/" << groupPlayback.elapsed << " us\n"
               << "- Zone worker intents/prepared/failures: " << zonePlayback.worker.pageIntentCount
               << "/" << zonePlayback.worker.pagePrepareCount << "/"
               << zonePlayback.worker.pagePrepareFailureCount << "\n"
               << "- Zone page cache bytes: " << zonePlayback.sources.pageBytes << "\n"
               << "- Zone maximum page read latency: "
               << zonePlayback.sources.maximumReadMicros << " us\n"
               << "- Zone maximum callback duration/budget: "
               << zonePlayback.maximumRenderMicros << "/5333 us\n"
               << "- Zone intent published/consumed/dropped/max-depth: "
               << zonePlayback.sources.intentPublished << "/"
               << zonePlayback.sources.intentConsumed << "/"
               << zonePlayback.sources.intentDropped << "/"
               << zonePlayback.sources.maximumIntentDepth << "\n"
               << "- Zone cache budget/peak/leased/retired bytes: "
               << zonePlayback.sources.pageCacheBudgetBytes << "/"
               << zonePlayback.sources.maximumAllocatedPageBytes << "/"
               << zonePlayback.sources.leasedPageBytes << "/"
               << zonePlayback.sources.retiredPageBytes << "\n"
               << "- Zone page misses/underrun frames/recoveries: " << zonePlayback.pageMisses
               << "/" << zonePlayback.underrunFrames << "/" << zonePlayback.recoveries << "\n"
               << "- Group intents/prepared/failures: "
               << groupPlayback.worker.pageIntentCount << "/"
               << groupPlayback.worker.pagePrepareCount << "/"
               << groupPlayback.worker.pagePrepareFailureCount << "\n"
               << "- Group page misses/underrun frames/recoveries: "
               << groupPlayback.pageMisses << "/" << groupPlayback.underrunFrames
               << "/" << groupPlayback.recoveries << "\n"
               << "- Constrained profile: 75 ms page-service poll\n"
               << "- Constrained peak/elapsed: " << constrainedPlayback.peak
               << "/" << constrainedPlayback.elapsed << " us\n"
               << "- Constrained intents/prepared/failures: "
               << constrainedPlayback.worker.pageIntentCount << "/"
               << constrainedPlayback.worker.pagePrepareCount << "/"
               << constrainedPlayback.worker.pagePrepareFailureCount << "\n"
               << "- Constrained page misses/underrun frames/recoveries: "
               << constrainedPlayback.pageMisses << "/" << constrainedPlayback.underrunFrames
               << "/" << constrainedPlayback.recoveries << "\n"
               << "- Peak process working set: " << peakWorkingSetBytes() << " bytes\n\n"
               << "## Package v2\n\n"
               << "- Package bytes: " << package.packageBytes << "\n"
               << "- Records: " << package.recordCount << "\n"
               << "- Export elapsed: " << package.totalMicros << " us\n"
               << "- Export throughput: " << package.throughput << " plaintext bytes/s\n"
               << "- Peak plaintext/sealed buffers: " << package.peakPlaintextBytes
               << "/" << package.peakSealedBytes << " bytes\n"
               << "- Structural verification bytes: " << package.verificationBytes << "\n"
               << "- Metadata open: " << package.metadataOpenMicros << " us\n"
               << "- Warm metadata reopen: " << package.warmMetadataOpenMicros << " us\n"
               << "- Head-ready/playable: " << package.playableMicros << " us\n"
               << "- Package resident-head bytes: " << package.headBytes << "\n"
               << "- Package first-note peak: " << package.firstNotePeak << "\n"
               << "- Actual-corpus cancellation latency/polls: "
               << package.cancellationMicros << " us/" << package.cancellationPolls << "\n"
               << "- Actual-corpus cancellation bytes processed: "
               << package.cancellationBytesProcessed << "\n\n"
               << "The run used real corpus WAV descriptors, bounded 16 KiB heads, the production "
                  "page-intent worker, the immutable render model, and callback-side activation.\n";
        require(report.good(), "Qualification report write failed.");

        std::cout << "Accurate Salamander qualification passed: sources="
                  << projection.sampleSources.size() << " zones=" << projection.zones.size()
                  << " fullPrepareMicros=" << fullPrepared.elapsed
                  << " headBytes=" << fullSources.headBytes
                  << " pagePrepared=" << zonePlayback.worker.pagePrepareCount
                  << " packageBytes=" << package.packageBytes
                  << " peakWorkingSet=" << peakWorkingSetBytes() << std::endl;
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "Accurate Salamander qualification failed: " << error.what() << std::endl;
        return 1;
    }
}
