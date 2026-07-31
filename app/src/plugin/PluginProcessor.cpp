#include "plugin/PluginProcessor.h"
#include "plugin/PluginEditor.h"

#include "drs/engine/AuthoringPreviewPreparation.h"
#include "drs/engine/CuratedDspCatalog.h"
#include "drs/engine/DspGraphPlan.h"
#include "drs/engine/DspRenderGeneration.h"
#include "drs/engine/RuntimeLoader.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <limits>
#include <optional>
#include <sstream>
#include <string_view>
#include <utility>

namespace drs::plugin
{
namespace
{
namespace fs = std::filesystem;

static_assert(std::atomic<std::uint64_t>::is_always_lock_free,
              "Realtime diagnostics require lock-free 64-bit atomics.");
static_assert(std::atomic<std::size_t>::is_always_lock_free,
              "Realtime diagnostics require lock-free size atomics.");
static_assert(std::atomic<int>::is_always_lock_free,
              "Realtime activation handoff requires lock-free index atomics.");
static_assert(std::atomic<std::uint32_t>::is_always_lock_free,
              "Realtime event queues require lock-free 32-bit atomics.");
static_assert(std::atomic<RealtimeGuardOperation>::is_always_lock_free,
              "Realtime test injection requires a lock-free operation atomic.");

void updateAtomicMaximum(std::atomic<std::uint64_t>& destination, std::uint64_t value)
{
    auto current = destination.load(std::memory_order_relaxed);
    while (current < value
           && !destination.compare_exchange_weak(current,
                                                 value,
                                                 std::memory_order_release,
                                                 std::memory_order_relaxed))
    {
    }
}

bool requestsExecutableCuratedDsp(const drs::engine::ImmutablePlaybackSnapshot& snapshot)
{
    return std::any_of(snapshot.fxSlots.begin(), snapshot.fxSlots.end(), [](const auto& slot)
    {
        return !slot.bypassed && !slot.unavailable && !slot.legacyInert
            && drs::engine::findCuratedDspEffect(slot.effectType, slot.effectVersion) != nullptr;
    });
}

std::optional<double> findMacroValue(const drs::engine::RuntimeSessionStateSnapshot& sessionState,
                                     std::string_view macroId)
{
    const auto iterator = std::find_if(sessionState.macroValues.begin(),
                                       sessionState.macroValues.end(),
                                       [&](const auto& macroValue)
                                       {
                                           return macroValue.id.size() == macroId.size()
                                               && std::equal(macroValue.id.begin(), macroValue.id.end(), macroId.begin());
                                       });
    if (iterator == sessionState.macroValues.end())
        return std::nullopt;

    return iterator->value;
}

std::string runtimeMacroIdFromHostParameterId(std::string_view hostParameterId)
{
    constexpr std::string_view prefix { "macro." };
    return hostParameterId.rfind(prefix, 0) == 0
        ? std::string(hostParameterId.substr(prefix.size()))
        : std::string(hostParameterId);
}

std::optional<juce::String> findPublishedHostParameterId(
    const drs::engine::ImmutablePublishedMacroBindingTablePtr& bindings,
    std::string_view macroId)
{
    if (bindings == nullptr)
        return std::nullopt;

    const auto iterator = std::find_if(bindings->bindings.begin(),
                                       bindings->bindings.end(),
                                       [&](const auto& binding)
                                       {
                                           return binding.assigned
                                               && (binding.stableAuthoredId == macroId
                                                   || runtimeMacroIdFromHostParameterId(binding.hostParameterId)
                                                       == macroId);
                                       });
    if (iterator == bindings->bindings.end())
        return std::nullopt;

    return juce::String::fromUTF8(iterator->hostParameterId.c_str());
}

double mapPublishedDspMacroValue(const drs::engine::PublishedMacroCallbackSlot& slot,
                                 const double value) noexcept
{
    const auto sourceSpan = slot.sourceMaximum - slot.sourceMinimum;
    if (!(sourceSpan > 0.0))
        return slot.destinationMinimum;

    const auto normalized = std::clamp((value - slot.sourceMinimum) / sourceSpan, 0.0, 1.0);
    if (slot.curve == drs::engine::PublishedMacroCurve::logarithmic
        && slot.destinationMinimum > 0.0 && slot.destinationMaximum > 0.0)
    {
        return slot.destinationMinimum * std::pow(
            slot.destinationMaximum / slot.destinationMinimum, normalized);
    }
    return slot.destinationMinimum
        + (slot.destinationMaximum - slot.destinationMinimum) * normalized;
}

int clampMidiValue(int value)
{
    return std::clamp(value, 0, 127);
}

std::uint64_t monotonicMicros()
{
    return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count());
}

bool isWavImportItemTerminal(const drs::app::WavImportItemStage stage) noexcept
{
    using Stage = drs::app::WavImportItemStage;
    return stage == Stage::ready
        || stage == Stage::failed
        || stage == Stage::canceled
        || stage == Stage::skipped;
}

std::string mapWavImportResponsivenessState(const drs::app::WavImportBatchSnapshot& snapshot)
{
    using BatchStage = drs::app::WavImportBatchStage;
    using Disposition = drs::app::WavImportTerminalDisposition;

    switch (snapshot.stage)
    {
        case BatchStage::queued:
        case BatchStage::staging:
        case BatchStage::inspecting:
            return "active";
        case BatchStage::canceled:
        case BatchStage::superseded:
            return "canceled";
        case BatchStage::failed:
            return "failed";
        case BatchStage::completed:
        case BatchStage::consumed:
            switch (snapshot.terminalDisposition)
            {
                case Disposition::canceled:
                case Disposition::superseded:
                    return "canceled";
                case Disposition::failed:
                    return "failed";
                case Disposition::partiallyCompleted:
                    return "completed-partial";
                case Disposition::completed:
                case Disposition::consumed:
                case Disposition::none:
                    return "completed";
            }
            break;
        case BatchStage::idle:
            break;
    }

    return "idle";
}

bool isRelevantWavImportSnapshot(const drs::app::WavImportBatchSnapshot& snapshot,
                                 const drs::engine::RuntimeProjectModel& project) noexcept
{
    if (snapshot.identity.generation == 0 || snapshot.stage == drs::app::WavImportBatchStage::idle)
        return false;
    if (!project.contentRootPath.empty() && !snapshot.identity.contentRootPath.empty()
        && snapshot.identity.contentRootPath != project.contentRootPath)
    {
        return false;
    }
    return true;
}

drs::app::AuthoringImportResponsivenessSnapshot buildImportResponsivenessSnapshotFromWavImport(
    const drs::app::WavImportBatchSnapshot& snapshot)
{
    drs::app::AuthoringImportResponsivenessSnapshot responsiveness;
    responsiveness.available = true;
    responsiveness.state = mapWavImportResponsivenessState(snapshot);
    responsiveness.totalItemCount = snapshot.totalItemCount;
    responsiveness.pendingCount = snapshot.totalItemCount > snapshot.completedItemCount
        ? snapshot.totalItemCount - snapshot.completedItemCount
        : 0;
    responsiveness.processedCount = snapshot.completedItemCount;
    responsiveness.warningItemCount = snapshot.warningItemCount;
    responsiveness.failedItemCount = snapshot.failedItemCount;
    responsiveness.canceledItemCount = snapshot.canceledItemCount;
    responsiveness.acceptedItemCount = snapshot.successfulItemCount;

    std::uint64_t totalCompletedDurationMicros = 0;
    std::size_t completedDurationCount = 0;
    for (const auto& item : snapshot.items)
    {
        if (!isWavImportItemTerminal(item.stage))
            continue;

        responsiveness.lastProcessedItemId = item.itemId;
        responsiveness.lastProcessDurationMicros = item.totalDurationMicros;
        responsiveness.maxProcessDurationMicros = std::max(responsiveness.maxProcessDurationMicros,
                                                           item.totalDurationMicros);
        totalCompletedDurationMicros += item.totalDurationMicros;
        ++completedDurationCount;
    }

    if (completedDurationCount > 0)
    {
        responsiveness.averageProcessDurationMicros
            = totalCompletedDurationMicros / completedDurationCount;
    }

    return responsiveness;
}

bool isRelevantProjectSourceValidationSnapshot(
    const drs::app::ProjectSourceValidationSnapshot& snapshot,
    const drs::engine::RuntimeProjectModel& project) noexcept
{
    if (snapshot.identity.generation == 0
        || snapshot.stage == drs::app::ProjectSourceValidationStage::idle)
    {
        return false;
    }

    if (!project.projectId.empty() && snapshot.identity.projectId != project.projectId)
        return false;

    if (!project.contentRootPath.empty() && !snapshot.identity.contentRootPath.empty()
        && snapshot.identity.contentRootPath != project.contentRootPath)
    {
        return false;
    }

    return true;
}

std::string mapProjectSourceValidationState(const drs::app::ProjectSourceValidationStage stage)
{
    using Stage = drs::app::ProjectSourceValidationStage;
    switch (stage)
    {
        case Stage::queued:
        case Stage::fingerprinting:
        case Stage::inspecting:
            return "active";
        case Stage::completed:
            return "completed";
        case Stage::canceled:
            return "canceled";
        case Stage::failed:
            return "failed";
        case Stage::idle:
            break;
    }

    return "idle";
}

drs::app::AuthoringSourceValidationSnapshot buildAuthoringSourceValidationSnapshot(
    const drs::app::ProjectSourceValidationSnapshot& snapshot)
{
    drs::app::AuthoringSourceValidationSnapshot validation;
    validation.available = true;
    validation.state = mapProjectSourceValidationState(snapshot.stage);
    validation.totalItemCount = snapshot.totalItemCount;
    validation.processedCount = snapshot.completedItemCount;
    validation.warningItemCount = snapshot.warningItemCount;
    validation.failedItemCount = snapshot.failedItemCount;
    validation.canceledItemCount = snapshot.canceledItemCount;
    validation.totalBytesProcessed = snapshot.totalBytesProcessed;
    validation.totalBytesExpected = snapshot.totalBytesExpected;
    validation.totalDurationMicros = snapshot.totalDurationMicros;
    validation.currentSourceId = snapshot.currentSourceId;
    validation.currentSourcePath = snapshot.currentSourcePath;
    return validation;
}

bool isWaveformPreviewServiceActiveStage(const drs::app::WaveformPreviewServiceStage stage) noexcept
{
    using Stage = drs::app::WaveformPreviewServiceStage;
    return stage == Stage::queued || stage == Stage::building;
}

std::string mapWaveformPreviewServiceState(const drs::app::WaveformPreviewServiceStage stage)
{
    using Stage = drs::app::WaveformPreviewServiceStage;
    switch (stage)
    {
        case Stage::queued:
        case Stage::building:
            return "Loading";
        case Stage::completed:
            return "Ready";
        case Stage::canceled:
            return "Canceled";
        case Stage::superseded:
            return "Stale";
        case Stage::failed:
            return "Unavailable";
        case Stage::idle:
            break;
    }

    return "Idle";
}

struct HostTransportObservation
{
    bool valid = false;
    bool isPlaying = false;
    bool hasTempo = false;
    bool hasTimeInSamples = false;
    bool hasTimeSignature = false;
    std::int64_t timeInSamples = 0;
    std::int32_t timeSignatureNumerator = 4;
    std::int32_t timeSignatureDenominator = 4;
    double tempoBpm = 120.0;
};

HostTransportObservation readHostTransportObservation(juce::AudioPlayHead* playHead)
{
    HostTransportObservation observation;
    if (playHead == nullptr)
        return observation;

    const auto position = playHead->getPosition();
    if (!position.hasValue())
        return observation;

    observation.valid = true;
    observation.isPlaying = position->getIsPlaying();
    if (const auto bpm = position->getBpm(); bpm && std::isfinite(*bpm)
        && *bpm >= 20.0 && *bpm <= 300.0)
    {
        observation.tempoBpm = *bpm;
        observation.hasTempo = true;
    }
    if (const auto timeSignature = position->getTimeSignature(); timeSignature
        && timeSignature->numerator > 0 && timeSignature->denominator > 0)
    {
        observation.timeSignatureNumerator = timeSignature->numerator;
        observation.timeSignatureDenominator = timeSignature->denominator;
        observation.hasTimeSignature = true;
    }
    if (const auto timeInSamples = position->getTimeInSamples())
    {
        observation.hasTimeInSamples = true;
        observation.timeInSamples = *timeInSamples;
    }

    return observation;
}

template <typename Finding>
drs::engine::AuthoringPreviewFailureFinding makePreviewFailureFinding(
    const Finding& finding)
{
    return drs::engine::classifyAuthoringPreviewFailure(
        finding.code, finding.path, finding.message);
}

drs::engine::AuthoringPreviewInvalidationCategory classifyPreviewInvalidation(
    const std::string& changeLabel,
    bool selectionChanged)
{
    using Category = drs::engine::AuthoringPreviewInvalidationCategory;
    if (selectionChanged)
        return Category::selection;

    auto normalized = changeLabel;
    std::transform(normalized.begin(), normalized.end(), normalized.begin(),
                   [](unsigned char character) { return static_cast<char>(std::tolower(character)); });
    if (normalized.find("gain") != std::string::npos)
        return Category::gain;
    if (normalized.find("pan") != std::string::npos)
        return Category::pan;
    if (normalized.find("root") != std::string::npos)
        return Category::rootKey;
    if (normalized.find("velocity") != std::string::npos)
        return Category::velocityRange;
    if (normalized.find("start") != std::string::npos)
        return Category::sampleStartOffset;
    if (normalized.find("loop") != std::string::npos)
        return Category::loop;
    if (normalized.find("source") != std::string::npos
        || normalized.find("sample") != std::string::npos)
        return Category::sourceAssignment;
    if (normalized.find("bound") != std::string::npos
        || normalized.find("key range") != std::string::npos)
        return Category::keyBounds;
    if (normalized.find("map") != std::string::npos
        || normalized.find("zone") != std::string::npos)
        return Category::mapping;
    return Category::authoredTopology;
}

std::optional<drs::engine::RuntimeProjectSampleSource> findProjectSampleSource(
    const drs::engine::RuntimeProjectModel& project,
    const std::string& sampleSourceId);

std::optional<drs::engine::RuntimeProjectZoneDefinition> findProjectZone(
    const drs::engine::RuntimeProjectModel& project,
    const std::string& zoneId)
{
    const auto iterator = std::find_if(project.authoring.zones.begin(),
                                       project.authoring.zones.end(),
                                       [&](const drs::engine::RuntimeProjectZoneDefinition& zone)
                                       {
                                           return zone.id == zoneId;
                                       });
    if (iterator == project.authoring.zones.end())
        return std::nullopt;

    return *iterator;
}

std::string buildSelectedZonePreviewFingerprint(
    const drs::engine::RuntimeProjectModel& project,
    const std::optional<drs::engine::RuntimeProjectZoneDefinition>& selectedZone)
{
    if (!selectedZone.has_value())
        return "no-selection";
    const auto& zone = *selectedZone;
    const auto sampleSource = findProjectSampleSource(project, zone.sampleSourceId);
    const auto sourcePath = sampleSource.has_value() ? sampleSource->path : std::string {};
    return zone.sampleSourceId + "|" + sourcePath + "|" + std::to_string(zone.rootKey)
        + "|" + std::to_string(zone.keyLow) + "|" + std::to_string(zone.keyHigh)
        + "|" + std::to_string(zone.velocityLow) + "|" + std::to_string(zone.velocityHigh)
        + "|" + std::to_string(zone.velocityCrossfade.fadeInLowVelocity)
        + "|" + std::to_string(zone.velocityCrossfade.fadeInHighVelocity)
        + "|" + std::to_string(zone.velocityCrossfade.fadeOutLowVelocity)
        + "|" + std::to_string(zone.velocityCrossfade.fadeOutHighVelocity)
        + "|" + std::to_string(zone.gainDb) + "|" + std::to_string(zone.pan)
        + "|" + std::to_string(zone.sampleStartFrame)
        + "|" + std::to_string(zone.loopEnabled)
        + "|" + std::to_string(static_cast<int>(zone.triggerMode))
        + "|" + std::to_string(zone.loopStartFrame)
        + "|" + std::to_string(zone.loopEndFrame);
}

std::string buildCurrentDraftPreviewFingerprint(const drs::engine::RuntimeProjectModel& project)
{
    constexpr std::uint64_t offsetBasis = 1469598103934665603ull;
    constexpr std::uint64_t prime = 1099511628211ull;
    auto hash = offsetBasis;
    for (const auto byte : drs::engine::serializeRuntimeProjectManifest(project, {}))
    {
        hash ^= static_cast<unsigned char>(byte);
        hash *= prime;
    }

    constexpr char digits[] = "0123456789abcdef";
    std::string result(16, '0');
    for (auto index = 0; index < 16; ++index)
    {
        result[15 - index] = digits[hash & 0x0f];
        hash >>= 4;
    }
    return result;
}

std::string buildSelectedGroupPreviewFingerprint(
    const drs::engine::RuntimeProjectModel& project,
    const std::optional<drs::engine::RuntimeProjectGroupDefinition>& selectedGroup)
{
    if (!selectedGroup.has_value())
        return "no-group";

    return selectedGroup->id + "|" + buildCurrentDraftPreviewFingerprint(project);
}

int computeToneRenderVelocity(const drs::engine::RuntimeSessionStateSnapshot& sessionState)
{
    const auto toneValue = findMacroValue(sessionState, "tone").value_or(0.35);
    const auto effectiveVelocity = static_cast<int>(std::lround(32.0 + toneValue * 95.0));
    return std::clamp(effectiveVelocity, 1, 127);
}

int computeMotionRenderNote(const drs::engine::RuntimeSessionStateSnapshot& sessionState, int playedNote)
{
    const auto motionValue = findMacroValue(sessionState, "motion").value_or(0.15);
    const auto semitoneOffset = static_cast<int>(std::lround((motionValue - 0.5) * 24.0));
    return clampMidiValue(playedNote + semitoneOffset);
}

drs::engine::RuntimeProjectModel buildInitialAuthoringProject()
{
    drs::engine::RuntimeProjectModel project;
    project.schemaName = "drs.project";
    project.schemaVersion = 5;
    project.displayName = "No Project Loaded";
    project.authoring.schemaName = "drs.authoring";
    project.authoring.schemaVersion = 4;
    project.authoring.notes = { "Open a project or create a new one to begin authoring." };
    project.notes = { "This session starts without loading the checked-in reference project." };
    return project;
}

std::optional<drs::engine::RuntimeProjectSampleSource> findProjectSampleSource(const drs::engine::RuntimeProjectModel& project,
                                                                               const std::string& sampleSourceId)
{
    const auto iterator = std::find_if(project.sampleSources.begin(),
                                       project.sampleSources.end(),
                                       [&](const drs::engine::RuntimeProjectSampleSource& sampleSource)
                                       {
                                           return sampleSource.id == sampleSourceId;
                                       });
    if (iterator == project.sampleSources.end())
        return std::nullopt;

    return *iterator;
}

struct AuthoringPreviewBlockingHint
{
    std::string prerequisite;
    std::string guidance;
};

std::string describeZoneLabel(const drs::engine::RuntimeProjectZoneDefinition& zone)
{
    return zone.displayName.empty() ? zone.id : zone.displayName;
}

std::string describeSampleFileLabel(const std::string& samplePath)
{
    if (samplePath.empty())
        return "the selected sample file";

    const auto fileName = fs::path(samplePath).filename().generic_string();
    return fileName.empty() ? samplePath : fileName;
}

AuthoringPreviewBlockingHint buildAuthoringPreviewBlockingHint(const drs::engine::AuthoringSession& authoringSession,
                                                              const std::string& failureState)
{
    if (failureState.empty())
        return {};

    const auto selectedZone = authoringSession.getSelectedZone();
    if (!selectedZone.has_value())
    {
        return { "Select a zone to audition.",
                 "Choose a zone in the map or Selected Zone list before auditioning the authoring preview." };
    }

    const auto zoneLabel = describeZoneLabel(*selectedZone);
    const auto sampleSource = findProjectSampleSource(authoringSession.getProject(), selectedZone->sampleSourceId);
    if (!sampleSource.has_value())
    {
        return { "Assign a sample source to the selected zone.",
                 "Zone '" + zoneLabel + "' points to a sample source that is no longer in the project. Reassign it to an imported sample, then audition again." };
    }

    if (sampleSource->path.empty())
    {
        return { "Choose a sample file for the selected zone.",
                 "Sample source '" + sampleSource->id + "' does not have an audio file path yet. Import or relink a source file for zone '" + zoneLabel + "'." };
    }

    const auto sampleFileLabel = describeSampleFileLabel(sampleSource->path);
    if (failureState == "Sample missing"
        || failureState.find("missing-sample-source-asset") != std::string::npos
        || failureState.find("not found") != std::string::npos
        || failureState.find("does not exist") != std::string::npos)
    {
        return { "Relink or re-import the selected sample file.",
                 "Restore or replace '" + sampleFileLabel + "' for zone '" + zoneLabel + "', then prepare the authoring preview again." };
    }

    if (failureState == "Sample format unsupported"
        || failureState.find("supported audio format") != std::string::npos
        || failureState.find("WAV and FLAC") != std::string::npos)
    {
        return { "Convert the selected sample to a supported format.",
                 "Zone '" + zoneLabel + "' is pointing at '" + sampleFileLabel + "'. Convert it to a supported WAV, AIFF, or FLAC file, then audition again." };
    }

    if (failureState.find("44100 Hz and 48000 Hz") != std::string::npos)
    {
        return { "Use a 44.1 kHz or 48 kHz sample for this zone.",
                 "Resample '" + sampleFileLabel + "' to 44.1 kHz or 48 kHz so zone '" + zoneLabel + "' can be prepared for preview." };
    }

    if (failureState == "Sample channel count invalid"
        || failureState.find("mono and stereo") != std::string::npos)
    {
        return { "Use a mono or stereo sample for this zone.",
                 "Replace '" + sampleFileLabel + "' with a mono or stereo file so zone '" + zoneLabel + "' can be prepared for preview." };
    }

    if (failureState == "Sample too large")
    {
        return { "Replace the selected sample with a smaller source file.",
                 "The current file '" + sampleFileLabel + "' exceeds the importer limit. Choose a smaller source for zone '" + zoneLabel + "' and prepare preview again." };
    }

    if (failureState == "Sample read failed" || failureState == "Sample length invalid")
    {
        return { "Repair or replace the selected sample file.",
                 "The source file '" + sampleFileLabel + "' could not be read cleanly for zone '" + zoneLabel + "'. Repair the file or replace it before previewing again." };
    }

    return { "Repair the selected sample source and prepare preview again.",
             "Zone '" + zoneLabel + "' still has a blocking sample-source problem. Recheck '" + sampleFileLabel + "', then audition the preview again." };
}

} // namespace

bool Processor::RealtimeNoteEventQueue::push(QueuedRealtimeNoteEvent event) noexcept
{
    const auto write = writeIndex.load(std::memory_order_relaxed);
    const auto next = (write + 1u) % storageCapacity;
    if (next == readIndex.load(std::memory_order_acquire))
        return false;

    events[write] = event;
    writeIndex.store(next, std::memory_order_release);
    return true;
}

bool Processor::RealtimeNoteEventQueue::pop(QueuedRealtimeNoteEvent& event) noexcept
{
    const auto read = readIndex.load(std::memory_order_relaxed);
    if (read == writeIndex.load(std::memory_order_acquire))
        return false;

    event = events[read];
    readIndex.store((read + 1u) % storageCapacity, std::memory_order_release);
    return true;
}

void Processor::RealtimeNoteEventQueue::reset() noexcept
{
    readIndex.store(0, std::memory_order_release);
    writeIndex.store(0, std::memory_order_release);
}

Processor::Processor()
    : Processor(drs::app::WaveformPreviewServiceOptions {})
{
}

Processor::Processor(drs::app::WaveformPreviewServiceOptions waveformPreviewServiceOptions)
    : juce::AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      authoringSession(buildInitialAuthoringProject()),
      parameterState(*this, nullptr, "macroParameters", buildParameterLayout(engineFacade)),
      waveformPreviewService(std::move(waveformPreviewServiceOptions))
{
    primeRealtimeSafetyState(512);
    initializeAuthoringImportMetrics();
    initializeAuthoringSourceValidationSnapshot();
    authoringSession.setDspParameterGesturePreviewListener(
        [this](const std::string& slotId, const std::string& parameterId, const double value)
        {
            authoringPreviewPlaybackContext.publishDspControlByIdentity(slotId, parameterId, value);
        });

    for (const auto& macro : engineFacade.getMacroDescriptors())
        parameterState.addParameterListener(buildMacroParameterId(macro.id), this);

    initializePublishedMacroRealtimeState();
    syncParametersFromEngine();
    serviceMessageThreadWork();
    refreshSerializedHostStatePublication(true);
    updateRealtimeSafetyState();
}

Processor::~Processor()
{
    stopTimer();
    sfzImportReviewService.shutdown();
    wavImportService.shutdown();
    projectSourceValidationService.shutdown();
    waveformPreviewService.shutdown();
    projectRestoreCoordinator.shutdown();

    for (const auto& macro : engineFacade.getMacroDescriptors())
        parameterState.removeParameterListener(buildMacroParameterId(macro.id), this);

    performancePlaybackContext.closeAtBlockBoundary();
    authoringPreviewPlaybackContext.closeAtBlockBoundary();
    performancePlaybackContext.serviceRetirements();
    authoringPreviewPlaybackContext.serviceRetirements();
    delete[] realtimeGuardTestAllocation;
}

void Processor::timerCallback()
{
    serviceMessageThreadWork();

    const auto restore = projectRestoreCoordinator.getSnapshot();
    if (restore == nullptr
        || restore->state == drs::engine::ProjectRestoreState::idle
        || restore->state == drs::engine::ProjectRestoreState::needsLocation
        || restore->state == drs::engine::ProjectRestoreState::ready
        || restore->state == drs::engine::ProjectRestoreState::active
        || restore->state == drs::engine::ProjectRestoreState::degraded
        || restore->state == drs::engine::ProjectRestoreState::failed)
    {
        stopTimer();
    }
}

void Processor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate > 0.0 ? sampleRate : 44100.0;
    performanceSurfaceNoteQueue.reset();
    authoringPreviewNoteQueue.reset();
    hasPreviousHostTransportObservation = false;
    previousHostTransportWasPlaying = false;
    previousHostTransportHadTimeInSamples = false;
    previousHostTransportTimeInSamples = 0;
    previousHostTransportBlockSize = 0;
    authoringPreviewCommandAdapter.clearOwnership();
    performancePlaybackContext.prepare(currentSampleRate);
    authoringPreviewPlaybackContext.prepare(currentSampleRate);
    primeRealtimeSafetyState(samplesPerBlock > 0 ? samplesPerBlock : 512);
    serviceMessageThreadWork();
    updateRealtimeSafetyState();
}

void Processor::releaseResources()
{
    authoringPreviewCommandAdapter.clearOwnership();
    hasPreviousHostTransportObservation = false;
    previousHostTransportWasPlaying = false;
    previousHostTransportHadTimeInSamples = false;
    previousHostTransportTimeInSamples = 0;
    previousHostTransportBlockSize = 0;
    performancePlaybackContext.resetAtBlockBoundary();
    authoringPreviewPlaybackContext.resetAtBlockBoundary();
    updateRealtimeSafetyState();
}

bool Processor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::mono()
        || layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void Processor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    const auto blockStartTime = std::chrono::steady_clock::now();
    juce::ScopedNoDenormals noDenormals;
    const ScopedRealtimeAudioThread audioCallbackScope(realtimeGuardState);
    runRealtimeGuardTestInjection();

    for (auto channel = 0; channel < buffer.getNumChannels(); ++channel)
        buffer.clear(channel, 0, buffer.getNumSamples());

    if (pendingRestoreAudioSilence.load(std::memory_order_acquire))
    {
        if (!restoreAudioSilenceApplied.exchange(true, std::memory_order_acq_rel))
            performancePlaybackContext.closeAtBlockBoundary();
    }
    else
    {
        restoreAudioSilenceApplied.store(false, std::memory_order_release);
    }

    drs::engine::SamplerEventBlock performanceEvents;
    drs::engine::SamplerEventBlock authoringPreviewEvents;
    const auto frameCount = buffer.getNumSamples();
    const auto hostTransport = readHostTransportObservation(getPlayHead());
    auto resetPerformanceForTransportDiscontinuity = false;
    if (hostTransport.valid && hasPreviousHostTransportObservation)
    {
        if (previousHostTransportWasPlaying && !hostTransport.isPlaying)
        {
            resetPerformanceForTransportDiscontinuity = true;
        }
        else if (previousHostTransportWasPlaying && hostTransport.isPlaying
                 && previousHostTransportHadTimeInSamples && hostTransport.hasTimeInSamples)
        {
            const auto expectedTimeInSamples = previousHostTransportTimeInSamples
                + static_cast<std::int64_t>(std::max(previousHostTransportBlockSize, 0));
            if (hostTransport.timeInSamples != expectedTimeInSamples)
                resetPerformanceForTransportDiscontinuity = true;
        }
    }
    if (resetPerformanceForTransportDiscontinuity)
        performancePlaybackContext.resetAtBlockBoundary();
    hasPreviousHostTransportObservation = hostTransport.valid;
    previousHostTransportWasPlaying = hostTransport.isPlaying;
    previousHostTransportHadTimeInSamples = hostTransport.hasTimeInSamples;
    previousHostTransportTimeInSamples = hostTransport.timeInSamples;
    previousHostTransportBlockSize = frameCount;
    if (authoringPreviewCloseRequested.exchange(false, std::memory_order_acq_rel))
        authoringPreviewPlaybackContext.closeAtBlockBoundary();
    drainRealtimeNoteEvents(performanceSurfaceNoteQueue, performanceEvents,
                            static_cast<std::uint32_t>(std::max(frameCount, 0)));
    drainRealtimeNoteEvents(authoringPreviewNoteQueue, authoringPreviewEvents,
                            static_cast<std::uint32_t>(std::max(frameCount, 0)));

    for (const auto metadata : midiMessages)
    {
        const auto* eventData = metadata.data;
        const auto command = metadata.numBytes > 0 ? static_cast<int>(eventData[0] & 0xf0u) : 0;
        const auto noteNumber = metadata.numBytes > 1 ? static_cast<int>(eventData[1] & 0x7fu) : 0;
        const auto velocity = metadata.numBytes > 2 ? static_cast<int>(eventData[2] & 0x7fu) : 0;
        const auto eventSample = frameCount > 0
            ? static_cast<std::uint32_t>(std::clamp(metadata.samplePosition, 0, frameCount - 1))
            : 0u;
        if (command == 0x90 && velocity > 0)
        {
            performanceEvents.push({ drs::engine::SamplerRenderEventType::noteOn,
                                     eventSample,
                                     static_cast<std::uint8_t>(noteNumber),
                                     static_cast<float>(velocity) / 127.0f });
        }
        else if (command == 0x80 || (command == 0x90 && velocity == 0))
        {
            performanceEvents.push({ drs::engine::SamplerRenderEventType::noteOff,
                                     eventSample,
                                     static_cast<std::uint8_t>(noteNumber),
                                     0.0f });
        }
        else if (command == 0xb0 && metadata.numBytes > 2 && eventData[1] == 64u)
        {
            performanceEvents.push({ drs::engine::SamplerRenderEventType::sustainPedal,
                                     eventSample,
                                     0,
                                     static_cast<float>(eventData[2] & 0x7fu) / 127.0f });
        }
        else if (command == 0xb0 && metadata.numBytes > 1 && eventData[1] == 123u)
        {
            performanceEvents.push({ drs::engine::SamplerRenderEventType::allNotesOff,
                                     eventSample,
                                     0,
                                     0.0f });
        }
        else if (command == 0xb0 && metadata.numBytes > 1 && eventData[1] == 120u)
        {
            performanceEvents.push({ drs::engine::SamplerRenderEventType::reset,
                                     eventSample,
                                     0,
                                     0.0f });
        }
    }

    diagnosticsPerformanceDroppedEventCount.fetch_add(performanceEvents.droppedEventCount(),
                                                       std::memory_order_relaxed);
    diagnosticsAuthoringPreviewDroppedEventCount.fetch_add(authoringPreviewEvents.droppedEventCount(),
                                                            std::memory_order_relaxed);

    if (frameCount > 0 && buffer.getNumChannels() > 0)
    {
        drs::engine::SamplerAudioBufferView output {
            buffer.getArrayOfWritePointers(),
            static_cast<std::uint32_t>(buffer.getNumChannels()),
            static_cast<std::uint32_t>(frameCount)
        };
        const auto performanceBeforeRender = performancePlaybackContext.getSnapshot();
        if (pendingPerformanceActivation != nullptr
            && performanceBeforeRender.hasPendingActivation
            && pendingPerformanceActivation->macroBindings != nullptr)
        {
            installPublishedMacroBindings(*pendingPerformanceActivation->macroBindings);
        }
        auto performanceMacroControls = buildPublishedMacroRenderControls();
        performanceMacroControls.transport.valid = hostTransport.valid;
        performanceMacroControls.transport.isPlaying = hostTransport.isPlaying;
        performanceMacroControls.transport.hasTempo = hostTransport.hasTempo;
        performanceMacroControls.transport.hasSamplePosition = hostTransport.hasTimeInSamples;
        performanceMacroControls.transport.samplePosition = hostTransport.timeInSamples;
        performanceMacroControls.transport.tempoBpm = hostTransport.tempoBpm;
        performanceMacroControls.transport.hasTimeSignature = hostTransport.hasTimeSignature;
        performanceMacroControls.transport.timeSignatureNumerator = hostTransport.timeSignatureNumerator;
        performanceMacroControls.transport.timeSignatureDenominator = hostTransport.timeSignatureDenominator;
        diagnosticActivePublishedMacroFixedVelocity.store(
            performanceMacroControls.overrideFixedVelocity
                ? performanceMacroControls.fixedVelocity : 0,
            std::memory_order_relaxed);
        diagnosticActivePublishedMacroMidiNoteOffset.store(
            performanceMacroControls.overrideMidiNoteOffset
                ? performanceMacroControls.midiNoteOffset : 0,
            std::memory_order_relaxed);
        const auto performanceRenderStart = std::chrono::steady_clock::now();
        const auto performanceResult = performancePlaybackContext.renderBlock(
            output, performanceEvents.view(), performanceMacroControls);
        const auto performanceRenderEnd = std::chrono::steady_clock::now();
        const auto previewResult = authoringPreviewPlaybackContext.renderBlock(
            output, authoringPreviewEvents.view());
        const auto previewRenderEnd = std::chrono::steady_clock::now();
        lastPerformanceRenderMicros = static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::microseconds>(performanceRenderEnd - performanceRenderStart).count());
        lastAuthoringPreviewRenderMicros = static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::microseconds>(previewRenderEnd - performanceRenderEnd).count());
        maxPerformanceRenderMicros = std::max(maxPerformanceRenderMicros, lastPerformanceRenderMicros);
        maxAuthoringPreviewRenderMicros = std::max(maxAuthoringPreviewRenderMicros,
                                                   lastAuthoringPreviewRenderMicros);
        const auto performanceSnapshot = performancePlaybackContext.getSnapshot();
        const auto previewSnapshot = authoringPreviewPlaybackContext.getSnapshot();
        performancePeakActiveVoiceCount = std::max(performancePeakActiveVoiceCount,
                                                   static_cast<std::size_t>(performanceSnapshot.activeVoiceCount));
        performancePeakReleasingVoiceCount = std::max(
            performancePeakReleasingVoiceCount,
            static_cast<std::size_t>(performanceSnapshot.releasingVoiceCount));
        authoringPreviewPeakActiveVoiceCount = std::max(
            authoringPreviewPeakActiveVoiceCount,
            static_cast<std::size_t>(previewSnapshot.activeVoiceCount));
        authoringPreviewPeakReleasingVoiceCount = std::max(
            authoringPreviewPeakReleasingVoiceCount,
            static_cast<std::size_t>(previewSnapshot.releasingVoiceCount));
        if (performanceResult.activationApplied)
            diagnosticsPerformanceActivationCount.fetch_add(1, std::memory_order_relaxed);
        if (previewResult.activationApplied)
            diagnosticsAuthoringPreviewActivationCount.fetch_add(1, std::memory_order_relaxed);
    }

    diagnosticsProcessBlockCount.fetch_add(1, std::memory_order_relaxed);
    diagnosticsLastProcessBlockAtMicros.store(monotonicMicros(), std::memory_order_release);
    const auto callbackBudgetMicros = RealtimeCallbackBudgetProfile::deadlineMicros(
        currentSampleRate,
        static_cast<std::size_t>(buffer.getNumSamples()));
    diagnosticsCallbackBudgetMicros.store(callbackBudgetMicros, std::memory_order_relaxed);
    const auto elapsedMicros = static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - blockStartTime).count());
    diagnosticsLastProcessBlockMicros.store(elapsedMicros, std::memory_order_relaxed);
    updateAtomicMaximum(diagnosticsMaxProcessBlockMicros, elapsedMicros);
#if JUCE_DEBUG
    // Debug CRT checks and unoptimised render code are intentionally instrumented.
    // Keep reporting the real host deadline, but use a small diagnostic allowance
    // so the guard detects pathological stalls rather than debugger overhead.
    const auto enforcementBudgetMicros = callbackBudgetMicros * 4u;
#else
    const auto enforcementBudgetMicros = callbackBudgetMicros;
#endif
    if (elapsedMicros > enforcementBudgetMicros)
    {
        diagnosticsOverBudgetCallbackCount.fetch_add(1, std::memory_order_relaxed);
        recordRealtimeGuardOperation(RealtimeGuardOperation::overBudget);
    }
    updateRealtimeSafetyState();
}

juce::AudioProcessorEditor* Processor::createEditor()
{
    return new Editor(*this);
}

const juce::String Processor::getName() const
{
    return "DecentRhapsodyStudioPlugin";
}

void Processor::getStateInformation(juce::MemoryBlock& destinationData)
{
    const auto serialized = std::atomic_load_explicit(&serializedHostStatePublication,
                                                      std::memory_order_acquire);
    if (serialized == nullptr || serialized->empty())
    {
        destinationData.reset();
        return;
    }

    destinationData.replaceWith(serialized->data(), serialized->size());
}

void Processor::setStateInformation(const void* data, int sizeInBytes)
{
    if (data == nullptr
        || sizeInBytes <= 0
        || static_cast<std::size_t>(sizeInBytes) > drs::engine::hostSessionStateMaxBytes)
        return;

    const auto* bytes = static_cast<const char*>(data);
    auto copiedState = std::make_shared<const std::string>(bytes, bytes + sizeInBytes);
    std::atomic_store_explicit(&latestSubmittedHostState,
                               copiedState,
                               std::memory_order_release);
    setPendingRestoreAudioPolicy(true);
    const auto generation = projectRestoreCoordinator.submit({ *copiedState, {}, {} });
    if (generation == 0)
        setPendingRestoreAudioPolicy(false);
    else
        startTimer(20);
}

bool Processor::retryProjectRestoreWithFile(const juce::File& locatedProjectFile)
{
    if (locatedProjectFile == juce::File())
        return false;

    const auto serialized = std::atomic_load_explicit(&latestSubmittedHostState,
                                                      std::memory_order_acquire);
    if (serialized == nullptr || serialized->empty())
        return false;

    setPendingRestoreAudioPolicy(true);
    const auto generation = projectRestoreCoordinator.submit(
        { *serialized, {}, locatedProjectFile.getFullPathName().toStdString() });
    if (generation == 0)
    {
        setPendingRestoreAudioPolicy(false);
        return false;
    }

    startTimer(20);
    return true;
}

bool Processor::retryProjectRestore()
{
    const auto serialized = std::atomic_load_explicit(&latestSubmittedHostState,
                                                      std::memory_order_acquire);
    if (serialized == nullptr || serialized->empty())
        return false;

    setPendingRestoreAudioPolicy(true);
    const auto generation = projectRestoreCoordinator.submit({ *serialized, {}, {} });
    if (generation == 0)
    {
        setPendingRestoreAudioPolicy(false);
        return false;
    }

    startTimer(20);
    return true;
}

void Processor::setMacroValueFromShell(const std::string& macroId, double value)
{
    auto parameterId = buildMacroParameterId(macroId);
    if (parameterState.getParameter(parameterId) == nullptr)
    {
        if (const auto publishedParameterId = findPublishedHostParameterId(
                engineFacade.getActivePublishedMacroBindings(), macroId);
            publishedParameterId.has_value())
        {
            parameterId = *publishedParameterId;
        }
    }

    if (auto* parameter = dynamic_cast<juce::RangedAudioParameter*>(
            parameterState.getParameter(parameterId)))
    {
        parameter->beginChangeGesture();
        parameter->setValueNotifyingHost(parameter->convertTo0to1(static_cast<float>(value)));
        parameter->endChangeGesture();
        return;
    }

    engineFacade.setMacroValue(macroId, value);
}

void Processor::queueAuthoringPreviewNoteOn(int midiNoteNumber, float velocity)
{
    drs::engine::AuthoringPreviewCommand command;
    const auto previewController = authoringPreviewController.getSnapshot();
    const auto prepareSelectedZoneOnDemand = !authoringPreviewPreparationAuthorized
        && !previewController.hasRequest;
    command.type = prepareSelectedZoneOnDemand
        ? drs::engine::AuthoringPreviewCommandType::auditionSelectedZone
        : drs::engine::AuthoringPreviewCommandType::noteOn;
    command.source = drs::engine::AuthoringPreviewAuditionSource::authoringKeyboard;
    command.midiNote = midiNoteNumber;
    command.velocity = velocity;
    submitAuthoringPreviewCommand(command);
}

bool Processor::requestAuthoringSourceValidation()
{
    const auto& project = authoringSession.getProject();
    if (project.sampleSources.empty())
        return false;

    drs::app::ProjectSourceValidationRequest request;
    request.sampleSources = project.sampleSources;
    request.projectId = project.projectId;
    request.baseRevision = authoringSession.getDocumentState().revision;
    request.contentRootPath = project.contentRootPath;
    return projectSourceValidationService.submit(std::move(request)).accepted;
}

bool Processor::cancelAuthoringSourceValidation()
{
    return projectSourceValidationService.cancel();
}

void Processor::requestAuthoringPreview(drs::engine::AuthoringPreviewScope scope)
{
    drs::engine::AuthoringPreviewCommand command;
    command.type = scope == drs::engine::AuthoringPreviewScope::currentDraft
        ? drs::engine::AuthoringPreviewCommandType::auditionCurrentDraft
        : (scope == drs::engine::AuthoringPreviewScope::selectedGroup
               ? drs::engine::AuthoringPreviewCommandType::auditionSelectedGroup
               : drs::engine::AuthoringPreviewCommandType::auditionSelectedZone);
    command.emitNote = false;
    submitAuthoringPreviewCommand(command);
}

void Processor::queueAuthoringPreviewNoteOff(int midiNoteNumber)
{
    drs::engine::AuthoringPreviewCommand command;
    command.type = drs::engine::AuthoringPreviewCommandType::noteOff;
    command.source = drs::engine::AuthoringPreviewAuditionSource::authoringKeyboard;
    command.midiNote = midiNoteNumber;
    submitAuthoringPreviewCommand(command);
}

bool Processor::submitAuthoringPreviewCommand(
    const drs::engine::AuthoringPreviewCommand& submittedCommand)
{
    auto command = submittedCommand;
    if (command.type == drs::engine::AuthoringPreviewCommandType::auditionSelectedZone
        && command.selectedZoneId.empty())
    {
        const auto selectedZone = authoringSession.getSelectedZone();
        if (selectedZone.has_value())
            command.selectedZoneId = selectedZone->id;
    }
    else if (command.type == drs::engine::AuthoringPreviewCommandType::auditionSelectedGroup)
    {
        if (command.selectedGroupId.empty())
        {
            const auto selectedGroup = authoringSession.getSelectedGroup();
            if (selectedGroup.has_value())
                command.selectedGroupId = selectedGroup->id;
        }
        if (command.selectedZoneId.empty())
        {
            const auto previewRequest = authoringSession.buildSelectedGroupPreviewRequest();
            if (previewRequest.available
                && previewRequest.groupId == command.selectedGroupId)
            {
                command.selectedZoneId = previewRequest.anchorZoneId;
            }
        }
    }

    const auto dispatch = authoringPreviewCommandAdapter.dispatch(command);
    if (!dispatch.accepted)
        return false;

    if (dispatch.preparationRequested)
    {
        authoringPreviewPreparationAuthorized = true;
        authoringPreviewRequestedScope = dispatch.requestedScope;
        authoringPreviewDirectAuditionRequested = true;
        pendingAuthoringPreviewZoneId = command.selectedZoneId;
        pendingAuthoringPreviewGroupId = command.selectedGroupId;
        serviceMessageThreadWork();
        if (command.emitNote)
        {
            const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(1000);
            while (std::chrono::steady_clock::now() < deadline)
            {
                const auto previewContext = authoringPreviewPlaybackContext.getSnapshot();
                const auto previewController = authoringPreviewController.getSnapshot();
                if (previewContext.hasPendingActivation
                    || previewContext.hasActiveActivation
                    || previewController.hasFailedRequest
                    || previewController.preparationState
                        == drs::engine::AuthoringPreviewPreparationState::failed)
                {
                    break;
                }

                if (!serviceMessageThreadWork())
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }
    }

    if (!dispatch.hasEvent)
    {
        publishAuthoringPreviewStatus();
        return true;
    }

    drs::engine::SamplerRenderEventType eventType;
    switch (dispatch.event.type)
    {
        case drs::engine::AuthoringPreviewEventType::noteOn:
            eventType = drs::engine::SamplerRenderEventType::noteOn;
            break;
        case drs::engine::AuthoringPreviewEventType::noteOff:
            eventType = drs::engine::SamplerRenderEventType::noteOff;
            break;
        case drs::engine::AuthoringPreviewEventType::allNotesOff:
            eventType = drs::engine::SamplerRenderEventType::allNotesOff;
            break;
        case drs::engine::AuthoringPreviewEventType::reset:
            eventType = drs::engine::SamplerRenderEventType::reset;
            break;
    }

    if (authoringPreviewNoteQueue.push({ eventType,
                                         clampMidiValue(dispatch.event.midiNote),
                                         std::clamp(dispatch.event.velocity, 0.0f, 1.0f),
                                         dispatch.event.sampleOffset }))
    {
        publishAuthoringPreviewStatus();
        return true;
    }

    diagnosticsAuthoringPreviewDroppedNoteCount.fetch_add(1, std::memory_order_relaxed);
    return false;
}

bool Processor::submitPerformancePublishCommand(
    const drs::engine::PerformancePublishCommand& command,
    drs::engine::PerformancePublishCommandSource source)
{
    const auto dispatch = performancePublishCommandAdapter.dispatch(command, source);
    if (!dispatch.accepted)
        return false;

    const auto accepted = engineFacade.publishCurrentDraft();
    performancePublishCommandAdapter.recordExecutionResult(accepted);
    publishMessageDiagnostics();
    return accepted;
}

drs::app::AuthoringWaveformPreview Processor::getAuthoringWaveformPreview()
{
    consumeAuthoringWaveformPreviewSnapshot();

    const auto selectedZone = authoringSession.getSelectedZone();
    if (!selectedZone.has_value())
        return { false, "No zone selected" };

    const auto projectSampleSource = findProjectSampleSource(authoringSession.getProject(), selectedZone->sampleSourceId);
    if (!projectSampleSource.has_value())
        return { false, "Selected zone sample source is missing from the project." };

    const auto currentStampIterator = authoringWaveformPreviewCurrentStampBySourceId.find(projectSampleSource->id);
    if (currentStampIterator != authoringWaveformPreviewCurrentStampBySourceId.end())
    {
        if (const auto* cacheEntry = findAuthoringWaveformPreviewCacheEntryForStamp(currentStampIterator->second);
            cacheEntry != nullptr)
        {
            auto preview = cacheEntry->preview;
            preview.state = "Ready";
            preview.loopEnabled = selectedZone->loopEnabled;
            preview.loopStartFrame = selectedZone->loopStartFrame;
            preview.loopEndFrame = selectedZone->loopEndFrame;
            return preview;
        }
    }

    if (!authoringWaveformPreviewLoadAuthorized)
    {
        drs::app::AuthoringWaveformPreview preview;
        preview.state = "Preview loads on demand";
        return preview;
    }

    auto preview = drs::app::AuthoringWaveformPreview {};
    if (const auto snapshot = waveformPreviewService.getSnapshot();
        snapshot != nullptr
            && snapshot->identity.sampleSourceId == projectSampleSource->id
            && currentStampIterator != authoringWaveformPreviewCurrentStampBySourceId.end()
            && snapshot->identity.requestStamp == currentStampIterator->second)
    {
        preview.state = !snapshot->status.empty()
            ? snapshot->status
            : mapWaveformPreviewServiceState(snapshot->stage);

        if (snapshot->result != nullptr)
        {
            preview.sourcePath = snapshot->result->metadata.sourcePath;
            preview.formatName = snapshot->result->metadata.formatName;
            preview.durationSeconds = snapshot->result->metadata.durationSeconds;
            preview.sampleRate = snapshot->result->metadata.sampleRate;
            preview.frameCount = snapshot->result->metadata.frameCount;
            preview.channelCount = snapshot->result->metadata.channelCount;
        }

        if (isWaveformPreviewServiceActiveStage(snapshot->stage))
            preview.state = "Loading";
        else if (snapshot->stage == drs::app::WaveformPreviewServiceStage::canceled)
            preview.state = "Canceled";
        else if (snapshot->stage == drs::app::WaveformPreviewServiceStage::failed)
            preview.state = "Unavailable";
    }
    else
    {
        preview.state = "Loading";
    }

    if (const auto* staleCacheEntry = findLatestAuthoringWaveformPreviewCacheEntryForSource(projectSampleSource->id);
        staleCacheEntry != nullptr)
    {
        preview = staleCacheEntry->preview;
        preview.state = "Stale";
    }

    preview.loopEnabled = selectedZone->loopEnabled;
    preview.loopStartFrame = selectedZone->loopStartFrame;
    preview.loopEndFrame = selectedZone->loopEndFrame;
    return preview;
}

void Processor::authorizeAuthoringWaveformPreviewLoad()
{
    authoringWaveformPreviewLoadAuthorized = true;
    consumeAuthoringWaveformPreviewSnapshot();

    const auto selectedZone = authoringSession.getSelectedZone();
    if (!selectedZone.has_value())
        return;

    const auto projectSampleSource = findProjectSampleSource(authoringSession.getProject(), selectedZone->sampleSourceId);
    if (!projectSampleSource.has_value())
        return;

    std::uint64_t sourceFileSizeBytes = 0;
    std::int64_t sourceModificationTicks = 0;
    describeAuthoringWaveformPreviewSource(*projectSampleSource,
                                           sourceFileSizeBytes,
                                           sourceModificationTicks);
    const auto requestStamp = buildAuthoringWaveformPreviewRequestStamp(*projectSampleSource,
                                                                        sourceFileSizeBytes,
                                                                        sourceModificationTicks);
    authoringWaveformPreviewCurrentStampBySourceId[projectSampleSource->id] = requestStamp;

    if (findAuthoringWaveformPreviewCacheEntryForStamp(requestStamp) != nullptr)
        return;

    if (const auto snapshot = waveformPreviewService.getSnapshot();
        snapshot != nullptr
            && snapshot->identity.sampleSourceId == projectSampleSource->id
            && snapshot->identity.requestStamp == requestStamp
            && isWaveformPreviewServiceActiveStage(snapshot->stage))
    {
        return;
    }

    drs::app::WaveformPreviewRequest request;
    request.projectId = authoringSession.getProject().projectId;
    request.baseRevision = authoringSession.getDocumentState().revision;
    request.contentRootPath = authoringSession.getProject().contentRootPath;
    request.sampleSourceId = projectSampleSource->id;
    request.sourcePath = projectSampleSource->path;
    request.sourceFileSizeBytes = sourceFileSizeBytes;
    request.sourceModificationTicks = sourceModificationTicks;
    request.displayPointCount = authoringWaveformPreviewPointCount;
    request.chunkFrameCount = authoringWaveformPreviewChunkFrameCount;
    request.channelReduction = authoringWaveformPreviewChannelReduction;
    request.requestStamp = requestStamp;
    waveformPreviewService.submit(std::move(request));
}

drs::app::AuthoringPreviewStatusSnapshot Processor::getAuthoringPreviewStatusSnapshot() const
{
    const auto published = std::atomic_load_explicit(&authoringPreviewStatusPublication,
                                                     std::memory_order_acquire);
    return published != nullptr ? *published : drs::app::AuthoringPreviewStatusSnapshot {};
}

void Processor::publishAuthoringPreviewStatus()
{
    using Presentation = drs::engine::AuthoringPreviewPresentationState;
    using Preparation = drs::engine::AuthoringPreviewPreparationState;
    using Activation = drs::engine::AuthoringPreviewActivationState;

    const auto diagnostics = getRealtimeSafetySnapshot();
    const auto controller = authoringPreviewController.getSnapshot();
    const auto command = authoringPreviewCommandAdapter.getSnapshot();
    const auto selectedZone = authoringSession.getSelectedZone();
    const auto selectedGroup = authoringSession.getSelectedGroup();

    drs::app::AuthoringPreviewStatusSnapshot status;
    status.available = diagnostics.available;
    status.preparationState = controller.preparationState;
    status.activationState = controller.activationState;
    status.scope = controller.hasRequest
        ? controller.currentRequest.identity.scope : authoringPreviewRequestedScope;
    status.requestId = controller.hasRequest
        ? controller.currentRequest.identity.requestId : 0;
    status.cancellationGeneration = controller.hasRequest
        ? controller.currentRequest.identity.cancellationGeneration : 0;
    status.draftRevision = authoringSession.getDocumentState().revision;
    status.activeRevision = controller.hasActiveRequest
        ? controller.activeRequestIdentity.draftRevision : 0;
    status.pendingRevision = controller.activationState == Activation::pending
        && controller.hasRequest ? controller.currentRequest.identity.draftRevision : 0;
    status.requestedRevision = controller.hasRequest
        ? controller.currentRequest.identity.draftRevision : 0;
    status.failedRevision = controller.hasFailedRequest
        ? controller.failedRequestIdentity.draftRevision : 0;
    status.audibleRevision = status.activeRevision;
    status.selectedZoneId = controller.hasRequest
        ? controller.currentRequest.identity.selectedZoneId
        : (selectedZone.has_value() ? selectedZone->id : std::string {});
    status.requestedPreparedBuildId = controller.acceptedPreparedBuildId;
    status.activePreparedBuildId = controller.activePreparedBuildId;
    status.requestedSnapshotDigest = controller.acceptedSnapshotDigest;
    status.requestedPreparedDigest = controller.acceptedPreparedDigest;
    status.activeSnapshotDigest = controller.activeSnapshotDigest;
    status.activePreparedDigest = controller.activePreparedDigest;
    status.auditionAvailable = engineFacade.getDraftPlaybackStatus().projectOpen
        && (status.scope == drs::engine::AuthoringPreviewScope::currentDraft
            || (status.scope == drs::engine::AuthoringPreviewScope::selectedZone
                && selectedZone.has_value())
            || (status.scope == drs::engine::AuthoringPreviewScope::selectedGroup
                && selectedGroup.has_value()));
    status.stopAvailable = command.ownedNoteCount > 0
        || diagnostics.authoringPreviewActiveVoiceCount > 0;
    status.usingLastKnownGood = controller.hasActiveRequest
        && (controller.hasFailedRequest
            || controller.activeRequestIdentity.draftRevision != status.draftRevision
            || (controller.hasRequest
                && controller.activeRequestIdentity != controller.currentRequest.identity));

    if (!controller.failureFinding.code.empty())
        status.findings.push_back(controller.failureFinding);
    status.failureState = controller.failureState;
    status.failureFamily = drs::engine::toString(controller.failureFinding.family);
    status.failureCode = controller.failureFinding.code;
    status.failurePath = controller.failureFinding.path;
    const auto blockingHint = buildAuthoringPreviewBlockingHint(authoringSession,
                                                                status.failureState);
    status.blockingPrerequisite = blockingHint.prerequisite;
    status.blockingGuidance = blockingHint.guidance;

    if (controller.preparationState == Preparation::failed)
        status.presentationState = Presentation::failed;
    else if (controller.preparationState == Preparation::canceled)
        status.presentationState = Presentation::canceled;
    else if (controller.preparationState == Preparation::superseded)
        status.presentationState = Presentation::superseded;
    else if (controller.activationState == Activation::pending)
        status.presentationState = Presentation::activating;
    else if (controller.preparationState == Preparation::queued)
        status.presentationState = Presentation::queued;
    else if (controller.preparationState == Preparation::preparing)
        status.presentationState = Presentation::preparing;
    else if (controller.hasActiveRequest
             && controller.activeRequestIdentity.draftRevision == status.draftRevision)
        status.presentationState = Presentation::active;
    else if (controller.hasActiveRequest)
        status.presentationState = Presentation::stale;
    else if (controller.preparationState == Preparation::ready)
        status.presentationState = Presentation::ready;
    else
        status.presentationState = Presentation::idle;

    if (status.scope == drs::engine::AuthoringPreviewScope::selectedZone
        && !selectedZone.has_value())
    {
        status.stateLabel = status.usingLastKnownGood
            ? "No Selection — Last Good Active" : "No Selection";
        status.creatorGuidance = "Select a zone to enable selected-zone Preview.";
    }
    else if (status.scope == drs::engine::AuthoringPreviewScope::selectedGroup
             && !selectedGroup.has_value())
    {
        status.stateLabel = status.usingLastKnownGood
            ? "No Group â€” Last Good Active" : "No Group";
        status.creatorGuidance = "Select a group to enable selected-group Preview.";
    }
    else
    {
        switch (status.presentationState)
        {
            case Presentation::queued:
                status.stateLabel = "Preparing";
                status.creatorGuidance = "Preview is coalescing recent authored changes.";
                break;
            case Presentation::preparing:
                status.stateLabel = "Preparing";
                status.creatorGuidance = "Preview is building the current authored content.";
                break;
            case Presentation::ready:
                status.stateLabel = "Ready";
                status.creatorGuidance = "Preview is prepared and ready to activate.";
                break;
            case Presentation::activating:
                status.stateLabel = "Preparing";
                status.creatorGuidance = "Preview is waiting for the next audio block boundary.";
                break;
            case Presentation::active:
                status.stateLabel = "Ready";
                status.creatorGuidance = "Preview matches the current authored revision.";
                break;
            case Presentation::stale:
                status.stateLabel = "Stale — Last Good Active";
                status.creatorGuidance = "The last known good Preview remains audible while the current draft is prepared.";
                break;
            case Presentation::failed:
                status.stateLabel = status.usingLastKnownGood
                    ? "Failed — Last Good Active" : "Failed";
                status.creatorGuidance = status.blockingGuidance.empty()
                    ? "Repair the reported Preview finding and audition again."
                    : status.blockingGuidance;
                break;
            case Presentation::canceled:
                status.stateLabel = "Canceled";
                status.creatorGuidance = "The Preview request was canceled safely.";
                break;
            case Presentation::superseded:
                status.stateLabel = "Superseded";
                status.creatorGuidance = "A newer authored revision replaced this Preview request.";
                break;
            case Presentation::idle:
            default:
                status.stateLabel = "Ready";
                status.creatorGuidance = "Preview is available for the current authored selection.";
                break;
        }
    }

    status.lastRequestToLaunchMicros = controller.lastRequestToLaunchMicros;
    status.maxRequestToLaunchMicros = controller.maxRequestToLaunchMicros;
    status.lastPreparationMicros = controller.lastPreparationMicros;
    status.maxPreparationMicros = controller.maxPreparationMicros;
    status.lastReadyToActivationMicros = controller.lastReadyToActivationMicros;
    status.maxReadyToActivationMicros = controller.maxReadyToActivationMicros;
    status.lastRequestToAudibleMicros = controller.lastRequestToAudibleMicros;
    status.maxRequestToAudibleMicros = controller.maxRequestToAudibleMicros;
    status.lastCancellationMicros = controller.lastCancellationMicros;
    status.maxCancellationMicros = controller.maxCancellationMicros;
    status.coalescedCount = controller.coalescedCount;
    status.canceledCount = controller.canceledCount;
    status.pendingDepth = controller.pendingDepth;
    status.maximumPendingDepth = controller.maximumPendingDepth;

    std::shared_ptr<const drs::app::AuthoringPreviewStatusSnapshot> immutable
        = std::make_shared<const drs::app::AuthoringPreviewStatusSnapshot>(std::move(status));
    std::atomic_store_explicit(&authoringPreviewStatusPublication,
                               std::move(immutable),
                               std::memory_order_release);
}

drs::app::AuthoringImportResponsivenessSnapshot Processor::getAuthoringImportResponsivenessSnapshot() const
{
    const auto serviceSnapshot = wavImportService.getSnapshot();
    if (serviceSnapshot != nullptr
        && isRelevantWavImportSnapshot(*serviceSnapshot, authoringSession.getProject()))
    {
        return buildImportResponsivenessSnapshotFromWavImport(*serviceSnapshot);
    }

    return authoringImportResponsivenessSnapshot;
}

drs::app::AuthoringSourceValidationSnapshot Processor::getAuthoringSourceValidationSnapshot() const
{
    const auto serviceSnapshot = projectSourceValidationService.getSnapshot();
    if (serviceSnapshot != nullptr
        && isRelevantProjectSourceValidationSnapshot(*serviceSnapshot, authoringSession.getProject()))
    {
        return buildAuthoringSourceValidationSnapshot(*serviceSnapshot);
    }

    return authoringSourceValidationSnapshot;
}

std::optional<drs::engine::HostProjectBinding> Processor::buildValidatedAuthoringProjectBinding(
    const juce::File& resolvedProjectFile,
    const drs::engine::RuntimeProjectModel& project) const
{
    if (resolvedProjectFile == juce::File()
        || !resolvedProjectFile.hasFileExtension(".drsproj")
        || !resolvedProjectFile.existsAsFile())
        return std::nullopt;

    const auto manifestPath = resolvedProjectFile.getFullPathName().toStdString();
    const auto loaded = drs::engine::loadRuntimeProjectManifest(manifestPath);
    if (!loaded.loaded
        || loaded.project.projectId.empty()
        || loaded.project.projectId != project.projectId)
        return std::nullopt;

    const auto manifestDigest
        = drs::engine::computeHostProjectManifestDigest(project, manifestPath);
    if (manifestDigest
        != drs::engine::computeHostProjectManifestDigest(loaded.project, manifestPath))
        return std::nullopt;

    drs::engine::HostProjectBinding binding;
    binding.projectId = project.projectId;
    binding.manifestPath = manifestPath;
    binding.manifestFileName = resolvedProjectFile.getFileName().toStdString();
    binding.manifestDigest = manifestDigest;
    binding.contentRootHint = resolvedProjectFile.getParentDirectory().getFullPathName().toStdString();
    return binding;
}

juce::File Processor::getAuthoringProjectFile() const
{
    if (authoringProjectBinding.manifestPath.empty())
        return {};

    return juce::File(juce::String::fromUTF8(authoringProjectBinding.manifestPath.c_str()));
}

bool Processor::bindAuthoringProjectFile(const juce::File& resolvedProjectFile)
{
    const auto binding = buildValidatedAuthoringProjectBinding(
        resolvedProjectFile,
        authoringSession.getProject());
    if (!binding.has_value())
        return false;

    authoringProjectBinding = *binding;
    refreshSerializedHostStatePublication(true);
    return true;
}

void Processor::clearAuthoringProjectFileBinding()
{
    authoringProjectBinding = {};
    refreshSerializedHostStatePublication(true);
}

bool Processor::replaceAuthoringProject(drs::engine::RuntimeProjectModel project,
                                        juce::File resolvedProjectFile)
{
    std::optional<drs::engine::HostProjectBinding> replacementBinding;
    if (resolvedProjectFile != juce::File())
    {
        replacementBinding = buildValidatedAuthoringProjectBinding(resolvedProjectFile, project);
        if (!replacementBinding.has_value())
            return false;
    }

    const auto& previousProject = authoringSession.getProject();
    const auto replacingDifferentProject = !previousProject.projectId.empty()
        && !project.projectId.empty()
        && previousProject.projectId != project.projectId;
    auto draftPlaybackProject = project;
    if (!engineFacade.replaceDraftPlaybackAuthoringProject(std::move(draftPlaybackProject)))
        return false;

    authoringSession.replaceProject(std::move(project));
    projectSourceValidationService.cancel("Authoring project replaced");
    authoringProjectBinding = replacementBinding.value_or(drs::engine::HostProjectBinding {});
    performancePlaybackContext.cancelPendingActivation();
    pendingPerformanceActivation.reset();
    engineFacade.closeDraftPlaybackProject(true);
    engineFacade.reopenDraftPlaybackProject(authoringSession.getDocumentState().revision, true);
    clearAuthoringWaveformPreviewCache();
    resetAuthoringPreviewPreparationAuthorization();
    resetAuthoringWaveformPreviewAuthorization();
    if (replacingDifferentProject)
    {
        authoringPreviewController.reset();
        authoringPreviewCommandAdapter.clearOwnership();
        authoringPreviewCloseRequested.store(true, std::memory_order_release);
    }
    authoringPreviewDirectAuditionRequested = false;
    authoringPreviewRequestedScope = drs::engine::AuthoringPreviewScope::selectedZone;
    pendingAuthoringPreviewZoneId.clear();
    pendingAuthoringPreviewGroupId.clear();
    observedDraftPlaybackProjectRevision = authoringSession.getDocumentState().revision;
    initializeAuthoringImportMetrics();
    initializeAuthoringSourceValidationSnapshot();
    serviceMessageThreadWork();
    updateRealtimeSafetyState();
    refreshSerializedHostStatePublication(true);
    return true;
}

bool Processor::applyAuthoringProjectMigration(drs::engine::RuntimeProjectModel migratedProject)
{
    if (migratedProject.projectId.empty()
        || migratedProject.projectId != authoringSession.getProject().projectId)
    {
        return false;
    }

    const auto migration = authoringSession.applyProjectMigration(std::move(migratedProject));
    if (!migration.applied)
        return false;

    projectSourceValidationService.cancel("Authoring project migrated");
    clearAuthoringWaveformPreviewCache();
    resetAuthoringPreviewPreparationAuthorization();
    resetAuthoringWaveformPreviewAuthorization();
    initializeAuthoringSourceValidationSnapshot();
    serviceMessageThreadWork();
    refreshSerializedHostStatePublication(true);
    return true;
}

void Processor::closeAuthoringProject(drs::engine::RuntimeProjectModel unloadedProject)
{
    projectSourceValidationService.cancel("Authoring project closed");
    engineFacade.cancelPreviewPreparation("Authoring project closed");
    performancePlaybackContext.cancelPendingActivation();
    pendingPerformanceActivation.reset();
    engineFacade.closeDraftPlaybackProject();
    authoringSession.replaceProject(std::move(unloadedProject));
    clearAuthoringProjectFileBinding();
    clearAuthoringWaveformPreviewCache();
    resetAuthoringPreviewPreparationAuthorization();
    resetAuthoringWaveformPreviewAuthorization();
    authoringPreviewController.reset();
    authoringPreviewCommandAdapter.clearOwnership();
    authoringPreviewCloseRequested.store(true, std::memory_order_release);
    authoringPreviewDirectAuditionRequested = false;
    authoringPreviewRequestedScope = drs::engine::AuthoringPreviewScope::selectedZone;
    pendingAuthoringPreviewZoneId.clear();
    pendingAuthoringPreviewGroupId.clear();
    observedDraftPlaybackProjectRevision = authoringSession.getDocumentState().revision;
    initializeAuthoringImportMetrics();
    initializeAuthoringSourceValidationSnapshot();
    updateRealtimeSafetyState();
    publishAuthoringPreviewStatus();
    refreshSerializedHostStatePublication(true);
}

void Processor::queuePerformanceSurfaceNoteOn(int midiNoteNumber, float velocity)
{
    if (!performanceSurfaceNoteQueue.push(
            { drs::engine::SamplerRenderEventType::noteOn,
              clampMidiValue(midiNoteNumber), std::clamp(velocity, 0.0f, 1.0f), 0 }))
        diagnosticsPerformanceDroppedNoteCount.fetch_add(1, std::memory_order_relaxed);
}

void Processor::queuePerformanceSurfaceNoteOff(int midiNoteNumber)
{
    if (!performanceSurfaceNoteQueue.push({ drs::engine::SamplerRenderEventType::noteOff,
                                            clampMidiValue(midiNoteNumber), 0.0f, 0 }))
        diagnosticsPerformanceDroppedNoteCount.fetch_add(1, std::memory_order_relaxed);
}

bool Processor::hasRecentAudioCallback(std::uint64_t maximumAgeMicros) const noexcept
{
    const auto lastCallback = diagnosticsLastProcessBlockAtMicros.load(std::memory_order_acquire);
    const auto now = monotonicMicros();
    return lastCallback != 0 && now >= lastCallback && now - lastCallback <= maximumAgeMicros;
}

void Processor::setRealtimeGuardTestInjection(RealtimeGuardOperation operation)
{
    if (realtimeGuardTestAllocation != nullptr)
    {
        delete[] realtimeGuardTestAllocation;
        realtimeGuardTestAllocation = nullptr;
    }

    if (operation == RealtimeGuardOperation::deallocation)
        realtimeGuardTestAllocation = new std::byte[64];

    realtimeGuardTestInjection.store(operation, std::memory_order_release);
}

void Processor::runRealtimeGuardTestInjection()
{
    const auto operation = realtimeGuardTestInjection.exchange(RealtimeGuardOperation::none,
                                                               std::memory_order_acq_rel);
    switch (operation)
    {
        case RealtimeGuardOperation::none:
            return;
        case RealtimeGuardOperation::allocation:
            realtimeGuardTestAllocation = new std::byte[64];
            return;
        case RealtimeGuardOperation::deallocation:
            delete[] realtimeGuardTestAllocation;
            realtimeGuardTestAllocation = nullptr;
            return;
        case RealtimeGuardOperation::blockingLock:
        {
            recordRealtimeGuardOperation(operation);
            const std::lock_guard<std::mutex> lock(realtimeGuardTestMutex);
            return;
        }
        case RealtimeGuardOperation::overBudget:
            recordRealtimeGuardOperation(operation);
            diagnosticsOverBudgetCallbackCount.fetch_add(1, std::memory_order_relaxed);
            return;
        case RealtimeGuardOperation::count:
            return;
        default:
            recordRealtimeGuardOperation(operation);
            return;
    }
}

std::string Processor::buildHostStatePublicationKey() const
{
    const auto& document = authoringSession.getDocumentState();
    const auto publish = engineFacade.getPerformancePublishControllerSnapshot();
    std::ostringstream key;
    key << engineFacade.getStateRevision()
        << '|' << document.revision
        << '|' << document.savedRevision
        << '|' << document.dirty
        << '|' << authoringSession.getProject().projectId
        << '|' << authoringProjectBinding.projectId
        << '|' << authoringProjectBinding.manifestPath
        << '|' << authoringProjectBinding.manifestDigest
        << '|' << publish.hasActiveRequest
        << '|' << publish.activeRequestIdentity.projectGeneration
        << '|' << publish.activeRequestIdentity.draftRevision
        << '|' << publish.activeRequestIdentity.authoredContentDigest
        << '|' << publish.activeRequestIdentity.macroSchemaDigest
        << '|' << publish.activePreparedDigest;
    return key.str();
}

void Processor::refreshSerializedHostStatePublication(const bool force)
{
    const auto key = buildHostStatePublicationKey();
    if (!force && key == hostStatePublicationKey)
        return;

    const auto& project = authoringSession.getProject();
    auto presetState = drs::engine::captureRuntimePresetState(
        engineFacade.getCurrentSessionState());
    if (const auto bindings = engineFacade.getActivePublishedMacroBindings(); bindings != nullptr)
    {
        presetState.dspGraphDigest = bindings->dspGraphDigest;
        for (const auto& binding : bindings->bindings)
        {
            if (binding.assigned
                && binding.renderTarget == drs::engine::PublishedMacroRenderTarget::dspControl)
            {
                presetState.dspMacroTargets.push_back(
                    { binding.stableAuthoredId, binding.dspSlotId, binding.dspParameterId });
            }
        }
    }
    if (project.projectId.empty())
    {
        auto immutable = std::make_shared<const std::string>(
            drs::engine::serializeRuntimePresetState(presetState));
        std::atomic_store_explicit(&serializedHostStatePublication,
                                   std::move(immutable),
                                   std::memory_order_release);
        hostStatePublicationKey = key;
        return;
    }

    drs::engine::HostSessionState state;
    state.presetState = presetState;
    state.projectBinding = authoringProjectBinding;
    if (state.projectBinding.projectId != project.projectId
        || state.projectBinding.manifestFileName.empty())
    {
        state.projectBinding = {};
        state.projectBinding.projectId = project.projectId;
        state.projectBinding.manifestFileName = "unsaved-project.drsproj";
        state.projectBinding.contentRootHint = project.contentRootPath;
    }

    const auto digestPath = state.projectBinding.manifestPath.empty()
        ? state.projectBinding.manifestFileName
        : state.projectBinding.manifestPath;
    state.projectBinding.manifestDigest
        = drs::engine::computeHostProjectManifestDigest(project, digestPath);

    const auto checkpoint = authoringSession.exportCheckpoint();
    state.authoringState.revision = checkpoint.revision;
    state.authoringState.savedRevision = checkpoint.savedRevision;
    state.authoringState.dirty = checkpoint.dirty;
    if (checkpoint.dirty || state.projectBinding.manifestPath.empty())
        state.authoringState.projectSnapshot = checkpoint.project;

    const auto publish = engineFacade.getPerformancePublishControllerSnapshot();
    if (publish.hasActiveRequest
        && publish.activeRequestIdentity.projectGeneration != 0
        && !publish.activeRequestIdentity.authoredContentDigest.empty()
        && !publish.activeRequestIdentity.macroSchemaDigest.empty()
        && !publish.activePreparedDigest.empty())
    {
        drs::engine::HostPublishedCheckpoint published;
        published.revision = publish.activeRequestIdentity.draftRevision;
        published.projectGeneration = publish.activeRequestIdentity.projectGeneration;
        published.authoredContentDigest
            = publish.activeRequestIdentity.authoredContentDigest;
        published.macroSchemaDigest
            = publish.activeRequestIdentity.macroSchemaDigest;
        published.preparedContentDigest = publish.activePreparedDigest;
        if (const auto bindings = engineFacade.getActivePublishedMacroBindings(); bindings != nullptr)
            published.dspGraphDigest = bindings->dspGraphDigest;
        state.publishedState = std::move(published);
    }

    const auto serialized = drs::engine::serializeHostSessionState(state, digestPath);
    if (!serialized.serialized)
        return;

    auto immutable = std::make_shared<const std::string>(serialized.text);
    std::atomic_store_explicit(&serializedHostStatePublication,
                               std::move(immutable),
                               std::memory_order_release);
    hostStatePublicationKey = key;
}

void Processor::setPendingRestoreAudioPolicy(const bool pending) noexcept
{
    pendingRestoreAudioSilence.store(pending, std::memory_order_release);
    if (pending)
        restoreAudioSilenceApplied.store(false, std::memory_order_release);
}

bool Processor::restorePublishIdentityMatches(
    const drs::engine::PerformancePublishControllerSnapshot& published) const
{
    if (!expectedRestoredPublishedState.has_value() || !published.hasActiveRequest)
        return false;

    const auto& expected = *expectedRestoredPublishedState;
    const auto& active = published.activeRequestIdentity;
    return active.projectGeneration == expected.projectGeneration
        && active.draftRevision == expected.revision
        && active.authoredContentDigest == expected.authoredContentDigest
        && active.macroSchemaDigest == expected.macroSchemaDigest
        && published.activePreparedDigest == expected.preparedContentDigest;
}

bool Processor::applyValidatedProjectRestore(
    const drs::engine::ProjectRestoreSnapshot& restore)
{
    if (!restore.hostState.has_value() || !restore.checkpoint.has_value())
        return false;

    const auto& hostState = *restore.hostState;
    const auto& checkpoint = *restore.checkpoint;
    const auto reference = engineFacade.loadPhase1ReferenceInstrument();
    const auto presetValidation = drs::engine::validateRuntimePresetState(
        hostState.presetState,
        reference.instrument);
    if (!reference.loaded || !presetValidation.valid)
        return false;

    drs::engine::RuntimeProjectDocumentCheckpointConstraints constraints;
    constraints.expectedProjectId = hostState.projectBinding.projectId;
    constraints.manifestPath = restore.resolvedManifestPath.empty()
        ? hostState.projectBinding.manifestPath
        : restore.resolvedManifestPath;
    const auto checkpointValidation
        = drs::engine::validateRuntimeProjectDocumentCheckpoint(checkpoint, constraints);
    if (!checkpointValidation.valid)
        return false;

    auto restoredSession = drs::engine::AuthoringSession(checkpoint.project);
    if (!restoredSession.restoreCheckpoint(checkpoint, constraints).applied)
        return false;

    auto restoredBinding = hostState.projectBinding;
    if (!restore.resolvedManifestPath.empty())
    {
        restoredBinding.manifestPath = restore.resolvedManifestPath;
        restoredBinding.manifestFileName
            = fs::u8path(restore.resolvedManifestPath).filename().string();
        restoredBinding.contentRootHint
            = fs::u8path(restore.resolvedManifestPath).parent_path().string();
    }
    const auto bindingPath = restoredBinding.manifestPath.empty()
        ? restoredBinding.manifestFileName
        : restoredBinding.manifestPath;
    if (!drs::engine::verifyHostProjectBinding(
             restoredBinding,
             checkpoint.project,
             bindingPath).matched())
        return false;

    auto playbackProject = checkpoint.project;
    if (!engineFacade.replaceDraftPlaybackAuthoringProject(std::move(playbackProject)))
        return false;

    performancePlaybackContext.cancelPendingActivation();
    pendingPerformanceActivation.reset();
    engineFacade.closeDraftPlaybackProject(false);
    if (!engineFacade.reopenDraftPlaybackProject(checkpoint.revision, false))
        return false;

    authoringSession = std::move(restoredSession);
    authoringSession.setDspParameterGesturePreviewListener(
        [this](const std::string& slotId, const std::string& parameterId, const double value)
        {
            authoringPreviewPlaybackContext.publishDspControlByIdentity(slotId, parameterId, value);
        });
    projectSourceValidationService.cancel("Authoring project restored");
    authoringProjectBinding = std::move(restoredBinding);
    clearAuthoringWaveformPreviewCache();
    resetAuthoringPreviewPreparationAuthorization();
    resetAuthoringWaveformPreviewAuthorization();
    authoringPreviewController.reset();
    authoringPreviewCommandAdapter.clearOwnership();
    authoringPreviewCloseRequested.store(true, std::memory_order_release);
    authoringPreviewDirectAuditionRequested = false;
    authoringPreviewRequestedScope = drs::engine::AuthoringPreviewScope::selectedZone;
    pendingAuthoringPreviewZoneId.clear();
    pendingAuthoringPreviewGroupId.clear();
    observedDraftPlaybackProjectRevision = checkpoint.revision;
    initializeAuthoringImportMetrics();
    initializeAuthoringSourceValidationSnapshot();

    const auto presetRestore = engineFacade.restorePresetStateJson(
        drs::engine::serializeRuntimePresetState(hostState.presetState));
    if (!presetRestore.restored)
        return false;
    syncParametersFromEngine();

    expectedRestoredPublishedState = hostState.publishedState;
    awaitingRestoreActivationGeneration = 0;
    if (expectedRestoredPublishedState.has_value())
    {
        if (!engineFacade.restorePerformancePublishProjectGeneration(
                expectedRestoredPublishedState->projectGeneration))
            return false;
        if (!engineFacade.publishCurrentDraft())
            return false;
        awaitingRestoreActivationGeneration = restore.generation;
        projectRestoreCoordinator.publishLifecycleState(
            restore.generation,
            drs::engine::ProjectRestoreState::preparing,
            {},
            "Rebuilding the exact published Performance identity");
    }

    refreshSerializedHostStatePublication(true);
    updateRealtimeSafetyState();
    publishAuthoringPreviewStatus();
    return true;
}

bool Processor::serviceProjectRestore()
{
    const auto restore = projectRestoreCoordinator.getSnapshot();
    if (restore == nullptr || restore->generation == 0)
        return false;

    if (awaitingRestoreActivationGeneration == restore->generation
        && expectedRestoredPublishedState.has_value())
    {
        const auto published = engineFacade.getPerformancePublishControllerSnapshot();
        if (published.activationState
                == drs::engine::PerformancePublishActivationState::active)
        {
            if (restorePublishIdentityMatches(published))
            {
                projectRestoreCoordinator.publishLifecycleState(
                    restore->generation,
                    drs::engine::ProjectRestoreState::active,
                    {},
                    "Exact restored Performance identity is active");
                awaitingRestoreActivationGeneration = 0;
                expectedRestoredPublishedState.reset();
                setPendingRestoreAudioPolicy(false);
                refreshSerializedHostStatePublication(true);
                return true;
            }

            projectRestoreCoordinator.publishLifecycleState(
                restore->generation,
                drs::engine::ProjectRestoreState::failed,
                drs::engine::ProjectRestoreFinding::preparationFailed,
                "The rebuilt Performance identity did not match the saved host checkpoint "
                "(generation " + std::to_string(published.activeRequestIdentity.projectGeneration)
                    + "/" + std::to_string(expectedRestoredPublishedState->projectGeneration)
                    + ", revision "
                    + std::to_string(published.activeRequestIdentity.draftRevision)
                    + "/" + std::to_string(expectedRestoredPublishedState->revision)
                    + ", authored "
                    + published.activeRequestIdentity.authoredContentDigest
                    + "/" + expectedRestoredPublishedState->authoredContentDigest
                    + ", macro " + published.activeRequestIdentity.macroSchemaDigest
                    + "/" + expectedRestoredPublishedState->macroSchemaDigest
                    + ", prepared " + published.activePreparedDigest
                    + "/" + expectedRestoredPublishedState->preparedContentDigest + ").");
            awaitingRestoreActivationGeneration = 0;
            expectedRestoredPublishedState.reset();
            return true;
        }

        if (published.hasFailedRequest)
        {
            projectRestoreCoordinator.publishLifecycleState(
                restore->generation,
                drs::engine::ProjectRestoreState::failed,
                drs::engine::ProjectRestoreFinding::preparationFailed,
                published.failureFinding.message.empty()
                    ? std::string("Restored Performance preparation failed.")
                    : published.failureFinding.message);
            awaitingRestoreActivationGeneration = 0;
            expectedRestoredPublishedState.reset();
            return true;
        }
        return false;
    }

    if (restore->generation == handledRestoreGeneration)
        return false;

    if (restore->state == drs::engine::ProjectRestoreState::failed)
    {
        handledRestoreGeneration = restore->generation;
        if (restore->expectedProjectId.empty())
            setPendingRestoreAudioPolicy(false);
        return true;
    }

    if (restore->state != drs::engine::ProjectRestoreState::ready)
        return false;

    if (restore->legacyPresetOnly && restore->legacyPreset.has_value())
    {
        const auto result = engineFacade.restorePresetStateJson(
            drs::engine::serializeRuntimePresetState(*restore->legacyPreset));
        handledRestoreGeneration = restore->generation;
        if (!result.restored)
        {
            projectRestoreCoordinator.publishLifecycleState(
                restore->generation,
                drs::engine::ProjectRestoreState::failed,
                drs::engine::ProjectRestoreFinding::invalidHostState,
                result.state);
            return true;
        }

        syncParametersFromEngine();
        setPendingRestoreAudioPolicy(false);
        projectRestoreCoordinator.publishLifecycleState(
            restore->generation,
            drs::engine::ProjectRestoreState::active,
            drs::engine::ProjectRestoreFinding::legacyUnboundProject,
            "Legacy preset restored without inferring a project");
        refreshSerializedHostStatePublication(true);
        return true;
    }

    handledRestoreGeneration = restore->generation;
    if (!applyValidatedProjectRestore(*restore))
    {
        projectRestoreCoordinator.publishLifecycleState(
            restore->generation,
            drs::engine::ProjectRestoreState::failed,
            drs::engine::ProjectRestoreFinding::checkpointInvalid,
            "The validated restore checkpoint could not be applied atomically.");
        return true;
    }

    if (!restore->hostState->publishedState.has_value())
    {
        projectRestoreCoordinator.publishLifecycleState(
            restore->generation,
            drs::engine::ProjectRestoreState::ready,
            {},
            "Authored project and preset restored; no published Performance checkpoint was saved");
    }
    return true;
}

bool Processor::serviceMessageThreadWork()
{
    auto servicedProjectRestore = serviceProjectRestore();
    const auto serviceTimeMicros = monotonicMicros();
    const auto authoringRevision = authoringSession.getDocumentState().revision;
    auto synchronizedDraftPlaybackProject = false;
    if (authoringRevision != observedDraftPlaybackProjectRevision)
    {
        auto draftPlaybackProject = authoringSession.getProject();
        if (engineFacade.replaceDraftPlaybackAuthoringProject(std::move(draftPlaybackProject)))
        {
            synchronizedDraftPlaybackProject = engineFacade.stageDraftRevision(authoringRevision);
            observedDraftPlaybackProjectRevision = authoringRevision;
        }
    }

    const auto servicedBackgroundWork = engineFacade.serviceBackgroundWork();
    const auto retiredCountBefore = diagnosticsRetiredActivationCount.load(std::memory_order_acquire);
    const auto reclaimedPreview = authoringPreviewPlaybackContext.serviceRetirements();
    const auto reclaimedPerformance = performancePlaybackContext.serviceRetirements();
    const auto reclaimed = reclaimedPreview + reclaimedPerformance;
    diagnosticsRetiredActivationCount.fetch_add(reclaimed, std::memory_order_relaxed);
    diagnosticsReclaimedActivationPayloadCount.fetch_add(reclaimed, std::memory_order_relaxed);

    auto synchronizedPerformancePublish = false;
    const auto publishControllerBeforeSynchronization
        = engineFacade.getPerformancePublishControllerSnapshot();
    if (pendingPerformanceActivation != nullptr
        && (!publishControllerBeforeSynchronization.hasRequest
            || publishControllerBeforeSynchronization.currentRequest.identity
                != pendingPerformanceActivation->requestIdentity
            || publishControllerBeforeSynchronization.activationState
                != drs::engine::PerformancePublishActivationState::pending
            || publishControllerBeforeSynchronization.pendingActivationToken
                != pendingPerformanceActivation->activationToken))
    {
        performancePlaybackContext.cancelPendingActivation();
        pendingPerformanceActivation.reset();
        synchronizedPerformancePublish = true;
    }
    const auto performanceContextBeforeSynchronization = performancePlaybackContext.getSnapshot();
    if (pendingPerformanceActivation != nullptr
        && performanceContextBeforeSynchronization.hasActiveActivation
        && !performanceContextBeforeSynchronization.hasPendingActivation
        && performanceContextBeforeSynchronization.activeRevision
            == pendingPerformanceActivation->revision
        && performanceContextBeforeSynchronization.activePreparedBuildId
            == pendingPerformanceActivation->preparedBuildId)
    {
        synchronizedPerformancePublish = engineFacade.acknowledgePerformanceActivation(
            pendingPerformanceActivation, serviceTimeMicros);
        if (synchronizedPerformancePublish)
            pendingPerformanceActivation.reset();
    }
    else if (pendingPerformanceActivation != nullptr
             && !performanceContextBeforeSynchronization.hasPendingActivation)
    {
        synchronizedPerformancePublish = engineFacade.rejectPerformanceActivationStaging(
            pendingPerformanceActivation,
            { drs::engine::PerformancePublishFindingSeverity::error,
              "performance-activation-apply-rejected",
              "performance.activationSlot",
              "The audio boundary rejected the authorized Performance activation payload." });
        pendingPerformanceActivation.reset();
    }

    auto synchronizedActivation = false;
    const auto stateRevision = engineFacade.getStateRevision();
    if (stateRevision != observedEngineStateRevision
        && (!pendingRestoreAudioSilence.load(std::memory_order_acquire)
            || awaitingRestoreActivationGeneration != 0))
    {
        observedEngineStateRevision = stateRevision;
        const auto performanceSnapshot = performancePlaybackContext.getSnapshot();
        synchronizedActivation = synchronizePerformanceActivation(
            !performanceSnapshot.hasActiveActivation);
    }

    const auto previewContextSnapshot = authoringPreviewPlaybackContext.getSnapshot();
    const auto& authoredProject = authoringSession.getProject();
    const auto selectedZone = authoringSession.getSelectedZone();
    const auto selectedGroup = authoringSession.getSelectedGroup();
    const auto selectedZoneId = selectedZone.has_value() ? selectedZone->id : std::string {};
    const auto selectedGroupId = selectedGroup.has_value() ? selectedGroup->id : std::string {};
    const auto requestedScope = authoringPreviewRequestedScope;
    const auto requestSelectedZoneId
        = authoringPreviewDirectAuditionRequested && !pendingAuthoringPreviewZoneId.empty()
            ? pendingAuthoringPreviewZoneId
            : (requestedScope == drs::engine::AuthoringPreviewScope::selectedZone
                   ? selectedZoneId
                   : (requestedScope == drs::engine::AuthoringPreviewScope::selectedGroup
                          ? authoringSession.buildSelectedGroupPreviewRequest().anchorZoneId
                          : std::string {}));
    const auto requestSelectedGroupId
        = authoringPreviewDirectAuditionRequested && !pendingAuthoringPreviewGroupId.empty()
            ? pendingAuthoringPreviewGroupId
            : (requestedScope == drs::engine::AuthoringPreviewScope::selectedGroup
                   ? selectedGroupId
                   : std::string {});
    const auto controllerBeforeRequest = authoringPreviewController.getSnapshot();
    const auto scopeChanged = controllerBeforeRequest.hasRequest
        && controllerBeforeRequest.currentRequest.identity.scope != requestedScope;
    const auto selectionChanged = controllerBeforeRequest.hasRequest
        && ((requestedScope == drs::engine::AuthoringPreviewScope::selectedZone
                && controllerBeforeRequest.currentRequest.identity.selectedZoneId
                    != requestSelectedZoneId)
            || (requestedScope == drs::engine::AuthoringPreviewScope::selectedGroup
                && (controllerBeforeRequest.currentRequest.identity.selectedGroupId
                        != requestSelectedGroupId
                    || controllerBeforeRequest.currentRequest.identity.selectedZoneId
                        != requestSelectedZoneId)));
    const auto requestReason = authoringPreviewDirectAuditionRequested
        ? (requestedScope == drs::engine::AuthoringPreviewScope::currentDraft
               ? drs::engine::AuthoringPreviewRequestReason::explicitCurrentDraftAudition
               : (requestedScope == drs::engine::AuthoringPreviewScope::selectedGroup
                      ? drs::engine::AuthoringPreviewRequestReason::explicitSelectedGroupAudition
                      : drs::engine::AuthoringPreviewRequestReason::explicitSelectedZoneAudition))
        : (!controllerBeforeRequest.hasRequest
        ? drs::engine::AuthoringPreviewRequestReason::projectOpened
        : (selectionChanged
               ? drs::engine::AuthoringPreviewRequestReason::selectionChanged
               : drs::engine::AuthoringPreviewRequestReason::authoringChanged));
    const auto observesNewRevision = !controllerBeforeRequest.hasRequest
        || controllerBeforeRequest.currentRequest.identity.draftRevision != authoringRevision;
    const auto invalidationCategory = scopeChanged
        ? drs::engine::AuthoringPreviewInvalidationCategory::previewScope
        : (observesNewRevision
        ? classifyPreviewInvalidation(authoringSession.getDocumentState().lastChangeLabel,
                                      selectionChanged)
        : controllerBeforeRequest.currentRequest.invalidationCategory);
    const auto authoredContentFingerprint
        = requestedScope == drs::engine::AuthoringPreviewScope::currentDraft
        ? buildCurrentDraftPreviewFingerprint(authoredProject)
        : (requestedScope == drs::engine::AuthoringPreviewScope::selectedGroup
               ? buildSelectedGroupPreviewFingerprint(authoredProject, selectedGroup)
               : buildSelectedZonePreviewFingerprint(authoredProject,
                                                    findProjectZone(authoredProject,
                                                                    requestSelectedZoneId)));
    const auto requestSignature = drs::engine::buildAuthoringPreviewRequestSignature(
        requestedScope,
        requestSelectedZoneId,
        invalidationCategory,
        authoredContentFingerprint,
        requestSelectedGroupId);
    const auto allowPassivePreviewPreparation
        = authoringPreviewDirectAuditionRequested || authoringPreviewPreparationAuthorized;
    auto requestResult = drs::engine::AuthoringPreviewRequestResult {};
    if (allowPassivePreviewPreparation)
    {
        requestResult = authoringPreviewController.request(
            requestedScope,
            authoringRevision,
            requestSelectedZoneId,
            requestReason,
            invalidationCategory,
            requestSignature,
            serviceTimeMicros,
            requestSelectedGroupId);
    }
    authoringPreviewDirectAuditionRequested = false;
    pendingAuthoringPreviewZoneId.clear();
    pendingAuthoringPreviewGroupId.clear();
    const auto canceledSupersededWork = requestResult.supersededPrevious
        && engineFacade.cancelPreviewPreparation();
    if (canceledSupersededWork && !requestResult.cancellationRequested)
        authoringPreviewController.recordWorkerCancellation();

    auto synchronizedAuthoringPreview = false;
    auto controllerSnapshot = authoringPreviewController.getSnapshot();
    if (controllerSnapshot.hasRequest
        && controllerSnapshot.activationState == drs::engine::AuthoringPreviewActivationState::pending
        && previewContextSnapshot.hasActiveActivation
        && !previewContextSnapshot.hasPendingActivation
        && previewContextSnapshot.activeRevision
            == controllerSnapshot.currentRequest.identity.draftRevision)
    {
        synchronizedAuthoringPreview = authoringPreviewController.markActive(
            controllerSnapshot.currentRequest.identity, serviceTimeMicros);
        controllerSnapshot = authoringPreviewController.getSnapshot();
    }
    const auto preparedPayload = engineFacade.getPreviewActivationPayload();
    const auto directAuditionContentPrepared = preparedPayload != nullptr
        && preparedPayload->revision == authoringRevision
        && preparedPayload->preparedBuildId != 0;
    const auto launch = authoringPreviewController.launchIfEligible(
        serviceTimeMicros, directAuditionContentPrepared);
    if (launch.launched)
    {
        if (!directAuditionContentPrepared && !engineFacade.refreshPreviewToCurrentDraft())
        {
            const auto& draftStatus = engineFacade.getDraftPlaybackStatus();
            if (!draftStatus.preview.findings.empty())
            {
                const auto& finding = draftStatus.preview.findings.front();
                const auto previewFinding = makePreviewFailureFinding(finding);
                authoringPreviewController.fail(launch.request.identity, previewFinding);
            }
            else
            {
                const auto previewFinding = drs::engine::classifyAuthoringPreviewFailure(
                    "preview-worker-request-rejected", "worker",
                    draftStatus.lastEvent.empty()
                        ? std::string("Preview worker request was rejected.")
                        : draftStatus.lastEvent);
                authoringPreviewController.fail(launch.request.identity, previewFinding);
            }
            synchronizedAuthoringPreview = true;
        }
        controllerSnapshot = authoringPreviewController.getSnapshot();
    }

    const auto preparedAfterLaunch = engineFacade.getPreviewActivationPayload();
    if (controllerSnapshot.hasRequest
        && controllerSnapshot.preparationState
            == drs::engine::AuthoringPreviewPreparationState::preparing
        && preparedAfterLaunch != nullptr
        && preparedAfterLaunch->revision == authoringRevision)
    {
        synchronizedAuthoringPreview = stageAuthoringPreviewActivation(
            controllerSnapshot.currentRequest,
            !previewContextSnapshot.hasActiveActivation) || synchronizedAuthoringPreview;
    }
    else if (controllerSnapshot.hasRequest
             && controllerSnapshot.preparationState
                 == drs::engine::AuthoringPreviewPreparationState::preparing)
    {
        const auto& draftStatus = engineFacade.getDraftPlaybackStatus();
        if (!draftStatus.pendingPreview.active && !draftStatus.preview.findings.empty())
        {
            const auto& finding = draftStatus.preview.findings.front();
            const auto previewFinding = makePreviewFailureFinding(finding);
            authoringPreviewController.fail(controllerSnapshot.currentRequest.identity,
                                             previewFinding);
            synchronizedAuthoringPreview = true;
        }
    }

    servicedProjectRestore = serviceProjectRestore() || servicedProjectRestore;
    refreshSerializedHostStatePublication();
    updateRealtimeSafetyState();
    publishAuthoringPreviewStatus();
    return servicedProjectRestore
        || servicedBackgroundWork
        || synchronizedDraftPlaybackProject
        || requestResult.accepted
        || requestResult.expeditedCurrent
        || canceledSupersededWork
        || synchronizedPerformancePublish
        || synchronizedAuthoringPreview
        || synchronizedActivation
        || diagnosticsRetiredActivationCount.load(std::memory_order_acquire) != retiredCountBefore;
}

juce::String Processor::buildMacroParameterId(const std::string& macroId)
{
    return "macro." + juce::String::fromUTF8(macroId.c_str());
}

juce::AudioProcessorValueTreeState::ParameterLayout Processor::buildParameterLayout(
    const drs::engine::EngineFacade& facade)
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    for (const auto& macro : facade.getMacroDescriptors())
    {
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID(buildMacroParameterId(macro.id), 1),
            juce::String::fromUTF8(macro.name.c_str()),
            juce::NormalisableRange<float>(static_cast<float>(macro.minValue),
                                           static_cast<float>(macro.maxValue)),
            static_cast<float>(macro.defaultValue)));
    }

    return layout;
}

void Processor::parameterChanged(const juce::String& parameterID, float newValue)
{
    if (isSynchronizingParameterState)
        return;

    const auto slot = std::find(hostMacroParameterIds.begin(), hostMacroParameterIds.end(), parameterID);
    if (slot == hostMacroParameterIds.end())
        return;

    const auto slotIndex = static_cast<std::size_t>(std::distance(hostMacroParameterIds.begin(), slot));
    hostMacroValues[slotIndex].store(newValue, std::memory_order_relaxed);
    hostMacroValueSequences[slotIndex].fetch_add(1, std::memory_order_release);

    engineFacade.setMacroValue(hostMacroStableIds[slotIndex], static_cast<double>(newValue));
}

void Processor::initializePublishedMacroRealtimeState()
{
    const auto macros = engineFacade.getMacroDescriptors();
    hostMacroParameterIds.reserve(std::min(macros.size(), maxPublishedMacroSlots));
    hostMacroStableIds.reserve(std::min(macros.size(), maxPublishedMacroSlots));
    activePublishedMacroCallbackView.hostSlotCount
        = std::min(macros.size(), maxPublishedMacroSlots);
    for (std::size_t index = 0; index < activePublishedMacroCallbackView.hostSlotCount; ++index)
    {
        const auto& macro = macros[index];
        hostMacroParameterIds.push_back(buildMacroParameterId(macro.id));
        hostMacroStableIds.push_back(macro.id);
        hostMacroValues[index].store(static_cast<float>(macro.currentValue), std::memory_order_relaxed);
        hostMacroValueSequences[index].store(1, std::memory_order_relaxed);
        auto& slot = activePublishedMacroCallbackView.slots[index];
        slot.assigned = true;
        slot.minValue = macro.minValue;
        slot.maxValue = macro.maxValue;
        slot.publishedValue = macro.currentValue;
        if (macro.id == "tone")
            slot.renderTarget = drs::engine::PublishedMacroRenderTarget::toneVelocity;
        else if (macro.id == "motion")
            slot.renderTarget = drs::engine::PublishedMacroRenderTarget::motionPitch;
    }
}

void Processor::installPublishedMacroBindings(
    const drs::engine::ImmutablePublishedMacroBindingTable& bindings) noexcept
{
    const auto& view = bindings.callbackView;
    activePublishedMacroCallbackView = view;
    diagnosticActivePublishedMacroRevision.store(view.revision, std::memory_order_relaxed);
    for (std::size_t index = 0; index < maxPublishedMacroSlots; ++index)
    {
        activePublishedMacroBaselines[index]
            = hostMacroValueSequences[index].load(std::memory_order_acquire);
    }
}

drs::engine::SamplerRenderControlValues Processor::buildPublishedMacroRenderControls() noexcept
{
    drs::engine::SamplerRenderControlValues controls;
    const auto slotCount = std::min(activePublishedMacroCallbackView.hostSlotCount,
                                    maxPublishedMacroSlots);
    for (std::size_t index = 0; index < slotCount; ++index)
    {
        const auto& slot = activePublishedMacroCallbackView.slots[index];
        if (!slot.assigned)
            continue;

        auto value = slot.publishedValue;
        if (hostMacroValueSequences[index].load(std::memory_order_acquire)
            > activePublishedMacroBaselines[index])
        {
            value = hostMacroValues[index].load(std::memory_order_relaxed);
        }
        value = std::clamp(value, slot.minValue, slot.maxValue);
        if (slot.renderTarget == drs::engine::PublishedMacroRenderTarget::toneVelocity)
        {
            controls.overrideFixedVelocity = true;
            controls.fixedVelocity = std::clamp(
                static_cast<int>(std::lround(32.0 + value * 95.0)), 1, 127);
        }
        else if (slot.renderTarget == drs::engine::PublishedMacroRenderTarget::motionPitch)
        {
            controls.overrideMidiNoteOffset = true;
            controls.midiNoteOffset = static_cast<int>(std::lround((value - 0.5) * 24.0));
        }
        else if (slot.renderTarget == drs::engine::PublishedMacroRenderTarget::dspControl)
        {
            performancePlaybackContext.publishActiveDspControl(
                slot.dspControlIndex, mapPublishedDspMacroValue(slot, value));
        }
    }
    return controls;
}

void Processor::syncEngineFromParameters()
{
    for (const auto& macro : engineFacade.getMacroDescriptors())
    {
        auto parameterId = buildMacroParameterId(macro.id);
        if (parameterState.getParameter(parameterId) == nullptr)
        {
            if (const auto publishedParameterId = findPublishedHostParameterId(
                    engineFacade.getActivePublishedMacroBindings(), macro.id);
                publishedParameterId.has_value())
            {
                parameterId = *publishedParameterId;
            }
        }

        if (auto* rawValue = parameterState.getRawParameterValue(parameterId))
            engineFacade.setMacroValue(macro.id, static_cast<double>(rawValue->load()));
    }
}

void Processor::syncParametersFromEngine()
{
    const juce::ScopedValueSetter<bool> syncGuard(isSynchronizingParameterState, true);

    const auto macros = engineFacade.getMacroDescriptors();
    for (std::size_t index = 0; index < macros.size(); ++index)
    {
        const auto& macro = macros[index];
        auto parameterId = buildMacroParameterId(macro.id);
        if (parameterState.getParameter(parameterId) == nullptr)
        {
            if (const auto publishedParameterId = findPublishedHostParameterId(
                    engineFacade.getActivePublishedMacroBindings(), macro.id);
                publishedParameterId.has_value())
            {
                parameterId = *publishedParameterId;
            }
        }

        if (auto* parameter = dynamic_cast<juce::RangedAudioParameter*>(
                parameterState.getParameter(parameterId)))
        {
            parameter->setValueNotifyingHost(parameter->convertTo0to1(static_cast<float>(macro.currentValue)));
        }
        if (index < maxPublishedMacroSlots)
        {
            hostMacroValues[index].store(static_cast<float>(macro.currentValue),
                                         std::memory_order_relaxed);
            hostMacroValueSequences[index].fetch_add(1, std::memory_order_release);
        }
    }
}

drs::app::AuthoringWaveformPreview Processor::buildAuthoringWaveformPreview(
    const drs::engine::WaveformPeakBuildResult& waveform,
    const bool loopEnabled,
    const std::uint64_t loopStartFrame,
    const std::uint64_t loopEndFrame) const
{
    drs::app::AuthoringWaveformPreview preview;
    preview.available = true;
    preview.state = "Ready";
    preview.sourcePath = waveform.metadata.sourcePath;
    preview.formatName = waveform.metadata.formatName;
    preview.durationSeconds = waveform.metadata.durationSeconds;
    preview.sampleRate = waveform.metadata.sampleRate;
    preview.frameCount = waveform.metadata.frameCount;
    preview.channelCount = waveform.metadata.channelCount;
    preview.loopEnabled = loopEnabled;
    preview.loopStartFrame = loopStartFrame;
    preview.loopEndFrame = loopEndFrame;
    preview.points.reserve(waveform.points.size());
    for (const auto& point : waveform.points)
        preview.points.push_back({ point.minValue, point.maxValue });
    return preview;
}

void Processor::consumeAuthoringWaveformPreviewSnapshot()
{
    const auto snapshot = waveformPreviewService.getSnapshot();
    if (snapshot == nullptr || snapshot->identity.generation == 0)
    {
        return;
    }

    if (isWaveformPreviewServiceActiveStage(snapshot->stage)
        || snapshot->stage == drs::app::WaveformPreviewServiceStage::idle
        || snapshot->identity.generation == authoringWaveformPreviewConsumedGeneration)
    {
        return;
    }

    authoringWaveformPreviewConsumedGeneration = snapshot->identity.generation;
    if (snapshot->stage != drs::app::WaveformPreviewServiceStage::completed || snapshot->result == nullptr)
        return;

    WaveformPreviewCacheEntry cacheEntry;
    cacheEntry.sampleSourceId = snapshot->identity.sampleSourceId;
    cacheEntry.sourcePath = snapshot->identity.sourcePath;
    cacheEntry.sourceFileSizeBytes = snapshot->identity.sourceFileSizeBytes;
    cacheEntry.sourceModificationTicks = snapshot->identity.sourceModificationTicks;
    cacheEntry.fingerprintHex = snapshot->result->metadata.sourceChecksumHex;
    cacheEntry.displayPointCount = snapshot->identity.displayPointCount;
    cacheEntry.channelReduction = snapshot->identity.channelReduction;
    cacheEntry.requestStamp = snapshot->identity.requestStamp;
    cacheEntry.preview = buildAuthoringWaveformPreview(*snapshot->result, false, 0, 0);
    authoringWaveformPreviewCache[cacheEntry.requestStamp] = cacheEntry;
    authoringWaveformPreviewLatestStampBySourceId[cacheEntry.sampleSourceId] = cacheEntry.requestStamp;
}

bool Processor::describeAuthoringWaveformPreviewSource(
    const drs::engine::RuntimeProjectSampleSource& sampleSource,
    std::uint64_t& fileSizeBytes,
    std::int64_t& modificationTicks) const
{
    fileSizeBytes = 0;
    modificationTicks = 0;

    const auto sourceFile = juce::File(juce::String::fromUTF8(sampleSource.path.c_str()));
    if (!sourceFile.existsAsFile())
        return false;

    fileSizeBytes = static_cast<std::uint64_t>(std::max<juce::int64>(0, sourceFile.getSize()));
    modificationTicks = sourceFile.getLastModificationTime().toMilliseconds();
    return true;
}

std::string Processor::buildAuthoringWaveformPreviewRequestStamp(
    const drs::engine::RuntimeProjectSampleSource& sampleSource,
    const std::uint64_t fileSizeBytes,
    const std::int64_t modificationTicks) const
{
    std::ostringstream stamp;
    stamp << sampleSource.id
          << "|" << sampleSource.path
          << "|size=" << fileSizeBytes
          << "|mtime=" << modificationTicks
          << "|points=" << authoringWaveformPreviewPointCount
          << "|policy=" << static_cast<int>(authoringWaveformPreviewChannelReduction);
    return stamp.str();
}

const Processor::WaveformPreviewCacheEntry* Processor::findAuthoringWaveformPreviewCacheEntryForStamp(
    const std::string& requestStamp) const
{
    const auto iterator = authoringWaveformPreviewCache.find(requestStamp);
    return iterator == authoringWaveformPreviewCache.end() ? nullptr : &iterator->second;
}

const Processor::WaveformPreviewCacheEntry* Processor::findLatestAuthoringWaveformPreviewCacheEntryForSource(
    const std::string& sampleSourceId) const
{
    const auto iterator = authoringWaveformPreviewLatestStampBySourceId.find(sampleSourceId);
    if (iterator == authoringWaveformPreviewLatestStampBySourceId.end())
        return nullptr;

    return findAuthoringWaveformPreviewCacheEntryForStamp(iterator->second);
}

void Processor::clearAuthoringWaveformPreviewCache()
{
    waveformPreviewService.cancel("Waveform preview cleared");
    if (const auto snapshot = waveformPreviewService.getSnapshot(); snapshot != nullptr)
        authoringWaveformPreviewConsumedGeneration = snapshot->identity.generation;
    authoringWaveformPreviewCache.clear();
    authoringWaveformPreviewLatestStampBySourceId.clear();
    authoringWaveformPreviewCurrentStampBySourceId.clear();
}

void Processor::resetAuthoringPreviewPreparationAuthorization() noexcept
{
    authoringPreviewPreparationAuthorized = false;
}

void Processor::resetAuthoringWaveformPreviewAuthorization() noexcept
{
    authoringWaveformPreviewLoadAuthorized = false;
}

void Processor::initializeAuthoringImportMetrics()
{
    const auto& project = authoringSession.getProject();
    authoringImportResponsivenessSnapshot.available = true;
    authoringImportResponsivenessSnapshot.state = project.sampleSources.empty() ? "idle" : "not-run";
    authoringImportResponsivenessSnapshot.totalItemCount = project.sampleSources.size();
    authoringImportResponsivenessSnapshot.pendingCount = 0;
    authoringImportResponsivenessSnapshot.processedCount = 0;
    authoringImportResponsivenessSnapshot.warningItemCount = 0;
    authoringImportResponsivenessSnapshot.failedItemCount = 0;
    authoringImportResponsivenessSnapshot.canceledItemCount = 0;
    authoringImportResponsivenessSnapshot.acceptedItemCount = 0;
    authoringImportResponsivenessSnapshot.lastProcessDurationMicros = 0;
    authoringImportResponsivenessSnapshot.averageProcessDurationMicros = 0;
    authoringImportResponsivenessSnapshot.maxProcessDurationMicros = 0;
    authoringImportResponsivenessSnapshot.lastProcessedItemId.clear();
}

void Processor::initializeAuthoringSourceValidationSnapshot()
{
    const auto& project = authoringSession.getProject();
    authoringSourceValidationSnapshot.available = true;
    authoringSourceValidationSnapshot.state = project.sampleSources.empty() ? "idle" : "not-run";
    authoringSourceValidationSnapshot.totalItemCount = project.sampleSources.size();
    authoringSourceValidationSnapshot.processedCount = 0;
    authoringSourceValidationSnapshot.warningItemCount = 0;
    authoringSourceValidationSnapshot.failedItemCount = 0;
    authoringSourceValidationSnapshot.canceledItemCount = 0;
    authoringSourceValidationSnapshot.totalBytesProcessed = 0;
    authoringSourceValidationSnapshot.totalBytesExpected = 0;
    authoringSourceValidationSnapshot.totalDurationMicros = 0;
    authoringSourceValidationSnapshot.currentSourceId.clear();
    authoringSourceValidationSnapshot.currentSourcePath.clear();
}

bool Processor::stageAuthoringPreviewActivation(const drs::engine::AuthoringPreviewRequest& request,
                                                bool installImmediately)
{
    const auto reclaimed = authoringPreviewPlaybackContext.serviceRetirements();
    diagnosticsRetiredActivationCount.fetch_add(reclaimed, std::memory_order_relaxed);
    diagnosticsReclaimedActivationPayloadCount.fetch_add(reclaimed, std::memory_order_relaxed);
    const auto currentRevision = authoringSession.getDocumentState().revision;
    const auto failPreviewActivation = [&](drs::engine::AuthoringPreviewFailureFinding finding)
    {
        authoringPreviewController.fail(request.identity, std::move(finding));
        return false;
    };

    if (!authoringPreviewController.isCurrent(request.identity)
        || request.identity.draftRevision != currentRevision)
        return false;

    const auto payload = engineFacade.getPreviewActivationPayload();
    const auto preparation = drs::engine::prepareAuthoringPreviewRenderModel(payload, request);
    if (!preparation.prepared || preparation.model == nullptr)
    {
        if (preparation.findings.empty())
            return failPreviewActivation(drs::engine::classifyAuthoringPreviewFailure(
                "preview-preparation-finding-missing", "preparation",
                "Authoring Preview preparation failed without a finding."));
        return failPreviewActivation(makePreviewFailureFinding(preparation.findings.front()));
    }
    if (!authoringPreviewController.acceptPrepared(request.identity,
                                                    preparation.scopedPayload->preparedBuildId,
                                                    monotonicMicros(),
                                                    preparation.scopedPayload->snapshotContentDigest,
                                                    preparation.scopedPayload->preparedContentDigest))
        return false;
    std::shared_ptr<drs::engine::DspRenderGeneration> dspGeneration;
    if (requestsExecutableCuratedDsp(*preparation.scopedPayload->snapshot))
    {
        const auto graphPlan = drs::engine::compileDspGraphPlan(*preparation.scopedPayload->snapshot);
        if (!graphPlan.compiled)
            return failPreviewActivation(drs::engine::classifyAuthoringPreviewFailure(
                "preview-dsp-graph-rejected", "preview.dspGraph",
                graphPlan.findings.empty() ? "Preview DSP graph compilation failed."
                                          : graphPlan.findings.front().message));
        std::string dspFailure;
        dspGeneration = drs::engine::createDspRenderGeneration(
            preparation.model,
            graphPlan.plan,
            static_cast<std::uint32_t>(std::max<std::size_t>(
                1, diagnosticsPreparedBlockSize.load(std::memory_order_acquire))),
            &dspFailure);
        if (dspGeneration == nullptr)
            return failPreviewActivation(drs::engine::classifyAuthoringPreviewFailure(
                "preview-dsp-generation-rejected", "preview.dspGeneration", dspFailure));
    }
    if (!authoringPreviewPlaybackContext.stageActivation(preparation.model, dspGeneration))
        return failPreviewActivation(drs::engine::classifyAuthoringPreviewFailure(
            "preview-activation-slot-exhausted", "preview.activationSlots",
            "Authoring Preview activation slots are exhausted."));
    if (!authoringPreviewController.markActivationPending(request.identity, monotonicMicros()))
        return false;

    if (installImmediately && authoringPreviewPlaybackContext.activatePendingForPreparation())
    {
        diagnosticsAuthoringPreviewActivationCount.fetch_add(1, std::memory_order_relaxed);
        authoringPreviewController.markActive(request.identity, monotonicMicros());
    }
    return true;
}

bool Processor::synchronizePerformanceActivation(bool installImmediately)
{
    const auto reclaimed = performancePlaybackContext.serviceRetirements();
    diagnosticsRetiredActivationCount.fetch_add(reclaimed, std::memory_order_relaxed);
    diagnosticsReclaimedActivationPayloadCount.fetch_add(reclaimed, std::memory_order_relaxed);
    if (pendingPerformanceActivation != nullptr)
        return false;

    const auto performanceSnapshot = performancePlaybackContext.getSnapshot();
    auto authorized = engineFacade.authorizePerformanceActivation(monotonicMicros());
    auto payload = authorized != nullptr
        ? authorized->playbackPayload
        : drs::engine::PlaybackActivationPayloadPtr {};
    const auto bootstrap = !performanceSnapshot.hasActiveActivation
        && (authorized == nullptr
            || authorized->requestIdentity.origin
                == drs::engine::PerformancePublishRequestOrigin::bootstrap);
    if (bootstrap && authorized == nullptr)
        payload = engineFacade.getBootstrapPerformanceActivationPayload();
    if (payload == nullptr)
        return false;

    const auto& sessionState = engineFacade.getCurrentSessionState();

    drs::engine::SamplerRenderModelBuildOptions options;
    options.selectedArticulationId = sessionState.selectedArticulationId;
    const auto& authoredRoutes = payload->snapshot->articulationRoutes;
    const auto containsAuthoredArticulation = [&](const std::string& articulationId)
    {
        return !articulationId.empty()
            && std::any_of(authoredRoutes.begin(), authoredRoutes.end(), [&](const auto& route)
            {
                return route.articulationId == articulationId && !route.zoneIds.empty();
            });
    };
    if (!containsAuthoredArticulation(options.selectedArticulationId))
    {
        const auto authoredDefault = std::find_if(authoredRoutes.begin(), authoredRoutes.end(),
                                                  [](const auto& route)
                                                  {
                                                      return route.articulationId == "default"
                                                          && !route.zoneIds.empty();
                                                  });
        const auto authoredFallback = authoredDefault != authoredRoutes.end()
            ? authoredDefault
            : std::find_if(authoredRoutes.begin(), authoredRoutes.end(), [](const auto& route)
            {
                return !route.articulationId.empty() && !route.zoneIds.empty();
            });
        options.selectedArticulationId = authoredFallback != authoredRoutes.end()
            ? authoredFallback->articulationId : std::string {};
    }
    if (authorized != nullptr && authorized->macroBindings != nullptr)
    {
        for (const auto& slot : authorized->macroBindings->callbackView.slots)
        {
            if (!slot.assigned)
                continue;
            const auto value = std::clamp(slot.publishedValue, slot.minValue, slot.maxValue);
            if (slot.renderTarget == drs::engine::PublishedMacroRenderTarget::toneVelocity)
                options.fixedVelocity = std::clamp(
                    static_cast<int>(std::lround(32.0 + value * 95.0)), 1, 127);
            else if (slot.renderTarget == drs::engine::PublishedMacroRenderTarget::motionPitch)
                options.midiNoteOffset = static_cast<int>(std::lround((value - 0.5) * 24.0));
        }
    }
    else
    {
        options.midiNoteOffset = computeMotionRenderNote(sessionState, 60) - 60;
        options.fixedVelocity = computeToneRenderVelocity(sessionState);
    }
    const auto modelResult = drs::engine::buildSamplerRenderModel(payload, options);
    if (!modelResult.built || modelResult.model == nullptr)
    {
        if (authorized != nullptr)
        {
            auto finding = drs::engine::PerformancePublishFinding {
                drs::engine::PerformancePublishFindingSeverity::error,
                "performance-render-model-rejected",
                "performance.renderModel",
                "The controller-authorized Performance payload could not build a render model."
            };
            if (!modelResult.findings.empty())
            {
                const auto& modelFinding = modelResult.findings.front();
                finding.code = modelFinding.code;
                finding.path = modelFinding.path;
                finding.message = modelFinding.message;
            }
            engineFacade.rejectPerformanceActivationStaging(
                authorized,
                std::move(finding));
        }
        return false;
    }
    std::shared_ptr<drs::engine::DspRenderGeneration> dspGeneration;
    if (requestsExecutableCuratedDsp(*payload->snapshot))
    {
        const auto graphPlan = drs::engine::compileDspGraphPlan(*payload->snapshot);
        if (!graphPlan.compiled)
        {
            if (authorized != nullptr)
                engineFacade.rejectPerformanceActivationStaging(
                    authorized,
                    { drs::engine::PerformancePublishFindingSeverity::error,
                      "performance-dsp-graph-rejected",
                      "performance.dspGraph",
                      graphPlan.findings.empty() ? "Performance DSP graph compilation failed."
                                                : graphPlan.findings.front().message });
            return false;
        }
        std::string dspFailure;
        dspGeneration = drs::engine::createDspRenderGeneration(
            modelResult.model,
            graphPlan.plan,
            static_cast<std::uint32_t>(std::max<std::size_t>(
                1, diagnosticsPreparedBlockSize.load(std::memory_order_acquire))),
            &dspFailure);
        if (dspGeneration == nullptr)
        {
            if (authorized != nullptr)
                engineFacade.rejectPerformanceActivationStaging(
                    authorized,
                    { drs::engine::PerformancePublishFindingSeverity::error,
                      "performance-dsp-generation-rejected",
                      "performance.dspGeneration",
                      dspFailure });
            return false;
        }
        if (authorized != nullptr && authorized->macroBindings != nullptr)
        {
            for (const auto& slot : authorized->macroBindings->callbackView.slots)
            {
                if (slot.assigned
                    && slot.renderTarget == drs::engine::PublishedMacroRenderTarget::dspControl)
                {
                    dspGeneration->publishControlValue(
                        dspGeneration->getControlGenerationIdentity(), slot.dspControlIndex,
                        mapPublishedDspMacroValue(slot, std::clamp(
                            slot.publishedValue, slot.minValue, slot.maxValue)));
                }
            }
        }
    }
    if (!performancePlaybackContext.stageActivation(modelResult.model, dspGeneration))
    {
        if (authorized != nullptr)
            engineFacade.rejectPerformanceActivationStaging(
                authorized,
                { drs::engine::PerformancePublishFindingSeverity::error,
                  "performance-activation-slot-rejected",
                  "performance.activationSlots",
                  "The bounded Performance activation slots rejected the authorized payload." });
        return false;
    }

    pendingPerformanceActivation = authorized;

    if (installImmediately && bootstrap
        && performancePlaybackContext.activatePendingForPreparation())
    {
        diagnosticsPerformanceActivationCount.fetch_add(1, std::memory_order_relaxed);
        if (pendingPerformanceActivation != nullptr)
        {
            engineFacade.acknowledgePerformanceActivation(
                pendingPerformanceActivation, monotonicMicros());
            pendingPerformanceActivation.reset();
        }
    }
    return true;
}

void Processor::drainRealtimeNoteEvents(RealtimeNoteEventQueue& queue,
                                        drs::engine::SamplerEventBlock& destination,
                                        std::uint32_t frameCount) noexcept
{
    QueuedRealtimeNoteEvent event;
    while (queue.pop(event))
    {
        const auto sampleOffset = frameCount == 0
            ? 0u
            : std::min(event.sampleOffset, frameCount - 1u);
        destination.push({ event.type,
                           sampleOffset,
                           static_cast<std::uint8_t>(clampMidiValue(event.midiNoteNumber)),
                           event.type == drs::engine::SamplerRenderEventType::noteOn
                               ? std::max(event.velocity, 1.0f / 127.0f)
                               : 0.0f });
    }
}

void Processor::primeRealtimeSafetyState(int samplesPerBlock)
{
    if (!performancePlaybackContext.getSnapshot().prepared)
        performancePlaybackContext.prepare(currentSampleRate);
    if (!authoringPreviewPlaybackContext.getSnapshot().prepared)
        authoringPreviewPlaybackContext.prepare(currentSampleRate);
    diagnosticsPreparedBlockSize.store(static_cast<std::size_t>(samplesPerBlock), std::memory_order_release);
    diagnosticsActiveVoiceCapacityLimit.store(maxRealtimeActiveVoices * 2, std::memory_order_release);
    diagnosticsPrimedActiveVoiceCapacity.store(maxRealtimeActiveVoices * 2, std::memory_order_release);
    updateRealtimeSafetyState();
}

void Processor::updateRealtimeSafetyState()
{
    if (isCurrentThreadRealtimeAudio())
    {
        publishAudioDiagnostics();
        return;
    }

    publishMessageDiagnostics();
}

Processor::AudioDiagnosticsValues Processor::captureActivationDiagnostics() const
{
    AudioDiagnosticsValues values;
    const auto preview = authoringPreviewPlaybackContext.getSnapshot();
    const auto performance = performancePlaybackContext.getSnapshot();
    values.hasActiveAuthoringPreviewActivation = preview.hasActiveActivation;
    values.hasPendingAuthoringPreviewActivation = preview.hasPendingActivation;
    values.hasActivePerformanceActivation = performance.hasActiveActivation;
    values.hasPendingPerformanceActivation = performance.hasPendingActivation;
    values.activeAuthoringPreviewRevision = preview.activeRevision;
    values.pendingAuthoringPreviewRevision = preview.pendingRevision;
    values.activePublishedRevision = performance.activeRevision;
    values.pendingPublishedRevision = performance.pendingRevision;
    values.activePreparedBuildId = performance.activePreparedBuildId;
    values.pendingPreparedBuildId = performance.pendingPreparedBuildId;
    values.retiredActivationBacklog = preview.retiredActivationBacklog
        + performance.retiredActivationBacklog;
    values.activeActivationPayloadBytes = preview.activeActivationPayloadBytes
        + performance.activeActivationPayloadBytes;
    values.pendingActivationPayloadBytes = preview.pendingActivationPayloadBytes
        + performance.pendingActivationPayloadBytes;
    values.retiredActivationPayloadBytes = preview.retiredActivationPayloadBytes
        + performance.retiredActivationPayloadBytes;
    values.lastActivationReclamationLatencyBlocks = std::max(
        preview.counters.lastReclamationLatencyBlocks,
        performance.counters.lastReclamationLatencyBlocks);
    values.maxActivationReclamationLatencyBlocks = std::max(
        preview.counters.maxReclamationLatencyBlocks,
        performance.counters.maxReclamationLatencyBlocks);
    values.performanceContextIdentity = static_cast<std::uint32_t>(performance.lane) + 1u;
    values.authoringPreviewContextIdentity = static_cast<std::uint32_t>(preview.lane) + 1u;
    values.performanceVoiceStealCount = performance.counters.stolenVoiceCount;
    values.performanceGenerationStealCount = performance.counters.generationStealCount;
    values.performanceReleasingVoiceStealCount = performance.counters.releasingVoiceStealCount;
    values.performanceActiveGeneration = performance.activeActivationGeneration;
    values.performanceActiveGenerationVoiceCount = performance.activeGenerationVoiceCount;
    values.performanceRetiredGenerationVoiceCount = performance.retiredGenerationVoiceCount;
    values.performanceSustainDeferredVoiceCount = performance.sustainDeferredVoiceCount;
    values.authoringPreviewVoiceStealCount = preview.counters.stolenVoiceCount;
    values.performanceDroppedEventCount = performance.counters.droppedEventCount
        + diagnosticsPerformanceDroppedEventCount.load(std::memory_order_relaxed);
    values.authoringPreviewDroppedEventCount = preview.counters.droppedEventCount
        + diagnosticsAuthoringPreviewDroppedEventCount.load(std::memory_order_relaxed);
    values.performanceDroppedNoteCount = diagnosticsPerformanceDroppedNoteCount.load(std::memory_order_relaxed);
    values.authoringPreviewDroppedNoteCount = diagnosticsAuthoringPreviewDroppedNoteCount.load(std::memory_order_relaxed);

    return values;
}

void Processor::publishAudioDiagnostics()
{
    auto values = captureActivationDiagnostics();
    const auto performance = performancePlaybackContext.getSnapshot();
    const auto preview = authoringPreviewPlaybackContext.getSnapshot();
    values.performanceActiveVoiceCount = performance.activeVoiceCount + performance.releasingVoiceCount;
    values.authoringPreviewActiveVoiceCount = preview.activeVoiceCount + preview.releasingVoiceCount;
    values.activeVoiceCapacity = maxRealtimeActiveVoices * 2;
    values.lastPerformanceRenderMicros = lastPerformanceRenderMicros;
    values.maxPerformanceRenderMicros = maxPerformanceRenderMicros;
    values.lastAuthoringPreviewRenderMicros = lastAuthoringPreviewRenderMicros;
    values.maxAuthoringPreviewRenderMicros = maxAuthoringPreviewRenderMicros;
    values.performancePeakActiveVoiceCount = performancePeakActiveVoiceCount;
    values.performancePeakReleasingVoiceCount = performancePeakReleasingVoiceCount;
    values.authoringPreviewPeakActiveVoiceCount = authoringPreviewPeakActiveVoiceCount;
    values.authoringPreviewPeakReleasingVoiceCount = authoringPreviewPeakReleasingVoiceCount;

    auto sequence = audioDiagnosticsPublication.sequence.fetch_add(1, std::memory_order_acq_rel) + 1;
    audioDiagnosticsPublication.performanceActiveVoiceCount.store(values.performanceActiveVoiceCount, std::memory_order_relaxed);
    audioDiagnosticsPublication.authoringPreviewActiveVoiceCount.store(values.authoringPreviewActiveVoiceCount, std::memory_order_relaxed);
    audioDiagnosticsPublication.activeVoiceCapacity.store(values.activeVoiceCapacity, std::memory_order_relaxed);
    audioDiagnosticsPublication.performanceContextIdentity.store(values.performanceContextIdentity, std::memory_order_relaxed);
    audioDiagnosticsPublication.authoringPreviewContextIdentity.store(values.authoringPreviewContextIdentity, std::memory_order_relaxed);
    audioDiagnosticsPublication.lastPerformanceRenderMicros.store(values.lastPerformanceRenderMicros, std::memory_order_relaxed);
    audioDiagnosticsPublication.maxPerformanceRenderMicros.store(values.maxPerformanceRenderMicros, std::memory_order_relaxed);
    audioDiagnosticsPublication.lastAuthoringPreviewRenderMicros.store(values.lastAuthoringPreviewRenderMicros, std::memory_order_relaxed);
    audioDiagnosticsPublication.maxAuthoringPreviewRenderMicros.store(values.maxAuthoringPreviewRenderMicros, std::memory_order_relaxed);
    audioDiagnosticsPublication.performancePeakActiveVoiceCount.store(values.performancePeakActiveVoiceCount, std::memory_order_relaxed);
    audioDiagnosticsPublication.performancePeakReleasingVoiceCount.store(values.performancePeakReleasingVoiceCount, std::memory_order_relaxed);
    audioDiagnosticsPublication.authoringPreviewPeakActiveVoiceCount.store(values.authoringPreviewPeakActiveVoiceCount, std::memory_order_relaxed);
    audioDiagnosticsPublication.authoringPreviewPeakReleasingVoiceCount.store(values.authoringPreviewPeakReleasingVoiceCount, std::memory_order_relaxed);
    audioDiagnosticsPublication.performanceVoiceStealCount.store(values.performanceVoiceStealCount, std::memory_order_relaxed);
    audioDiagnosticsPublication.performanceGenerationStealCount.store(values.performanceGenerationStealCount, std::memory_order_relaxed);
    audioDiagnosticsPublication.performanceReleasingVoiceStealCount.store(values.performanceReleasingVoiceStealCount, std::memory_order_relaxed);
    audioDiagnosticsPublication.performanceActiveGeneration.store(values.performanceActiveGeneration, std::memory_order_relaxed);
    audioDiagnosticsPublication.performanceActiveGenerationVoiceCount.store(values.performanceActiveGenerationVoiceCount, std::memory_order_relaxed);
    audioDiagnosticsPublication.performanceRetiredGenerationVoiceCount.store(values.performanceRetiredGenerationVoiceCount, std::memory_order_relaxed);
    audioDiagnosticsPublication.performanceSustainDeferredVoiceCount.store(values.performanceSustainDeferredVoiceCount, std::memory_order_relaxed);
    audioDiagnosticsPublication.authoringPreviewVoiceStealCount.store(values.authoringPreviewVoiceStealCount, std::memory_order_relaxed);
    audioDiagnosticsPublication.performanceDroppedEventCount.store(values.performanceDroppedEventCount, std::memory_order_relaxed);
    audioDiagnosticsPublication.authoringPreviewDroppedEventCount.store(values.authoringPreviewDroppedEventCount, std::memory_order_relaxed);
    audioDiagnosticsPublication.performanceDroppedNoteCount.store(values.performanceDroppedNoteCount, std::memory_order_relaxed);
    audioDiagnosticsPublication.authoringPreviewDroppedNoteCount.store(values.authoringPreviewDroppedNoteCount, std::memory_order_relaxed);
    audioDiagnosticsPublication.activeAuthoringPreviewRevision.store(values.activeAuthoringPreviewRevision, std::memory_order_relaxed);
    audioDiagnosticsPublication.pendingAuthoringPreviewRevision.store(values.pendingAuthoringPreviewRevision, std::memory_order_relaxed);
    audioDiagnosticsPublication.activePublishedRevision.store(values.activePublishedRevision, std::memory_order_relaxed);
    audioDiagnosticsPublication.pendingPublishedRevision.store(values.pendingPublishedRevision, std::memory_order_relaxed);
    audioDiagnosticsPublication.activePreparedBuildId.store(values.activePreparedBuildId, std::memory_order_relaxed);
    audioDiagnosticsPublication.pendingPreparedBuildId.store(values.pendingPreparedBuildId, std::memory_order_relaxed);
    audioDiagnosticsPublication.retiredActivationBacklog.store(values.retiredActivationBacklog, std::memory_order_relaxed);
    audioDiagnosticsPublication.activeActivationPayloadBytes.store(values.activeActivationPayloadBytes, std::memory_order_relaxed);
    audioDiagnosticsPublication.pendingActivationPayloadBytes.store(values.pendingActivationPayloadBytes, std::memory_order_relaxed);
    audioDiagnosticsPublication.retiredActivationPayloadBytes.store(values.retiredActivationPayloadBytes, std::memory_order_relaxed);
    audioDiagnosticsPublication.lastActivationReclamationLatencyBlocks.store(
        values.lastActivationReclamationLatencyBlocks, std::memory_order_relaxed);
    audioDiagnosticsPublication.maxActivationReclamationLatencyBlocks.store(
        values.maxActivationReclamationLatencyBlocks, std::memory_order_relaxed);
    audioDiagnosticsPublication.hasActiveAuthoringPreviewActivation.store(values.hasActiveAuthoringPreviewActivation, std::memory_order_relaxed);
    audioDiagnosticsPublication.hasPendingAuthoringPreviewActivation.store(values.hasPendingAuthoringPreviewActivation, std::memory_order_relaxed);
    audioDiagnosticsPublication.hasActivePerformanceActivation.store(values.hasActivePerformanceActivation, std::memory_order_relaxed);
    audioDiagnosticsPublication.hasPendingPerformanceActivation.store(values.hasPendingPerformanceActivation, std::memory_order_relaxed);
    audioDiagnosticsPublication.sequence.store(sequence + 1, std::memory_order_release);
}

Processor::AudioDiagnosticsValues Processor::readAudioDiagnostics(std::uint64_t& sequence) const
{
    AudioDiagnosticsValues values;
    for (;;)
    {
        const auto before = audioDiagnosticsPublication.sequence.load(std::memory_order_acquire);
        if ((before & 1u) != 0)
            continue;

        values.performanceActiveVoiceCount = audioDiagnosticsPublication.performanceActiveVoiceCount.load(std::memory_order_relaxed);
        values.authoringPreviewActiveVoiceCount = audioDiagnosticsPublication.authoringPreviewActiveVoiceCount.load(std::memory_order_relaxed);
        values.activeVoiceCapacity = audioDiagnosticsPublication.activeVoiceCapacity.load(std::memory_order_relaxed);
        values.performanceContextIdentity = audioDiagnosticsPublication.performanceContextIdentity.load(std::memory_order_relaxed);
        values.authoringPreviewContextIdentity = audioDiagnosticsPublication.authoringPreviewContextIdentity.load(std::memory_order_relaxed);
        values.lastPerformanceRenderMicros = audioDiagnosticsPublication.lastPerformanceRenderMicros.load(std::memory_order_relaxed);
        values.maxPerformanceRenderMicros = audioDiagnosticsPublication.maxPerformanceRenderMicros.load(std::memory_order_relaxed);
        values.lastAuthoringPreviewRenderMicros = audioDiagnosticsPublication.lastAuthoringPreviewRenderMicros.load(std::memory_order_relaxed);
        values.maxAuthoringPreviewRenderMicros = audioDiagnosticsPublication.maxAuthoringPreviewRenderMicros.load(std::memory_order_relaxed);
        values.performancePeakActiveVoiceCount = audioDiagnosticsPublication.performancePeakActiveVoiceCount.load(std::memory_order_relaxed);
        values.performancePeakReleasingVoiceCount = audioDiagnosticsPublication.performancePeakReleasingVoiceCount.load(std::memory_order_relaxed);
        values.authoringPreviewPeakActiveVoiceCount = audioDiagnosticsPublication.authoringPreviewPeakActiveVoiceCount.load(std::memory_order_relaxed);
        values.authoringPreviewPeakReleasingVoiceCount = audioDiagnosticsPublication.authoringPreviewPeakReleasingVoiceCount.load(std::memory_order_relaxed);
        values.performanceVoiceStealCount = audioDiagnosticsPublication.performanceVoiceStealCount.load(std::memory_order_relaxed);
        values.performanceGenerationStealCount = audioDiagnosticsPublication.performanceGenerationStealCount.load(std::memory_order_relaxed);
        values.performanceReleasingVoiceStealCount = audioDiagnosticsPublication.performanceReleasingVoiceStealCount.load(std::memory_order_relaxed);
        values.performanceActiveGeneration = audioDiagnosticsPublication.performanceActiveGeneration.load(std::memory_order_relaxed);
        values.performanceActiveGenerationVoiceCount = audioDiagnosticsPublication.performanceActiveGenerationVoiceCount.load(std::memory_order_relaxed);
        values.performanceRetiredGenerationVoiceCount = audioDiagnosticsPublication.performanceRetiredGenerationVoiceCount.load(std::memory_order_relaxed);
        values.performanceSustainDeferredVoiceCount = audioDiagnosticsPublication.performanceSustainDeferredVoiceCount.load(std::memory_order_relaxed);
        values.authoringPreviewVoiceStealCount = audioDiagnosticsPublication.authoringPreviewVoiceStealCount.load(std::memory_order_relaxed);
        values.performanceDroppedEventCount = audioDiagnosticsPublication.performanceDroppedEventCount.load(std::memory_order_relaxed);
        values.authoringPreviewDroppedEventCount = audioDiagnosticsPublication.authoringPreviewDroppedEventCount.load(std::memory_order_relaxed);
        values.performanceDroppedNoteCount = audioDiagnosticsPublication.performanceDroppedNoteCount.load(std::memory_order_relaxed);
        values.authoringPreviewDroppedNoteCount = audioDiagnosticsPublication.authoringPreviewDroppedNoteCount.load(std::memory_order_relaxed);
        values.activeAuthoringPreviewRevision = audioDiagnosticsPublication.activeAuthoringPreviewRevision.load(std::memory_order_relaxed);
        values.pendingAuthoringPreviewRevision = audioDiagnosticsPublication.pendingAuthoringPreviewRevision.load(std::memory_order_relaxed);
        values.activePublishedRevision = audioDiagnosticsPublication.activePublishedRevision.load(std::memory_order_relaxed);
        values.pendingPublishedRevision = audioDiagnosticsPublication.pendingPublishedRevision.load(std::memory_order_relaxed);
        values.activePreparedBuildId = audioDiagnosticsPublication.activePreparedBuildId.load(std::memory_order_relaxed);
        values.pendingPreparedBuildId = audioDiagnosticsPublication.pendingPreparedBuildId.load(std::memory_order_relaxed);
        values.retiredActivationBacklog = audioDiagnosticsPublication.retiredActivationBacklog.load(std::memory_order_relaxed);
        values.activeActivationPayloadBytes = audioDiagnosticsPublication.activeActivationPayloadBytes.load(std::memory_order_relaxed);
        values.pendingActivationPayloadBytes = audioDiagnosticsPublication.pendingActivationPayloadBytes.load(std::memory_order_relaxed);
        values.retiredActivationPayloadBytes = audioDiagnosticsPublication.retiredActivationPayloadBytes.load(std::memory_order_relaxed);
        values.lastActivationReclamationLatencyBlocks =
            audioDiagnosticsPublication.lastActivationReclamationLatencyBlocks.load(std::memory_order_relaxed);
        values.maxActivationReclamationLatencyBlocks =
            audioDiagnosticsPublication.maxActivationReclamationLatencyBlocks.load(std::memory_order_relaxed);
        values.hasActiveAuthoringPreviewActivation = audioDiagnosticsPublication.hasActiveAuthoringPreviewActivation.load(std::memory_order_relaxed);
        values.hasPendingAuthoringPreviewActivation = audioDiagnosticsPublication.hasPendingAuthoringPreviewActivation.load(std::memory_order_relaxed);
        values.hasActivePerformanceActivation = audioDiagnosticsPublication.hasActivePerformanceActivation.load(std::memory_order_relaxed);
        values.hasPendingPerformanceActivation = audioDiagnosticsPublication.hasPendingPerformanceActivation.load(std::memory_order_relaxed);

        const auto after = audioDiagnosticsPublication.sequence.load(std::memory_order_acquire);
        if (before == after)
        {
            sequence = after;
            return values;
        }
    }
}

ProcessorRealtimeSafetySnapshot Processor::composeDiagnosticsSnapshot(
    const AudioDiagnosticsValues& audioValues,
    std::uint64_t publicationSequence) const
{
    ProcessorRealtimeSafetySnapshot snapshot;
    snapshot.available = true;
    snapshot.publicationSequence = publicationSequence;
    snapshot.processBlockCount = diagnosticsProcessBlockCount.load(std::memory_order_acquire);
    snapshot.preparedBlockSize = diagnosticsPreparedBlockSize.load(std::memory_order_acquire);
    snapshot.authoringSampleLoadsOnAudioThread = diagnosticsAuthoringSampleLoadsOnAudioThread.load(std::memory_order_acquire);
    snapshot.performanceActiveVoiceCount = audioValues.performanceActiveVoiceCount;
    snapshot.authoringPreviewActiveVoiceCount = audioValues.authoringPreviewActiveVoiceCount;
    snapshot.activeVoiceCapacity = std::max(audioValues.activeVoiceCapacity,
                                            diagnosticsPrimedActiveVoiceCapacity.load(std::memory_order_acquire));
    snapshot.activeVoiceCapacityLimit = diagnosticsActiveVoiceCapacityLimit.load(std::memory_order_acquire);
    snapshot.performanceContextIdentity = audioValues.performanceContextIdentity;
    snapshot.authoringPreviewContextIdentity = audioValues.authoringPreviewContextIdentity;
    snapshot.lastPerformanceRenderMicros = audioValues.lastPerformanceRenderMicros;
    snapshot.maxPerformanceRenderMicros = audioValues.maxPerformanceRenderMicros;
    snapshot.lastAuthoringPreviewRenderMicros = audioValues.lastAuthoringPreviewRenderMicros;
    snapshot.maxAuthoringPreviewRenderMicros = audioValues.maxAuthoringPreviewRenderMicros;
    snapshot.performancePeakActiveVoiceCount = audioValues.performancePeakActiveVoiceCount;
    snapshot.performancePeakReleasingVoiceCount = audioValues.performancePeakReleasingVoiceCount;
    snapshot.authoringPreviewPeakActiveVoiceCount = audioValues.authoringPreviewPeakActiveVoiceCount;
    snapshot.authoringPreviewPeakReleasingVoiceCount = audioValues.authoringPreviewPeakReleasingVoiceCount;
    snapshot.performanceVoiceStealCount = audioValues.performanceVoiceStealCount;
    snapshot.performanceGenerationStealCount = audioValues.performanceGenerationStealCount;
    snapshot.performanceReleasingVoiceStealCount = audioValues.performanceReleasingVoiceStealCount;
    snapshot.performanceActiveGeneration = audioValues.performanceActiveGeneration;
    snapshot.performanceActiveGenerationVoiceCount = audioValues.performanceActiveGenerationVoiceCount;
    snapshot.performanceRetiredGenerationVoiceCount = audioValues.performanceRetiredGenerationVoiceCount;
    snapshot.performanceSustainDeferredVoiceCount = audioValues.performanceSustainDeferredVoiceCount;
    snapshot.authoringPreviewVoiceStealCount = audioValues.authoringPreviewVoiceStealCount;
    snapshot.performanceDroppedEventCount = audioValues.performanceDroppedEventCount;
    snapshot.authoringPreviewDroppedEventCount = audioValues.authoringPreviewDroppedEventCount;
    snapshot.performanceDroppedNoteCount = audioValues.performanceDroppedNoteCount;
    snapshot.authoringPreviewDroppedNoteCount = audioValues.authoringPreviewDroppedNoteCount;
    snapshot.authoringPreviewActivationCount = diagnosticsAuthoringPreviewActivationCount.load(std::memory_order_acquire);
    snapshot.performanceActivationCount = diagnosticsPerformanceActivationCount.load(std::memory_order_acquire);
    snapshot.retiredActivationCount = diagnosticsRetiredActivationCount.load(std::memory_order_acquire);
    snapshot.retiredActivationBacklog = audioValues.retiredActivationBacklog;
    snapshot.reclaimedActivationPayloadCount = diagnosticsReclaimedActivationPayloadCount.load(std::memory_order_acquire);
    snapshot.activeActivationPayloadBytes = audioValues.activeActivationPayloadBytes;
    snapshot.pendingActivationPayloadBytes = audioValues.pendingActivationPayloadBytes;
    snapshot.retiredActivationPayloadBytes = audioValues.retiredActivationPayloadBytes;
    snapshot.lastActivationReclamationLatencyBlocks =
        audioValues.lastActivationReclamationLatencyBlocks;
    snapshot.maxActivationReclamationLatencyBlocks =
        audioValues.maxActivationReclamationLatencyBlocks;
    applyRealtimeGuardDiagnostics(snapshot);
    snapshot.callbackBudgetMicros = diagnosticsCallbackBudgetMicros.load(std::memory_order_acquire);
    snapshot.lastProcessBlockMicros = diagnosticsLastProcessBlockMicros.load(std::memory_order_acquire);
    snapshot.maxProcessBlockMicros = diagnosticsMaxProcessBlockMicros.load(std::memory_order_acquire);
    snapshot.overBudgetCallbackCount = diagnosticsOverBudgetCallbackCount.load(std::memory_order_acquire);
    snapshot.currentAuthoringPreviewDraftRevision = diagnosticsCurrentAuthoringPreviewDraftRevision.load(std::memory_order_acquire);
    snapshot.activeAuthoringPreviewRevision = audioValues.activeAuthoringPreviewRevision;
    snapshot.pendingAuthoringPreviewRevision = audioValues.pendingAuthoringPreviewRevision;
    snapshot.activePublishedRevision = audioValues.activePublishedRevision;
    snapshot.pendingPublishedRevision = audioValues.pendingPublishedRevision;
    snapshot.activePublishedMacroRevision
        = diagnosticActivePublishedMacroRevision.load(std::memory_order_acquire);
    snapshot.activePublishedMacroFixedVelocity
        = diagnosticActivePublishedMacroFixedVelocity.load(std::memory_order_acquire);
    snapshot.activePublishedMacroMidiNoteOffset
        = diagnosticActivePublishedMacroMidiNoteOffset.load(std::memory_order_acquire);
    snapshot.activePreparedBuildId = audioValues.activePreparedBuildId;
    snapshot.pendingPreparedBuildId = audioValues.pendingPreparedBuildId;
    const auto controller = authoringPreviewController.getSnapshot();
    snapshot.authoringPreviewFailureState = controller.hasFailedRequest
        && controller.failedRequestIdentity.draftRevision
            == snapshot.currentAuthoringPreviewDraftRevision
        ? controller.failureState : std::string {};

    if (audioValues.hasPendingAuthoringPreviewActivation
        && snapshot.pendingAuthoringPreviewRevision == snapshot.currentAuthoringPreviewDraftRevision)
        snapshot.authoringPreviewRevisionState = "Preparing";
    else if (!snapshot.authoringPreviewFailureState.empty())
        snapshot.authoringPreviewRevisionState = "Failed";
    else if (audioValues.hasActiveAuthoringPreviewActivation
             && snapshot.activeAuthoringPreviewRevision == snapshot.currentAuthoringPreviewDraftRevision)
        snapshot.authoringPreviewRevisionState = "Ready";
    else if (audioValues.hasActiveAuthoringPreviewActivation)
        snapshot.authoringPreviewRevisionState = "Stale";
    else
        snapshot.authoringPreviewRevisionState = "Idle";

    if (snapshot.getAudioThreadViolationCount() > 0)
        snapshot.state = "Realtime callback violations recorded";
    else if (!audioValues.hasActivePerformanceActivation && audioValues.hasPendingPerformanceActivation)
        snapshot.state = "Published activation pending";
    else if (!audioValues.hasActiveAuthoringPreviewActivation && audioValues.hasPendingAuthoringPreviewActivation)
        snapshot.state = "Authoring preview activation pending";
    else if (!audioValues.hasActivePerformanceActivation)
        snapshot.state = "Published activation unavailable";
    else
        snapshot.state = "Realtime callback primed";
    return snapshot;
}

void Processor::applyRealtimeGuardDiagnostics(ProcessorRealtimeSafetySnapshot& snapshot) const
{
    const auto guard = realtimeGuardState.snapshot();
    snapshot.allocationsOnAudioThread = guard.allocationCount;
    snapshot.deallocationsOnAudioThread = guard.deallocationCount;
    snapshot.blockingLockAttemptsOnAudioThread = guard.blockingLockCount;
    snapshot.waitsOnAudioThread = guard.waitCount;
    snapshot.fileOpenEntriesOnAudioThread = guard.fileOpenCount;
    snapshot.fileReadEntriesOnAudioThread = guard.fileReadCount;
    snapshot.samplePathResolutionsOnAudioThread = guard.pathResolutionCount;
    snapshot.sampleDecodeEntriesOnAudioThread = guard.sampleDecodeCount;
    snapshot.streamDecodeEntriesOnAudioThread = guard.streamDecodeCount;
    snapshot.largeResourceDestructionsOnAudioThread = guard.largeResourceDestructionCount;
    snapshot.finalSharedOwnershipReleasesOnAudioThread = guard.finalSharedOwnershipReleaseCount;
    snapshot.largeResourceReleasesOnAudioThread = guard.largeResourceDestructionCount
        + guard.finalSharedOwnershipReleaseCount;
}

void Processor::publishMessageDiagnostics()
{
    diagnosticsCurrentAuthoringPreviewDraftRevision.store(authoringSession.getDocumentState().revision,
                                                          std::memory_order_release);
    std::uint64_t audioSequence = 0;
    auto audioValues = readAudioDiagnostics(audioSequence);
    const auto activationValues = captureActivationDiagnostics();
    audioValues.activeAuthoringPreviewRevision = activationValues.activeAuthoringPreviewRevision;
    audioValues.pendingAuthoringPreviewRevision = activationValues.pendingAuthoringPreviewRevision;
    audioValues.activePublishedRevision = activationValues.activePublishedRevision;
    audioValues.pendingPublishedRevision = activationValues.pendingPublishedRevision;
    audioValues.activePreparedBuildId = activationValues.activePreparedBuildId;
    audioValues.pendingPreparedBuildId = activationValues.pendingPreparedBuildId;
    audioValues.retiredActivationBacklog = activationValues.retiredActivationBacklog;
    audioValues.activeActivationPayloadBytes = activationValues.activeActivationPayloadBytes;
    audioValues.pendingActivationPayloadBytes = activationValues.pendingActivationPayloadBytes;
    audioValues.retiredActivationPayloadBytes = activationValues.retiredActivationPayloadBytes;
    audioValues.lastActivationReclamationLatencyBlocks =
        activationValues.lastActivationReclamationLatencyBlocks;
    audioValues.maxActivationReclamationLatencyBlocks =
        activationValues.maxActivationReclamationLatencyBlocks;
    audioValues.performanceContextIdentity = activationValues.performanceContextIdentity;
    audioValues.authoringPreviewContextIdentity = activationValues.authoringPreviewContextIdentity;
    audioValues.performanceVoiceStealCount = activationValues.performanceVoiceStealCount;
    audioValues.performanceGenerationStealCount = activationValues.performanceGenerationStealCount;
    audioValues.performanceReleasingVoiceStealCount = activationValues.performanceReleasingVoiceStealCount;
    audioValues.performanceActiveGeneration = activationValues.performanceActiveGeneration;
    audioValues.performanceActiveGenerationVoiceCount = activationValues.performanceActiveGenerationVoiceCount;
    audioValues.performanceRetiredGenerationVoiceCount = activationValues.performanceRetiredGenerationVoiceCount;
    audioValues.performanceSustainDeferredVoiceCount = activationValues.performanceSustainDeferredVoiceCount;
    audioValues.authoringPreviewVoiceStealCount = activationValues.authoringPreviewVoiceStealCount;
    audioValues.performanceDroppedEventCount = activationValues.performanceDroppedEventCount;
    audioValues.authoringPreviewDroppedEventCount = activationValues.authoringPreviewDroppedEventCount;
    audioValues.performanceDroppedNoteCount = activationValues.performanceDroppedNoteCount;
    audioValues.authoringPreviewDroppedNoteCount = activationValues.authoringPreviewDroppedNoteCount;
    audioValues.hasActiveAuthoringPreviewActivation = activationValues.hasActiveAuthoringPreviewActivation;
    audioValues.hasPendingAuthoringPreviewActivation = activationValues.hasPendingAuthoringPreviewActivation;
    audioValues.hasActivePerformanceActivation = activationValues.hasActivePerformanceActivation;
    audioValues.hasPendingPerformanceActivation = activationValues.hasPendingPerformanceActivation;
    auto snapshot = std::make_shared<const ProcessorRealtimeSafetySnapshot>(
        composeDiagnosticsSnapshot(audioValues, audioSequence));
    std::atomic_store_explicit(&publishedRealtimeSafetySnapshot, std::move(snapshot), std::memory_order_release);
}

ProcessorRealtimeSafetySnapshot Processor::getRealtimeSafetySnapshot() const
{
    auto published = std::atomic_load_explicit(&publishedRealtimeSafetySnapshot, std::memory_order_acquire);
    auto snapshot = published != nullptr ? *published : ProcessorRealtimeSafetySnapshot {};

    std::uint64_t audioSequence = 0;
    const auto audioValues = readAudioDiagnostics(audioSequence);
    if (audioSequence <= snapshot.publicationSequence)
        return snapshot;

    // Readers may need callback counters before the next message-service tick. Overlay only
    // primitives onto this private value copy; formatted strings remain message-owned.
    snapshot.publicationSequence = audioSequence;
    snapshot.processBlockCount = diagnosticsProcessBlockCount.load(std::memory_order_acquire);
    snapshot.authoringSampleLoadsOnAudioThread = diagnosticsAuthoringSampleLoadsOnAudioThread.load(std::memory_order_acquire);
    snapshot.performanceActiveVoiceCount = audioValues.performanceActiveVoiceCount;
    snapshot.authoringPreviewActiveVoiceCount = audioValues.authoringPreviewActiveVoiceCount;
    snapshot.activeVoiceCapacity = std::max(audioValues.activeVoiceCapacity,
                                            diagnosticsPrimedActiveVoiceCapacity.load(std::memory_order_acquire));
    snapshot.performanceContextIdentity = audioValues.performanceContextIdentity;
    snapshot.authoringPreviewContextIdentity = audioValues.authoringPreviewContextIdentity;
    snapshot.lastPerformanceRenderMicros = audioValues.lastPerformanceRenderMicros;
    snapshot.maxPerformanceRenderMicros = audioValues.maxPerformanceRenderMicros;
    snapshot.lastAuthoringPreviewRenderMicros = audioValues.lastAuthoringPreviewRenderMicros;
    snapshot.maxAuthoringPreviewRenderMicros = audioValues.maxAuthoringPreviewRenderMicros;
    snapshot.performancePeakActiveVoiceCount = audioValues.performancePeakActiveVoiceCount;
    snapshot.performancePeakReleasingVoiceCount = audioValues.performancePeakReleasingVoiceCount;
    snapshot.authoringPreviewPeakActiveVoiceCount = audioValues.authoringPreviewPeakActiveVoiceCount;
    snapshot.authoringPreviewPeakReleasingVoiceCount = audioValues.authoringPreviewPeakReleasingVoiceCount;
    snapshot.performanceVoiceStealCount = audioValues.performanceVoiceStealCount;
    snapshot.performanceGenerationStealCount = audioValues.performanceGenerationStealCount;
    snapshot.performanceReleasingVoiceStealCount = audioValues.performanceReleasingVoiceStealCount;
    snapshot.performanceActiveGeneration = audioValues.performanceActiveGeneration;
    snapshot.performanceActiveGenerationVoiceCount = audioValues.performanceActiveGenerationVoiceCount;
    snapshot.performanceRetiredGenerationVoiceCount = audioValues.performanceRetiredGenerationVoiceCount;
    snapshot.performanceSustainDeferredVoiceCount = audioValues.performanceSustainDeferredVoiceCount;
    snapshot.authoringPreviewVoiceStealCount = audioValues.authoringPreviewVoiceStealCount;
    snapshot.performanceDroppedEventCount = audioValues.performanceDroppedEventCount;
    snapshot.authoringPreviewDroppedEventCount = audioValues.authoringPreviewDroppedEventCount;
    snapshot.performanceDroppedNoteCount = audioValues.performanceDroppedNoteCount;
    snapshot.authoringPreviewDroppedNoteCount = audioValues.authoringPreviewDroppedNoteCount;
    snapshot.authoringPreviewActivationCount = diagnosticsAuthoringPreviewActivationCount.load(std::memory_order_acquire);
    snapshot.performanceActivationCount = diagnosticsPerformanceActivationCount.load(std::memory_order_acquire);
    snapshot.retiredActivationCount = diagnosticsRetiredActivationCount.load(std::memory_order_acquire);
    snapshot.retiredActivationBacklog = audioValues.retiredActivationBacklog;
    snapshot.reclaimedActivationPayloadCount = diagnosticsReclaimedActivationPayloadCount.load(std::memory_order_acquire);
    snapshot.activeActivationPayloadBytes = audioValues.activeActivationPayloadBytes;
    snapshot.pendingActivationPayloadBytes = audioValues.pendingActivationPayloadBytes;
    snapshot.retiredActivationPayloadBytes = audioValues.retiredActivationPayloadBytes;
    snapshot.lastActivationReclamationLatencyBlocks =
        audioValues.lastActivationReclamationLatencyBlocks;
    snapshot.maxActivationReclamationLatencyBlocks =
        audioValues.maxActivationReclamationLatencyBlocks;
    applyRealtimeGuardDiagnostics(snapshot);
    snapshot.callbackBudgetMicros = diagnosticsCallbackBudgetMicros.load(std::memory_order_acquire);
    snapshot.lastProcessBlockMicros = diagnosticsLastProcessBlockMicros.load(std::memory_order_acquire);
    snapshot.maxProcessBlockMicros = diagnosticsMaxProcessBlockMicros.load(std::memory_order_acquire);
    snapshot.overBudgetCallbackCount = diagnosticsOverBudgetCallbackCount.load(std::memory_order_acquire);
    snapshot.activeAuthoringPreviewRevision = audioValues.activeAuthoringPreviewRevision;
    snapshot.pendingAuthoringPreviewRevision = audioValues.pendingAuthoringPreviewRevision;
    snapshot.activePublishedRevision = audioValues.activePublishedRevision;
    snapshot.pendingPublishedRevision = audioValues.pendingPublishedRevision;
    snapshot.activePublishedMacroRevision
        = diagnosticActivePublishedMacroRevision.load(std::memory_order_acquire);
    snapshot.activePublishedMacroFixedVelocity
        = diagnosticActivePublishedMacroFixedVelocity.load(std::memory_order_acquire);
    snapshot.activePublishedMacroMidiNoteOffset
        = diagnosticActivePublishedMacroMidiNoteOffset.load(std::memory_order_acquire);
    snapshot.activePreparedBuildId = audioValues.activePreparedBuildId;
    snapshot.pendingPreparedBuildId = audioValues.pendingPreparedBuildId;

    // These strings belong only to the returned copy. Re-evaluate them off audio so the
    // public snapshot remains behaviorally current after a block-boundary activation.
    if (audioValues.hasPendingAuthoringPreviewActivation
        && snapshot.pendingAuthoringPreviewRevision == snapshot.currentAuthoringPreviewDraftRevision)
        snapshot.authoringPreviewRevisionState = "Preparing";
    else if (!snapshot.authoringPreviewFailureState.empty())
        snapshot.authoringPreviewRevisionState = "Failed";
    else if (audioValues.hasActiveAuthoringPreviewActivation
             && snapshot.activeAuthoringPreviewRevision == snapshot.currentAuthoringPreviewDraftRevision)
        snapshot.authoringPreviewRevisionState = "Ready";
    else if (audioValues.hasActiveAuthoringPreviewActivation)
        snapshot.authoringPreviewRevisionState = "Stale";
    else
        snapshot.authoringPreviewRevisionState = "Idle";

    if (snapshot.getAudioThreadViolationCount() > 0)
        snapshot.state = "Realtime callback violations recorded";
    else if (!audioValues.hasActivePerformanceActivation && audioValues.hasPendingPerformanceActivation)
        snapshot.state = "Published activation pending";
    else if (!audioValues.hasActiveAuthoringPreviewActivation && audioValues.hasPendingAuthoringPreviewActivation)
        snapshot.state = "Authoring preview activation pending";
    else if (!audioValues.hasActivePerformanceActivation)
        snapshot.state = "Published activation unavailable";
    else
        snapshot.state = "Realtime callback primed";
    return snapshot;
}
} // namespace drs::plugin

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new drs::plugin::Processor();
}
