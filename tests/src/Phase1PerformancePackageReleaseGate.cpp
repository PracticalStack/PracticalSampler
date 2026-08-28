#include "drs/engine/DspGraphPlan.h"
#include "drs/engine/EngineFacade.h"
#include "drs/engine/PackageReader.h"
#include "drs/engine/PackageReaderDispatch.h"
#include "drs/engine/PackageV2.h"
#include "drs/engine/PerformancePackage.h"
#include "drs/engine/PlayableInstrumentLicense.h"
#include "drs/engine/PlaybackSnapshot.h"
#include "drs/engine/PreparedPlayback.h"
#include "drs/engine/SamplerRenderModel.h"
#include "plugin/PluginEditor.h"
#include "plugin/PluginProcessor.h"
#include "shared/PerformancePackageExportService.h"
#include "shared/PlayableInstrumentLicenseViewer.h"
#include "shared/WorkspaceMenuPolicy.h"
#include "standalone/MainComponent.h"
#include "Phase1PerformancePackageSupport.h"
#include "PerformancePackageExportSecurityTestSupport.h"
#include "Sprint4OfflineRenderHarness.h"

#include <juce_audio_processors_headless/juce_audio_processors_headless.h>

#include <json/json.hpp>

#include <algorithm>
#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

namespace
{
namespace fs = std::filesystem;
using ordered_json = nlohmann::ordered_json;
namespace package_support = drs::tests::performance_package;
using Clock = std::chrono::steady_clock;

void require(bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

std::string summarizeIssues(const std::vector<std::string>& issues)
{
    if (issues.empty())
        return "(none)";

    std::string summary;
    for (std::size_t index = 0; index < issues.size(); ++index)
    {
        if (index != 0)
            summary += " | ";
        summary += issues[index];
    }
    return summary;
}

bool hasEmptyProjectBindingAndPackageBinding(const std::string& serializedState)
{
    const auto parsed = drs::engine::parseHostSessionState(serializedState);
    if (!parsed.isValidHostState()
        || !parsed.hostState.has_value()
        || !parsed.hostState->performancePackageBinding.has_value())
    {
        return false;
    }

    const auto& binding = parsed.hostState->projectBinding;
    return binding.projectId.empty()
        && binding.manifestPath.empty()
        && binding.manifestFileName.empty()
        && binding.manifestDigest.empty()
        && binding.contentRootHint.empty()
        && binding.portableRelativePath.empty();
}

void writeTextFile(const fs::path& path, const std::string& text)
{
    fs::create_directories(path.parent_path());
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    require(output.good(), "Could not open performance package release gate artifact for writing: " + path.generic_string());
    output << text;
    require(output.good(), "Could not finish writing performance package release gate artifact: " + path.generic_string());
}

void writeBinaryFile(const fs::path& path, const std::vector<std::uint8_t>& bytes)
{
    fs::create_directories(path.parent_path());
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    require(output.good(), "Could not open release-gate binary fixture: " + path.generic_string());
    output.write(reinterpret_cast<const char*>(bytes.data()),
                 static_cast<std::streamsize>(bytes.size()));
    require(output.good(), "Could not finish release-gate binary fixture: " + path.generic_string());
}

juce::Component* findDescendantById(juce::Component& root, const juce::String& componentId)
{
    if (root.getComponentID() == componentId)
        return &root;

    for (int index = 0; index < root.getNumChildComponents(); ++index)
    {
        if (auto* match = findDescendantById(*root.getChildComponent(index), componentId))
            return match;
    }

    return nullptr;
}

std::string expectedLicenseDisplayText()
{
    const auto bytes = package_support::buildLicenseTextFixture();
    return { bytes.begin() + 3, bytes.end() };
}

bool validateLicenseViewer(const std::shared_ptr<const std::string>& licenseText)
{
    if (licenseText == nullptr || *licenseText != expectedLicenseDisplayText())
        return false;

    drs::app::PlayableInstrumentLicenseViewer viewer(licenseText);
    viewer.setSize(280, 240);
    viewer.resized();
    auto* editor = dynamic_cast<juce::TextEditor*>(
        findDescendantById(viewer, "playableInstrumentLicenseText"));
    auto* closeButton = dynamic_cast<juce::TextButton*>(
        findDescendantById(viewer, "playableInstrumentLicenseCloseButton"));
    if (editor == nullptr || closeButton == nullptr || !editor->isReadOnly()
        || !editor->isMultiLine() || editor->getText().toStdString() != *licenseText
        || closeButton->getButtonText() != "Close")
    {
        return false;
    }

    editor->selectAll();
    return editor->getHighlightedText() == editor->getText()
        && viewer.getLocalBounds().contains(editor->getBounds())
        && viewer.getLocalBounds().contains(closeButton->getBounds());
}

drs::engine::RuntimeProjectModel buildLicensedAuthoringProjectFixture(
    const fs::path& contentRoot)
{
    auto project = package_support::buildAuthoringProjectFixture();
    const auto sourceRoot = fs::path(project.contentRootPath);
    for (const auto& sample : project.sampleSources)
    {
        const auto source = sourceRoot / sample.path;
        const auto destination = contentRoot / sample.path;
        fs::create_directories(destination.parent_path());
        require(fs::copy_file(source, destination, fs::copy_options::overwrite_existing),
                "Could not stage the licensed release-gate sample fixture.");
    }
    writeBinaryFile(contentRoot / drs::engine::playableInstrumentLicenseFileName,
                    package_support::buildLicenseTextFixture());
    project.contentRootPath = contentRoot.generic_string();
    return project;
}

float renderQueuedPerformanceSurfaceMagnitude(drs::plugin::Processor& processor,
                                              const int midiNoteNumber,
                                              const float velocity,
                                              const int blockCount = 8)
{
    processor.queuePerformanceSurfaceNoteOn(midiNoteNumber, velocity);

    float maxMagnitude = 0.0f;
    juce::MidiBuffer emptyMidiBuffer;
    for (int blockIndex = 0; blockIndex < blockCount; ++blockIndex)
    {
        juce::AudioBuffer<float> buffer(2, 512);
        buffer.clear();
        processor.processBlock(buffer, emptyMidiBuffer);
        maxMagnitude = std::max(maxMagnitude, buffer.getMagnitude(0, buffer.getNumSamples()));
    }

    processor.queuePerformanceSurfaceNoteOff(midiNoteNumber);
    return maxMagnitude;
}

ordered_json buildCompatibilityPolicySection()
{
    ordered_json section;
    section["policyId"] = drs::engine::performancePackageCompatibilityPolicyId;
    section["schemaName"] = drs::engine::performancePackageSchemaName;
    section["schemaVersion"] = drs::engine::performancePackageSchemaVersion;
    section["schemaMajorVersion"] = drs::engine::performancePackageSchemaMajorVersion;
    section["schemaMinorVersion"] = drs::engine::performancePackageSchemaMinorVersion;
    section["futureMinorPolicy"] = drs::engine::performancePackageFutureMinorPolicy;
    section["futureMajorPolicy"] = drs::engine::performancePackageFutureMajorPolicy;
    section["passed"] = drs::engine::performancePackageSchemaMajorVersion == 1
        && drs::engine::performancePackageSchemaMinorVersion == 0
        && drs::engine::performancePackageSchemaVersion == 1;
    return section;
}

ordered_json buildDeterminismSection(const fs::path& scratchDirectory)
{
    const auto& cryptoProvider = drs::engine::getDeterministicPackageCryptoProvider();
    const auto firstPlan = package_support::buildPackagePlan(scratchDirectory / "first",
                                                             scratchDirectory / "first" / "deterministic-a.drpkg");
    const auto secondPlan = package_support::buildPackagePlan(scratchDirectory / "second",
                                                              scratchDirectory / "second" / "deterministic-b.drpkg");

    const auto firstWrite = drs::engine::writePerformancePackage(firstPlan, cryptoProvider);
    const auto secondWrite = drs::engine::writePerformancePackage(secondPlan, cryptoProvider);
    require(firstWrite.written, "First deterministic package write should succeed.");
    require(secondWrite.written, "Second deterministic package write should succeed.");

    const auto firstBytes = package_support::readBinaryFile(firstPlan.outputPackagePath);
    const auto secondBytes = package_support::readBinaryFile(secondPlan.outputPackagePath);

    ordered_json section;
    section["firstPackagePath"] = firstPlan.outputPackagePath;
    section["secondPackagePath"] = secondPlan.outputPackagePath;
    section["firstPackageBytes"] = firstBytes.size();
    section["secondPackageBytes"] = secondBytes.size();
    section["byteIdentical"] = firstBytes == secondBytes;
    section["passed"] = firstBytes == secondBytes;
    return section;
}

ordered_json buildFailureReportingSection()
{
    const auto& cryptoProvider = drs::engine::getDeterministicPackageCryptoProvider();
    const auto corpusPaths = package_support::getCheckedInCorpusPaths();
    const auto indexRoot = ordered_json::parse(package_support::readTextFile(corpusPaths.index));

    ordered_json fixtures = ordered_json::array();
    bool passed = true;
    for (const auto& fixture : indexRoot.at("fixtures"))
    {
        const auto fixtureId = fixture.at("id").get<std::string>();
        const auto fixturePath = corpusPaths.root / fixture.at("path").get<std::string>();
        const auto expectedFailureCategory = fixture.at("expectedFailureCategory").get<std::string>();
        const auto expectedIssueSubstring = fixture.at("expectedIssueSubstring").get<std::string>();

        const auto load = drs::engine::loadPerformancePackage(fixturePath.generic_string(),
                                                              cryptoProvider,
                                                              drs::engine::performancePackageSchemaVersion);
        const bool shouldLoad = fixtureId == "valid";
        const bool stateMatched = shouldLoad ? load.loaded : !load.loaded;
        const bool categoryMatched = std::string(drs::engine::toString(load.failureCategory))
            == expectedFailureCategory;
        const bool issueMatched = expectedIssueSubstring.empty()
            || package_support::containsIssue(load.issues, expectedIssueSubstring);

        ordered_json caseResult;
        caseResult["id"] = fixtureId;
        caseResult["path"] = fixturePath.generic_string();
        caseResult["loaded"] = load.loaded;
        caseResult["failureCategory"] = drs::engine::toString(load.failureCategory);
        caseResult["expectedFailureCategory"] = expectedFailureCategory;
        caseResult["issueMatched"] = issueMatched;
        caseResult["state"] = load.state;
        caseResult["issues"] = load.issues;
        caseResult["passed"] = stateMatched && categoryMatched && issueMatched;
        fixtures.push_back(std::move(caseResult));
        passed = passed && stateMatched && categoryMatched && issueMatched;
    }

    ordered_json section;
    section["corpusIndexPath"] = corpusPaths.index.generic_string();
    section["fixtures"] = std::move(fixtures);
    section["passed"] = passed;
    return section;
}

ordered_json buildMenuCutoverSection()
{
    ordered_json section;
    section["saveProjectLabel"] = drs::app::saveProjectMenuLabel;
    section["saveProjectAsLabel"] = drs::app::saveProjectAsMenuLabel;
    section["exportPlayableInstrumentLabel"] = drs::app::exportPerformancePackageMenuLabel;
    section["openPlayablePackageLabel"] = drs::app::openPerformancePackageMenuLabel;
    section["helpText"] = drs::app::projectDirectoryHelpText;
    section["labelsDistinct"]
        = std::string(drs::app::saveProjectMenuLabel) != std::string(drs::app::exportPerformancePackageMenuLabel);
    section["helpMentionsProjectExtension"]
        = std::string(drs::app::projectDirectoryHelpText).find(".drsproj") != std::string::npos;
    section["helpMentionsPackageExtension"]
        = std::string(drs::app::projectDirectoryHelpText).find(".drpkg") != std::string::npos;
    section["passed"] = section["labelsDistinct"].get<bool>()
        && section["helpMentionsProjectExtension"].get<bool>()
        && section["helpMentionsPackageExtension"].get<bool>();
    return section;
}

drs::engine::RuntimeProjectFxSlotDefinition makeFxSlot(
    std::string id,
    std::string displayName,
    std::string effectType,
    std::vector<drs::engine::RuntimeProjectFxSlotDefinition::ParameterValue> parameters,
    const bool bypassed = false)
{
    drs::engine::RuntimeProjectFxSlotDefinition slot;
    slot.id = std::move(id);
    slot.displayName = std::move(displayName);
    slot.effectType = std::move(effectType);
    slot.effectVersion = 1;
    slot.bypassed = bypassed;
    slot.parameters = std::move(parameters);
    return slot;
}

void addFxRoutingParityGraph(drs::engine::RuntimeProjectModel& project)
{
    require(project.authoring.groups.size() == 2 && project.authoring.zones.size() == 2,
            "FX/routing parity fixture requires two groups and two zones.");

    // Both channel-layout routes use the default articulation so one deterministic
    // MIDI timeline can address mono Pad A3 and stereo Lead A4 independently.
    project.authoring.zones[0].keyHigh = 59;
    project.authoring.zones[1].keyLow = 60;
    project.authoring.zones[1].articulationId = "sustain";
    project.authoring.groups[0].routingBusId = "bus-group-pad-core";
    project.authoring.groups[1].routingBusId = "bus-group-lead-core";

    project.authoring.fxSlots = {
        makeFxSlot("pad-eq", "Pad EQ", "drs.compactEq",
                   { { "mode", 1.0 }, { "frequencyHz", 1800.0 }, { "q", 0.8 },
                     { "gainDb", 3.0 }, { "mix", 1.0 } }),
        makeFxSlot("pad-bypassed-gain", "Bypassed Pad Gain", "drs.gain",
                   { { "gainDb", 12.0 }, { "polarity", 0.0 }, { "mute", 0.0 } }, true),
        makeFxSlot("lead-eq", "Lead EQ", "drs.compactEq",
                   { { "mode", 2.0 }, { "frequencyHz", 4200.0 }, { "q", 0.65 },
                     { "gainDb", -2.0 }, { "mix", 0.8 } }),
        makeFxSlot("pad-chorus", "Pad Chorus", "drs.chorus",
                   { { "rateHz", 0.7 }, { "depthMs", 3.5 }, { "baseDelayMs", 14.0 },
                     { "width", 0.9 }, { "mix", 0.35 } }),
        makeFxSlot("lead-bypassed-gain", "Bypassed Lead Chain Gain", "drs.gain",
                   { { "gainDb", -9.0 }, { "polarity", 0.0 }, { "mute", 0.0 } }),
        makeFxSlot("master-delay", "Master Delay", "drs.stereoDelay",
                   { { "timeMs", 70.0 }, { "sync", 0.0 }, { "divisionBeats", 0.5 },
                     { "feedback", 0.3 }, { "pingPong", 1.0 }, { "tone", 0.75 },
                     { "width", 1.0 }, { "mix", 0.22 } }),
        makeFxSlot("master-room", "Master Room", "drs.algorithmicReverb",
                   { { "preDelayMs", 8.0 }, { "size", 0.45 }, { "decaySeconds", 0.8 },
                     { "damping", 0.55 }, { "width", 1.0 }, { "mix", 0.2 } })
    };
    project.authoring.routingBuses = {
        { "bus-zone-pad-a3", "Pad Zone Insert", "zones/pad-a3",
          { "pad-eq", "pad-bypassed-gain" }, false },
        { "bus-zone-lead-a4", "Lead Zone Insert", "zones/lead-a4",
          { "lead-eq" }, false },
        { "bus-group-pad-core", "Pad Group Insert", "groups/pad-core",
          { "pad-chorus" }, false },
        { "bus-group-lead-core", "Lead Group Insert", "groups/lead-core",
          { "lead-bypassed-gain" }, true },
        { "bus-master", "Master Insert", "master",
          { "master-delay", "master-room" }, false }
    };
}

drs::app::PerformancePackageExportRequest buildFxRoutingParityRequest(
    const fs::path& scratchDirectory)
{
    drs::app::PerformancePackageExportRequest request;
    request.project = package_support::buildAuthoringProjectFixture();
    const auto sourceContentRoot = fs::path(request.project.contentRootPath);
    const auto contentRoot = scratchDirectory / "content";
    fs::create_directories(contentRoot / "Images");
    for (const auto& sample : request.project.sampleSources)
    {
        const auto source = sourceContentRoot / sample.path;
        const auto destination = contentRoot / sample.path;
        fs::create_directories(destination.parent_path());
        require(fs::copy_file(source, destination, fs::copy_options::overwrite_existing),
                "FX/routing parity fixture could not stage a source sample.");
    }
    package_support::writeBinaryFile(
        contentRoot / "Images" / "background.jpg",
        package_support::buildBackgroundImageJpegFixture());
    request.project.contentRootPath = contentRoot.generic_string();
    addFxRoutingParityGraph(request.project);
    request.sessionState.loadProfileId = "balanced";
    request.projectId = request.project.projectId;
    request.baseRevision = 1;
    request.packagePath = (scratchDirectory / "fx-routing-parity.drpkg").generic_string();
    request.securityContext = drs::tests::makePerformancePackageExportTestSecurityContext();
    return request;
}

struct FxRoutingSourceArtifacts
{
    drs::engine::PlaybackSnapshotBuildResult snapshot;
    drs::engine::PlaybackActivationPayloadPtr payload;
    drs::engine::SamplerRenderModelPtr model;
    drs::engine::ImmutableDspGraphPlan graphPlan;
};

FxRoutingSourceArtifacts buildFxRoutingSourceArtifacts(
    const drs::engine::RuntimeProjectModel& project)
{
    drs::engine::PlaybackSnapshotBuilder snapshotBuilder;
    const auto snapshotRequest = snapshotBuilder.requestBuild(1, true);
    auto snapshot = snapshotBuilder.buildSnapshot(snapshotRequest, project);
    require(snapshot.built && snapshot.activationEligible,
            "FX/routing source snapshot did not build.");

    drs::engine::PreparedPlaybackService preparedService("px05-source-parity", 1, false);
    const drs::engine::RuntimeStreamLoadResult noCompiledStream;
    const auto preparedRequest = preparedService.requestBuild(snapshot, noCompiledStream);
    require(preparedRequest.accepted,
            "FX/routing source preparation request was rejected.");
    auto prepared = preparedService.prepare(preparedRequest, snapshot, noCompiledStream);
    require(prepared.built && prepared.activationEligible,
            "FX/routing source preparation failed: " + prepared.state);
    auto payload = drs::engine::buildPlaybackActivationPayload(
        drs::engine::PlaybackActivationLane::performance,
        snapshot.requestedDraftRevision,
        &snapshot,
        &prepared);
    const auto model = drs::engine::buildSamplerRenderModel(payload);
    require(model.built && model.model != nullptr,
            "FX/routing source render model did not build.");
    const auto graphPlan = drs::engine::compileDspGraphPlan(snapshot.snapshot);
    require(graphPlan.compiled,
            "FX/routing source graph plan did not compile.");
    return { std::move(snapshot), std::move(payload), model.model, graphPlan.plan };
}

bool sameGraphParameters(const drs::engine::ImmutableDspGraphPlan& left,
                         const drs::engine::ImmutableDspGraphPlan& right)
{
    if (left.parameters.size() != right.parameters.size())
        return false;
    for (std::size_t index = 0; index < left.parameters.size(); ++index)
    {
        if (left.parameters[index].id != right.parameters[index].id
            || left.parameters[index].value != right.parameters[index].value)
            return false;
    }
    return true;
}

drs::tests::OfflineRenderArtifact renderFxRoutingParityCase(
    std::string scenarioId,
    const drs::engine::SamplerRenderModelPtr& model,
    const drs::engine::ImmutableDspGraphPlan& graphPlan,
    const std::uint8_t midiNote,
    const bool withGraph = true)
{
    drs::tests::OfflineRenderRequest request;
    request.scenarioId = std::move(scenarioId);
    request.model = model;
    request.sampleRate = 48000.0;
    request.frameCount = 144000;
    request.partitionSize = 128;
    request.events = {
        { 0, drs::engine::SamplerRenderEventType::noteOn, midiNote, 0.8f },
        { 12000, drs::engine::SamplerRenderEventType::noteOff, midiNote, 0.0f }
    };
    if (withGraph)
        request.dspGraphPlan = graphPlan;
    return drs::tests::renderOffline(request);
}

ordered_json buildFxRoutingParitySection(const fs::path& scratchDirectory)
{
    auto request = buildFxRoutingParityRequest(scratchDirectory);
    const auto source = buildFxRoutingSourceArtifacts(request.project);
    const auto exported = drs::app::executePerformancePackageExport(request);
    require(exported.exported,
            "FX/routing parity package export failed: " + summarizeIssues(exported.issues));
    drs::engine::PerformancePackageV3ActivationSecurityContext activationSecurity;
    activationSecurity.compatibilityId = request.securityContext->compatibilityId;
    activationSecurity.keyProvider = request.securityContext->keyProvider;
    activationSecurity.trustStore = request.securityContext->trustStore;
    const auto package = drs::engine::loadPerformancePackageV3Metadata(
        request.packagePath, activationSecurity);
    require(package.loaded && package.package != nullptr,
            "FX/routing parity package metadata did not reopen: " + summarizeIssues(package.issues));
    auto activation = drs::engine::preparePerformancePackageV3Activation(
        package.metadata, package.package, package.contentKey, package.sampleDescriptors);
    require(activation.prepared && activation.activationPayload != nullptr
                && activation.activationPayload->snapshot != nullptr
                && activation.renderModel != nullptr,
            "FX/routing parity package did not prepare: " + summarizeIssues(activation.issues));
    const auto packagePlanResult = drs::engine::compileDspGraphPlan(
        *activation.activationPayload->snapshot);
    require(packagePlanResult.compiled,
            "FX/routing parity package graph plan did not compile.");
    const auto& packagePlan = packagePlanResult.plan;
    const auto planEquivalent = source.graphPlan.planDigest == packagePlan.planDigest
        && source.graphPlan.nodes.size() == packagePlan.nodes.size()
        && sameGraphParameters(source.graphPlan, packagePlan);
    require(planEquivalent,
            "Source and reopened package graph plans or parameter vectors differ.");
    require(package.metadata.manifest.schemaVersion
                == drs::engine::performancePackageFxRoutingSchemaVersion
            && package.metadata.manifest.minimumReaderSchemaVersion
                == drs::engine::performancePackageFxRoutingMinimumReaderSchemaVersion
            && package.metadata.instrument.instrument.schemaVersion
                == drs::engine::runtimeInstrumentFxRoutingSchemaVersion,
            "FX/routing parity package did not retain its compatibility versions.");

    drs::engine::SamplerRenderModelBuildOptions sourceModelOptions;
    sourceModelOptions.midiNoteOffset = activation.renderModel->getMidiNoteOffset();
    sourceModelOptions.fixedVelocity = activation.renderModel->getFixedVelocity();
    const auto normalizedSourceModel = drs::engine::buildSamplerRenderModel(
        source.payload, sourceModelOptions);
    require(normalizedSourceModel.built && normalizedSourceModel.model != nullptr,
            "The source parity model could not apply the package performance options.");

    const auto oldReader = drs::engine::loadPerformancePackageV3Metadata(
        request.packagePath, activationSecurity,
        drs::engine::performancePackageLegacySchemaVersion);
    const auto oldReaderRejected = !oldReader.loaded
        && oldReader.metadata.failureCategory
            == drs::engine::PerformancePackageFailureCategory::playbackCompatibilityFailure;
    require(oldReaderRejected,
            "A reader limited to schema v1 accepted a graph-bearing package.");
    const auto corpus = package_support::getCheckedInCorpusPaths();
    const auto legacyPackage = drs::engine::loadPerformancePackage(
        corpus.valid.generic_string(),
        drs::engine::getDeterministicPackageCryptoProvider(),
        drs::engine::performancePackageFxRoutingMinimumReaderSchemaVersion);
    require(legacyPackage.loaded,
            "The current reader no longer accepts the checked-in package-v1 corpus fixture.");

    const auto monoSource = renderFxRoutingParityCase(
        "px05-mono-source", normalizedSourceModel.model, source.graphPlan, 57);
    const auto monoPackage = renderFxRoutingParityCase(
        "px05-mono-package", activation.renderModel, packagePlan, 57);
    const auto monoRepeat = renderFxRoutingParityCase(
        "px05-mono-package-repeat", activation.renderModel, packagePlan, 57);
    const auto stereoSource = renderFxRoutingParityCase(
        "px05-stereo-source", normalizedSourceModel.model, source.graphPlan, 69);
    const auto stereoPackage = renderFxRoutingParityCase(
        "px05-stereo-package", activation.renderModel, packagePlan, 69);
    const auto stereoRepeat = renderFxRoutingParityCase(
        "px05-stereo-package-repeat", activation.renderModel, packagePlan, 69);
    const auto monoDry = renderFxRoutingParityCase(
        "px05-mono-dry-oracle", activation.renderModel, packagePlan, 57, false);
    const auto monoSourceDry = renderFxRoutingParityCase(
        "px05-mono-source-dry", normalizedSourceModel.model, source.graphPlan, 57, false);
    const auto stereoSourceDry = renderFxRoutingParityCase(
        "px05-stereo-source-dry", normalizedSourceModel.model, source.graphPlan, 69, false);
    const auto stereoPackageDry = renderFxRoutingParityCase(
        "px05-stereo-package-dry", activation.renderModel, packagePlan, 69, false);
    const auto monoComparison = drs::tests::compareOfflineArtifacts(monoSource, monoPackage);
    const auto monoRepeatComparison = drs::tests::compareOfflineArtifacts(monoPackage, monoRepeat);
    const auto stereoComparison = drs::tests::compareOfflineArtifacts(stereoSource, stereoPackage);
    const auto stereoRepeatComparison = drs::tests::compareOfflineArtifacts(stereoPackage, stereoRepeat);
    const auto dryComparison = drs::tests::compareOfflineArtifacts(monoPackage, monoDry);
    const auto monoSamplerComparison = drs::tests::compareOfflineArtifacts(
        monoSourceDry, monoDry);
    const auto stereoSamplerComparison = drs::tests::compareOfflineArtifacts(
        stereoSourceDry, stereoPackageDry);
    constexpr std::int64_t tailEvidenceFrame = 12000 + 24000;
    const auto tailsPresent = monoPackage.summary.lastNonZeroFrame > tailEvidenceFrame
        && stereoPackage.summary.lastNonZeroFrame > tailEvidenceFrame;
    const auto audioPassed = monoComparison.equivalent
        && monoRepeatComparison.equivalent
        && stereoComparison.equivalent
        && stereoRepeatComparison.equivalent
        && !dryComparison.equivalent
        && monoPackage.summary.peak > 1.0e-5
        && stereoPackage.summary.peak > 1.0e-5
        && tailsPresent;
    require(audioPassed,
            "Source/package DSP audio evidence failed: mono="
                + std::to_string(monoComparison.equivalent)
                + " monoRepeat=" + std::to_string(monoRepeatComparison.equivalent)
                + " stereo=" + std::to_string(stereoComparison.equivalent)
                + " stereoRepeat=" + std::to_string(stereoRepeatComparison.equivalent)
                + " dryDifferent=" + std::to_string(!dryComparison.equivalent)
                + " monoSampler=" + std::to_string(monoSamplerComparison.equivalent)
                + " stereoSampler=" + std::to_string(stereoSamplerComparison.equivalent)
                + " monoPeak=" + std::to_string(monoPackage.summary.peak)
                + " stereoPeak=" + std::to_string(stereoPackage.summary.peak)
                + " monoLast=" + std::to_string(monoPackage.summary.lastNonZeroFrame)
                + " stereoLast=" + std::to_string(stereoPackage.summary.lastNonZeroFrame)
                + " monoMismatch='" + monoComparison.message + "' channel="
                + std::to_string(monoComparison.channel) + " frame="
                + std::to_string(monoComparison.frame) + " expected="
                + std::to_string(monoComparison.expected) + " actual="
                + std::to_string(monoComparison.actual)
                + " stereoMismatch='" + stereoComparison.message + "' channel="
                + std::to_string(stereoComparison.channel) + " frame="
                + std::to_string(stereoComparison.frame) + " expected="
                + std::to_string(stereoComparison.expected) + " actual="
                + std::to_string(stereoComparison.actual)
                + " monoSamplerMismatch='" + monoSamplerComparison.message + "' frame="
                + std::to_string(monoSamplerComparison.frame)
                + " stereoSamplerMismatch='" + stereoSamplerComparison.message + "' frame="
                + std::to_string(stereoSamplerComparison.frame) + ".");

    ordered_json cases = ordered_json::array();
    const auto appendCase = [&](const char* id,
                                const drs::tests::OfflineRenderArtifact& sourceArtifact,
                                const drs::tests::OfflineRenderArtifact& packageArtifact,
                                const bool passed)
    {
        cases.push_back({
            { "id", id },
            { "sourceChecksum", sourceArtifact.summary.quantizedChecksum },
            { "packageChecksum", packageArtifact.summary.quantizedChecksum },
            { "sourcePeak", sourceArtifact.summary.peak },
            { "packagePeak", packageArtifact.summary.peak },
            { "packageLastNonZeroFrame", packageArtifact.summary.lastNonZeroFrame },
            { "passed", passed }
        });
    };
    appendCase("mono-zone-group-master", monoSource, monoPackage,
               monoComparison.equivalent && monoRepeatComparison.equivalent);
    appendCase("stereo-zone-bypassed-group-master", stereoSource, stereoPackage,
               stereoComparison.equivalent && stereoRepeatComparison.equivalent);

    ordered_json section;
    section["packagePath"] = request.packagePath;
    section["packageSchemaVersion"] = package.metadata.manifest.schemaVersion;
    section["minimumReaderSchemaVersion"]
        = package.metadata.manifest.minimumReaderSchemaVersion;
    section["runtimeInstrumentSchemaVersion"]
        = package.metadata.instrument.instrument.schemaVersion;
    section["graphDigest"] = packagePlan.authoredGraphDigest;
    section["planDigest"] = packagePlan.planDigest;
    section["nodeCount"] = packagePlan.nodes.size();
    section["parameterCount"] = packagePlan.parameters.size();
    section["authoredSlotCount"] = request.project.authoring.fxSlots.size();
    section["authoredBusCount"] = request.project.authoring.routingBuses.size();
    section["oldReaderRejected"] = oldReaderRejected;
    section["legacyPackageLoaded"] = legacyPackage.loaded;
    section["dryFallbackPrevented"] = !dryComparison.equivalent;
    section["statefulTailsPresent"] = tailsPresent;
    section["cases"] = std::move(cases);
    section["passed"] = planEquivalent && oldReaderRejected && legacyPackage.loaded && audioPassed;
    return section;
}

ordered_json buildLicenseEvidenceSection(
    const fs::path& packagePath,
    const drs::plugin::PerformancePackageExportResult& exportResult,
    const std::shared_ptr<const drs::app::PerformancePackageExportSecurityContext>& security)
{
    drs::engine::PerformancePackageV3ActivationSecurityContext activationSecurity;
    activationSecurity.compatibilityId = security->compatibilityId;
    activationSecurity.keyProvider = security->keyProvider;
    activationSecurity.trustStore = security->trustStore;
    const auto loaded = drs::engine::loadPerformancePackageV3Metadata(
        packagePath.generic_string(), activationSecurity);
    require(loaded.loaded && loaded.package != nullptr,
            "The licensed release-gate package did not reopen.");

    const auto expectedBytes = package_support::buildLicenseTextFixture();
    const auto licenseRecordCount = std::count_if(
        loaded.package->package.records.begin(), loaded.package->package.records.end(), [](const auto& record)
        {
            return record.recordKind == "license-text";
        });
    const auto firstLicenseRecord = std::find_if(
        loaded.package->package.records.begin(), loaded.package->package.records.end(), [](const auto& record)
        {
            return record.recordKind == "license-text";
        });
    require(firstLicenseRecord != loaded.package->package.records.end(),
            "The licensed release-gate package did not contain a license record.");

    const auto tamperedPath = packagePath.parent_path() / "release-gate-license-tampered.drpkg";
    require(fs::copy_file(packagePath, tamperedPath, fs::copy_options::overwrite_existing),
            "Could not copy the release-gate package for license corruption.");
    {
        std::fstream file(tamperedPath, std::ios::binary | std::ios::in | std::ios::out);
        const auto offset = firstLicenseRecord->ciphertextOffset;
        file.seekg(static_cast<std::streamoff>(offset));
        char value = 0;
        file.read(&value, 1);
        value ^= 0x01;
        file.seekp(static_cast<std::streamoff>(offset));
        file.write(&value, 1);
        require(file.good(), "Could not corrupt the authenticated license record.");
    }
    const auto tampered = drs::engine::loadPerformancePackageV3Metadata(
        tamperedPath.generic_string(), activationSecurity);
    const auto corruptionRejected = !tampered.loaded
        && tampered.failure == drs::engine::PerformancePackageV3ActivationFailure::signature;

    ordered_json section;
    section["declared"] = loaded.metadata.manifest.license.payloadId
        == drs::engine::playableInstrumentLicensePayloadId;
    section["loaded"] = loaded.metadata.licenseText.loaded;
    section["payloadId"] = loaded.metadata.licenseText.payload.payloadId;
    section["mediaType"] = loaded.metadata.licenseText.payload.mediaType;
    section["logicalPath"] = loaded.metadata.licenseText.payload.logicalPath;
    section["licenseBytes"] = loaded.metadata.licenseText.payload.plaintextBytes.size();
    section["licenseRecordCount"] = licenseRecordCount;
    section["exportedPackageBytes"] = exportResult.packageBytes;
    section["filesystemPackageBytes"] = fs::file_size(packagePath);
    section["exportedPayloadCount"] = exportResult.payloadCount;
    section["exactBytes"] = loaded.metadata.licenseText.payload.plaintextBytes == expectedBytes;
    section["schemaUnchanged"] = loaded.metadata.manifest.schemaVersion
        == drs::engine::performancePackageLegacySchemaVersion;
    section["corruptionRejected"] = corruptionRejected;
    section["corruptionFailureCategory"]
        = drs::engine::toString(tampered.failure);
    section["passed"] = section["declared"].get<bool>()
        && section["loaded"].get<bool>()
        && section["exactBytes"].get<bool>()
        && section["schemaUnchanged"].get<bool>()
        && corruptionRejected
        && licenseRecordCount == 1
        && exportResult.packageBytes == fs::file_size(packagePath);
    return section;
}

ordered_json buildReopenAndUxSection(const fs::path& scratchDirectory)
{
    juce::ScopedJuceInitialiser_GUI gui;

    const auto exportedPackagePath = scratchDirectory / "release-gate-exported.drpkg";
    const auto packageFile = juce::File(juce::String::fromUTF8(exportedPackagePath.generic_string().c_str()));
    const auto security = drs::tests::makePerformancePackageExportTestSecurityContext();
    auto runtimeSecurity
        = std::make_shared<drs::engine::PerformancePackageV3ActivationSecurityContext>();
    runtimeSecurity->compatibilityId = security->compatibilityId;
    runtimeSecurity->keyProvider = security->keyProvider;
    runtimeSecurity->trustStore = security->trustStore;

    drs::standalone::MainComponent standalone(false);
    standalone.addToDesktop(0);
    standalone.setVisible(true);
    require(standalone.getProcessor().getPerformancePackageExportService()
                .setSecurityContext(security),
            "Release gate standalone shell requires test-only V3 security provisioning.");
    require(standalone.getProcessor().setPerformancePackageActivationSecurityContext(
                runtimeSecurity),
            "Release gate standalone shell requires test-only V3 reader provisioning.");
    require(standalone.getProcessor().replaceAuthoringProject(
                buildLicensedAuthoringProjectFixture(scratchDirectory / "licensed-content")),
            "Release gate standalone shell should accept the authoring export fixture.");

    const auto standaloneExportStarted = Clock::now();
    const auto exportResult = standalone.getProcessor().exportPerformancePackage(packageFile);
    const auto standaloneExportElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        Clock::now() - standaloneExportStarted);
    require(exportResult.exported,
            "Release gate package export should succeed. state="
                + exportResult.state + " issues=" + summarizeIssues(exportResult.issues));
    const auto licenseEvidence = buildLicenseEvidenceSection(
        exportedPackagePath, exportResult, security);

    const auto standaloneOpenStarted = Clock::now();
    const auto standaloneLoad = standalone.getProcessor().loadPerformancePackageWorkspace(packageFile);
    const auto standaloneOpenElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        Clock::now() - standaloneOpenStarted);
    require(standaloneLoad.loaded,
            "Release gate standalone reopen should succeed. state="
                + standaloneLoad.state + " issues=" + summarizeIssues(standaloneLoad.issues));
    const auto standaloneLicense
        = standalone.getEngineFacade().getPerformancePackageLicenseText();
    const auto standaloneViewLicenseAvailable = drs::app::shouldShowViewLicenseMenuItem(
        true, standaloneLicense != nullptr);
    const auto standaloneViewerValid = validateLicenseViewer(standaloneLicense);

    standalone.resized();
    standalone.getProcessor().prepareToPlay(44100.0, 512);
    standalone.getProcessor().serviceMessageThreadWork();
    const auto standaloneMagnitude = renderQueuedPerformanceSurfaceMagnitude(standalone.getProcessor(), 69, 0.8f);
    const auto standaloneRealtime = standalone.getProcessor().getRealtimeSafetySnapshot();
    auto* standaloneTabs = dynamic_cast<juce::TabbedComponent*>(findDescendantById(standalone, "workspaceTabs"));
    const bool standaloneProjectBindingSuppressed
        = standalone.getProcessor().waitForHostStatePublication()
        && hasEmptyProjectBindingAndPackageBinding(standalone.exportStateJson());

    drs::plugin::Processor processor;
    require(processor.setPerformancePackageActivationSecurityContext(runtimeSecurity),
            "Release gate plugin requires test-only V3 security provisioning.");
    std::unique_ptr<juce::AudioProcessorEditor> editor(processor.createEditor());
    require(editor != nullptr, "Release gate plugin editor should construct.");
    editor->addToDesktop(0);
    editor->setVisible(true);

    const auto pluginOpenStarted = Clock::now();
    const auto pluginLoad = processor.loadPerformancePackageWorkspace(packageFile);
    const auto pluginOpenElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        Clock::now() - pluginOpenStarted);
    require(pluginLoad.loaded,
            "Release gate plugin reopen should succeed. state="
                + pluginLoad.state + " issues=" + summarizeIssues(pluginLoad.issues));
    const auto pluginLicense = processor.getEngineFacade().getPerformancePackageLicenseText();
    const auto pluginViewLicenseAvailable = drs::app::shouldShowViewLicenseMenuItem(
        true, pluginLicense != nullptr);
    const auto pluginViewerValid = validateLicenseViewer(pluginLicense);
    editor.reset();
    editor.reset(processor.createEditor());
    require(editor != nullptr
                && processor.getEngineFacade().getPerformancePackageLicenseText() == pluginLicense,
            "Release gate plugin editor reopen must preserve immutable license ownership.");

    editor->resized();
    processor.prepareToPlay(44100.0, 512);
    processor.serviceMessageThreadWork();
    const auto pluginMagnitude = renderQueuedPerformanceSurfaceMagnitude(processor, 69, 0.8f);
    const auto pluginRealtime = processor.getRealtimeSafetySnapshot();
    auto* pluginTabs = dynamic_cast<juce::TabbedComponent*>(findDescendantById(*editor, "workspaceTabs"));
    juce::MemoryBlock pluginState;
    require(processor.waitForHostStatePublication(),
            "Release-gate package state did not reach background host-state publication.");
    processor.getStateInformation(pluginState);
    const std::string serializedPluginState(static_cast<const char*>(pluginState.getData()), pluginState.getSize());
    const bool pluginProjectBindingSuppressed
        = hasEmptyProjectBindingAndPackageBinding(serializedPluginState);

    ordered_json section;
    section["exportedPackagePath"] = exportedPackagePath.generic_string();
    section["exportedPackageBytes"] = exportResult.packageBytes;
    section["payloadCount"] = exportResult.payloadCount;
    section["license"] = licenseEvidence;
    section["standaloneExportMs"] = standaloneExportElapsed.count();
    section["standaloneOpenMs"] = standaloneOpenElapsed.count();
    section["standaloneAudibleMagnitude"] = standaloneMagnitude;
    section["standalonePerformanceOnly"] = standalone.getProcessor().getWorkspaceDocumentState().workspaceMode
        == drs::engine::WorkspaceMode::performanceOnly;
    section["standaloneAuthoringSuppressed"] = !standalone.getProcessor().getWorkspaceDocumentState().authoringAvailable;
    section["standaloneTabCount"] = standaloneTabs != nullptr ? standaloneTabs->getNumTabs() : 0;
    section["standaloneHasAuthoringZoneSelector"] = findDescendantById(standalone, "authoringZoneSelector") != nullptr;
    section["standaloneProjectBindingSuppressed"] = standaloneProjectBindingSuppressed;
    section["standaloneViewLicenseAvailable"] = standaloneViewLicenseAvailable;
    section["standaloneViewerValid"] = standaloneViewerValid;
    section["standaloneRealtimeViolations"]
        = standaloneRealtime.getAudioThreadViolationCount();
    section["pluginOpenMs"] = pluginOpenElapsed.count();
    section["pluginAudibleMagnitude"] = pluginMagnitude;
    section["pluginPerformanceOnly"] = processor.getWorkspaceDocumentState().workspaceMode
        == drs::engine::WorkspaceMode::performanceOnly;
    section["pluginAuthoringSuppressed"] = !processor.getWorkspaceDocumentState().authoringAvailable;
    section["pluginTabCount"] = pluginTabs != nullptr ? pluginTabs->getNumTabs() : 0;
    section["pluginHasAuthoringZoneSelector"] = findDescendantById(*editor, "authoringZoneSelector") != nullptr;
    section["pluginProjectBindingSuppressed"] = pluginProjectBindingSuppressed;
    section["pluginViewLicenseAvailable"] = pluginViewLicenseAvailable;
    section["pluginViewerValid"] = pluginViewerValid;
    section["pluginEditorReopenRetainedLicense"]
        = processor.getEngineFacade().getPerformancePackageLicenseText() == pluginLicense;
    section["pluginRealtimeViolations"] = pluginRealtime.getAudioThreadViolationCount();
    section["passed"] = standaloneMagnitude > 0.0001f
        && pluginMagnitude > 0.0001f
        && section["standalonePerformanceOnly"].get<bool>()
        && section["pluginPerformanceOnly"].get<bool>()
        && section["standaloneAuthoringSuppressed"].get<bool>()
        && section["pluginAuthoringSuppressed"].get<bool>()
        && section["standaloneTabCount"].get<int>() == 1
        && section["pluginTabCount"].get<int>() == 1
        && !section["standaloneHasAuthoringZoneSelector"].get<bool>()
        && !section["pluginHasAuthoringZoneSelector"].get<bool>()
        && standaloneProjectBindingSuppressed
        && pluginProjectBindingSuppressed
        && licenseEvidence.at("passed").get<bool>()
        && standaloneViewLicenseAvailable
        && pluginViewLicenseAvailable
        && standaloneViewerValid
        && pluginViewerValid
        && standaloneRealtime.getAudioThreadViolationCount() == 0
        && pluginRealtime.getAudioThreadViolationCount() == 0;
    return section;
}

std::string currentIsoDate()
{
    const auto now = std::chrono::system_clock::now();
    const auto currentTime = std::chrono::system_clock::to_time_t(now);
    std::tm localTime {};
    localtime_s(&localTime, &currentTime);

    char buffer[11] {};
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d", &localTime);
    return buffer;
}
} // namespace

int main(int argc, char* argv[])
{
    try
    {
        const auto outputPath = argc > 1
            ? fs::path(argv[1])
            : fs::temp_directory_path() / "drs-phase1-performance-package-release-gate.json";
        const auto scratchDirectory = fs::temp_directory_path() / "drs-phase1-performance-package-release-gate";
        std::error_code errorCode;
        fs::remove_all(scratchDirectory, errorCode);
        fs::create_directories(scratchDirectory);

        ordered_json report;
        report["schemaName"] = "drs.performancePackageReleaseGate";
        report["schemaVersion"] = 1;
        report["capturedOn"] = currentIsoDate();
        std::cout << "Release gate: compatibility policy" << std::endl;
        report["compatibilityPolicy"] = buildCompatibilityPolicySection();
        std::cout << "Release gate: deterministic package bytes" << std::endl;
        report["determinism"] = buildDeterminismSection(scratchDirectory / "determinism");
        std::cout << "Release gate: failure reporting" << std::endl;
        report["failureReporting"] = buildFailureReportingSection();
        std::cout << "Release gate: menu cutover" << std::endl;
        report["menuCutover"] = buildMenuCutoverSection();
        std::cout << "Release gate: reopen and performance-only UX" << std::endl;
        report["reopenAndPerformanceOnlyUx"] = buildReopenAndUxSection(scratchDirectory / "reopen");
        std::cout << "Release gate: FX-routing parity" << std::endl;
        report["fxRoutingParity"] = buildFxRoutingParitySection(
            scratchDirectory / "fx-routing-parity");
        report["passed"] = report["compatibilityPolicy"].at("passed").get<bool>()
            && report["determinism"].at("passed").get<bool>()
            && report["failureReporting"].at("passed").get<bool>()
            && report["menuCutover"].at("passed").get<bool>()
            && report["fxRoutingParity"].at("passed").get<bool>()
            && report["reopenAndPerformanceOnlyUx"].at("passed").get<bool>();

        writeTextFile(outputPath, report.dump(2) + "\n");
        require(report["passed"].get<bool>(),
                "Performance package release gate did not pass every section. See "
                    + outputPath.generic_string());
        std::cout << "Performance package release gate passed. Artifact: "
                  << outputPath.generic_string() << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Performance package release gate failed: " << exception.what() << std::endl;
        return 1;
    }
}
