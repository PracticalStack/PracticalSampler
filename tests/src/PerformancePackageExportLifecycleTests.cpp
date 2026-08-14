#include "Phase1PerformancePackageSupport.h"
#include "drs/engine/DspGraphPlan.h"
#include "drs/engine/EngineFacade.h"
#include "drs/engine/PackageReaderDispatch.h"
#include "shared/PerformancePackageExportService.h"
#include "shared/PerformancePackageProjection.h"

#include <juce_audio_formats/juce_audio_formats.h>

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <string_view>

namespace
{
namespace fs = std::filesystem;
using namespace std::chrono_literals;

void require(const bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

bool containsIssue(const std::vector<std::string>& issues, const std::string_view text)
{
    return std::any_of(issues.begin(), issues.end(), [&](const auto& issue)
    {
        return issue.find(text) != std::string::npos;
    });
}

drs::app::PerformancePackageExportRequest makeRequest(const fs::path& outputPackagePath)
{
    drs::app::PerformancePackageExportRequest request;
    request.project = drs::tests::performance_package::buildAuthoringProjectFixture();
    const auto sourceContentRoot = fs::path(request.project.contentRootPath);
    const auto contentRoot = outputPackagePath.parent_path()
        / (outputPackagePath.stem().string() + "-content");
    fs::create_directories(contentRoot / "Images");
    for (const auto& sample : request.project.sampleSources)
    {
        const auto source = sourceContentRoot / sample.path;
        const auto destination = contentRoot / sample.path;
        fs::create_directories(destination.parent_path());
        require(fs::copy_file(source, destination,
                              fs::copy_options::overwrite_existing),
                "The package export fixture must stage its sample sources.");
    }
    const auto backgroundBytes
        = drs::tests::performance_package::buildBackgroundImageJpegFixture();
    std::ofstream backgroundImage(contentRoot / "Images" / "background.jpg",
                                  std::ios::binary | std::ios::trunc);
    backgroundImage.write(reinterpret_cast<const char*>(backgroundBytes.data()),
                          static_cast<std::streamsize>(backgroundBytes.size()));
    backgroundImage.close();
    require(backgroundImage.good(),
            "The package export fixture must author its background JPEG.");
    request.project.contentRootPath = contentRoot.generic_string();
    require(!request.project.authoring.zones.empty(),
            "The package export fixture must contain a route.");
    auto& semanticRoute = request.project.authoring.zones.front();
    semanticRoute.fineTuneCents = 17.0;
    semanticRoute.amplitudeVelocityTracking = 37.0;
    semanticRoute.releaseSeconds = 1.25;
    semanticRoute.releaseShape = -6.0;
    semanticRoute.controllerConditions = { { 23, 0, 63 } };
    semanticRoute.performance.event = drs::engine::PerformanceEventKind::release;
    request.sessionState.loadProfileId = "balanced";
    request.projectId = request.project.projectId;
    request.baseRevision = 1;
    request.packagePath = outputPackagePath.generic_string();
    return request;
}

void addAuthoredFxRoutingGraph(drs::app::PerformancePackageExportRequest& request)
{
    require(!request.project.authoring.groups.empty(),
            "The graph export fixture must contain an authored group.");
    request.project.authoring.groups.front().routingBusId = "bus-group-pad-core";

    drs::engine::RuntimeProjectFxSlotDefinition drive;
    drive.id = "drive";
    drive.displayName = "Drive";
    drive.effectType = "drs.saturator";
    drive.effectVersion = 1;
    drive.bypassed = false;
    drive.parameters = {
        { "character", 0.0 },
        { "driveDb", 7.5 },
        { "tone", 0.55 },
        { "mix", 0.8 },
        { "outputDb", -1.0 }
    };
    request.project.authoring.fxSlots.push_back(std::move(drive));

    drs::engine::RuntimeProjectRoutingBusDefinition bus;
    bus.id = "bus-group-pad-core";
    bus.displayName = "Pad Core Insert";
    bus.inputSourceId = "groups/pad-core";
    bus.fxSlotIds = { "drive" };
    bus.chainBypassed = false;
    request.project.authoring.routingBuses.push_back(std::move(bus));
}

void replaceRequestSamplesWithFlac(drs::app::PerformancePackageExportRequest& request)
{
    const auto contentRoot = fs::path(request.project.contentRootPath);
    for (auto& sampleSource : request.project.sampleSources)
    {
        const auto wavPath = contentRoot / sampleSource.path;
        const auto imported = drs::engine::importSampleFile(wavPath.generic_string());
        require(imported.imported,
                "The FLAC export regression fixture must decode its source WAV.");
        require(imported.sample.metadata.frameCount
                    <= static_cast<std::uint64_t>(std::numeric_limits<int>::max()),
                "The FLAC export regression fixture must fit in a JUCE audio buffer.");

        juce::AudioBuffer<float> buffer(
            static_cast<int>(imported.sample.metadata.channelCount),
            static_cast<int>(imported.sample.metadata.frameCount));
        for (std::size_t channel = 0;
             channel < imported.sample.normalizedChannels.size();
             ++channel)
        {
            const auto& samples = imported.sample.normalizedChannels[channel];
            std::copy(samples.begin(), samples.end(), buffer.getWritePointer(static_cast<int>(channel)));
        }

        auto relativeFlacPath = fs::path(sampleSource.path);
        relativeFlacPath.replace_extension(".flac");
        const auto flacPath = contentRoot / relativeFlacPath;
        auto fileOutput = std::make_unique<juce::FileOutputStream>(
            juce::File(flacPath.generic_string()));
        require(fileOutput->openedOk(),
                "The FLAC export regression fixture must create its FLAC file.");
        std::unique_ptr<juce::OutputStream> output = std::move(fileOutput);
        juce::FlacAudioFormat format;
        auto writer = format.createWriterFor(
            output,
            juce::AudioFormatWriterOptions {}
                .withSampleRate(imported.sample.metadata.sampleRate)
                .withNumChannels(buffer.getNumChannels())
                .withBitsPerSample(24));
        require(writer != nullptr
                    && writer->writeFromAudioSampleBuffer(buffer, 0, buffer.getNumSamples()),
                "The FLAC export regression fixture must encode its sample data.");
        writer.reset();
        require(fs::remove(wavPath),
                "The FLAC export regression fixture must not retain its source WAV.");
        sampleSource.path = relativeFlacPath.generic_string();
    }
}
} // namespace

int main()
{
    using namespace drs::app;

    try
    {
        using Stage = PerformancePackageExportStage;
        require(isPerformancePackageExportStageTransitionAllowed(Stage::idle, Stage::queued)
                    && isPerformancePackageExportStageTransitionAllowed(Stage::queued, Stage::validating)
                    && isPerformancePackageExportStageTransitionAllowed(Stage::validating, Stage::compiling)
                    && isPerformancePackageExportStageTransitionAllowed(Stage::compiling, Stage::writingStream)
                    && isPerformancePackageExportStageTransitionAllowed(Stage::writingStream, Stage::sealingPackage)
                    && isPerformancePackageExportStageTransitionAllowed(Stage::sealingPackage, Stage::verifying)
                    && isPerformancePackageExportStageTransitionAllowed(Stage::verifying, Stage::completed)
                    && isPerformancePackageExportStageTransitionAllowed(Stage::completed, Stage::consumed),
                "The owned playable-package export lifecycle must remain executable.");
        require(isPerformancePackageExportStageTransitionAllowed(Stage::queued, Stage::canceled)
                    && isPerformancePackageExportStageTransitionAllowed(Stage::writingStream, Stage::canceled)
                    && isPerformancePackageExportStageTransitionAllowed(Stage::sealingPackage, Stage::failed),
                "Queued and in-flight export work must retain terminal exits.");
        require(!isPerformancePackageExportStageTransitionAllowed(Stage::idle, Stage::completed)
                    && !isPerformancePackageExportStageTransitionAllowed(Stage::queued, Stage::sealingPackage)
                    && !isPerformancePackageExportStageTransitionAllowed(Stage::failed, Stage::completed),
                "Invalid export lifecycle shortcuts must be rejected.");

        auto unsupportedProject = drs::tests::performance_package::buildAuthoringProjectFixture();
        drs::engine::RuntimeProjectMacroDefinition targetedMacro;
        targetedMacro.id = "unsupported-target";
        targetedMacro.name = "Unsupported Target";
        targetedMacro.targets.push_back({});
        unsupportedProject.authoring.macros.push_back(std::move(targetedMacro));
        unsupportedProject.authoring.performanceBanks.push_back(
            { "unsupported-bank", "Unsupported Bank", {}, {}, {} });
        unsupportedProject.authoring.groups.front().pan = 0.25;
        unsupportedProject.authoring.zones.front().loopEnabled = true;
        const auto compatibilityProjection = projectPerformancePackage(
            unsupportedProject, {}, PerformancePackageProjectionContext { "unsupported" });
        require(!compatibilityProjection.projected
                    && containsIssue(compatibilityProjection.issues, "target mappings")
                    && containsIssue(compatibilityProjection.issues, "performance banks")
                    && containsIssue(compatibilityProjection.issues, "non-default pan")
                    && containsIssue(compatibilityProjection.issues, "loop settings"),
                "FX/routing support must not loosen unrelated export compatibility blockers.");

        const auto tempRoot = fs::temp_directory_path() / "drs-performance-package-export-lifecycle";
        fs::remove_all(tempRoot);
        fs::create_directories(tempRoot);

        std::mutex gateMutex;
        std::condition_variable gateCondition;
        auto streamReached = false;
        auto releaseStream = false;

        PerformancePackageExportServiceOptions options;
        options.stageObserver = [&](const Stage stage)
        {
            if (stage != Stage::writingStream)
                return;

            std::unique_lock<std::mutex> lock(gateMutex);
            streamReached = true;
            gateCondition.notify_all();
            gateCondition.wait(lock, [&] { return releaseStream; });
        };

        PerformancePackageExportService service(std::move(options));
        auto client = service.openClient();
        auto request = makeRequest(tempRoot / "cancel-me.drpkg");

        const auto accepted = client.submit(request);
        require(accepted.disposition == PerformancePackageExportSubmitDisposition::accepted,
                "The first export request must be accepted.");
        const auto duplicate = client.submit(request);
        require(duplicate.disposition == PerformancePackageExportSubmitDisposition::busy,
                "A duplicate export request must be rejected as busy.");

        {
            std::unique_lock<std::mutex> lock(gateMutex);
            require(gateCondition.wait_for(lock, 20s, [&] { return streamReached; }),
                    "The deterministic hook must pause at the stream-writing checkpoint.");
        }

        require(client.cancel("Lifecycle test cancellation"),
                "An active export request must accept cancellation.");
        {
            std::lock_guard<std::mutex> lock(gateMutex);
            releaseStream = true;
        }
        gateCondition.notify_all();

        require(client.waitForTerminal(20s),
                "Canceled export must reach a terminal state.");
        const auto canceled = client.getSnapshot();
        require(canceled && canceled->identity.generation == accepted.identity.generation
                    && canceled->stage == Stage::canceled,
                "The current generation must publish one typed canceled export snapshot.");

        const auto metrics = service.getMetrics();
        require(metrics.requestedCount == 1
                    && metrics.rejectedBusyCount == 1
                    && metrics.canceledCount == 1
                    && metrics.maximumPendingCount <= 1
                    && metrics.maximumInFlightCount == 1,
                "Lifecycle metrics must prove bounded export scheduling and cancellation.");

        service.shutdown();
        service.shutdown();
        require(service.getMetrics().liveWorkerCount == 0,
                "Idempotent export shutdown must leave no live worker.");

        PerformancePackageExportService completionService;
        auto completionClient = completionService.openClient();
        const auto completedAccepted = completionClient.submit(makeRequest(tempRoot / "completed.drpkg"));
        require(completedAccepted.disposition == PerformancePackageExportSubmitDisposition::accepted,
                "The completion export request must be accepted.");
        require(completionClient.waitForTerminal(60s),
                "A valid export request must reach a terminal state.");

        const auto completed = completionClient.getSnapshot();
        require(completed && completed->stage == Stage::completed && completed->result != nullptr
                    && completed->result->exported,
                "A successful export must publish a completed snapshot with an exported result.");
        require(fs::exists(tempRoot / "completed.drpkg"),
                "The completed export should materialize the playable package on disk.");
        require(completed->result->packageBytes > 0 && completed->result->payloadCount > 0,
                "The completed export should report non-empty package metrics.");
        completionService.shutdown();

        const auto completedPackage = (tempRoot / "completed.drpkg").generic_string();
        const auto packageV2 = drs::engine::loadPerformancePackageV2Metadata(completedPackage);
        require(packageV2.loaded && packageV2.package != nullptr,
                "The exported semantic package must reopen through the package-v2 metadata path.");
        require(packageV2.metadata.manifest.schemaVersion
                    == drs::engine::performancePackageLegacySchemaVersion
                    && packageV2.metadata.manifest.minimumReaderSchemaVersion
                        == drs::engine::performancePackageLegacySchemaVersion
                    && packageV2.metadata.instrument.instrument.schemaVersion
                        < drs::engine::runtimeInstrumentFxRoutingSchemaVersion,
                "Graph-free export must retain legacy package and runtime instrument versions.");
        const auto expectedBackgroundBytes
            = drs::tests::performance_package::buildBackgroundImageJpegFixture();
        require(packageV2.metadata.backgroundImage.loaded
                    && packageV2.metadata.backgroundImage.payload.payloadId == "background-image"
                    && packageV2.metadata.backgroundImage.payload.payloadKind == "backgroundImage"
                    && packageV2.metadata.backgroundImage.payload.mediaType == "image/jpeg"
                    && packageV2.metadata.backgroundImage.payload.plaintextBytes
                        == expectedBackgroundBytes,
                "The package-v2 metadata path must reconstruct the exported background JPEG.");
        auto preparedActivation = drs::engine::preparePerformancePackageV2Activation(
            packageV2.metadata, packageV2.package, packageV2.sampleDescriptors);
        require(preparedActivation.prepared
                    && preparedActivation.activationPayload != nullptr
                    && preparedActivation.activationPayload->snapshot != nullptr
                    && preparedActivation.activationPayload->prepared != nullptr
                    && preparedActivation.renderModel != nullptr,
                "Package-v2 activation preparation must accept non-default route topology.");
        const auto& preparedRoutes = preparedActivation.activationPayload->prepared->zones;
        const auto preparedRoute = std::find_if(
            preparedRoutes.begin(), preparedRoutes.end(), [](const auto& route)
            {
                return route.zoneId == "pad-a3";
            });
        require(preparedRoute != preparedRoutes.end()
                    && preparedRoute->fineTuneCents == 17.0
                    && preparedRoute->amplitudeVelocityTracking == 37.0
                    && preparedRoute->releaseSeconds == 1.25
                    && preparedRoute->releaseShape == -6.0
                    && preparedRoute->controllerConditions
                        == std::vector<drs::engine::RuntimeControllerCondition> { { 23, 0, 63 } },
                "Package-v2 reconstruction must retain tuning, velocity tracking, release time, and controller conditions.");
        const auto& renderRoutes = preparedActivation.renderModel->getRoutes();
        const auto renderRoute = std::find_if(
            renderRoutes.begin(), renderRoutes.end(), [](const auto& route)
            {
                return route.zoneId == "pad-a3";
            });
        require(renderRoute != renderRoutes.end()
                    && renderRoute->releaseSeconds == 1.25
                    && renderRoute->releaseShape == -6.0
                    && renderRoute->performanceEvent
                        == drs::engine::PerformanceEventKind::release,
                "Package-v2 reconstruction must rebuild release envelopes and non-note-on performance events.");
        drs::engine::EngineFacade facade;
        const auto activated = facade.activatePreparedPerformancePackageSession(
            std::move(preparedActivation));
        require(activated.activated,
                "The reconstructed semantic package must activate through the production engine facade.");
        const auto performanceSnapshot = facade.getPerformanceSnapshot();
        require(performanceSnapshot.backgroundArtworkSourceKey == "package://background-image"
                    && performanceSnapshot.backgroundArtworkJpgBytes != nullptr
                    && *performanceSnapshot.backgroundArtworkJpgBytes == expectedBackgroundBytes,
                "Prepared package activation must expose the v2 background JPEG to Performance.");

        auto graphRequest = makeRequest(tempRoot / "completed-fx-routing.drpkg");
        addAuthoredFxRoutingGraph(graphRequest);
        drs::engine::PlaybackSnapshotBuilder sourceSnapshotBuilder;
        const auto sourceSnapshotRequest = sourceSnapshotBuilder.requestBuild(1, true);
        const auto sourceSnapshot = sourceSnapshotBuilder.buildSnapshot(
            sourceSnapshotRequest, graphRequest.project);
        require(sourceSnapshot.built && sourceSnapshot.activationEligible,
                "The authored FX/routing source must produce an activation-eligible snapshot.");
        const auto sourceGraphPlan = drs::engine::compileDspGraphPlan(sourceSnapshot.snapshot);
        require(sourceGraphPlan.compiled && sourceGraphPlan.plan.nodes.size() == 1
                    && !sourceGraphPlan.plan.directFastPath,
                "The authored FX/routing source must compile one executable DSP node.");
        const auto graphExport = executePerformancePackageExport(graphRequest);
        require(graphExport.exported,
                "An authored FX/routing graph must export through the shared production path.");
        const auto graphPackage = drs::engine::loadPerformancePackageV2Metadata(
            (tempRoot / "completed-fx-routing.drpkg").generic_string());
        require(graphPackage.loaded
                    && graphPackage.metadata.manifest.schemaVersion
                        == drs::engine::performancePackageFxRoutingSchemaVersion
                    && graphPackage.metadata.manifest.minimumReaderSchemaVersion
                        == drs::engine::performancePackageFxRoutingMinimumReaderSchemaVersion
                    && graphPackage.metadata.instrument.instrument.schemaVersion
                        == drs::engine::runtimeInstrumentFxRoutingSchemaVersion,
                "Graph-bearing export must select package schema 2, minimum reader 2, and runtime instrument v4.");
        const auto& graphInstrument = graphPackage.metadata.instrument.instrument;
        require(graphInstrument.fxSlots.size() == 1
                    && graphInstrument.fxSlots.front().id == "drive"
                    && !graphInstrument.fxSlots.front().bypassed
                    && graphInstrument.fxSlots.front().parameters.size() == 5
                    && graphInstrument.fxSlots.front().parameters.at(1).id == "driveDb"
                    && graphInstrument.fxSlots.front().parameters.at(1).value == 7.5
                    && graphInstrument.routingBuses.size() == 1
                    && graphInstrument.routingBuses.front().id == "bus-group-pad-core"
                    && !graphInstrument.routingBuses.front().chainBypassed
                    && graphInstrument.routingBuses.front().fxSlotIds
                        == std::vector<std::string> { "drive" }
                    && graphInstrument.groups.front().routingBusId == "bus-group-pad-core",
                "Graph-bearing export must preserve slot order, parameters, bypass state, bus chains, and group assignment.");

        auto graphActivation = drs::engine::preparePerformancePackageV2Activation(
            graphPackage.metadata, graphPackage.package, graphPackage.sampleDescriptors);
        require(graphActivation.prepared
                    && graphActivation.activationPayload != nullptr
                    && graphActivation.activationPayload->snapshot != nullptr
                    && graphActivation.renderModel != nullptr,
                "A reopened graph-bearing package must prepare an immutable activation payload.");
        const auto& reopenedSnapshot = *graphActivation.activationPayload->snapshot;
        const auto reopenedGroup = std::find_if(
            reopenedSnapshot.groupRoutes.begin(), reopenedSnapshot.groupRoutes.end(),
            [](const auto& group) { return group.groupId == "pad-core"; });
        require(reopenedSnapshot.fxSlots.size() == 1
                    && reopenedSnapshot.fxSlots.front().catalogResolved
                    && reopenedSnapshot.routingBuses.size() == 1
                    && reopenedGroup != reopenedSnapshot.groupRoutes.end()
                    && reopenedGroup->routingSourceId == "groups/pad-core"
                    && reopenedGroup->routingBusId == "bus-group-pad-core"
                    && reopenedSnapshot.dspGraphDigest == sourceSnapshot.snapshot.dspGraphDigest,
                "Package preparation must hydrate catalog metadata, buses, group routing, and the authored graph digest.");
        const auto reopenedGraphPlan = drs::engine::compileDspGraphPlan(reopenedSnapshot);
        require(reopenedGraphPlan.compiled
                    && reopenedGraphPlan.plan.planDigest == sourceGraphPlan.plan.planDigest
                    && reopenedGraphPlan.plan.nodes.size() == sourceGraphPlan.plan.nodes.size()
                    && reopenedGraphPlan.plan.parameters.size()
                        == sourceGraphPlan.plan.parameters.size()
                    && reopenedGraphPlan.plan.nodes.front().slotId
                        == sourceGraphPlan.plan.nodes.front().slotId,
                "The reopened package must compile the same immutable DSP plan as its authoring source.");

        const auto graphActivated = facade.activatePreparedPerformancePackageSession(
            std::move(graphActivation));
        require(graphActivated.activated,
                "The graph-bearing package must activate through the production facade.");
        const auto activeGraphPayload = facade.getPerformancePackageActivationPayload();
        const auto activeGraphModel = facade.getPerformancePackageRenderModel();
        require(activeGraphPayload != nullptr && activeGraphModel != nullptr,
                "Successful graph activation must publish both immutable package artifacts.");

        auto malformedMetadata = graphPackage.metadata;
        malformedMetadata.instrument.instrument.routingBuses.front().fxSlotIds.push_back(
            "missing-slot");
        auto malformedActivation = drs::engine::preparePerformancePackageV2Activation(
            malformedMetadata, graphPackage.package, graphPackage.sampleDescriptors);
        require(!malformedActivation.prepared
                    && containsIssue(malformedActivation.issues, "unknown FX slot"),
                "Malformed package graphs must fail before activation payload publication.");
        const auto rejectedReplacement = facade.activatePreparedPerformancePackageSession(
            std::move(malformedActivation));
        require(!rejectedReplacement.activated
                    && facade.getPerformancePackageActivationPayload() == activeGraphPayload
                    && facade.getPerformancePackageRenderModel() == activeGraphModel,
                "A malformed replacement package must preserve the currently active package generation.");

        PerformancePackageExportService flacService;
        auto flacClient = flacService.openClient();
        auto flacRequest = makeRequest(tempRoot / "completed-flac.drpkg");
        replaceRequestSamplesWithFlac(flacRequest);
        const auto flacAccepted = flacClient.submit(flacRequest);
        require(flacAccepted.disposition == PerformancePackageExportSubmitDisposition::accepted,
                "A FLAC-backed export request must be accepted.");
        require(flacClient.waitForTerminal(60s),
                "A FLAC-backed export request must reach a terminal state.");
        const auto flacCompleted = flacClient.getSnapshot();
        require(flacCompleted && flacCompleted->stage == Stage::completed
                    && flacCompleted->result != nullptr
                    && flacCompleted->result->exported,
                "A FLAC-backed instrument must export as a playable package.");
        flacService.shutdown();

        const auto flacPackage = drs::engine::loadPerformancePackageV2Metadata(
            (tempRoot / "completed-flac.drpkg").generic_string());
        require(flacPackage.loaded
                    && flacPackage.package != nullptr
                    && flacPackage.metadata.stream.loaded
                    && flacPackage.metadata.stream.container.samples.size()
                        == flacRequest.project.sampleSources.size()
                    && std::all_of(flacPackage.metadata.stream.container.samples.begin(),
                                   flacPackage.metadata.stream.container.samples.end(),
                                   [](const auto& sample)
                                   {
                                       return sample.formatName == "FLAC file"
                                           && sample.sampleRate > 0.0
                                           && sample.channelCount > 0
                                           && sample.frameCount > 0;
                                   }),
                "The exported package must retain FLAC source metadata and playable sample records.");
        auto flacActivation = drs::engine::preparePerformancePackageV2Activation(
            flacPackage.metadata, flacPackage.package, flacPackage.sampleDescriptors);
        require(flacActivation.prepared && flacActivation.renderModel != nullptr,
                "The FLAC-backed package must reopen through production activation preparation.");

        fs::remove_all(tempRoot);

        std::cout << "Playable package export lifecycle tests passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Playable package export lifecycle tests failed: "
                  << exception.what() << std::endl;
        return 1;
    }
}
