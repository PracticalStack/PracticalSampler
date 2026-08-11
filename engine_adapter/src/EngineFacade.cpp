#include "drs/engine/EngineFacade.h"
#include "drs/engine/CuratedDspCatalog.h"
#include "drs/engine/DspGraphPlan.h"
#include "drs/engine/DspParameterControl.h"
#include "drs/engine/HiseFrontendBridge.h"
#include "drs/engine/HiseProjectContent.h"
#include "drs/engine/PerformancePublishPreparation.h"
#include "drs/engine/RuntimeLoadProfile.h"
#include "drs/engine/RuntimePresetState.h"
#include "drs/engine/RuntimeLoader.h"
#include "drs/engine/SfzImportProjection.h"
#include "drs/engine/SfzImportReport.h"
#include "drs/engine/RuntimeStreamingService.h"
#include "drs/engine/RuntimeStream.h"
#include "drs/engine/RuntimeVoice.h"
#include "drs/engine/HiseVendorInfo.generated.h"

#include <json/json.hpp>

#include <algorithm>
#include <chrono>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <thread>
#include <unordered_map>
#include <unordered_set>

namespace drs::engine
{
namespace
{
using Clock = std::chrono::steady_clock;
namespace fs = std::filesystem;
using ordered_json = nlohmann::ordered_json;

std::string summarizeIssues(const std::vector<std::string>& issues)
{
    if (issues.empty())
        return {};

    if (issues.size() == 1)
        return issues.front();

    return issues.front() + " (+" + std::to_string(issues.size() - 1) + " more)";
}

std::string summarizeSnapshotFindings(const std::vector<PlaybackSnapshotFinding>& findings)
{
    if (findings.empty())
        return {};

    if (findings.size() == 1)
        return findings.front().message;

    return findings.front().message + " (+" + std::to_string(findings.size() - 1) + " more)";
}

std::string summarizeDigest(const std::string& digest)
{
    if (digest.empty())
        return "none";

    constexpr std::size_t prefixLength = 18;
    if (digest.size() <= prefixLength)
        return digest;

    return digest.substr(0, prefixLength) + "...";
}

void addSnapshotFinding(PlaybackSnapshotBuildResult& result,
                        PlaybackSnapshotFindingSeverity severity,
                        const std::string& code,
                        const std::string& path,
                        const std::string& message)
{
    result.findings.push_back({ severity, code, path, message });
}

std::uint64_t monotonicMicros() noexcept
{
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(
            Clock::now().time_since_epoch()).count());
}

std::string computeFnv1a64Digest(const std::string& text)
{
    std::uint64_t hash = 14695981039346656037ull;
    for (const auto character : text)
    {
        hash ^= static_cast<unsigned char>(character);
        hash *= 1099511628211ull;
    }

    std::ostringstream stream;
    stream << "fnv1a64:" << std::hex << std::setw(16) << std::setfill('0') << hash;
    return stream.str();
}

PerformancePublishFinding makePerformancePublishFinding(
    const PlaybackSnapshotFinding& finding)
{
    PerformancePublishFindingSeverity severity = PerformancePublishFindingSeverity::information;
    switch (finding.severity)
    {
        case PlaybackSnapshotFindingSeverity::warning:
            severity = PerformancePublishFindingSeverity::warning;
            break;
        case PlaybackSnapshotFindingSeverity::error:
            severity = PerformancePublishFindingSeverity::error;
            break;
    }
    return { severity, finding.code, finding.path, finding.message };
}

PerformancePublishFinding makePerformancePublishFailure(std::string code,
                                                         std::string path,
                                                         std::string message)
{
    return { PerformancePublishFindingSeverity::error,
             std::move(code), std::move(path), std::move(message) };
}

std::string buildMacroSummary(const RuntimeSessionStateSnapshot& sessionState)
{
    if (sessionState.macroValues.empty())
        return "none";

    std::ostringstream stream;

    for (std::size_t index = 0; index < sessionState.macroValues.size(); ++index)
    {
        if (index != 0)
            stream << ", ";

        stream << sessionState.macroValues[index].id << "=" << sessionState.macroValues[index].value;
    }

    return stream.str();
}

std::optional<double> findMacroValue(const RuntimeSessionStateSnapshot& sessionState, const std::string& macroId)
{
    const auto iterator = std::find_if(sessionState.macroValues.begin(),
                                       sessionState.macroValues.end(),
                                       [&](const RuntimePresetMacroValue& macroValue)
                                       {
                                           return macroValue.id == macroId;
                                       });
    if (iterator == sessionState.macroValues.end())
        return std::nullopt;

    return iterator->value;
}

double normalizeMacroValue(double value)
{
    constexpr auto precisionScale = 1000000.0;
    return std::round(value * precisionScale) / precisionScale;
}

double computePreparationCacheHitRate(const std::size_t hits, const std::size_t misses)
{
    const auto total = hits + misses;
    if (total == 0)
        return 0.0;

    return static_cast<double>(hits) / static_cast<double>(total);
}

int clampMidiValue(int value)
{
    return std::clamp(value, 0, 127);
}

int computeTonePreviewVelocity(const RuntimeSessionStateSnapshot& sessionState, int fallbackVelocity)
{
    const auto toneValue = findMacroValue(sessionState, "tone").value_or(0.35);
    const auto effectiveVelocity = static_cast<int>(std::lround(32.0 + toneValue * 95.0));
    return std::clamp(effectiveVelocity, 1, 127);
}

int computeMotionPreviewNote(const RuntimeSessionStateSnapshot& sessionState, int playedNote)
{
    const auto motionValue = findMacroValue(sessionState, "motion").value_or(0.5);
    const auto semitoneOffset = static_cast<int>(std::lround((motionValue - 0.5) * 24.0));
    return clampMidiValue(playedNote + semitoneOffset);
}

std::string buildToneCurrentEffect(const RuntimeSessionStateSnapshot& sessionState)
{
    const auto toneValue = findMacroValue(sessionState, "tone").value_or(0.35);
    if (toneValue >= 0.75)
        return "Accent attack";
    if (toneValue >= 0.4)
        return "Balanced attack";
    return "Soft attack";
}

std::string buildMotionCurrentEffect(const RuntimeSessionStateSnapshot& sessionState)
{
    const auto motionValue = findMacroValue(sessionState, "motion").value_or(0.5);
    const auto semitoneOffset = static_cast<int>(std::lround((motionValue - 0.5) * 24.0));
    if (semitoneOffset == 0)
        return "Centered pitch";

    const auto direction = semitoneOffset > 0 ? "+" : "";
    return direction + std::to_string(semitoneOffset) + " st";
}

std::string buildAppliedMacroSummary(const RuntimeSessionStateSnapshot& sessionState)
{
    return "Tone: " + buildToneCurrentEffect(sessionState) + " | Motion: " + buildMotionCurrentEffect(sessionState);
}

std::string resolveAuthoredArticulationSelection(const RuntimeProjectModel& project,
                                                 const std::string& currentSelection)
{
    std::vector<std::string> authoredArticulations;
    authoredArticulations.reserve(project.authoring.zones.size());
    std::unordered_set<std::string> seenArticulations;

    for (const auto& zone : project.authoring.zones)
    {
        if (zone.articulationId.empty())
            continue;
        if (seenArticulations.insert(zone.articulationId).second)
            authoredArticulations.push_back(zone.articulationId);
    }

    if (authoredArticulations.empty())
        return currentSelection;

    if (!currentSelection.empty()
        && seenArticulations.count(currentSelection) > 0)
    {
        return currentSelection;
    }

    if (seenArticulations.count("default") > 0)
        return "default";

    return authoredArticulations.front();
}

RuntimeInstrumentModel buildProjectPresetValidationInstrument(
    const RuntimeInstrumentModel& referenceInstrument,
    const RuntimeProjectModel& project)
{
    auto instrument = referenceInstrument;
    instrument.articulations.clear();

    std::unordered_set<std::string> seenArticulationIds;
    for (const auto& articulation : project.authoring.articulations)
    {
        if (articulation.id.empty() || !seenArticulationIds.insert(articulation.id).second)
            continue;
        instrument.articulations.push_back({
            articulation.id,
            articulation.displayName.empty() ? articulation.id : articulation.displayName,
            articulation.isDefault,
            articulation.activation
        });
    }

    // Older authored schemas may express articulation membership only on zones. The project
    // loader validates and migrates those documents, but retaining this fallback keeps the
    // hosted-state contract correct for every validated RuntimeProjectModel caller.
    for (const auto& zone : project.authoring.zones)
    {
        if (zone.articulationId.empty()
            || !seenArticulationIds.insert(zone.articulationId).second)
        {
            continue;
        }
        instrument.articulations.push_back({
            zone.articulationId,
            zone.articulationId,
            instrument.articulations.empty() || zone.articulationId == "default",
            std::nullopt
        });
    }

    for (const auto& authoredMacro : project.authoring.macros)
    {
        const auto existing = std::find_if(
            instrument.macros.begin(), instrument.macros.end(),
            [&](const RuntimeMacroDefinition& macro) { return macro.id == authoredMacro.id; });
        const RuntimeMacroDefinition projected {
            authoredMacro.id,
            authoredMacro.name,
            authoredMacro.defaultValue,
            authoredMacro.minValue,
            authoredMacro.maxValue
        };
        if (existing == instrument.macros.end())
            instrument.macros.push_back(projected);
        else
            *existing = projected;
    }

    return instrument;
}

std::string runtimeMacroIdFromHostParameterId(const std::string& hostParameterId)
{
    constexpr std::string_view prefix { "macro." };
    return hostParameterId.rfind(prefix.data(), 0) == 0
        ? hostParameterId.substr(prefix.size())
        : hostParameterId;
}

std::string buildPublishedHostSlotPlaceholderId(const std::size_t slotIndex)
{
    return "__published-host-slot__." + std::to_string(slotIndex);
}

bool isPublishedHostSlotPlaceholderId(const std::string& stableAuthoredId)
{
    constexpr std::string_view prefix { "__published-host-slot__." };
    return stableAuthoredId.rfind(prefix.data(), 0) == 0;
}

std::string humanizePresentationIdentifier(const std::string& identifier)
{
    std::string result;
    result.reserve(identifier.size());
    bool capitalize = true;
    for (const auto character : identifier)
    {
        if (character == '.' || character == '_' || character == '-')
        {
            if (!result.empty() && result.back() != ' ')
                result.push_back(' ');
            capitalize = true;
            continue;
        }
        if (std::isupper(static_cast<unsigned char>(character)) && !result.empty()
            && result.back() != ' ')
            result.push_back(' ');
        result.push_back(capitalize ? static_cast<char>(std::toupper(static_cast<unsigned char>(character)))
                                    : character);
        capitalize = false;
    }
    return result.empty() ? "Control" : result;
}

std::string curatedParameterLabel(const std::string& parameterId)
{
    const auto hasSuffix = [&](const std::string_view suffix)
    {
        return parameterId.size() > suffix.size()
            && parameterId.compare(parameterId.size() - suffix.size(), suffix.size(), suffix) == 0;
    };
    if (hasSuffix("Db"))
        return humanizePresentationIdentifier(parameterId.substr(0, parameterId.size() - 2));
    if (hasSuffix("Ms"))
        return humanizePresentationIdentifier(parameterId.substr(0, parameterId.size() - 2));
    if (hasSuffix("Hz"))
        return humanizePresentationIdentifier(parameterId.substr(0, parameterId.size() - 2));
    return humanizePresentationIdentifier(parameterId);
}

std::string valueUnitLabel(const CuratedDspParameterUnit unit)
{
    switch (unit)
    {
        case CuratedDspParameterUnit::decibels: return "dB";
        case CuratedDspParameterUnit::milliseconds: return "ms";
        case CuratedDspParameterUnit::seconds: return "s";
        case CuratedDspParameterUnit::hertz: return "Hz";
        case CuratedDspParameterUnit::ratio: return "ratio";
        case CuratedDspParameterUnit::semitones: return "semitones";
        case CuratedDspParameterUnit::normalized:
        case CuratedDspParameterUnit::boolean: return {};
    }
    return {};
}

PublishedMacroControlKind controlKindForUnit(const CuratedDspParameterUnit unit)
{
    if (unit == CuratedDspParameterUnit::boolean)
        return PublishedMacroControlKind::toggle;
    if (unit == CuratedDspParameterUnit::decibels)
        return PublishedMacroControlKind::fader;
    return PublishedMacroControlKind::knob;
}

std::string sourceLabelForDspSlot(const ImmutablePlaybackSnapshot& snapshot,
                                  const std::string& slotId)
{
    const auto bus = std::find_if(snapshot.routingBuses.begin(), snapshot.routingBuses.end(),
                                  [&](const auto& candidate)
                                  {
                                      return std::find(candidate.fxSlotIds.begin(), candidate.fxSlotIds.end(), slotId)
                                          != candidate.fxSlotIds.end();
                                  });
    if (bus == snapshot.routingBuses.end())
        return "Instrument";

    constexpr std::string_view groupPrefix { "groups/" };
    constexpr std::string_view zonePrefix { "zones/" };
    if (bus->inputSourceId.rfind(groupPrefix.data(), 0) == 0)
    {
        const auto groupId = bus->inputSourceId.substr(groupPrefix.size());
        const auto group = std::find_if(snapshot.groupRoutes.begin(), snapshot.groupRoutes.end(),
                                        [&](const auto& route) { return route.groupId == groupId; });
        return group != snapshot.groupRoutes.end() && !group->displayName.empty()
            ? group->displayName : (groupId.empty() ? "Instrument" : groupId);
    }
    if (bus->inputSourceId.rfind(zonePrefix.data(), 0) == 0)
    {
        const auto zoneId = bus->inputSourceId.substr(zonePrefix.size());
        const auto zone = std::find_if(snapshot.zones.begin(), snapshot.zones.end(),
                                       [&](const auto& candidate) { return candidate.id == zoneId; });
        return zone != snapshot.zones.end() && !zone->displayName.empty()
            ? zone->displayName : (zoneId.empty() ? "Instrument" : zoneId);
    }
    return "Instrument";
}

std::vector<PublishedMacroBindingBuildRequest::PresentationHint> buildPresentationHints(
    const ImmutablePlaybackSnapshot& snapshot)
{
    std::vector<PublishedMacroBindingBuildRequest::PresentationHint> hints;
    hints.reserve(snapshot.macroDefaults.size());
    for (std::size_t authoredOrder = 0; authoredOrder < snapshot.macroDefaults.size(); ++authoredOrder)
    {
        const auto& macro = snapshot.macroDefaults[authoredOrder];
        PublishedMacroPresentation presentation;
        presentation.authoredLabel = macro.name.empty()
            ? humanizePresentationIdentifier(macro.id) : macro.name;
        presentation.sectionLabel = "Instrument";
        presentation.parameterLabel = "Control";
        presentation.authoredOrder = authoredOrder;

        const auto target = std::find_if(macro.targets.begin(), macro.targets.end(), [](const auto& candidate)
        {
            return !candidate.dspSlotId.empty() && !candidate.dspParameterId.empty();
        });
        if (target != macro.targets.end())
        {
            presentation.sectionLabel = sourceLabelForDspSlot(snapshot, target->dspSlotId);
            presentation.parameterLabel = curatedParameterLabel(target->dspParameterId);
            const auto slot = std::find_if(snapshot.fxSlots.begin(), snapshot.fxSlots.end(),
                                           [&](const auto& candidate) { return candidate.id == target->dspSlotId; });
            if (slot != snapshot.fxSlots.end())
            {
                if (const auto* effect = findCuratedDspEffect(slot->effectType, slot->effectVersion))
                {
                    const auto parameter = std::find_if(effect->parameters.begin(), effect->parameters.end(),
                                                        [&](const auto& candidate)
                                                        { return candidate.id == target->dspParameterId; });
                    if (parameter != effect->parameters.end())
                    {
                        presentation.valueUnit = valueUnitLabel(parameter->unit);
                        presentation.controlKind = controlKindForUnit(parameter->unit);
                    }
                }
            }
        }
        else if (macro.id == "tone" || macro.id == "motion")
        {
            presentation.parameterLabel = macro.id == "tone" ? "Tone" : "Motion";
        }

        presentation.accessibilityDescription = presentation.authoredLabel + ", "
            + presentation.sectionLabel + ", " + presentation.parameterLabel;
        if (!presentation.valueUnit.empty())
            presentation.accessibilityDescription += ", " + presentation.valueUnit;
        hints.push_back({ macro.id, std::move(presentation) });
    }
    return hints;
}

EngineMacroDescriptor makePublishedMacroDescriptor(const PublishedMacroBinding& binding,
                                                   const RuntimeSessionStateSnapshot& sessionState)
{
    const auto runtimeId = runtimeMacroIdFromHostParameterId(binding.hostParameterId);
    const auto currentValue = findMacroValue(sessionState, runtimeId)
        .value_or(binding.publishedValue);

    auto ownershipKey = binding.hostParameterId;
    auto soundIntent = std::string("Published performance control.");
    auto currentEffect = std::string {};
    if (binding.renderTarget == PublishedMacroRenderTarget::toneVelocity)
    {
        ownershipKey = "preview.triggerVelocity";
        soundIntent = "Published control shapes the fixed playback velocity.";
        currentEffect = buildToneCurrentEffect(sessionState);
    }
    else if (binding.renderTarget == PublishedMacroRenderTarget::motionPitch)
    {
        ownershipKey = "preview.noteTravel";
        soundIntent = "Published control offsets the played pitch.";
        currentEffect = buildMotionCurrentEffect(sessionState);
    }
    else if (binding.renderTarget == PublishedMacroRenderTarget::dspControl)
    {
        ownershipKey = "published.dsp." + binding.dspSlotId + "." + binding.dspParameterId;
        soundIntent = binding.exposedInPerformance
            ? "Published exposed control routed into the active DSP graph."
            : "Published helper control routed into the active DSP graph.";
        currentEffect = binding.dspSlotId + " / " + binding.dspParameterId;
    }

    const auto displayMinimum = binding.renderTarget == PublishedMacroRenderTarget::dspControl
        ? binding.destinationMinimum : binding.minValue;
    const auto displayMaximum = binding.renderTarget == PublishedMacroRenderTarget::dspControl
        ? binding.destinationMaximum : binding.maxValue;

    return {
        runtimeId,
        binding.publishedName,
        binding.minValue,
        binding.maxValue,
        binding.defaultValue,
        std::clamp(currentValue, binding.minValue, binding.maxValue),
        std::move(ownershipKey),
        std::move(soundIntent),
        std::move(currentEffect),
        true,
        binding.exposedInPerformance,
        binding.presentation.sectionLabel,
        binding.presentation.parameterLabel,
        binding.presentation.valueUnit,
        binding.presentation.controlKind,
        binding.presentation.authoredOrder,
        binding.presentation.accessibilityDescription,
        displayMinimum,
        displayMaximum,
        binding.stableAuthoredId,
        binding.controlLaw
    };
}

std::vector<PublishedMacroHostSlotDefinition> buildPublishedHostSlots(
    const std::vector<PlaybackSnapshotMacroDefault>& authoredMacros,
    const ImmutablePublishedMacroBindingTablePtr& previousActiveTable)
{
    const auto& topology = publishedMacroHostTopology();
    std::vector<PublishedMacroHostSlotDefinition> hostSlots;
    hostSlots.reserve(topology.size());
    std::vector<std::string> assignedStableIds(topology.size());
    std::vector<bool> retainedStableIds(topology.size(), false);
    for (std::size_t index = 0; index < assignedStableIds.size(); ++index)
        assignedStableIds[index] = buildPublishedHostSlotPlaceholderId(index);

    std::unordered_set<std::string> assignedIds;
    assignedIds.reserve(authoredMacros.size());
    std::unordered_set<std::string> authoredIds;
    authoredIds.reserve(authoredMacros.size());
    for (const auto& macro : authoredMacros)
        authoredIds.insert(macro.id);

    if (previousActiveTable != nullptr)
    {
        for (const auto& binding : previousActiveTable->bindings)
        {
            if (binding.hostSlotIndex >= topology.size() || binding.stableAuthoredId.empty()
                || (!isPublishedHostSlotPlaceholderId(binding.stableAuthoredId)
                    && !authoredIds.count(binding.stableAuthoredId)))
            {
                continue;
            }

            assignedStableIds[binding.hostSlotIndex] = binding.stableAuthoredId;
            retainedStableIds[binding.hostSlotIndex]
                = !isPublishedHostSlotPlaceholderId(binding.stableAuthoredId);
        }
    }

    auto assignAuthoredMacros = [&](bool exposedOnly)
    {
        for (const auto& macro : authoredMacros)
        {
            if (macro.exposedInPerformance != exposedOnly || assignedIds.count(macro.id))
                continue;

            const auto retainedSlot = std::find(assignedStableIds.begin(),
                                                assignedStableIds.end(),
                                                macro.id);
            if (retainedSlot != assignedStableIds.end())
            {
                assignedIds.insert(macro.id);
                continue;
            }

            auto openSlot = assignedStableIds.end();
            for (std::size_t index = 0; index < assignedStableIds.size(); ++index)
            {
                if (!retainedStableIds[index]
                    && isPublishedHostSlotPlaceholderId(assignedStableIds[index]))
                {
                    openSlot = assignedStableIds.begin() + static_cast<std::ptrdiff_t>(index);
                    break;
                }
            }
            if (openSlot == assignedStableIds.end())
                return;

            *openSlot = macro.id;
            assignedIds.insert(macro.id);
            retainedStableIds[static_cast<std::size_t>(openSlot - assignedStableIds.begin())]
                = true;
        }
    };

    assignAuthoredMacros(true);
    assignAuthoredMacros(false);

    for (std::size_t index = 0; index < assignedStableIds.size(); ++index)
        hostSlots.push_back(
            { topology[index].slotIndex, topology[index].hostParameterId, assignedStableIds[index] });

    return hostSlots;
}

std::vector<PublishedMacroCurrentValue> buildPublishedCurrentValues(
    const RuntimeSessionStateSnapshot& sessionState,
    const std::vector<PlaybackSnapshotMacroDefault>& authoredMacros,
    const ImmutablePublishedMacroBindingTablePtr& previousActiveTable)
{
    std::vector<PublishedMacroCurrentValue> currentValues;
    std::unordered_set<std::string> insertedIds;

    if (previousActiveTable != nullptr)
    {
        currentValues.reserve(previousActiveTable->bindings.size());
        for (const auto& binding : previousActiveTable->bindings)
        {
            if (!binding.assigned || !insertedIds.insert(binding.stableAuthoredId).second)
                continue;

            const auto slotValue = findMacroValue(sessionState,
                                                  runtimeMacroIdFromHostParameterId(binding.hostParameterId));
            if (!slotValue.has_value() || !std::isfinite(*slotValue))
                continue;

            currentValues.push_back({ binding.stableAuthoredId, *slotValue });
        }

        return currentValues;
    }

    currentValues.reserve(authoredMacros.size());
    for (const auto& macro : authoredMacros)
    {
        const auto value = findMacroValue(sessionState, macro.id);
        if (!value.has_value() || !std::isfinite(*value))
            continue;
        currentValues.push_back({ macro.id, *value });
    }

    return currentValues;
}

struct PublishedMacroPreflightResult
{
    std::size_t exposedCount = 0;
    std::size_t hiddenCount = 0;
    std::optional<PerformancePublishFinding> finding;
};

std::string describeAuthoredMacro(const RuntimeProjectMacroDefinition& macro,
                                  const std::size_t index)
{
    if (!macro.name.empty())
        return macro.name;
    if (!macro.id.empty())
        return macro.id;
    return "Macro " + std::to_string(index + 1);
}

PublishedMacroPreflightResult preflightPublishedMacros(
    const RuntimeProjectAuthoringState& authoring)
{
    PublishedMacroPreflightResult result;
    for (const auto& macro : authoring.macros)
    {
        if (macro.exposedInPerformance)
            ++result.exposedCount;
        else
            ++result.hiddenCount;
    }

    const auto makeFinding = [](std::string code, std::string path, std::string message)
    {
        return PerformancePublishFinding {
            PerformancePublishFindingSeverity::error,
            std::move(code),
            std::move(path),
            std::move(message)
        };
    };

    if (result.exposedCount > maximumExposedPerformanceControls)
    {
        const auto overflowIndex = std::find_if(authoring.macros.begin(), authoring.macros.end(),
                                                 [exposed = std::size_t { 0 }](const auto& macro) mutable
                                                 {
                                                     return macro.exposedInPerformance
                                                         && ++exposed > maximumExposedPerformanceControls;
                                                 });
        const auto index = static_cast<std::size_t>(
            std::distance(authoring.macros.begin(), overflowIndex));
        result.finding = makeFinding(
            "published-macro-exposed-capacity-exceeded",
            "authoring.macros[" + std::to_string(index) + "].exposedInPerformance",
            "Performance supports at most " + std::to_string(maximumExposedPerformanceControls)
                + " exposed controls; '" + describeAuthoredMacro(*overflowIndex, index)
                + "' is control " + std::to_string(maximumExposedPerformanceControls + 1)
                + ". Hide it or reduce exposed controls to "
                + std::to_string(maximumExposedPerformanceControls) + ".");
        return result;
    }

    if (authoring.macros.size() > maximumPublishedMacroHostSlots)
    {
        const auto index = maximumPublishedMacroHostSlots;
        result.finding = makeFinding(
            "published-macro-authored-capacity-exceeded",
            "authoring.macros[" + std::to_string(index) + "]",
            "Performance supports at most " + std::to_string(maximumPublishedMacroHostSlots)
                + " authored macros; '" + describeAuthoredMacro(authoring.macros[index], index)
                + "' is macro " + std::to_string(index + 1)
                + ". Remove a macro before publishing.");
        return result;
    }

    std::unordered_set<std::string> macroIds;
    for (std::size_t macroIndex = 0; macroIndex < authoring.macros.size(); ++macroIndex)
    {
        const auto& macro = authoring.macros[macroIndex];
        const auto path = "authoring.macros[" + std::to_string(macroIndex) + "]";
        if (macro.id.empty() || !macroIds.insert(macro.id).second)
        {
            result.finding = makeFinding(
                "published-macro-authored-id-invalid", path + ".id",
                "Published macro '" + describeAuthoredMacro(macro, macroIndex)
                    + "' needs a unique stable ID before publishing.");
            return result;
        }
        if (!std::isfinite(macro.minValue) || !std::isfinite(macro.maxValue)
            || !std::isfinite(macro.defaultValue) || macro.minValue > macro.maxValue
            || macro.defaultValue < macro.minValue || macro.defaultValue > macro.maxValue)
        {
            result.finding = makeFinding(
                "published-macro-authored-range-invalid", path,
                "Published macro '" + describeAuthoredMacro(macro, macroIndex)
                    + "' needs a finite default inside its minimum and maximum range.");
            return result;
        }

        for (std::size_t targetIndex = 0; targetIndex < macro.targets.size(); ++targetIndex)
        {
            const auto& target = macro.targets[targetIndex];
            const auto targetPath = path + ".targets[" + std::to_string(targetIndex) + "]";
            const auto hasSlotId = !target.dspSlotId.empty();
            const auto hasParameterId = !target.dspParameterId.empty();
            if (hasSlotId != hasParameterId)
            {
                result.finding = makeFinding(
                    "published-macro-dsp-target-invalid", targetPath,
                    "Published macro '" + describeAuthoredMacro(macro, macroIndex)
                        + "' must provide both DSP slot and parameter IDs for a structured target.");
                return result;
            }
            if (!hasSlotId)
                continue;

            const auto slot = std::find_if(authoring.fxSlots.begin(), authoring.fxSlots.end(),
                                           [&](const auto& candidate)
                                           {
                                               return candidate.id == target.dspSlotId;
                                           });
            const auto parameterExists = slot != authoring.fxSlots.end()
                && std::any_of(slot->parameters.begin(), slot->parameters.end(),
                               [&](const auto& parameter)
                               {
                                   return parameter.id == target.dspParameterId;
                               });
            if (!parameterExists)
            {
                result.finding = makeFinding(
                    "published-macro-dsp-target-missing", targetPath,
                    "Published macro '" + describeAuthoredMacro(macro, macroIndex)
                        + "' targets missing DSP control '" + target.dspSlotId + "."
                        + target.dspParameterId + "'. Repair the target before publishing.");
                return result;
            }
        }
    }
    return result;
}

std::size_t assignedBindingCount(const ImmutablePublishedMacroBindingTablePtr& table)
{
    if (table == nullptr)
        return 0;
    return static_cast<std::size_t>(std::count_if(
        table->bindings.begin(), table->bindings.end(), [](const auto& binding)
        {
            return binding.assigned;
        }));
}

std::string resolveDraftSurfaceSource(const DraftPlaybackStatus& status)
{
    if (status.performance.available)
        return "published draft";

    if (status.preview.available)
        return "preview fallback";

    return "default fallback";
}

std::string resolveRendererMode(bool referenceInstrumentActive)
{
    return referenceInstrumentActive ? "reference-backed" : "inactive";
}

constexpr std::size_t preparedCacheRetentionWorkingSetCount = 2;

std::uint64_t saturatingAdd(const std::uint64_t left, const std::uint64_t right)
{
    const auto maxValue = std::numeric_limits<std::uint64_t>::max();
    return right > maxValue - left ? maxValue : left + right;
}

std::uint64_t saturatingMultiply(const std::uint64_t value, const std::size_t multiplier)
{
    if (value == 0 || multiplier == 0)
        return 0;

    const auto maxValue = std::numeric_limits<std::uint64_t>::max();
    return value > maxValue / multiplier ? maxValue : value * multiplier;
}

template <typename Snapshot>
void applyPreparedCachePressurePolicy(Snapshot& snapshot)
{
    snapshot.preparedCacheRetentionWorkingSetCount = preparedCacheRetentionWorkingSetCount;
    snapshot.preparedCacheWorkingSetBytes =
        std::max({snapshot.previewPreparedBytes,
                  snapshot.publishedPreparedBytes,
                  snapshot.preparedWorkerActiveOwnershipBytes});
    snapshot.preparedCacheByteBudget = saturatingMultiply(snapshot.preparedCacheWorkingSetBytes,
                                                          snapshot.preparedCacheRetentionWorkingSetCount);
    snapshot.preparedCacheResidentBytes = saturatingAdd(snapshot.preparedWorkerActiveOwnershipBytes,
                                                        snapshot.preparedWorkerRetiredBytes);
    snapshot.preparedCacheHeadroomBytes =
        snapshot.preparedCacheByteBudget > snapshot.preparedCacheResidentBytes
            ? snapshot.preparedCacheByteBudget - snapshot.preparedCacheResidentBytes
            : 0;

    if (snapshot.preparedCacheResidentBytes == 0 && snapshot.preparedCacheWorkingSetBytes == 0)
    {
        snapshot.preparedCachePressureState = "Idle";
    }
    else if (snapshot.preparedCacheResidentBytes > snapshot.preparedCacheByteBudget)
    {
        snapshot.preparedCachePressureState = "Over budget";
    }
    else if (snapshot.preparedCacheResidentBytes > snapshot.preparedCacheWorkingSetBytes)
    {
        snapshot.preparedCachePressureState = "Replacement set retained";
    }
    else
    {
        snapshot.preparedCachePressureState = "Nominal";
    }
}

void syncDraftPlaybackIntoDiagnostics(const DraftPlaybackStatus& status,
                                      EngineDiagnosticsSnapshot& diagnosticsSnapshot)
{
    diagnosticsSnapshot.draftRevision = status.draftRevision;
    diagnosticsSnapshot.previewRevision = status.preview.revision;
    diagnosticsSnapshot.publishedRevision = status.performance.revision;
    diagnosticsSnapshot.previewBuildId = status.preview.buildId;
    diagnosticsSnapshot.publishedBuildId = status.performance.buildId;
    diagnosticsSnapshot.previewPreparedBuildId = status.preview.preparedBuildId;
    diagnosticsSnapshot.publishedPreparedBuildId = status.performance.preparedBuildId;
    diagnosticsSnapshot.previewPending = status.pendingPreview.active;
    diagnosticsSnapshot.publishedPending = status.pendingPerformance.active;
    diagnosticsSnapshot.previewActivationEligible = status.preview.activationEligible;
    diagnosticsSnapshot.publishedActivationEligible = status.performance.activationEligible;
    diagnosticsSnapshot.previewRevisionState = status.preview.state;
    diagnosticsSnapshot.previewContentDigest = status.preview.contentDigest;
    diagnosticsSnapshot.publishedContentDigest = status.performance.contentDigest;
    diagnosticsSnapshot.previewPreparedContentDigest = status.preview.preparedContentDigest;
    diagnosticsSnapshot.publishedPreparedContentDigest = status.performance.preparedContentDigest;
    diagnosticsSnapshot.publishedRouteDigest = status.performance.routeDigest;
    diagnosticsSnapshot.publishedSourceProvenanceDigest = status.performance.sourceProvenanceDigest;
    diagnosticsSnapshot.publishedMacroSchemaDigest = status.performance.macroSchemaDigest;
    diagnosticsSnapshot.previewPreparedSampleCount = status.preview.preparedSampleCount;
    diagnosticsSnapshot.previewPreparedStreamCount = status.preview.preparedStreamCount;
    diagnosticsSnapshot.previewPreparedZoneCount = status.preview.preparedZoneCount;
    diagnosticsSnapshot.previewPreparedOwnershipRecordCount = status.preview.preparedOwnershipRecordCount;
    diagnosticsSnapshot.publishedPreparedSampleCount = status.performance.preparedSampleCount;
    diagnosticsSnapshot.publishedPreparedStreamCount = status.performance.preparedStreamCount;
    diagnosticsSnapshot.publishedPreparedZoneCount = status.performance.preparedZoneCount;
    diagnosticsSnapshot.publishedPreparedOwnershipRecordCount = status.performance.preparedOwnershipRecordCount;
    diagnosticsSnapshot.previewPreparedBytes = status.preview.preparedBytes;
    diagnosticsSnapshot.publishedPreparedBytes = status.performance.preparedBytes;
    diagnosticsSnapshot.previewPreparedOwnershipBytes = status.preview.preparedOwnershipBytes;
    diagnosticsSnapshot.publishedPreparedOwnershipBytes = status.performance.preparedOwnershipBytes;
    diagnosticsSnapshot.previewPreparedBuildMicros = status.preview.preparedBuildDurationMicros;
    diagnosticsSnapshot.publishedPreparedBuildMicros = status.performance.preparedBuildDurationMicros;
    diagnosticsSnapshot.previewPreparedDecodedBytes = status.preview.preparedDecodedBytes;
    diagnosticsSnapshot.publishedPreparedDecodedBytes = status.performance.preparedDecodedBytes;
    diagnosticsSnapshot.previewPreparedSampleDataBytes = status.preview.preparedSampleDataBytes;
    diagnosticsSnapshot.publishedPreparedSampleDataBytes = status.performance.preparedSampleDataBytes;
    diagnosticsSnapshot.previewActivationPayloadBytes = status.preview.activationPayloadRetainedBytes;
    diagnosticsSnapshot.publishedActivationPayloadBytes = status.performance.activationPayloadRetainedBytes;
    diagnosticsSnapshot.retainedActivationPayloadBytes = status.preview.activationPayloadRetainedBytes
        + status.performance.activationPayloadRetainedBytes;
    diagnosticsSnapshot.previewPreparationCacheHits = status.preview.preparationCacheHitCount;
    diagnosticsSnapshot.previewPreparationCacheMisses = status.preview.preparationCacheMissCount;
    diagnosticsSnapshot.publishedPreparationCacheHits = status.performance.preparationCacheHitCount;
    diagnosticsSnapshot.publishedPreparationCacheMisses = status.performance.preparationCacheMissCount;
    diagnosticsSnapshot.previewPreparationCacheHitRate = computePreparationCacheHitRate(
        diagnosticsSnapshot.previewPreparationCacheHits,
        diagnosticsSnapshot.previewPreparationCacheMisses);
    diagnosticsSnapshot.publishedPreparationCacheHitRate = computePreparationCacheHitRate(
        diagnosticsSnapshot.publishedPreparationCacheHits,
        diagnosticsSnapshot.publishedPreparationCacheMisses);
    diagnosticsSnapshot.previewFindings = status.preview.findings;
    diagnosticsSnapshot.publishedFindings = status.performance.findings;
    if (status.performance.playableRangeAvailable && status.performance.available)
    {
        diagnosticsSnapshot.playableRangeAvailable = true;
        diagnosticsSnapshot.lowestPlayableNote = status.performance.lowestPlayableNote;
        diagnosticsSnapshot.highestPlayableNote = status.performance.highestPlayableNote;
        diagnosticsSnapshot.playableRangeSource = "published";
    }
    else if (status.preview.playableRangeAvailable && status.preview.available)
    {
        diagnosticsSnapshot.playableRangeAvailable = true;
        diagnosticsSnapshot.lowestPlayableNote = status.preview.lowestPlayableNote;
        diagnosticsSnapshot.highestPlayableNote = status.preview.highestPlayableNote;
        diagnosticsSnapshot.playableRangeSource = "preview";
    }
    else
    {
        diagnosticsSnapshot.playableRangeSource = "default";
    }
    diagnosticsSnapshot.surfaceStateSource = resolveDraftSurfaceSource(status);
    diagnosticsSnapshot.draftPlaybackEvent = status.lastEvent;
}

void syncPreparedPlaybackWorkerIntoDiagnostics(const PreparedPlaybackWorkerStatus& workerStatus,
                                               EngineDiagnosticsSnapshot& diagnosticsSnapshot)
{
    diagnosticsSnapshot.preparedScheduler = workerStatus;
    diagnosticsSnapshot.preparedWorkerPendingCount = workerStatus.pendingWorkCount;
    diagnosticsSnapshot.preparedWorkerConfiguredMaxPendingCount = workerStatus.configuredMaxPendingWorkCount;
    diagnosticsSnapshot.preparedWorkerConfiguredMaxInFlightCount = workerStatus.configuredMaxInFlightWorkCount;
    diagnosticsSnapshot.preparedWorkerCancellationCount = workerStatus.cancellationCount;
    diagnosticsSnapshot.preparedWorkerSupersededCount = workerStatus.supersededCount;
    diagnosticsSnapshot.preparedWorkerFailureCount = workerStatus.failureCount;
    diagnosticsSnapshot.preparedWorkerMaxPendingCount = workerStatus.maxPendingWorkCount;
    diagnosticsSnapshot.preparedWorkerActiveOwnershipRecordCount = workerStatus.activeOwnershipRecordCount;
    diagnosticsSnapshot.preparedWorkerActiveOwnershipBytes = workerStatus.activeOwnershipBytes;
    diagnosticsSnapshot.preparedWorkerRetiredOwnershipRecordCount = workerStatus.retiredOwnershipRecordCount;
    diagnosticsSnapshot.preparedWorkerRetiredBytes = workerStatus.retiredBytesAwaitingCleanup;
    diagnosticsSnapshot.preparedWorkerEvent = workerStatus.lastEvent;
    diagnosticsSnapshot.preparedWorkerLastCancellationLane = workerStatus.lastCancellationLane;
    diagnosticsSnapshot.preparedWorkerLastCancellationReason = workerStatus.lastCancellationReason;
    diagnosticsSnapshot.preparedWorkerLastSupersededLane = workerStatus.lastSupersededLane;
    diagnosticsSnapshot.preparedWorkerLastSupersededReason = workerStatus.lastSupersededReason;
}

void syncSessionSelectionsIntoDiagnostics(const RuntimeSessionStateSnapshot& sessionState,
                                          EngineDiagnosticsSnapshot& diagnosticsSnapshot)
{
    diagnosticsSnapshot.presetId = sessionState.presetId;
    diagnosticsSnapshot.loadProfileId = sessionState.loadProfileId;
    diagnosticsSnapshot.selectedArticulationId = sessionState.selectedArticulationId;
}

const RuntimeArticulationDefinition* findArticulationDefinition(const RuntimeInstrumentModel& instrument,
                                                                const std::string& articulationId)
{
    const auto iterator = std::find_if(instrument.articulations.begin(),
                                       instrument.articulations.end(),
                                       [&](const RuntimeArticulationDefinition& articulation)
                                       {
                                           return articulation.id == articulationId;
                                       });
    return iterator != instrument.articulations.end() ? &(*iterator) : nullptr;
}

std::string resolveArticulationName(const RuntimeManifestLoadResult& manifest,
                                    const RuntimeSessionStateSnapshot& sessionState)
{
    if (!manifest.loaded)
        return {};

    if (const auto* articulation = findArticulationDefinition(manifest.instrument, sessionState.selectedArticulationId))
        return articulation->name;

    return {};
}

std::string buildLoadIndicator(const RuntimeManifestLoadResult& manifest,
                               const RuntimeStreamLoadResult& stream,
                               const RuntimeSessionStateSnapshot& sessionState)
{
    if (!manifest.loaded)
        return "Manifest unavailable";

    if (!stream.loaded)
        return "Stream unavailable";

    if (!sessionState.transientMetrics.lastFailure.empty())
        return "Attention required";

    return "Reference instrument ready";
}

template <typename TPredicate>
bool waitUntil(TPredicate predicate, std::chrono::milliseconds timeout)
{
    const auto deadline = Clock::now() + timeout;

    while (Clock::now() < deadline)
    {
        if (predicate())
            return true;

        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }

    return predicate();
}

void drainVoice(RuntimeVoice& voice, RuntimeStreamingService& service)
{
    const auto drained = waitUntil(
        [&]
        {
            const auto advance = voice.advanceFrames(8192, service);
            return advance.voiceFinished
                || voice.getSnapshot().state == RuntimeVoiceLifecycleState::finished;
        },
        std::chrono::milliseconds(1500));

    if (!drained)
        throw std::runtime_error("Diagnostics voice did not finish draining in time.");
}

std::vector<RuntimeMacroValueSnapshot> toVoiceMacroValues(const RuntimeSessionStateSnapshot& sessionState)
{
    std::vector<RuntimeMacroValueSnapshot> macroValues;
    macroValues.reserve(sessionState.macroValues.size());

    for (const auto& macroValue : sessionState.macroValues)
        macroValues.push_back({ macroValue.id, macroValue.value });

    return macroValues;
}

std::string readTextFile(const std::string& path)
{
    std::ifstream input(path, std::ios::binary);
    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

void writeTextFile(const fs::path& path, const std::string& text)
{
    fs::create_directories(path.parent_path());
    std::ofstream output(path, std::ios::binary);
    output << text;
}

std::string getFailureCategoryId(const EngineContentFailureCategory category)
{
    switch (category)
    {
    case EngineContentFailureCategory::missingContent:
        return "missing-content";
    case EngineContentFailureCategory::badChecksum:
        return "bad-checksum";
    case EngineContentFailureCategory::schemaMismatch:
        return "schema-mismatch";
    case EngineContentFailureCategory::partialCompiledArtifact:
        return "partial-compiled-artifact";
    }

    return "unknown";
}

fs::path getFailureFixturePath(const EngineContentFailureCategory category)
{
    const auto runtimeRoot = fs::path(getPhase1RuntimeRootPath());

    switch (category)
    {
    case EngineContentFailureCategory::missingContent:
        return runtimeRoot / "negative-corpus" / "missing-sample-file" / "missing-sample-file.drinst";
    case EngineContentFailureCategory::schemaMismatch:
        return runtimeRoot / "negative-corpus" / "schema-mismatch" / "schema-mismatch.drinst";
    case EngineContentFailureCategory::partialCompiledArtifact:
        return runtimeRoot / "negative-corpus" / "partial-compiled-artifact" / "partial-compiled-artifact.drinst";
    case EngineContentFailureCategory::badChecksum:
        break;
    }

    return {};
}

fs::path buildChecksumMismatchFixture()
{
    const auto referencePath = fs::path(getPhase1ReferenceStreamContainerPath());
    const auto tempDirectory = fs::temp_directory_path() / "drs-phase1-failure-probes";
    const auto outputPath = tempDirectory / "checksum-mismatch.drstrm";

    auto checksumCorruptJson = ordered_json::parse(readTextFile(referencePath.generic_string()));
    for (auto& sample : checksumCorruptJson["samples"])
    {
        const fs::path sourcePath(sample["sourcePath"].get<std::string>());
        sample["sourcePath"] = (referencePath.parent_path() / sourcePath).lexically_normal().generic_string();
    }
    checksumCorruptJson["samples"][0]["sourceChecksumHex"] = "deadbeefdeadbeef";
    writeTextFile(outputPath, checksumCorruptJson.dump(2) + "\n");
    return outputPath;
}

PlaybackSnapshotBuildResult buildPerformancePackagePlaybackSnapshot(
    const PerformancePackageLoadResult& packageLoad)
{
    PlaybackSnapshotBuildResult result;
    result.built = false;
    result.activationEligible = false;
    result.buildId = 1;
    result.cancellationId = 1;
    result.requestedDraftRevision = 0;
    result.activationRequested = true;
    result.lifecycleState = PlaybackSnapshotLifecycleState::failed;
    result.state = "Performance package snapshot failed";

    if (!packageLoad.loaded)
    {
        addSnapshotFinding(result,
                           PlaybackSnapshotFindingSeverity::error,
                           "package-not-loaded",
                           "package",
                           "The performance package must load before activation can be prepared.");
        return result;
    }

    const auto& manifest = packageLoad.manifest;
    const auto& instrument = packageLoad.instrument.instrument;
    const auto& stream = packageLoad.stream.container;

    result.snapshot.schemaName = "drs.playbackSnapshot";
    result.snapshot.schemaVersion = 1;
    result.snapshot.projectId = manifest.packageId;
    result.snapshot.displayName = !manifest.displayName.empty()
        ? manifest.displayName
        : instrument.displayName;
    result.snapshot.sourceProjectSchemaName = manifest.schemaName;
    result.snapshot.sourceProjectSchemaVersion = manifest.schemaVersion;
    result.snapshot.sourceAuthoringSchemaName = manifest.schemaName;
    result.snapshot.sourceAuthoringSchemaVersion = manifest.schemaVersion;
    result.snapshot.draftRevision = 0;
    result.snapshot.masterGainDb = manifest.masterGainDb;
    result.snapshot.notes = manifest.notes;
    result.snapshot.notes.insert(result.snapshot.notes.end(),
                                 instrument.validationNotes.begin(),
                                 instrument.validationNotes.end());
    result.snapshot.notes.insert(result.snapshot.notes.end(),
                                 stream.notes.begin(),
                                 stream.notes.end());

    std::unordered_map<std::string, std::string> sampleIdByPath;
    sampleIdByPath.reserve(stream.samples.size());
    result.snapshot.sampleIdentities.reserve(stream.samples.size());
    for (const auto& sample : stream.samples)
    {
        PlaybackSnapshotSampleIdentity identity;
        identity.sampleSourceId = sample.sampleId;
        identity.sourcePath = sample.sourcePath;
        identity.role = sample.role;
        result.snapshot.sampleIdentities.push_back(std::move(identity));
        sampleIdByPath.emplace(sample.sourcePath, sample.sampleId);
    }

    result.snapshot.macroDefaults.reserve(instrument.macros.size());
    for (const auto& macro : instrument.macros)
    {
        PlaybackSnapshotMacroDefault snapshotMacro;
        snapshotMacro.id = macro.id;
        snapshotMacro.name = macro.name;
        snapshotMacro.defaultValue = macro.defaultValue;
        snapshotMacro.minValue = macro.minValue;
        snapshotMacro.maxValue = macro.maxValue;
        snapshotMacro.exposedInPerformance = true;
        result.snapshot.macroDefaults.push_back(std::move(snapshotMacro));
    }

    result.snapshot.articulationDefinitions.reserve(instrument.articulations.size());
    for (const auto& articulation : instrument.articulations)
    {
        result.snapshot.articulationDefinitions.push_back({
            articulation.id,
            articulation.name,
            articulation.isDefault,
            0,
            articulation.activation
        });
    }

    std::unordered_map<std::string, double> manifestGroupGainById;
    manifestGroupGainById.reserve(manifest.groupRoutes.size());
    for (const auto& route : manifest.groupRoutes)
        manifestGroupGainById.emplace(route.groupId, route.gainDb);

    std::unordered_map<std::string, std::size_t> groupRouteIndices;
    result.snapshot.groupRoutes.reserve(instrument.groups.size());
    for (const auto& group : instrument.groups)
    {
        PlaybackSnapshotGroupRoute route;
        route.groupId = group.id;
        route.displayName = group.name;
        route.articulationIds = group.articulationIds;
        route.routingSourceId = "master";
        route.workspaceVisible = true;
        route.gainDb = manifestGroupGainById.count(group.id) != 0
            ? manifestGroupGainById.at(group.id)
            : group.gainDb;
        route.routingBusId = "master";
        groupRouteIndices.emplace(group.id, result.snapshot.groupRoutes.size());
        result.snapshot.groupRoutes.push_back(std::move(route));
    }

    if (result.snapshot.groupRoutes.empty())
    {
        std::unordered_set<std::string> implicitGroupIds;
        for (const auto& zone : instrument.zones)
        {
            if (zone.groupId.empty() || !implicitGroupIds.insert(zone.groupId).second)
                continue;

            PlaybackSnapshotGroupRoute route;
            route.groupId = zone.groupId;
            route.displayName = zone.groupId;
            route.routingSourceId = "master";
            route.workspaceVisible = true;
            if (manifestGroupGainById.count(route.groupId) != 0)
                route.gainDb = manifestGroupGainById.at(route.groupId);
            route.routingBusId = "master";
            groupRouteIndices.emplace(route.groupId, result.snapshot.groupRoutes.size());
            result.snapshot.groupRoutes.push_back(std::move(route));
        }
    }

    std::unordered_map<std::string, std::size_t> articulationRouteIndices;
    result.snapshot.zones.reserve(instrument.zones.size());
    for (const auto& zone : instrument.zones)
    {
        const auto sampleIdIterator = sampleIdByPath.find(zone.samplePath);
        if (sampleIdIterator == sampleIdByPath.end())
        {
            addSnapshotFinding(result,
                               PlaybackSnapshotFindingSeverity::error,
                               "package-zone-sample-missing",
                               "instrument.zones",
                               "Zone '" + zone.id + "' references sample path '" + zone.samplePath
                                   + "' that is not present in the compiled package stream.");
            continue;
        }

        PlaybackSnapshotZone snapshotZone;
        snapshotZone.id = zone.id;
        snapshotZone.sampleSourceId = sampleIdIterator->second;
        snapshotZone.displayName = zone.id;
        snapshotZone.groupId = zone.groupId;
        snapshotZone.articulationId = zone.articulationId;
        snapshotZone.rootKey = zone.rootKey;
        snapshotZone.keyLow = zone.keyLow;
        snapshotZone.keyHigh = zone.keyHigh;
        snapshotZone.velocityLow = zone.velocityLow;
        snapshotZone.velocityHigh = zone.velocityHigh;
        snapshotZone.velocityCrossfade = zone.velocityCrossfade;
        snapshotZone.velocityCrossfadeRuntime = zone.velocityCrossfadeRuntime;
        snapshotZone.gainDb = zone.gainDb;
        snapshotZone.pan = 0.0;
        snapshotZone.sampleStartFrame = zone.sampleStartFrame;
        snapshotZone.releaseSeconds = zone.releaseSeconds;
        snapshotZone.roundRobin = zone.roundRobin;
        snapshotZone.roundRobinLength = zone.roundRobinLength;
        snapshotZone.roundRobinPosition = zone.roundRobinPosition;
        snapshotZone.triggerMode = zone.triggerMode;
        snapshotZone.performance = zone.performance;
        snapshotZone.exclusiveGroupId = zone.exclusiveGroupId;
        snapshotZone.exclusiveTargetGroupIds = zone.exclusiveTargetGroupIds;
        snapshotZone.chokeReleaseSeconds = zone.chokeReleaseSeconds;
        snapshotZone.fineTuneCents = zone.fineTuneCents;
        snapshotZone.amplitudeVelocityTracking = zone.amplitudeVelocityTracking;
        snapshotZone.controllerConditions = zone.controllerConditions;
        result.snapshot.zones.push_back(std::move(snapshotZone));

        if (!zone.articulationId.empty())
        {
            auto [iterator, inserted] = articulationRouteIndices.emplace(
                zone.articulationId, result.snapshot.articulationRoutes.size());
            if (inserted)
                result.snapshot.articulationRoutes.push_back({ zone.articulationId, {} });
            result.snapshot.articulationRoutes[iterator->second].zoneIds.push_back(zone.id);
        }

        if (const auto groupIterator = groupRouteIndices.find(zone.groupId);
            groupIterator != groupRouteIndices.end())
        {
            auto& route = result.snapshot.groupRoutes[groupIterator->second];
            route.zoneIds.push_back(zone.id);
            if (route.auditionAnchorZoneId.empty())
                route.auditionAnchorZoneId = zone.id;
            if (!zone.articulationId.empty()
                && std::find(route.articulationIds.begin(),
                             route.articulationIds.end(),
                             zone.articulationId) == route.articulationIds.end())
            {
                route.articulationIds.push_back(zone.articulationId);
            }
        }
    }

    result.snapshot.roundRobinResetRules = instrument.roundRobinResetRules;
    result.snapshot.controllerDefaults = instrument.controllerDefaults;

    if (result.snapshot.sampleIdentities.empty())
    {
        addSnapshotFinding(result,
                           PlaybackSnapshotFindingSeverity::error,
                           "no-sample-identities",
                           "stream.samples",
                           "A playable package requires at least one compiled stream sample.");
    }

    if (result.snapshot.zones.empty())
    {
        addSnapshotFinding(result,
                           PlaybackSnapshotFindingSeverity::error,
                           "no-playable-zones",
                           "instrument.zones",
                           "A playable package requires at least one runtime zone.");
    }

    const auto hasErrors = std::any_of(result.findings.begin(),
                                       result.findings.end(),
                                       [](const auto& finding)
                                       {
                                           return finding.severity
                                               == PlaybackSnapshotFindingSeverity::error;
                                       });
    if (!hasErrors)
    {
        result.snapshot.dspGraphDigest = computePlaybackSnapshotDspGraphDigest(result.snapshot);
        result.snapshot.contentDigest = "fnv1a64:"
            + computeFnv1a64Digest(serializeImmutablePlaybackSnapshot(result.snapshot));
        result.built = true;
        result.activationEligible = true;
        result.lifecycleState = PlaybackSnapshotLifecycleState::ready;
        result.state = "Performance package snapshot ready";
    }

    return result;
}

SamplerRenderModelBuildResult buildPerformancePackageRenderModel(
    const PerformancePackageLoadResult& packageLoad,
    const PlaybackActivationPayloadPtr& payload)
{
    SamplerRenderModelBuildOptions options;
    const auto sessionState = buildDefaultRuntimeSessionState(packageLoad.instrument);
    options.selectedArticulationId = sessionState.selectedArticulationId;
    options.fixedVelocity = computeTonePreviewVelocity(sessionState, 64);
    options.midiNoteOffset = computeMotionPreviewNote(sessionState, 60) - 60;
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
                return route.articulationId == "default" && !route.zoneIds.empty();
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
    return buildSamplerRenderModel(payload, options);
}

PreparedPerformancePackageActivationResult buildPreparedPerformancePackageActivation(
    const PerformancePackageLoadResult& packageLoad,
    const PerformancePackagePreparationTimings& priorTimings)
{
    PreparedPerformancePackageActivationResult result;
    result.failureCategory = PerformancePackageFailureCategory::playbackCompatibilityFailure;
    result.state = "Performance package activation preparation failed";
    result.packageLoad = packageLoad;
    result.timings = priorTimings;

    const auto snapshotStartedAt = Clock::now();
    result.snapshotResult = buildPerformancePackagePlaybackSnapshot(packageLoad);
    result.timings.snapshotBuildMicros = static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(Clock::now() - snapshotStartedAt).count());
    if (!result.snapshotResult.built || !result.snapshotResult.activationEligible)
    {
        result.state = result.snapshotResult.state;
        for (const auto& finding : result.snapshotResult.findings)
        {
            if (finding.severity == PlaybackSnapshotFindingSeverity::error)
                result.issues.push_back(finding.message);
        }
        if (result.issues.empty())
            result.issues.push_back("The performance package snapshot could not be activated.");
        result.timings.totalMicros = result.timings.packageLoadMicros + result.timings.snapshotBuildMicros;
        return result;
    }

    PreparedPlaybackService preparedPlayback("phase1-prepared-playback-v2", 1, false);
    const auto preparedRequest = preparedPlayback.requestBuild(result.snapshotResult);
    if (!preparedRequest.accepted)
    {
        result.state = preparedRequest.state;
        result.issues.push_back(preparedRequest.state);
        result.timings.totalMicros = result.timings.packageLoadMicros + result.timings.snapshotBuildMicros;
        return result;
    }

    const auto preparedStartedAt = Clock::now();
    result.preparedResult = preparedPlayback.prepare(preparedRequest,
                                                     result.snapshotResult,
                                                     packageLoad.stream);
    result.timings.preparedBuildMicros = static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(Clock::now() - preparedStartedAt).count());
    if (!result.preparedResult.built || !result.preparedResult.activationEligible)
    {
        result.state = result.preparedResult.state;
        for (const auto& finding : result.preparedResult.findings)
        {
            if (finding.severity == PlaybackSnapshotFindingSeverity::error)
                result.issues.push_back(finding.message);
        }
        if (result.issues.empty())
            result.issues.push_back("Prepared playback could not be built from the performance package.");
        result.timings.totalMicros = result.timings.packageLoadMicros
            + result.timings.snapshotBuildMicros
            + result.timings.preparedBuildMicros;
        return result;
    }

    const auto payloadStartedAt = Clock::now();
    result.activationPayload = buildPlaybackActivationPayload(
        PlaybackActivationLane::performance,
        result.snapshotResult.requestedDraftRevision,
        &result.snapshotResult,
        &result.preparedResult);
    result.timings.activationPayloadMicros = static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(Clock::now() - payloadStartedAt).count());
    result.timings.totalMicros = result.timings.packageLoadMicros
        + result.timings.snapshotBuildMicros
        + result.timings.preparedBuildMicros
        + result.timings.activationPayloadMicros;
    if (result.activationPayload == nullptr)
    {
        result.issues.push_back("The performance package activation payload could not be constructed.");
        return result;
    }

    const auto renderModelStartedAt = Clock::now();
    const auto renderModel = buildPerformancePackageRenderModel(
        packageLoad, result.activationPayload);
    result.timings.renderModelBuildMicros = static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(
            Clock::now() - renderModelStartedAt).count());
    result.timings.totalMicros += result.timings.renderModelBuildMicros;
    if (!renderModel.built || renderModel.model == nullptr)
    {
        result.issues.push_back(renderModel.findings.empty()
            ? std::string("The performance package render model could not be constructed.")
            : renderModel.findings.front().message);
        return result;
    }
    result.renderModel = renderModel.model;
    // The immutable activation payload and render model own the published copies.
    // Release the worker's mutable construction graphs before handing the result
    // to the message thread so activation does not reclaim corpus-scale vectors.
    result.snapshotResult = {};
    result.preparedResult = {};

    result.prepared = true;
    result.failureCategory = PerformancePackageFailureCategory::none;
    result.state = "Performance package activation prepared";
    return result;
}
} // namespace

EngineFacade::EngineFacade()
    : bundledReferenceManifest(loadPhase1ReferenceInstrumentManifest()),
      referenceManifest(bundledReferenceManifest),
      preparedPlaybackService("phase1-prepared-playback-v2", 2, true)
{
    if (referenceManifest.loaded)
    {
        currentSessionState = buildDefaultRuntimeSessionState(referenceManifest);
        currentSessionState.transientMetrics.integrationState = "Default preset state loaded";
        bundledReferenceStream = loadPhase1ReferenceStreamContainer();
        referenceStream = bundledReferenceStream;
        preparedPlaybackService.setBackgroundWorkerStream(referenceStream);
        referenceInstrumentActive = referenceStream.loaded;
    }
    else
    {
        currentSessionState.transientMetrics.integrationState = "Reference manifest unavailable";
        currentSessionState.transientMetrics.lastFailure = referenceManifest.state;
        referenceInstrumentActive = false;
    }

    initializeDraftPlaybackContract(false);
    refreshDiagnosticsSnapshot();
    markStateChanged();
}

EnginePerformancePackageActivationResult EngineFacade::activatePerformancePackageSession(
    const PerformancePackageLoadResult& packageLoad)
{
    return activatePreparedPerformancePackageSession(
        preparePerformancePackageActivation(packageLoad));
}

EnginePerformancePackageActivationResult EngineFacade::openPerformancePackageSession(
    const PerformancePackageLoadResult& packageLoad)
{
    EnginePerformancePackageActivationResult result;
    result.failureCategory = packageLoad.failureCategory;
    result.state = packageLoad.state.empty()
        ? std::string("Performance package open failed")
        : packageLoad.state;
    result.issues = packageLoad.issues;

    if (!packageLoad.loaded)
        return result;

    clearPendingPreparedCompletions();
    ++performancePublishProjectGeneration;
    performancePublishController.reset(true, true);
    draftPlaybackContract.closeProject();
    authoringProject = {};
    packagePerformanceActivationPayload.reset();
    packagePerformanceRenderModel.reset();
    packageBackgroundArtworkPayloadId.clear();
    packageBackgroundArtworkJpgBytes.reset();
    referenceManifest = packageLoad.instrument;
    referenceStream = packageLoad.stream;
    preparedPlaybackService.setBackgroundWorkerStream(referenceStream);
    referenceInstrumentActive = true;
    currentSessionState = buildDefaultRuntimeSessionState(referenceManifest);
    if (!packageLoad.manifest.defaultLoadProfile.empty())
        currentSessionState.loadProfileId = packageLoad.manifest.defaultLoadProfile;
    if (packageLoad.backgroundImage.loaded)
    {
        packageBackgroundArtworkPayloadId = packageLoad.backgroundImage.payload.payloadId;
        packageBackgroundArtworkJpgBytes = std::make_shared<const std::vector<std::uint8_t>>(
            packageLoad.backgroundImage.payload.plaintextBytes);
    }

    const auto snapshotResult = buildPerformancePackagePlaybackSnapshot(packageLoad);
    if (!snapshotResult.built || !snapshotResult.activationEligible)
    {
        currentSessionState.transientMetrics.integrationState = "Performance package open failed";
        currentSessionState.transientMetrics.lastFailure =
            summarizeSnapshotFindings(snapshotResult.findings);
        refreshDiagnosticsSnapshot();
        markStateChanged();
        result.failureCategory = PerformancePackageFailureCategory::playbackCompatibilityFailure;
        result.state = snapshotResult.state.empty()
            ? std::string("Performance package open failed")
            : snapshotResult.state;
        result.issues = snapshotResult.findings.empty()
            ? std::vector<std::string> { "The performance package snapshot could not be activated." }
            : std::vector<std::string> {};
        for (const auto& finding : snapshotResult.findings)
        {
            if (finding.severity == PlaybackSnapshotFindingSeverity::error)
                result.issues.push_back(finding.message);
        }
        return result;
    }

    const auto canQueuePreparedBuild = packageLoad.stream.container.payloadEmbedded;
    const auto queuedPreparedBuild = canQueuePreparedBuild
        ? enqueuePerformancePackagePreparedBuild(snapshotResult)
        : false;
    currentSessionState.transientMetrics.integrationState = canQueuePreparedBuild
        ? (queuedPreparedBuild
               ? std::string("Performance package loaded; preparing playback")
               : std::string("Performance package loaded"))
        : std::string("Performance package loaded; sample payload deferred");
    currentSessionState.transientMetrics.lastFailure = canQueuePreparedBuild
        ? (queuedPreparedBuild ? std::string {} : std::string("Prepared playback queue is full."))
        : std::string {};
    previewPlaybackSnapshot = {};
    syncPreviewSnapshotFromDraftPlayback();
    refreshDiagnosticsSnapshot();
    markStateChanged();

    result.activated = true;
    result.failureCategory = PerformancePackageFailureCategory::none;
    result.state = canQueuePreparedBuild
        ? (queuedPreparedBuild
               ? std::string("Performance package opened; preparing playback")
               : std::string("Performance package opened"))
        : std::string("Performance package opened");
    result.issues = canQueuePreparedBuild
        ? (queuedPreparedBuild
               ? std::vector<std::string> {}
               : std::vector<std::string> {
                     "Prepared playback is still pending because the preparation queue is full." })
        : std::vector<std::string> {};
    return result;
}

EnginePerformancePackageActivationResult EngineFacade::activatePreparedPerformancePackageSession(
    PreparedPerformancePackageActivationResult preparedActivation)
{
    EnginePerformancePackageActivationResult result;
    result.failureCategory = preparedActivation.failureCategory;
    result.state = preparedActivation.state.empty()
        ? std::string("Performance package activation failed")
        : preparedActivation.state;
    result.issues = preparedActivation.issues;

    if (!preparedActivation.prepared)
        return result;

    packagePerformanceActivationPayload = preparedActivation.activationPayload;
    packagePerformanceRenderModel = preparedActivation.renderModel;
    if (packagePerformanceActivationPayload == nullptr)
    {
        result.failureCategory = PerformancePackageFailureCategory::playbackCompatibilityFailure;
        result.state = "Performance package activation failed";
        result.issues.push_back("The performance package activation payload could not be constructed.");
        return result;
    }
    if (packagePerformanceRenderModel == nullptr)
    {
        result.failureCategory = PerformancePackageFailureCategory::playbackCompatibilityFailure;
        result.state = "Performance package activation failed";
        result.issues.push_back("The prepared performance package render model is unavailable.");
        packagePerformanceActivationPayload.reset();
        packagePerformanceRenderModel.reset();
        return result;
    }
    if (packagePerformanceActivationPayload->prepared != nullptr)
        for (const auto& sample : packagePerformanceActivationPayload->prepared->samples)
            preparedPlaybackService.registerPageServiceSource(sample.dataSource);

    auto& packageLoad = preparedActivation.packageLoad;
    clearPendingPreparedCompletions();
    ++performancePublishProjectGeneration;
    performancePublishController.reset(true, true);
    draftPlaybackContract.closeProject();
    authoringProject = {};
    packageBackgroundArtworkPayloadId.clear();
    packageBackgroundArtworkJpgBytes.reset();
    referenceManifest = std::move(packageLoad.instrument);
    referenceStream = std::move(packageLoad.stream);
    referenceInstrumentActive = true;
    currentSessionState = buildDefaultRuntimeSessionState(referenceManifest);
    if (!packageLoad.manifest.defaultLoadProfile.empty())
        currentSessionState.loadProfileId = packageLoad.manifest.defaultLoadProfile;
    if (packageLoad.backgroundImage.loaded)
    {
        packageBackgroundArtworkPayloadId = packageLoad.backgroundImage.payload.payloadId;
        packageBackgroundArtworkJpgBytes = std::make_shared<const std::vector<std::uint8_t>>(
            packageLoad.backgroundImage.payload.plaintextBytes);
    }
    currentSessionState.transientMetrics.integrationState = "Performance package loaded";
    currentSessionState.transientMetrics.lastFailure.clear();
    previewPlaybackSnapshot = {};
    syncPreviewSnapshotFromDraftPlayback();
    refreshDiagnosticsSnapshot();
    markStateChanged();

    result.activated = true;
    result.failureCategory = PerformancePackageFailureCategory::none;
    result.state = "Performance package activated";
    result.issues.clear();
    return result;
}

void EngineFacade::restoreBundledReferenceRuntimeSession()
{
    bundledReferenceManifest = loadPhase1ReferenceInstrumentManifest();
    if (bundledReferenceManifest.loaded)
        bundledReferenceStream = loadPhase1ReferenceStreamContainer();
    else
        bundledReferenceStream = {};

    referenceManifest = bundledReferenceManifest;
    referenceStream = bundledReferenceStream;
    packagePerformanceActivationPayload.reset();
    packagePerformanceRenderModel.reset();
    packageBackgroundArtworkPayloadId.clear();
    packageBackgroundArtworkJpgBytes.reset();
    preparedPlaybackService.setBackgroundWorkerStream(referenceStream);

    if (referenceManifest.loaded)
    {
        currentSessionState = buildDefaultRuntimeSessionState(referenceManifest);
        currentSessionState.transientMetrics.integrationState = "Default preset state loaded";
        currentSessionState.transientMetrics.lastFailure.clear();
        referenceInstrumentActive = referenceStream.loaded;
    }
    else
    {
        currentSessionState = {};
        currentSessionState.transientMetrics.integrationState = "Reference manifest unavailable";
        currentSessionState.transientMetrics.lastFailure = referenceManifest.state;
        referenceInstrumentActive = false;
    }

    previewPlaybackSnapshot = {};
    syncPreviewSnapshotFromDraftPlayback();
    refreshDiagnosticsSnapshot();
    markStateChanged();
}

std::vector<HiseFrontendExportProfile> EngineFacade::getFrontendExportProfiles() const
{
    return {
        {
            "HISE frontend plugin",
            HiseFrontendTargetKind::plugin,
            true,
            false,
            true,
            false,
            false,
            true,
            "hi_backend/backend/ProjectTemplate.cpp",
            "Selected first integration target. Uses USE_FRONTEND, disables IS_STANDALONE_APP, and expects VST SDK inputs for exporter workflows."
        },
        {
            "HISE frontend standalone",
            HiseFrontendTargetKind::standalone,
            true,
            true,
            false,
            true,
            true,
            false,
            "hi_backend/backend/StandaloneProjectTemplate.cpp",
            "Frontend export for a standalone app target. Enables IS_STANDALONE_APP and typically needs the ASIO SDK for Windows low-latency device support."
        }
    };
}

bool EngineFacade::serviceBackgroundWork()
{
    const auto serviceStartedAtMicros = monotonicMicros();
    const auto retiredCacheEntries = preparedPlaybackService.serviceRetiredCacheCleanup(1);
    const auto appliedCompletions = pumpPreparedPlaybackWorkerCompletions();
    preparedPlaybackService.recordMessageThreadServiceDuration(
        monotonicMicros() - serviceStartedAtMicros);

    if (retiredCacheEntries != 0 || packagePerformanceActivationPayload != nullptr)
        refreshDiagnosticsSnapshot();

    return retiredCacheEntries != 0 || appliedCompletions;
}

EngineStatusSnapshot EngineFacade::getStatusSnapshot() const
{
    using namespace generated;

    std::ostringstream detail;
    const auto profiles = getFrontendExportProfiles();
    const auto linkedFrontend = getLinkedHiseFrontendSnapshot();
    const auto contentSnapshot = getHiseProjectContentSnapshot();
    const auto runtimeManifest = loadPhase1ReferenceInstrument();
    const auto runtimeStream = loadPhase1ReferenceStream();
    const auto diagnostics = getDiagnosticsSnapshot();

    detail << "HISE root: " << hiseVendorRoot << "\n";
    detail << "Pinned HISE commit: " << hiseCurrentGitHash << "\n";
    detail << "hi_core module version: " << hiseHiCoreVersion << "\n";
    detail << "REST API version macro: " << hiseRestApiVersion << "\n";
    detail << "Nested HISE JUCE snapshot: " << (hiseNestedJucePresent ? "present" : "missing") << "\n";
    detail << "Projucer Windows binary: " << (hiseProjucerWindowsPresent ? "present" : "missing") << "\n";
    detail << "HISE SDK zip: " << (hiseSdkZipPresent ? "present" : "missing") << "\n";
    detail << "HISE SDK extracted: " << (hiseSdkExtracted ? "yes" : "no") << "\n";
    detail << "Linked frontend bridge: " << (linkedFrontend.linked ? "yes" : "no") << "\n";

    if (linkedFrontend.linked)
    {
        detail << "Linked plugin name: " << linkedFrontend.pluginName << "\n";
        detail << "Linked manufacturer: " << linkedFrontend.manufacturer << "\n";
        detail << "Linked HISE build sub-version: " << linkedFrontend.buildSubVersion << "\n";
        detail << "Linked macro profile: USE_BACKEND=" << (linkedFrontend.useBackend ? "1" : "0")
               << ", USE_FRONTEND=" << (linkedFrontend.useFrontend ? "1" : "0")
               << ", FRONTEND_IS_PLUGIN=" << (linkedFrontend.frontendIsPlugin ? "1" : "0")
               << ", IS_STANDALONE_APP=" << (linkedFrontend.isStandaloneApp ? "1" : "0")
               << ", IS_STANDALONE_FRONTEND=" << (linkedFrontend.isStandaloneFrontend ? "1" : "0") << "\n";
    }

    detail << "\nProject content seam:\n";
    detail << "Repo root: " << contentSnapshot.repoRoot << "\n";
    detail << "Repo HISE content root: " << contentSnapshot.repoContentRoot
           << " (" << (contentSnapshot.repoContentRootExists ? "present" : "missing") << ")\n";
    detail << "Runtime AppData root: "
           << (contentSnapshot.runtimeAppDataRoot.empty() ? "unavailable" : contentSnapshot.runtimeAppDataRoot) << "\n";
    detail << "Discovered repo user presets: " << contentSnapshot.presetFileCount << "\n";
    detail << "Discovered repo sample maps: " << contentSnapshot.sampleMapFileCount << "\n";
    detail << "Repo content directories:\n";

    for (const auto& directory : contentSnapshot.repoDirectories)
    {
        detail << "- " << directory.name
               << ": " << (directory.exists ? "present" : "missing")
               << ", matching files=" << directory.matchingFileCount
               << ", path=" << directory.absolutePath << "\n";
    }

    detail << "Runtime content directories:\n";

    for (const auto& directory : contentSnapshot.runtimeDirectories)
    {
        detail << "- " << directory.name
               << ": " << (directory.exists ? "present" : "missing")
               << ", matching files=" << directory.matchingFileCount
               << ", path=" << directory.absolutePath << "\n";
    }

    detail << "\nConcrete HISE frontend target profiles:\n";

    for (const auto& profile : profiles)
    {
        std::string sdkSummary;

        if (profile.requiresAsioSdk)
            sdkSummary += "ASIO";

        if (profile.requiresVst3Sdk)
        {
            if (!sdkSummary.empty())
                sdkSummary += ", ";

            sdkSummary += "VST3";
        }

        if (sdkSummary.empty())
            sdkSummary = "none";

        detail << "- " << profile.name << "\n";
        detail << "  template: " << profile.sourceTemplate << "\n";
        detail << "  USE_FRONTEND=1"
               << ", IS_STANDALONE_APP=" << (profile.isStandaloneApp ? "1" : "0")
               << ", FRONTEND_IS_PLUGIN=" << (profile.frontendIsPlugin ? "1" : "0")
               << ", IS_STANDALONE_FRONTEND=" << (profile.isStandaloneFrontend ? "1" : "0") << "\n";
        detail << "  requires SDKs: " << sdkSummary << "\n";
        detail << "  summary: " << profile.summary << "\n";
    }

    detail << "\nPhase 1 runtime bootstrap:\n";
    detail << "Runtime fixture root: " << getPhase1RuntimeRootPath() << "\n";
    detail << "Reference corpus index: " << getPhase1ReferenceCorpusIndexPath() << "\n";
    detail << "Reference manifest: " << runtimeManifest.manifestPath
           << " (" << (runtimeManifest.manifestFound ? "present" : "missing") << ")\n";
    detail << "Reference load state: " << runtimeManifest.state << "\n";

    if (runtimeManifest.loaded)
    {
        detail << "Loaded instrument: " << runtimeManifest.instrument.displayName
               << " [" << runtimeManifest.instrument.instrumentId << "]\n";
        detail << "Schema: " << runtimeManifest.instrument.schemaName
               << " v" << runtimeManifest.instrument.schemaVersion << "\n";
        detail << "Source project: " << runtimeManifest.instrument.sourceProjectPath << "\n";
        detail << "Compiled stream asset: " << runtimeManifest.instrument.compiledStreamAssetPath << "\n";
        detail << "Load profile: " << runtimeManifest.instrument.defaultLoadProfile << "\n";
        detail << "Counts: macros=" << runtimeManifest.metrics.macroCount
               << ", articulations=" << runtimeManifest.metrics.articulationCount
               << ", groups=" << runtimeManifest.metrics.groupCount
               << ", zones=" << runtimeManifest.metrics.zoneCount << "\n";
        detail << "Streaming seam: " << (runtimeManifest.metrics.usesStreaming ? "present" : "missing")
               << ", total prefetch bytes=" << runtimeManifest.metrics.totalPrefetchBytes << "\n";
        detail << "Baseline metrics: manifest bytes=" << runtimeManifest.metrics.manifestSizeBytes
               << ", load micros=" << runtimeManifest.metrics.loadDurationMicros
               << ", source project resolved=" << (runtimeManifest.metrics.sourceProjectResolved ? "yes" : "no")
               << ", stream asset resolved=" << (runtimeManifest.metrics.compiledStreamAssetResolved ? "yes" : "no") << "\n";
        detail << "Stream reader state: " << runtimeStream.state << "\n";

        if (runtimeStream.loaded)
        {
            detail << "Stream container: " << runtimeStream.container.containerId
                   << ", samples=" << runtimeStream.metrics.sampleCount
                   << ", pages=" << runtimeStream.metrics.pageCount
                   << ", checksum validations=" << runtimeStream.metrics.checksumValidatedCount << "\n";
        }
    }

    detail << "\nCurrent session state:\n";
    detail << "Preset id: " << (currentSessionState.presetId.empty() ? "unavailable" : currentSessionState.presetId) << "\n";
    detail << "Target instrument: "
           << (currentSessionState.targetInstrumentId.empty() ? "unavailable" : currentSessionState.targetInstrumentId)
           << " [" << (currentSessionState.targetInstrumentSchemaName.empty() ? "n/a" : currentSessionState.targetInstrumentSchemaName)
           << " v" << currentSessionState.targetInstrumentSchemaVersion << "]\n";
    detail << "Selected load profile: "
           << (currentSessionState.loadProfileId.empty() ? "unavailable" : currentSessionState.loadProfileId) << "\n";
    detail << "Selected articulation: "
           << (currentSessionState.selectedArticulationId.empty() ? "unavailable" : currentSessionState.selectedArticulationId) << "\n";
    detail << "Macro values: " << buildMacroSummary(currentSessionState) << "\n";
    detail << "Draft playback: draft=" << diagnostics.draftRevision
           << ", preview=" << diagnostics.previewRevision
           << " (" << (diagnostics.previewRevisionState.empty() ? "unavailable" : diagnostics.previewRevisionState) << ")"
           << ", published=" << diagnostics.publishedRevision
           << " (" << toString(diagnostics.publishedPresentationState) << ")\n";
    detail << "Draft playback event: "
           << (diagnostics.draftPlaybackEvent.empty() ? "not reported" : diagnostics.draftPlaybackEvent) << "\n";
    detail << "Snapshot ids: previewBuild=" << diagnostics.previewBuildId
           << ", publishBuild=" << diagnostics.publishedBuildId << "\n";
    detail << "Snapshot digests: preview=" << summarizeDigest(diagnostics.previewContentDigest)
           << ", publish=" << summarizeDigest(diagnostics.publishedContentDigest) << "\n";
    detail << "Snapshot findings: preview=" << diagnostics.previewFindings.size()
           << ", publish=" << diagnostics.publishedFindings.size() << "\n";
    detail << "Prepared playback ids: preview=" << diagnostics.previewPreparedBuildId
           << ", publish=" << diagnostics.publishedPreparedBuildId << "\n";
    detail << "Prepared playback digests: preview=" << summarizeDigest(diagnostics.previewPreparedContentDigest)
           << ", publish=" << summarizeDigest(diagnostics.publishedPreparedContentDigest) << "\n";
    detail << "Prepared playback assets: preview samples=" << diagnostics.previewPreparedSampleCount
           << ", preview streams=" << diagnostics.previewPreparedStreamCount
           << ", preview ownership=" << diagnostics.previewPreparedOwnershipRecordCount
           << ", publish samples=" << diagnostics.publishedPreparedSampleCount
           << ", publish streams=" << diagnostics.publishedPreparedStreamCount
           << ", publish ownership=" << diagnostics.publishedPreparedOwnershipRecordCount << "\n";
    detail << "Playable range: source=" << (diagnostics.playableRangeSource.empty() ? "unavailable" : diagnostics.playableRangeSource)
           << ", available=" << (diagnostics.playableRangeAvailable ? "yes" : "no")
           << ", low=" << diagnostics.lowestPlayableNote
           << ", high=" << diagnostics.highestPlayableNote << "\n";
    detail << "Surface provenance: source=" << (diagnostics.surfaceStateSource.empty() ? "unavailable" : diagnostics.surfaceStateSource)
           << ", renderer=" << (diagnostics.rendererMode.empty() ? "unavailable" : diagnostics.rendererMode) << "\n";
    detail << "Prepared playback residency: previewResidentBytes=" << diagnostics.previewPreparedBytes
           << ", publishResidentBytes=" << diagnostics.publishedPreparedBytes
           << ", previewOwnership=" << diagnostics.previewPreparedOwnershipBytes
           << ", publishOwnership=" << diagnostics.publishedPreparedOwnershipBytes << "\n";
    detail << "Prepared build metrics: previewBuildMicros=" << diagnostics.previewPreparedBuildMicros
           << ", publishBuildMicros=" << diagnostics.publishedPreparedBuildMicros
           << ", previewDecodedBytes=" << diagnostics.previewPreparedDecodedBytes
           << ", publishDecodedBytes=" << diagnostics.publishedPreparedDecodedBytes
           << ", previewSampleDataBytes=" << diagnostics.previewPreparedSampleDataBytes
           << ", publishSampleDataBytes=" << diagnostics.publishedPreparedSampleDataBytes
           << ", previewResidentMatchesOwnership="
           << (diagnostics.previewPreparedBytes == diagnostics.previewPreparedOwnershipBytes ? "yes" : "no")
           << ", publishResidentMatchesOwnership="
           << (diagnostics.publishedPreparedBytes == diagnostics.publishedPreparedOwnershipBytes ? "yes" : "no")
           << ", previewHitRate=" << diagnostics.previewPreparationCacheHitRate
           << ", publishHitRate=" << diagnostics.publishedPreparationCacheHitRate << "\n";
    detail << "Prepared worker: pending=" << diagnostics.preparedWorkerPendingCount
           << ", queueLimit=" << diagnostics.preparedWorkerConfiguredMaxPendingCount
           << ", inFlightLimit=" << diagnostics.preparedWorkerConfiguredMaxInFlightCount
           << ", canceled=" << diagnostics.preparedWorkerCancellationCount
           << ", superseded=" << diagnostics.preparedWorkerSupersededCount
           << ", failures=" << diagnostics.preparedWorkerFailureCount
           << ", maxPending=" << diagnostics.preparedWorkerMaxPendingCount
           << ", activeOwnership=" << diagnostics.preparedWorkerActiveOwnershipRecordCount
           << ", activeBytes=" << diagnostics.preparedWorkerActiveOwnershipBytes
           << ", retiredOwnership=" << diagnostics.preparedWorkerRetiredOwnershipRecordCount
           << ", retiredBytes=" << diagnostics.preparedWorkerRetiredBytes << "\n";
    detail << "Prepared cache policy: workingSets=" << diagnostics.preparedCacheRetentionWorkingSetCount
           << ", workingSetBytes=" << diagnostics.preparedCacheWorkingSetBytes
           << ", budgetBytes=" << diagnostics.preparedCacheByteBudget
           << ", residentBytes=" << diagnostics.preparedCacheResidentBytes
           << ", headroomBytes=" << diagnostics.preparedCacheHeadroomBytes
           << ", state="
           << (diagnostics.preparedCachePressureState.empty() ? "not reported" : diagnostics.preparedCachePressureState)
           << "\n";
    detail << "Prepared worker event: "
           << (diagnostics.preparedWorkerEvent.empty() ? "not reported" : diagnostics.preparedWorkerEvent) << "\n";
    detail << "Prepared worker queue reasons: cancel["
           << (diagnostics.preparedWorkerLastCancellationLane.empty()
                   ? "not reported"
                   : diagnostics.preparedWorkerLastCancellationLane)
           << "]="
           << (diagnostics.preparedWorkerLastCancellationReason.empty()
                   ? "not reported"
                   : diagnostics.preparedWorkerLastCancellationReason)
           << ", supersede["
           << (diagnostics.preparedWorkerLastSupersededLane.empty()
                   ? "not reported"
                   : diagnostics.preparedWorkerLastSupersededLane)
           << "]="
           << (diagnostics.preparedWorkerLastSupersededReason.empty()
                   ? "not reported"
                   : diagnostics.preparedWorkerLastSupersededReason)
           << "\n";
    detail << "State recall status: "
           << (currentSessionState.transientMetrics.integrationState.empty()
                   ? "not reported"
                   : currentSessionState.transientMetrics.integrationState)
           << "\n";

    if (!currentSessionState.transientMetrics.lastFailure.empty())
        detail << "Last state recall failure: " << currentSessionState.transientMetrics.lastFailure << "\n";

    detail << "\nRuntime diagnostics:\n";
    detail << "Headline: " << (diagnostics.headline.empty() ? "unavailable" : diagnostics.headline) << "\n";
    detail << "Load profile: " << (diagnostics.loadProfileId.empty() ? "unavailable" : diagnostics.loadProfileId)
           << ", articulation: "
           << (diagnostics.selectedArticulationId.empty() ? "unavailable" : diagnostics.selectedArticulationId) << "\n";
    detail << "Cache budget: " << diagnostics.configuredMaxCachedPages
           << " resident pages, max prefetch per voice=" << diagnostics.maxPrefetchBytesPerVoice << " bytes\n";
    detail << "Voices: active=" << diagnostics.activeVoiceCount
           << ", peak=" << diagnostics.peakActiveVoiceCount
           << ", routed zones=" << diagnostics.routedZones.size() << "\n";
    detail << "Stream counters: pageMisses=" << diagnostics.pageMissCount
           << ", cacheHits=" << diagnostics.cacheHitCount
           << ", cacheMisses=" << diagnostics.cacheMissCount
           << ", backgroundReads=" << diagnostics.backgroundReadCount << "\n";
    detail << "Cache residency: resident=" << diagnostics.residentPageCount
           << ", pending=" << diagnostics.pendingPageCount
           << ", purgePasses=" << diagnostics.purgePassCount
           << ", dormantPurges=" << diagnostics.dormantPurgeCount
           << ", cumulativeEvictions=" << diagnostics.evictedPageCount
           << ", lastPurgeEvictions=" << diagnostics.lastPurgeEvictedPageCount << "\n";
    detail << "Read latency micros: average=" << diagnostics.averageReadLatencyMicros
           << ", max=" << diagnostics.maxReadLatencyMicros << "\n";

    if (!diagnostics.failureState.empty())
        detail << "Diagnostics failure state: " << diagnostics.failureState << "\n";

    if (!diagnostics.lastContentProbeCategory.empty())
    {
        detail << "Last content probe: " << diagnostics.lastContentProbeCategory
               << " (" << (diagnostics.lastContentProbeFailedGracefully ? "failed gracefully" : "did not fail gracefully")
               << ")\n";
        detail << "Last content probe state: " << diagnostics.lastContentProbeState << "\n";
    }

    if (!diagnostics.previewFindings.empty())
    {
        detail << "Preview snapshot findings:\n";
        for (const auto& finding : diagnostics.previewFindings)
            detail << "- [" << toString(finding.severity) << "] " << finding.code << ": " << finding.message << "\n";
    }

    if (!diagnostics.publishedFindings.empty())
    {
        detail << "Publish snapshot findings:\n";
        for (const auto& finding : diagnostics.publishedFindings)
            detail << "- [" << toString(finding.severity) << "] " << finding.code << ": " << finding.message << "\n";
    }

    if (!diagnostics.routedZones.empty())
    {
        detail << "Routed zones:\n";
        for (const auto& zone : diagnostics.routedZones)
            detail << "- " << zone << "\n";
    }

    if (!diagnostics.lastContentProbeIssues.empty())
    {
        detail << "Last content probe issues:\n";
        for (const auto& issue : diagnostics.lastContentProbeIssues)
            detail << "- " << issue << "\n";
    }

    if (!diagnostics.issues.empty())
    {
        detail << "Diagnostics issues:\n";
        for (const auto& issue : diagnostics.issues)
            detail << "- " << issue << "\n";
    }

    if (!runtimeManifest.issues.empty())
    {
        detail << "Runtime manifest issues:\n";

        for (const auto& issue : runtimeManifest.issues)
            detail << "- " << issue << "\n";
    }

    if (runtimeManifest.loaded && !runtimeStream.issues.empty())
    {
        detail << "Runtime stream issues:\n";

        for (const auto& issue : runtimeStream.issues)
            detail << "- " << issue << "\n";
    }

    std::vector<std::string> nextSteps;

    if (!hiseProjucerWindowsPresent)
        nextSteps.emplace_back("Decide how Windows developers obtain Projucer, because the vendored HISE tree does not currently include a Windows Projucer binary.");

    if (hiseVendorPresent && hiseNestedJucePresent && linkedFrontend.linked)
        nextSteps.emplace_back("Use the linked frontend-profile and content seam as the hand-off point for the next runtime service, such as processor construction boundaries or preset loading orchestration.");
    else if (hiseVendorPresent && hiseNestedJucePresent)
        nextSteps.emplace_back("Promote the selected HISE plugin frontend profile from a compile-only probe to a minimal linked runtime seam.");

    if (contentSnapshot.repoContentRootExists && contentSnapshot.presetFileCount == 0 && contentSnapshot.sampleMapFileCount == 0)
        nextSteps.emplace_back("Populate content/hise_project/UserPresets and content/hise_project/SampleMaps with the first Decent Rhapsody authoring assets so the adapter can validate real content, not just empty folders.");
    else if (!contentSnapshot.repoContentRootExists)
        nextSteps.emplace_back("Create the product-owned content/hise_project layout so HISE authoring assets have a stable location outside third_party.");

    if (hiseSdkExtracted)
        nextSteps.emplace_back("The bundled HISE SDK inputs are extracted. Validate which parts are still needed versus optional for Decent Rhapsody Studio's Windows workflow.");
    else if (hiseSdkZipPresent)
        nextSteps.emplace_back("Extract third_party/hise/tools/SDK/sdk.zip so HISE's ASIO and VST3 SDK inputs are available.");

    if (hiseProjectTemplatePresent)
        nextSteps.emplace_back("Compare the generated product-owned AppConfig against HISE's frontend export templates to close any remaining macro or include-path gaps.");

    if (!runtimeManifest.manifestFound)
        nextSteps.emplace_back("Create and commit the Phase 1 reference instrument manifest under content/runtime/phase1/reference-corpus so the runtime model has a product-owned load target.");
    else if (!runtimeManifest.loaded)
        nextSteps.emplace_back("Fix the Phase 1 reference instrument manifest issues so the minimal loader can become the hand-off point for the import compiler in Sprint 2.");
    else
        nextSteps.emplace_back("Promote the Phase 1 reference instrument loader from fixture-backed manifest parsing to compiled content emitted by the Sprint 2 import pipeline.");

    nextSteps.emplace_back("Prepare the medium internal streaming case and synthetic stress manifest described by the Phase 1 reference corpus plan before Sprint 3 streaming work begins.");

    if (nextSteps.empty())
        nextSteps.emplace_back("Promote the adapter from metadata probe to a minimal compiled HISE-backed runtime object.");

    const auto integrationState = (hiseVendorPresent && hiseNestedJucePresent && linkedFrontend.linked)
        ? "Plugin frontend profile bridge linked"
        : (hiseVendorPresent && hiseNestedJucePresent)
            ? "Plugin frontend compile probe established"
        : "HISE vendor snapshot incomplete";

    return {
        "HISE vendor handshake",
        diagnostics.hasFailure ? diagnostics.failureState : integrationState,
        diagnostics,
        detail.str(),
        nextSteps
    };
}

RuntimeManifestLoadResult EngineFacade::loadPhase1ReferenceInstrument() const
{
    return referenceInstrumentActive ? referenceManifest : RuntimeManifestLoadResult {};
}

RuntimeStreamLoadResult EngineFacade::loadPhase1ReferenceStream() const
{
    return referenceInstrumentActive ? referenceStream : RuntimeStreamLoadResult {};
}

EngineDiagnosticsSnapshot EngineFacade::getDiagnosticsSnapshot() const
{
    return diagnosticsSnapshot;
}

EnginePerformanceSnapshot EngineFacade::getPerformanceSnapshot() const
{
    EnginePerformanceSnapshot snapshot;
    const auto& draftStatus = draftPlaybackContract.getStatus();
    snapshot.loaded = referenceInstrumentActive && referenceManifest.loaded && referenceStream.loaded
        && draftStatus.performance.available;
    snapshot.draftRevision = draftStatus.draftRevision;
    snapshot.previewRevision = draftStatus.preview.revision;
    snapshot.publishedRevision = draftStatus.performance.revision;
    snapshot.previewBuildId = draftStatus.preview.buildId;
    snapshot.publishedBuildId = draftStatus.performance.buildId;
    snapshot.previewPreparedBuildId = draftStatus.preview.preparedBuildId;
    snapshot.publishedPreparedBuildId = draftStatus.performance.preparedBuildId;
    snapshot.instrumentDisplayName = (referenceInstrumentActive && referenceManifest.loaded)
        ? referenceManifest.instrument.displayName
        : "No instrument loaded";
    snapshot.contentRootPath = authoringProject.loaded
        ? authoringProject.project.contentRootPath
        : std::string {};
    if (!authoringProject.loaded
        && referenceInstrumentActive
        && packageBackgroundArtworkJpgBytes != nullptr
        && !packageBackgroundArtworkPayloadId.empty())
    {
        snapshot.backgroundArtworkSourceKey = "package://" + packageBackgroundArtworkPayloadId;
        snapshot.backgroundArtworkJpgBytes = packageBackgroundArtworkJpgBytes;
    }
    snapshot.presetId = referenceInstrumentActive ? currentSessionState.presetId : "none";
    snapshot.loadProfileId = referenceInstrumentActive ? currentSessionState.loadProfileId : "none";
    snapshot.selectedArticulationId = referenceInstrumentActive ? currentSessionState.selectedArticulationId : std::string {};
    snapshot.selectedArticulationName = referenceInstrumentActive
        ? resolveArticulationName(referenceManifest, currentSessionState)
        : std::string {};
    snapshot.previewPending = draftStatus.pendingPreview.active;
    snapshot.publishedPending = draftStatus.pendingPerformance.active;
    snapshot.previewActivationEligible = draftStatus.preview.activationEligible;
    snapshot.publishedActivationEligible = draftStatus.performance.activationEligible;
    snapshot.previewRevisionState = draftStatus.preview.state;
    if (const auto presentation = getPerformancePublishPresentationSnapshot())
        snapshot.publishedPresentationState = presentation->state;
    snapshot.previewContentDigest = draftStatus.preview.contentDigest;
    snapshot.publishedContentDigest = draftStatus.performance.contentDigest;
    snapshot.previewPreparedContentDigest = draftStatus.preview.preparedContentDigest;
    snapshot.publishedPreparedContentDigest = draftStatus.performance.preparedContentDigest;
    snapshot.publishedRouteDigest = draftStatus.performance.routeDigest;
    snapshot.publishedSourceProvenanceDigest = draftStatus.performance.sourceProvenanceDigest;
    snapshot.publishedMacroSchemaDigest = draftStatus.performance.macroSchemaDigest;
    snapshot.surfaceStateSource = resolveDraftSurfaceSource(draftStatus);
    snapshot.rendererMode = resolveRendererMode(referenceInstrumentActive);
    if (draftStatus.performance.playableRangeAvailable && draftStatus.performance.available)
    {
        snapshot.playableRangeAvailable = true;
        snapshot.lowestPlayableNote = draftStatus.performance.lowestPlayableNote;
        snapshot.highestPlayableNote = draftStatus.performance.highestPlayableNote;
        snapshot.playableRangeSource = "published";
    }
    else if (draftStatus.preview.playableRangeAvailable && draftStatus.preview.available)
    {
        snapshot.playableRangeAvailable = true;
        snapshot.lowestPlayableNote = draftStatus.preview.lowestPlayableNote;
        snapshot.highestPlayableNote = draftStatus.preview.highestPlayableNote;
        snapshot.playableRangeSource = "preview";
    }
    else
    {
        snapshot.playableRangeSource = "default";
    }
    snapshot.draftPlaybackEvent = draftStatus.lastEvent;
    snapshot.loadIndicator = referenceInstrumentActive
        ? buildLoadIndicator(referenceManifest, referenceStream, currentSessionState)
        : "No instrument loaded";
    const auto workerStatus = preparedPlaybackService.getWorkerStatus();
    snapshot.preparedScheduler = workerStatus;
    snapshot.preparedWorkerPendingCount = workerStatus.pendingWorkCount;
    snapshot.preparedWorkerConfiguredMaxPendingCount = workerStatus.configuredMaxPendingWorkCount;
    snapshot.preparedWorkerConfiguredMaxInFlightCount = workerStatus.configuredMaxInFlightWorkCount;
    snapshot.preparedWorkerCancellationCount = workerStatus.cancellationCount;
    snapshot.preparedWorkerSupersededCount = workerStatus.supersededCount;
    snapshot.preparedWorkerFailureCount = workerStatus.failureCount;
    snapshot.preparedWorkerActiveOwnershipRecordCount = workerStatus.activeOwnershipRecordCount;
    snapshot.preparedWorkerActiveOwnershipBytes = workerStatus.activeOwnershipBytes;
    snapshot.preparedWorkerRetiredOwnershipRecordCount = workerStatus.retiredOwnershipRecordCount;
    snapshot.preparedWorkerRetiredBytes = workerStatus.retiredBytesAwaitingCleanup;
    snapshot.preparedWorkerEvent = workerStatus.lastEvent;
    snapshot.preparedWorkerLastCancellationLane = workerStatus.lastCancellationLane;
    snapshot.preparedWorkerLastCancellationReason = workerStatus.lastCancellationReason;
    snapshot.preparedWorkerLastSupersededLane = workerStatus.lastSupersededLane;
    snapshot.preparedWorkerLastSupersededReason = workerStatus.lastSupersededReason;
    snapshot.previewPreparedSampleCount = draftStatus.preview.preparedSampleCount;
    snapshot.previewPreparedStreamCount = draftStatus.preview.preparedStreamCount;
    snapshot.previewPreparedOwnershipRecordCount = draftStatus.preview.preparedOwnershipRecordCount;
    snapshot.publishedPreparedSampleCount = draftStatus.performance.preparedSampleCount;
    snapshot.publishedPreparedStreamCount = draftStatus.performance.preparedStreamCount;
    snapshot.publishedPreparedOwnershipRecordCount = draftStatus.performance.preparedOwnershipRecordCount;
    snapshot.previewPreparedBytes = draftStatus.preview.preparedBytes;
    snapshot.publishedPreparedBytes = draftStatus.performance.preparedBytes;
    snapshot.previewPreparedOwnershipBytes = draftStatus.preview.preparedOwnershipBytes;
    snapshot.publishedPreparedOwnershipBytes = draftStatus.performance.preparedOwnershipBytes;
    snapshot.previewPreparedBuildMicros = draftStatus.preview.preparedBuildDurationMicros;
    snapshot.publishedPreparedBuildMicros = draftStatus.performance.preparedBuildDurationMicros;
    snapshot.previewPreparedDecodedBytes = draftStatus.preview.preparedDecodedBytes;
    snapshot.publishedPreparedDecodedBytes = draftStatus.performance.preparedDecodedBytes;
    snapshot.previewPreparedSampleDataBytes = draftStatus.preview.preparedSampleDataBytes;
    snapshot.publishedPreparedSampleDataBytes = draftStatus.performance.preparedSampleDataBytes;
    snapshot.previewActivationPayloadBytes = draftStatus.preview.activationPayloadRetainedBytes;
    snapshot.publishedActivationPayloadBytes = draftStatus.performance.activationPayloadRetainedBytes;
    snapshot.retainedActivationPayloadBytes = draftStatus.preview.activationPayloadRetainedBytes
        + draftStatus.performance.activationPayloadRetainedBytes;
    snapshot.previewPreparationCacheHits = draftStatus.preview.preparationCacheHitCount;
    snapshot.previewPreparationCacheMisses = draftStatus.preview.preparationCacheMissCount;
    snapshot.publishedPreparationCacheHits = draftStatus.performance.preparationCacheHitCount;
    snapshot.publishedPreparationCacheMisses = draftStatus.performance.preparationCacheMissCount;
    snapshot.previewPreparationCacheHitRate = computePreparationCacheHitRate(snapshot.previewPreparationCacheHits,
                                                                             snapshot.previewPreparationCacheMisses);
    snapshot.publishedPreparationCacheHitRate = computePreparationCacheHitRate(snapshot.publishedPreparationCacheHits,
                                                                               snapshot.publishedPreparationCacheMisses);
    applyPreparedCachePressurePolicy(snapshot);
    snapshot.previewFindings = draftStatus.preview.findings;
    snapshot.publishedFindings = draftStatus.performance.findings;

    if (referenceInstrumentActive)
    {
        snapshot.previewPlayback = previewPlaybackSnapshot;
        snapshot.previewPlayback.ready = snapshot.loaded;
        snapshot.previewPlayback.appliedMacroSummary = buildAppliedMacroSummary(currentSessionState);
        if (snapshot.previewPlayback.state.empty())
            snapshot.previewPlayback.state = snapshot.loaded ? "Ready to audition" : snapshot.loadIndicator;
    }
    else
    {
        snapshot.previewPlayback.state = snapshot.loadIndicator;
        snapshot.previewPlayback.appliedMacroSummary = "No instrument loaded";
    }

    return snapshot;
}

std::vector<EngineArticulationDescriptor> EngineFacade::getArticulationDescriptors() const
{
    std::vector<EngineArticulationDescriptor> descriptors;

    if (!referenceInstrumentActive || !referenceManifest.loaded)
        return descriptors;

    descriptors.reserve(referenceManifest.instrument.articulations.size());
    for (const auto& articulation : referenceManifest.instrument.articulations)
    {
        descriptors.push_back({
            articulation.id,
            articulation.name,
            articulation.isDefault,
            articulation.id == currentSessionState.selectedArticulationId
        });
    }

    return descriptors;
}

std::vector<EngineMacroDescriptor> EngineFacade::getMacroDescriptors() const
{
    std::vector<EngineMacroDescriptor> descriptors;

    if (!referenceInstrumentActive || !referenceManifest.loaded)
        return descriptors;

    if (const auto activeBindings = getActivePublishedMacroBindings(); activeBindings != nullptr)
    {
        descriptors.reserve(activeBindings->bindings.size());
        for (const auto& binding : activeBindings->bindings)
        {
            if (!binding.assigned)
                continue;
            descriptors.push_back(makePublishedMacroDescriptor(binding, currentSessionState));
        }

        return descriptors;
    }

    descriptors.reserve(referenceManifest.instrument.macros.size());

    for (const auto& macro : referenceManifest.instrument.macros)
    {
        auto currentValue = macro.defaultValue;
        const auto currentIterator = std::find_if(currentSessionState.macroValues.begin(),
                                                  currentSessionState.macroValues.end(),
                                                  [&](const RuntimePresetMacroValue& value)
                                                  {
                                                      return value.id == macro.id;
                                                  });
        if (currentIterator != currentSessionState.macroValues.end())
            currentValue = currentIterator->value;

        descriptors.push_back({
            macro.id,
            macro.name,
            macro.minValue,
            macro.maxValue,
            macro.defaultValue,
            currentValue,
            macro.id == "tone" ? "preview.triggerVelocity" : "preview.noteTravel",
            macro.id == "tone"
                ? "Biases the reference preview from softer attacks into accent territory."
                : "Offsets the previewed note pitch to add movement across the reference range.",
            macro.id == "tone" ? buildToneCurrentEffect(currentSessionState) : buildMotionCurrentEffect(currentSessionState),
            false,
            true
        });
    }

    return descriptors;
}

bool EngineFacade::setSelectedArticulation(const std::string& articulationId)
{
    if (!referenceInstrumentActive || !referenceManifest.loaded)
        return false;

    if (findArticulationDefinition(referenceManifest.instrument, articulationId) == nullptr)
        return false;

    if (currentSessionState.selectedArticulationId == articulationId)
        return true;

    currentSessionState.selectedArticulationId = articulationId;
    currentSessionState.transientMetrics.integrationState = "Performance surface articulation updated";
    previewPlaybackSnapshot = {};
    syncPreviewSnapshotFromDraftPlayback();
    previewPlaybackSnapshot.articulationId = articulationId;
    syncSessionSelectionsIntoDiagnostics(currentSessionState, diagnosticsSnapshot);
    markStateChanged();
    return true;
}

bool EngineFacade::setMacroValue(const std::string& macroId, double value)
{
    if (!referenceInstrumentActive || !referenceManifest.loaded)
        return false;

    auto minimum = 0.0;
    auto maximum = 1.0;
    auto found = false;
    if (const auto activeBindings = getActivePublishedMacroBindings(); activeBindings != nullptr)
    {
        const auto binding = std::find_if(activeBindings->bindings.begin(), activeBindings->bindings.end(),
                                          [&](const PublishedMacroBinding& candidate)
                                          {
                                              return candidate.assigned
                                                  && runtimeMacroIdFromHostParameterId(
                                                         candidate.hostParameterId) == macroId;
                                          });
        if (binding != activeBindings->bindings.end())
        {
            minimum = binding->minValue;
            maximum = binding->maxValue;
            found = true;
        }
    }

    if (!found)
    {
        const auto definition = std::find_if(referenceManifest.instrument.macros.begin(),
                                             referenceManifest.instrument.macros.end(),
                                             [&](const RuntimeMacroDefinition& macro)
                                             {
                                                 return macro.id == macroId;
                                             });
        if (definition == referenceManifest.instrument.macros.end())
            return false;
        minimum = definition->minValue;
        maximum = definition->maxValue;
    }

    const auto clampedValue = normalizeMacroValue(std::clamp(value, minimum, maximum));
    const auto currentIterator = std::find_if(currentSessionState.macroValues.begin(),
                                              currentSessionState.macroValues.end(),
                                              [&](const RuntimePresetMacroValue& currentValue)
                                              {
                                                  return currentValue.id == macroId;
                                              });

    if (currentIterator != currentSessionState.macroValues.end())
    {
        if (currentIterator->value == clampedValue)
            return true;

        currentIterator->value = clampedValue;
    }
    else
    {
        currentSessionState.macroValues.push_back({ macroId, clampedValue });
    }

    syncSessionSelectionsIntoDiagnostics(currentSessionState, diagnosticsSnapshot);
    markStateChanged();
    return true;
}

bool EngineFacade::stageDraftRevision(std::size_t revision)
{
    pumpPreparedPlaybackWorkerCompletions();

    if (!referenceInstrumentActive || !draftPlaybackContract.getStatus().projectOpen)
        return false;

    if (!draftPlaybackContract.setDraftRevision(revision))
        return false;

    currentSessionState.transientMetrics.integrationState = "Draft revision staged";
    previewPlaybackSnapshot = {};
    syncPreviewSnapshotFromDraftPlayback();
    refreshDiagnosticsSnapshot();
    markStateChanged();
    return true;
}

bool EngineFacade::refreshPreviewToCurrentDraft()
{
    return refreshPreviewForPreparationScope({}, true);
}

bool EngineFacade::refreshPreviewForPreparationScope(
    const PlaybackPreparationScopeRequest& scopeRequest,
    const bool forceRebuild)
{
    pumpPreparedPlaybackWorkerCompletions();

    if (!referenceInstrumentActive || !referenceManifest.loaded || !referenceStream.loaded || !authoringProject.loaded)
        return false;

    const auto& currentStatus = draftPlaybackContract.getStatus();
    const auto currentPayload = currentStatus.preview.activationPayload;
    const auto currentPayloadMatchesScope = currentPayload != nullptr
        && currentPayload->preparationScope == scopeRequest.scope
        && currentPayload->preparationSelectedZoneId == scopeRequest.selectedZoneId
        && currentPayload->preparationSelectedGroupId == scopeRequest.selectedGroupId;
    if ((currentStatus.pendingPreview.active
         && currentStatus.pendingPreview.requestedRevision == currentStatus.draftRevision)
        || (currentStatus.preview.revision == currentStatus.draftRevision
            && currentPayloadMatchesScope && !forceRebuild))
    {
        return true;
    }

    const auto request = draftPlaybackContract.requestPreviewBuild();
    if (!request.accepted)
        return false;

    const auto buildResult = scopePlaybackSnapshotForPreparation(
        buildCurrentPlaybackSnapshot(false), scopeRequest);
    if (!buildResult.built || !buildResult.activationEligible)
    {
        const auto preparedResult = buildRejectedPreparedPlayback(buildResult);
        const auto applied = draftPlaybackContract.completePreviewBuild(request.requestId, buildResult, preparedResult);
        if (!applied)
            return false;

        currentSessionState.transientMetrics.integrationState = "Preview revision failed";
        currentSessionState.transientMetrics.lastFailure = summarizeSnapshotFindings(draftPlaybackContract.getStatus().preview.findings);
        previewPlaybackSnapshot = {};
        syncPreviewSnapshotFromDraftPlayback();
        refreshDiagnosticsSnapshot();
        markStateChanged();
        return false;
    }

    if (!enqueuePreparedPlaybackBuild(request.requestId, buildResult, PreparedPlaybackWorkLane::preview))
    {
        PreparedPlaybackBuildResult queueRejected;
        queueRejected.snapshotBuildId = buildResult.buildId;
        queueRejected.requestedDraftRevision = buildResult.requestedDraftRevision;
        queueRejected.activationRequested = buildResult.activationRequested;
        queueRejected.lifecycleState = PlaybackSnapshotLifecycleState::failed;
        queueRejected.state = "Prepared playback queue is full";
        queueRejected.metrics.failureCount = 1;
        queueRejected.findings.push_back({
            PlaybackSnapshotFindingSeverity::error,
            "prepared-queue-full",
            "preparedWorker",
            "Prepared playback queue is full."
        });
        draftPlaybackContract.completePreviewBuild(request.requestId, buildResult, queueRejected);
        currentSessionState.transientMetrics.integrationState = "Preview revision failed";
        currentSessionState.transientMetrics.lastFailure = "Prepared playback queue is full.";
        previewPlaybackSnapshot = {};
        syncPreviewSnapshotFromDraftPlayback();
        refreshDiagnosticsSnapshot();
        markStateChanged();
        return false;
    }

    currentSessionState.transientMetrics.integrationState = "Preview revision preparing";
    currentSessionState.transientMetrics.lastFailure.clear();
    previewPlaybackSnapshot = {};
    syncPreviewSnapshotFromDraftPlayback();
    refreshDiagnosticsSnapshot();
    markStateChanged();
    return true;
}

bool EngineFacade::cancelPreviewPreparation(const std::string& reason)
{
    const auto pending = draftPlaybackContract.getStatus().pendingPreview;
    const auto canceledQueued = preparedPlaybackService.cancelQueuedPreviewBuilds(reason);
    for (const auto& result : canceledQueued)
        pendingPreparedCompletions.erase(result.buildId);

    auto canceledContract = false;
    if (pending.active)
        canceledContract = draftPlaybackContract.cancelPreviewBuild(pending.requestId);

    for (auto iterator = pendingPreparedCompletions.begin(); iterator != pendingPreparedCompletions.end();)
    {
        if (iterator->second.lane == PreparedPlaybackWorkLane::preview)
            iterator = pendingPreparedCompletions.erase(iterator);
        else
            ++iterator;
    }

    if (!canceledQueued.empty() || canceledContract)
    {
        currentSessionState.transientMetrics.integrationState = "Preview preparation canceled";
        previewPlaybackSnapshot = {};
        syncPreviewSnapshotFromDraftPlayback();
        refreshDiagnosticsSnapshot();
        markStateChanged();
        return true;
    }
    return false;
}

bool EngineFacade::publishCurrentDraft()
{
    const auto commandReceivedAtMicros = monotonicMicros();
    pumpPreparedPlaybackWorkerCompletions();

    if (!referenceInstrumentActive || !referenceManifest.loaded || !referenceStream.loaded || !authoringProject.loaded)
        return false;

    const auto buildResult = buildCurrentPlaybackSnapshot(true);
    const auto authoredDigest = !buildResult.snapshot.contentDigest.empty()
        ? buildResult.snapshot.contentDigest
        : computeFnv1a64Digest(authoringProject.project.projectId + ":"
                               + std::to_string(draftPlaybackContract.getStatus().draftRevision));
    const auto macroSchemaDigest = computePlaybackSnapshotMacroSchemaDigest(buildResult.snapshot);
    const auto controllerRequest = performancePublishController.request(
        performancePublishProjectGeneration,
        draftPlaybackContract.getStatus().draftRevision,
        authoredDigest,
        macroSchemaDigest,
        monotonicMicros());
    if (controllerRequest.duplicateSuppressed)
        return true;
    if (!controllerRequest.accepted
        || !performancePublishController.markPreparing(controllerRequest.request.identity,
                                                        monotonicMicros()))
        return false;

    const auto macroPreflight = preflightPublishedMacros(authoringProject.project.authoring);
    const auto activeBindings = getActivePublishedMacroBindings();
    const auto activeSlotCount = assignedBindingCount(activeBindings);
    performancePublishController.setMacroCapacityDiagnostics(
        macroPreflight.exposedCount,
        macroPreflight.hiddenCount,
        activeSlotCount,
        macroPreflight.exposedCount + macroPreflight.hiddenCount,
        maximumPublishedMacroHostSlots - std::min(activeSlotCount, maximumPublishedMacroHostSlots),
        activeSlotCount);
    if (macroPreflight.finding.has_value())
    {
        performancePublishController.fail(controllerRequest.request.identity, *macroPreflight.finding);
        currentSessionState.transientMetrics.integrationState = "Publish preflight failed";
        currentSessionState.transientMetrics.lastFailure = "[" + macroPreflight.finding->code
            + "] " + macroPreflight.finding->message;
        refreshDiagnosticsSnapshot();
        markStateChanged();
        return false;
    }

    const auto request = draftPlaybackContract.requestPerformanceBuild();
    if (!request.accepted)
    {
        performancePublishController.fail(
            controllerRequest.request.identity,
            makePerformancePublishFailure("publish-request-rejected", "draftPlaybackContract",
                                          request.state.empty()
                                              ? std::string("Publish request was rejected.")
                                              : request.state));
        currentSessionState.transientMetrics.integrationState = "Publish preparation failed";
        currentSessionState.transientMetrics.lastFailure = "Publish request was rejected.";
        refreshDiagnosticsSnapshot();
        markStateChanged();
        return false;
    }

    if (!buildResult.built || !buildResult.activationEligible)
    {
        const auto preparedResult = buildRejectedPreparedPlayback(buildResult);
        const auto applied = draftPlaybackContract.completePerformanceBuild(request.requestId, buildResult, preparedResult);
        if (!applied)
            return false;

        const auto finding = !buildResult.findings.empty()
            ? makePerformancePublishFinding(buildResult.findings.front())
            : makePerformancePublishFailure("publish-snapshot-failed", "snapshot",
                                            buildResult.state.empty()
                                                ? std::string("Publish snapshot construction failed.")
                                                : buildResult.state);
        performancePublishController.fail(controllerRequest.request.identity, finding);

        currentSessionState.transientMetrics.integrationState = "Publish preparation failed";
        currentSessionState.transientMetrics.lastFailure = summarizeSnapshotFindings(draftPlaybackContract.getStatus().performance.findings);
        previewPlaybackSnapshot = {};
        syncPreviewSnapshotFromDraftPlayback();
        refreshDiagnosticsSnapshot();
        markStateChanged();
        return false;
    }

    if (!enqueuePreparedPlaybackBuild(request.requestId, buildResult, PreparedPlaybackWorkLane::performance))
    {
        PreparedPlaybackBuildResult queueRejected;
        queueRejected.snapshotBuildId = buildResult.buildId;
        queueRejected.requestedDraftRevision = buildResult.requestedDraftRevision;
        queueRejected.activationRequested = buildResult.activationRequested;
        queueRejected.lifecycleState = PlaybackSnapshotLifecycleState::failed;
        queueRejected.state = "Prepared playback queue is full";
        queueRejected.metrics.failureCount = 1;
        queueRejected.findings.push_back({
            PlaybackSnapshotFindingSeverity::error,
            "prepared-queue-full",
            "preparedWorker",
            "Prepared playback queue is full."
        });
        draftPlaybackContract.completePerformanceBuild(request.requestId, buildResult, queueRejected);
        performancePublishController.fail(
            controllerRequest.request.identity,
            makePerformancePublishFailure("prepared-queue-full", "preparedWorker",
                                          "Prepared playback queue is full."));
        currentSessionState.transientMetrics.integrationState = "Publish preparation failed";
        currentSessionState.transientMetrics.lastFailure = "Prepared playback queue is full.";
        previewPlaybackSnapshot = {};
        syncPreviewSnapshotFromDraftPlayback();
        refreshDiagnosticsSnapshot();
        markStateChanged();
        return false;
    }

    preparedPlaybackService.recordCommandToQueuedDuration(
        monotonicMicros() - commandReceivedAtMicros);

    currentSessionState.transientMetrics.integrationState = "Publish preparation queued";
    currentSessionState.transientMetrics.lastFailure.clear();
    previewPlaybackSnapshot = {};
    syncPreviewSnapshotFromDraftPlayback();
    refreshDiagnosticsSnapshot();
    markStateChanged();
    return true;
}

PlaybackActivationPayloadPtr EngineFacade::getBootstrapPerformanceActivationPayload() const
{
    const auto controller = performancePublishController.getSnapshot();
    const auto& performance = draftPlaybackContract.getStatus().performance;
    if (controller.hasRequest
        || !performance.available
        || !performance.activationEligible
        || performance.activationPayload == nullptr)
    {
        return {};
    }
    return performance.activationPayload;
}

PerformancePublishActivationPayloadPtr EngineFacade::authorizePerformanceActivation(
    std::uint64_t nowMicros)
{
    const auto controller = performancePublishController.getSnapshot();
    const auto activeSlotCount = assignedBindingCount(getActivePublishedMacroBindings());
    if (!controller.hasRequest
        || controller.preparationState != PerformancePublishPreparationState::ready
        || controller.activationState != PerformancePublishActivationState::noActivation)
    {
        return {};
    }

    const auto failAuthorization = [this, &controller](PerformancePublishFinding finding)
    {
        const auto message = finding.message;
        const auto code = finding.code;
        performancePublishController.fail(controller.currentRequest.identity, std::move(finding));
        currentSessionState.transientMetrics.integrationState = "Publish activation staging failed";
        currentSessionState.transientMetrics.lastFailure = "[" + code + "] " + message;
        refreshDiagnosticsSnapshot();
        markStateChanged();
        return PerformancePublishActivationPayloadPtr {};
    };

    const auto& prepared = draftPlaybackContract.getStatus().performance;
    const auto& payload = prepared.activationPayload;
    const auto exactPayload = prepared.available
        && prepared.activationEligible
        && payload != nullptr
        && payload->lane == PlaybackActivationLane::performance
        && payload->activationEligible
        && payload->lifecycleState == PlaybackSnapshotLifecycleState::active
        && payload->snapshot != nullptr
        && payload->prepared != nullptr
        && payload->revision == controller.currentRequest.identity.draftRevision
        && payload->snapshotBuildId != 0
        && payload->preparedBuildId == controller.acceptedPreparedBuildId
        && payload->snapshotContentDigest == controller.currentRequest.identity.authoredContentDigest
        && payload->preparedContentDigest == controller.acceptedPreparedDigest
        && payload->routeDigest == controller.acceptedRouteDigest
        && payload->sourceProvenanceDigest == controller.acceptedSourceProvenanceDigest
        && payload->macroSchemaDigest == controller.acceptedMacroSchemaDigest
        && payload->macroSchemaDigest == controller.currentRequest.identity.macroSchemaDigest;
    if (!exactPayload)
    {
        return failAuthorization(makePerformancePublishFailure(
                "performance-activation-payload-mismatch",
                "performance.activationPayload",
                "The prepared Performance payload no longer matches the controller-authorized identity."));
    }

    auto authorization = std::make_shared<PerformancePublishActivationPayload>();
    authorization->activationToken = nextPerformanceActivationToken++;
    if (nextPerformanceActivationToken == 0)
        nextPerformanceActivationToken = 1;
    authorization->requestIdentity = controller.currentRequest.identity;
    authorization->revision = payload->revision;
    authorization->snapshotBuildId = payload->snapshotBuildId;
    authorization->preparedBuildId = payload->preparedBuildId;
    authorization->snapshotContentDigest = payload->snapshotContentDigest;
    authorization->preparedContentDigest = payload->preparedContentDigest;
    authorization->routeDigest = payload->routeDigest;
    authorization->sourceProvenanceDigest = payload->sourceProvenanceDigest;
    authorization->macroSchemaDigest = payload->macroSchemaDigest;
    authorization->retainedPreparedBytes = payload->retainedPreparedBytes;
    authorization->playbackPayload = payload;

    PublishedMacroBindingBuildRequest macroBindingRequest;
    macroBindingRequest.revision = payload->revision;
    macroBindingRequest.macroSchemaDigest = payload->macroSchemaDigest;
    macroBindingRequest.dspGraphDigest = payload->snapshot->dspGraphDigest;
    macroBindingRequest.authoredMacros = payload->snapshot->macroDefaults;
    macroBindingRequest.presentationHints = buildPresentationHints(*payload->snapshot);
    macroBindingRequest.previousActiveTable = getActivePublishedMacroBindings();
    macroBindingRequest.hostSlots = buildPublishedHostSlots(
        macroBindingRequest.authoredMacros,
        macroBindingRequest.previousActiveTable);
    macroBindingRequest.currentValues = buildPublishedCurrentValues(
        currentSessionState,
        macroBindingRequest.authoredMacros,
        macroBindingRequest.previousActiveTable);

    DspParameterControlLayout dspControlLayout;
    const auto requiresDspMacroControls = std::any_of(
        macroBindingRequest.authoredMacros.begin(), macroBindingRequest.authoredMacros.end(),
        [](const auto& macro)
        {
            return std::any_of(macro.targets.begin(), macro.targets.end(), [](const auto& target)
            {
                return !target.dspSlotId.empty() || !target.dspParameterId.empty();
            });
        });
    if (requiresDspMacroControls)
    {
        const auto graphPlan = compileDspGraphPlan(*payload->snapshot);
        if (!graphPlan.compiled)
        {
            return failAuthorization(makePerformancePublishFailure(
                    "performance-macro-dsp-graph-rejected", "performance.macroBindings",
                    "The structured DSP macro target could not compile the published graph."));
        }
        const auto controlLayout = compileDspParameterControlLayout(graphPlan.plan);
        if (!controlLayout.compiled)
        {
            return failAuthorization(makePerformancePublishFailure(
                    "performance-macro-dsp-control-layout-rejected", "performance.macroBindings",
                    "The structured DSP macro target could not compile the published control layout."));
        }
        dspControlLayout = controlLayout.layout;
        macroBindingRequest.dspControlLayout = &dspControlLayout;
    }

    const auto macroBindingResult = buildPublishedMacroBindingTable(macroBindingRequest);
    if (!macroBindingResult.built || macroBindingResult.table == nullptr)
    {
        const auto finding = !macroBindingResult.findings.empty()
            ? PerformancePublishFinding {
                macroBindingResult.findings.front().severity
                        == PublishedMacroBindingFindingSeverity::error
                    ? PerformancePublishFindingSeverity::error
                    : PerformancePublishFindingSeverity::warning,
                macroBindingResult.findings.front().code,
                macroBindingResult.findings.front().path,
                macroBindingResult.findings.front().message }
            : makePerformancePublishFailure(
                "performance-macro-binding-rejected",
                "performance.macroBindings",
                "The immutable published macro binding table could not be constructed.");
        return failAuthorization(finding);
    }
    performancePublishController.setMacroCapacityDiagnostics(
        macroBindingResult.table->assignedExposedCount,
        macroBindingResult.table->assignedHiddenCount,
        macroBindingResult.table->assignedExposedCount + macroBindingResult.table->assignedHiddenCount,
        macroBindingResult.table->unassignedExposedCount + macroBindingResult.table->unassignedHiddenCount,
        maximumPublishedMacroHostSlots
            - std::min(maximumPublishedMacroHostSlots,
                       macroBindingResult.table->assignedExposedCount
                           + macroBindingResult.table->assignedHiddenCount),
        activeSlotCount);
    authorization->macroBindings = macroBindingResult.table;
    if (!performancePublishController.authorizeActivation(*authorization, nowMicros))
        return {};
    return authorization;
}

bool EngineFacade::rejectPerformanceActivationStaging(
    const PerformancePublishActivationPayloadPtr& payload,
    PerformancePublishFinding finding)
{
    if (payload == nullptr
        || !performancePublishController.rejectActivationStaging(*payload, std::move(finding)))
    {
        return false;
    }
    currentSessionState.transientMetrics.integrationState = "Publish activation staging failed";
    currentSessionState.transientMetrics.lastFailure =
        performancePublishController.getSnapshot().failureFinding.message;
    refreshDiagnosticsSnapshot();
    markStateChanged();
    return true;
}

bool EngineFacade::acknowledgePerformanceActivation(
    const PerformancePublishActivationPayloadPtr& payload,
    std::uint64_t nowMicros)
{
    if (payload == nullptr
        || !performancePublishController.acknowledgeActivation(*payload, nowMicros))
    {
        return false;
    }
    if (payload->macroBindings != nullptr)
    {
        const auto& bindings = *payload->macroBindings;
        const auto assignedCount = bindings.assignedExposedCount + bindings.assignedHiddenCount;
        performancePublishController.setMacroCapacityDiagnostics(
            bindings.assignedExposedCount,
            bindings.assignedHiddenCount,
            assignedCount,
            bindings.unassignedExposedCount + bindings.unassignedHiddenCount,
            maximumPublishedMacroHostSlots
                - std::min(maximumPublishedMacroHostSlots, assignedCount),
            assignedCount);
    }
    currentSessionState.transientMetrics.integrationState = "Published revision active";
    currentSessionState.transientMetrics.lastFailure.clear();
    refreshDiagnosticsSnapshot();
    markStateChanged();
    return true;
}

void EngineFacade::closeDraftPlaybackProject(bool preservePublishedPerformance)
{
    clearPendingPreparedCompletions();
    ++performancePublishProjectGeneration;
    performancePublishController.reset(!preservePublishedPerformance, true);
    draftPlaybackContract.closeProject();
    referenceInstrumentActive = false;
    currentSessionState.transientMetrics.integrationState = "Draft playback project closed";
    currentSessionState.transientMetrics.lastFailure.clear();
    previewPlaybackSnapshot = {};
    syncPreviewSnapshotFromDraftPlayback();
    refreshDiagnosticsSnapshot();
    markStateChanged();
}

bool EngineFacade::reopenDraftPlaybackProject(std::size_t revision,
                                              bool preservePublishedPerformance)
{
    clearPendingPreparedCompletions();
    if (!referenceManifest.loaded)
        return false;

    ++performancePublishProjectGeneration;
    performancePublishController.reset(!preservePublishedPerformance, true);
    draftPlaybackContract.reopenProject(revision);
    referenceInstrumentActive = true;
    currentSessionState.transientMetrics.integrationState = "Draft playback project reopened";
    currentSessionState.transientMetrics.lastFailure.clear();
    previewPlaybackSnapshot = {};
    syncPreviewSnapshotFromDraftPlayback();
    refreshDiagnosticsSnapshot();
    markStateChanged();
    return true;
}

bool EngineFacade::replaceDraftPlaybackAuthoringProject(RuntimeProjectModel project)
{
    const auto validation = validateRuntimeProjectModel(project);
    if (!validation.valid)
        return false;

    const auto replacesProjectIdentity = authoringProject.loaded
        && authoringProject.project.projectId != project.projectId;
    clearPendingPreparedCompletions();
    if (replacesProjectIdentity)
    {
        ++performancePublishProjectGeneration;
        performancePublishController.reset(true, true);
    }
    authoringProject = {};
    authoringProject.manifestFound = true;
    authoringProject.loaded = true;
    authoringProject.state = "Draft playback authoring project replaced";
    authoringProject.project = std::move(project);
    packageBackgroundArtworkPayloadId.clear();
    packageBackgroundArtworkJpgBytes.reset();
    currentSessionState.selectedArticulationId = resolveAuthoredArticulationSelection(
        authoringProject.project,
        currentSessionState.selectedArticulationId);
    currentSessionState.transientMetrics.integrationState = "Draft playback authoring project replaced";
    currentSessionState.transientMetrics.lastFailure.clear();
    previewPlaybackSnapshot = {};
    syncPreviewSnapshotFromDraftPlayback();
    refreshDiagnosticsSnapshot();
    markStateChanged();
    return true;
}

bool EngineFacade::restorePerformancePublishProjectGeneration(
    const std::uint64_t projectGeneration)
{
    const auto controller = performancePublishController.getSnapshot();
    if (projectGeneration == 0
        || controller.preparationState == PerformancePublishPreparationState::preparing
        || controller.activationState == PerformancePublishActivationState::pending)
        return false;

    performancePublishProjectGeneration = projectGeneration;
    return true;
}

PlaybackSnapshotBuildResult EngineFacade::buildCurrentPlaybackSnapshot(bool activationRequested)
{
    if (!authoringProject.loaded)
    {
        PlaybackSnapshotBuildResult result;
        result.state = "Authoring project unavailable";
        result.lifecycleState = PlaybackSnapshotLifecycleState::failed;
        result.findings.push_back({
            PlaybackSnapshotFindingSeverity::error,
            "missing-authoring-project",
            "authoringProject",
            "Phase 2 authoring reference project is unavailable for snapshot construction."
        });
        return result;
    }

    const auto request = playbackSnapshotBuilder.requestBuild(draftPlaybackContract.getStatus().draftRevision,
                                                              activationRequested);
    return playbackSnapshotBuilder.buildSnapshot(request, authoringProject.project);
}

PreparedPlaybackBuildResult EngineFacade::buildRejectedPreparedPlayback(const PlaybackSnapshotBuildResult& snapshotResult)
{
    const auto request = preparedPlaybackService.requestBuild(snapshotResult, referenceStream);
    return preparedPlaybackService.prepare(request, snapshotResult, referenceStream);
}

bool EngineFacade::enqueuePreparedPlaybackBuild(std::uint64_t contractRequestId,
                                                const PlaybackSnapshotBuildResult& snapshotResult,
                                                PreparedPlaybackWorkLane lane,
                                                bool bootstrapPerformance)
{
    auto publishIdentity = PerformancePublishRequestIdentity {};
    if (lane == PreparedPlaybackWorkLane::performance)
    {
        auto controller = performancePublishController.getSnapshot();
        const auto macroSchemaDigest = computePlaybackSnapshotMacroSchemaDigest(snapshotResult.snapshot);
        const auto exactPreparingRequest = controller.hasRequest
            && controller.preparationState == PerformancePublishPreparationState::preparing
            && controller.currentRequest.identity.projectGeneration == performancePublishProjectGeneration
            && controller.currentRequest.identity.draftRevision == snapshotResult.snapshot.draftRevision
            && controller.currentRequest.identity.authoredContentDigest == snapshotResult.snapshot.contentDigest
            && controller.currentRequest.identity.macroSchemaDigest == macroSchemaDigest;
        if (!exactPreparingRequest)
        {
            const auto request = performancePublishController.request(
                performancePublishProjectGeneration,
                snapshotResult.snapshot.draftRevision,
                snapshotResult.snapshot.contentDigest,
                macroSchemaDigest,
                monotonicMicros(),
                bootstrapPerformance
                    ? PerformancePublishRequestOrigin::bootstrap
                    : PerformancePublishRequestOrigin::explicitCommand);
            if (!request.accepted
                || !performancePublishController.markPreparing(request.request.identity, monotonicMicros()))
                return false;
            controller = performancePublishController.getSnapshot();
        }
        if (!controller.hasRequest)
            return false;
        publishIdentity = controller.currentRequest.identity;
    }

    auto submitResult = lane == PreparedPlaybackWorkLane::performance
        ? preparedPlaybackService.enqueuePublishBuild(snapshotResult)
        : preparedPlaybackService.enqueuePreviewBuild(snapshotResult);

    for (const auto& displacedResult : submitResult.displacedResults)
        pendingPreparedCompletions.erase(displacedResult.buildId);

    if (!submitResult.accepted)
        return false;

    if (lane == PreparedPlaybackWorkLane::preview)
        discardSupersededPreviewPendingPreparedCompletions(submitResult.request.buildId);

    pendingPreparedCompletions[submitResult.request.buildId] = {
        lane,
        false,
        contractRequestId,
        snapshotResult,
        std::move(publishIdentity)
    };
    return true;
}

bool EngineFacade::enqueuePerformancePackagePreparedBuild(const PlaybackSnapshotBuildResult& snapshotResult)
{
    auto submitResult = preparedPlaybackService.enqueuePublishBuild(snapshotResult);

    for (const auto& displacedResult : submitResult.displacedResults)
        pendingPreparedCompletions.erase(displacedResult.buildId);

    if (!submitResult.accepted)
        return false;

    pendingPreparedCompletions[submitResult.request.buildId] = {
        PreparedPlaybackWorkLane::performance,
        true,
        0,
        snapshotResult,
        {}
    };
    return true;
}

bool EngineFacade::pumpPreparedPlaybackWorkerCompletions()
{
    auto completedResults = preparedPlaybackService.drainCompletedBuilds();
    bool appliedCompletion = false;

    for (const auto& stepResult : completedResults)
    {
        const auto pendingIterator = pendingPreparedCompletions.find(stepResult.result.buildId);
        if (pendingIterator == pendingPreparedCompletions.end())
            continue;

        const auto pendingCompletion = pendingIterator->second;
        bool applied = false;

        if (pendingCompletion.performancePackage)
        {
            const auto payload = buildPlaybackActivationPayload(
                PlaybackActivationLane::performance,
                pendingCompletion.snapshotResult.requestedDraftRevision,
                &pendingCompletion.snapshotResult,
                &stepResult.result);
            if (payload != nullptr)
            {
                packagePerformanceActivationPayload = payload;
                SamplerRenderModelBuildOptions renderOptions;
                renderOptions.selectedArticulationId = currentSessionState.selectedArticulationId;
                const auto renderModel = buildSamplerRenderModel(payload, renderOptions);
                packagePerformanceRenderModel = renderModel.built
                    ? renderModel.model : SamplerRenderModelPtr {};
                currentSessionState.transientMetrics.integrationState
                    = "Performance package prepared for activation";
                currentSessionState.transientMetrics.lastFailure.clear();
                applied = true;
            }
            else
            {
                packagePerformanceActivationPayload.reset();
                packagePerformanceRenderModel.reset();
                currentSessionState.transientMetrics.integrationState
                    = "Performance package preparation failed";
                currentSessionState.transientMetrics.lastFailure =
                    summarizeSnapshotFindings(stepResult.result.findings);
            }
        }
        else if (pendingCompletion.lane == PreparedPlaybackWorkLane::preview)
        {
            applied = draftPlaybackContract.completePreviewBuild(
                pendingCompletion.contractRequestId,
                pendingCompletion.snapshotResult,
                stepResult.result);

            if (applied)
            {
                if (stepResult.result.built && stepResult.result.activationEligible)
                {
                    currentSessionState.transientMetrics.integrationState = "Preview revision prepared";
                    currentSessionState.transientMetrics.lastFailure.clear();
                }
                else
                {
                    currentSessionState.transientMetrics.integrationState = "Preview revision failed";
                    currentSessionState.transientMetrics.lastFailure =
                        summarizeSnapshotFindings(draftPlaybackContract.getStatus().preview.findings);
                }
            }
        }
        else
        {
            const auto preparation = validatePerformancePublishPreparation(
                pendingCompletion.publishIdentity,
                pendingCompletion.snapshotResult,
                stepResult.result);
            auto contractResult = stepResult.result;
            if (!preparation.activationEligible)
            {
                contractResult.built = false;
                contractResult.activationEligible = false;
                contractResult.lifecycleState = PlaybackSnapshotLifecycleState::failed;
                contractResult.state = "Full-project Performance preparation failed conformance";
                contractResult.findings = preparation.findings;
                contractResult.metrics.failureCount = 1;
            }
            applied = draftPlaybackContract.completePerformanceBuild(
                pendingCompletion.contractRequestId,
                pendingCompletion.snapshotResult,
                contractResult);

            const auto& publishResult = preparation.publishResult;

            const auto preparedAccepted = applied && publishResult.activationEligible
                && performancePublishController.acceptPrepared(publishResult, monotonicMicros());
            if (!preparedAccepted && performancePublishController.isCurrent(publishResult.identity))
            {
                const auto finding = !preparation.findings.empty()
                    ? makePerformancePublishFinding(preparation.findings.front())
                    : makePerformancePublishFailure(
                        "publish-preparation-failed", "preparedWorker",
                        stepResult.result.state.empty()
                            ? std::string("Publish preparation did not produce an eligible result.")
                            : stepResult.result.state);
                performancePublishController.fail(publishResult.identity, finding);
            }

            if (applied)
            {
                if (preparation.completeProject && preparation.activationEligible)
                {
                    currentSessionState.transientMetrics.integrationState = "Published revision prepared for activation";
                    currentSessionState.transientMetrics.lastFailure.clear();
                }
                else
                {
                    currentSessionState.transientMetrics.integrationState = "Publish preparation failed";
                    currentSessionState.transientMetrics.lastFailure =
                        summarizeSnapshotFindings(draftPlaybackContract.getStatus().performance.findings);
                }
            }
        }

        pendingPreparedCompletions.erase(pendingIterator);
        appliedCompletion = appliedCompletion || applied;
    }

    if (appliedCompletion)
    {
        previewPlaybackSnapshot = {};
        syncPreviewSnapshotFromDraftPlayback();
        refreshDiagnosticsSnapshot();
        markStateChanged();
    }

    return appliedCompletion;
}

void EngineFacade::markStateChanged()
{
    ++stateRevision;
}

void EngineFacade::clearPendingPreparedCompletions()
{
    const auto canceledPreview = preparedPlaybackService.cancelQueuedPreviewBuilds(
        "Prepared playback preview build canceled before worker execution");
    const auto canceledPerformance = preparedPlaybackService.cancelQueuedPublishBuilds(
        "Prepared playback publish build canceled before worker execution");

    for (const auto& result : canceledPreview)
        pendingPreparedCompletions.erase(result.buildId);

    for (const auto& result : canceledPerformance)
        pendingPreparedCompletions.erase(result.buildId);

    pendingPreparedCompletions.clear();
    preparedPlaybackService.drainCompletedBuilds();
}

void EngineFacade::discardSupersededPreviewPendingPreparedCompletions(const std::uint64_t newestBuildId)
{
    for (auto iterator = pendingPreparedCompletions.begin(); iterator != pendingPreparedCompletions.end();)
    {
        if (iterator->second.lane == PreparedPlaybackWorkLane::preview
            && iterator->first != newestBuildId)
        {
            iterator = pendingPreparedCompletions.erase(iterator);
            continue;
        }

        ++iterator;
    }
}

bool EngineFacade::beginDraftPlaybackDeviceRestart()
{
    clearPendingPreparedCompletions();
    if (!draftPlaybackContract.beginDeviceRestart())
        return false;

    currentSessionState.transientMetrics.integrationState = "Device restart in progress";
    previewPlaybackSnapshot = {};
    syncPreviewSnapshotFromDraftPlayback();
    refreshDiagnosticsSnapshot();
    markStateChanged();
    return true;
}

bool EngineFacade::completeDraftPlaybackDeviceRestart(bool restored)
{
    pumpPreparedPlaybackWorkerCompletions();
    if (!draftPlaybackContract.completeDeviceRestart(restored))
        return false;

    currentSessionState.transientMetrics.integrationState = restored
        ? "Device restart recovered"
        : "Device restart failed";
    if (!restored)
        currentSessionState.transientMetrics.lastFailure = "Device restart failed to restore the published revision.";
    else
        currentSessionState.transientMetrics.lastFailure.clear();
    previewPlaybackSnapshot = {};
    syncPreviewSnapshotFromDraftPlayback();
    refreshDiagnosticsSnapshot();
    markStateChanged();
    return true;
}

bool EngineFacade::waitForPreparedPlaybackIdle(std::chrono::milliseconds timeout)
{
    const auto deadline = Clock::now() + timeout;

    while (Clock::now() <= deadline)
    {
        serviceBackgroundWork();
        const auto workerStatus = preparedPlaybackService.getWorkerStatus();
        if (pendingPreparedCompletions.empty()
            && !draftPlaybackContract.getStatus().pendingPreview.active
            && !draftPlaybackContract.getStatus().pendingPerformance.active
            && workerStatus.pendingWorkCount == 0
            && workerStatus.inFlightWorkCount == 0)
        {
            return true;
        }

        const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(deadline - Clock::now());
        if (remaining.count() > 0)
            preparedPlaybackService.waitForWorkerIdle(remaining.count() > 25 ? 25 : static_cast<std::uint64_t>(remaining.count()));
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }

    serviceBackgroundWork();
    const auto workerStatus = preparedPlaybackService.getWorkerStatus();
    return pendingPreparedCompletions.empty()
        && !draftPlaybackContract.getStatus().pendingPreview.active
        && !draftPlaybackContract.getStatus().pendingPerformance.active
        && workerStatus.pendingWorkCount == 0
        && workerStatus.inFlightWorkCount == 0;
}

EnginePreviewPlaybackSnapshot EngineFacade::auditionPreviewNote(int midiNote, int velocity)
{
    pumpPreparedPlaybackWorkerCompletions();
    previewPlaybackSnapshot = {};
    syncPreviewSnapshotFromDraftPlayback();
    previewPlaybackSnapshot.midiNote = midiNote;
    previewPlaybackSnapshot.velocity = velocity;
    previewPlaybackSnapshot.effectiveMidiNote = midiNote;
    previewPlaybackSnapshot.effectiveVelocity = velocity;
    previewPlaybackSnapshot.articulationId = currentSessionState.selectedArticulationId;
    previewPlaybackSnapshot.appliedMacroSummary = buildAppliedMacroSummary(currentSessionState);

    if (!referenceInstrumentActive)
    {
        previewPlaybackSnapshot.state = "Preview unavailable";
        previewPlaybackSnapshot.errorMessage = "No instrument is loaded.";
        return previewPlaybackSnapshot;
    }

    if (!referenceManifest.loaded)
    {
        previewPlaybackSnapshot.state = "Preview unavailable";
        previewPlaybackSnapshot.errorMessage = referenceManifest.state;
        return previewPlaybackSnapshot;
    }

    if (!referenceStream.loaded)
    {
        previewPlaybackSnapshot.state = "Preview unavailable";
        previewPlaybackSnapshot.errorMessage = referenceStream.state;
        return previewPlaybackSnapshot;
    }

    if ((!draftPlaybackContract.getStatus().preview.available
         || draftPlaybackContract.getStatus().preview.revision != draftPlaybackContract.getStatus().draftRevision)
        && !refreshPreviewToCurrentDraft())
    {
        previewPlaybackSnapshot.state = "Preview unavailable";
        previewPlaybackSnapshot.errorMessage = "Preview revision could not be prepared for the current draft.";
        syncPreviewSnapshotFromDraftPlayback();
        return previewPlaybackSnapshot;
    }

    syncPreviewSnapshotFromDraftPlayback();
    const auto loadProfile = findPhase1RuntimeLoadProfile(currentSessionState.loadProfileId);
    if (!loadProfile.has_value())
    {
        previewPlaybackSnapshot.state = "Preview unavailable";
        previewPlaybackSnapshot.errorMessage = "Unknown load profile '" + currentSessionState.loadProfileId + "'.";
        return previewPlaybackSnapshot;
    }

    previewPlaybackSnapshot.ready = true;
    previewPlaybackSnapshot.attempted = true;

    const auto effectiveMidiNote = computeMotionPreviewNote(currentSessionState, midiNote);
    const auto effectiveVelocity = computeTonePreviewVelocity(currentSessionState, velocity);
    previewPlaybackSnapshot.effectiveMidiNote = effectiveMidiNote;
    previewPlaybackSnapshot.effectiveVelocity = effectiveVelocity;

    RuntimeStreamingService service(
        referenceStream.container,
        buildRuntimeStreamingServiceOptions(*loadProfile, 2500));
    RuntimeVoice previewVoice;
    std::string errorMessage;

    const auto allocated = previewVoice.allocate(referenceManifest.instrument,
                                                 referenceStream.container,
                                                 {
                                                     nextPreviewVoiceId++,
                                                     "",
                                                     effectiveMidiNote,
                                                     effectiveVelocity,
                                                     toVoiceMacroValues(currentSessionState),
                                                     currentSessionState.selectedArticulationId
                                                 },
                                                 errorMessage);
    if (!allocated)
    {
        previewPlaybackSnapshot.state = "Preview allocation failed";
        previewPlaybackSnapshot.errorMessage = errorMessage;
        return previewPlaybackSnapshot;
    }

    previewVoice.advanceFrames(4096, service);
    const auto boundaryAdvance = previewVoice.advanceFrames(64, service);
    previewPlaybackSnapshot.waitedForPage = boundaryAdvance.waitingForPage;

    if (previewPlaybackSnapshot.waitedForPage)
    {
        const auto pageReady = waitUntil(
            [&]
            {
                const auto snapshot = previewVoice.getSnapshot();
                return !snapshot.sampleId.empty()
                    && service.isPageReady({ snapshot.sampleId, 0 });
            },
            std::chrono::milliseconds(500));

        if (pageReady)
        {
            const auto resumedAdvance = previewVoice.advanceFrames(64, service);
            previewPlaybackSnapshot.acquiredPageLease = resumedAdvance.acquiredPageLease;
        }
    }

    previewPlaybackSnapshot.zoneId = previewVoice.getSnapshot().zoneId;
    previewVoice.beginRelease();
    previewPlaybackSnapshot.voiceFinished = waitUntil(
        [&]
        {
            const auto advance = previewVoice.advanceFrames(8192, service);
            return advance.voiceFinished
                || previewVoice.getSnapshot().state == RuntimeVoiceLifecycleState::finished;
        },
        std::chrono::milliseconds(1500));
    previewPlaybackSnapshot.succeeded = previewPlaybackSnapshot.voiceFinished;
    previewPlaybackSnapshot.state = previewPlaybackSnapshot.succeeded
        ? "Preview played"
        : "Preview did not finish cleanly";
    syncPreviewSnapshotFromDraftPlayback();
    markStateChanged();

    return previewPlaybackSnapshot;
}

SfzImportAnalysisResult EngineFacade::analyzeSfzImportDocument(const std::string& sfzPath) const
{
    return ::drs::engine::analyzeSfzImportDocument(sfzPath);
}

SfzImportProjectionResult EngineFacade::projectSfzImportDocument(const RuntimeProjectModel& baseProject,
                                                                 const std::string& sfzPath) const
{
    return ::drs::engine::projectSfzImportDocument(baseProject, sfzPath);
}

std::string EngineFacade::exportPresetStateJson() const
{
    return serializeRuntimePresetState(captureRuntimePresetState(currentSessionState));
}

RuntimePresetStateValidationResult EngineFacade::validateProjectPresetState(
    const RuntimePresetState& presetState,
    const RuntimeProjectModel& project) const
{
    RuntimePresetStateValidationResult result;
    result.state = "Project preset state validation failed";

    const auto projectValidation = validateRuntimeProjectModel(project);
    if (!projectValidation.valid)
    {
        result.issues = projectValidation.issues;
        return result;
    }

    if (!referenceManifest.loaded)
    {
        result.issues.push_back(
            "Reference instrument manifest is unavailable, so project preset state cannot be validated.");
        return result;
    }

    const auto contextualInstrument = buildProjectPresetValidationInstrument(
        referenceManifest.instrument, project);
    result = validateRuntimePresetState(presetState, contextualInstrument);
    result.state = result.valid
        ? "Project preset state validated"
        : "Project preset state validation failed";
    return result;
}

EnginePresetStateRestoreResult EngineFacade::restoreProjectPresetState(
    const RuntimePresetState& presetState,
    const RuntimeProjectModel& project)
{
    EnginePresetStateRestoreResult restoreResult;
    restoreResult.state = "Project preset state restore failed";

    if (!authoringProject.loaded
        || authoringProject.project.projectId != project.projectId)
    {
        restoreResult.issues.push_back(
            "The project preset state does not match the active authored draft project.");
    }
    else
    {
        const auto validation = validateProjectPresetState(presetState, project);
        if (!validation.valid)
            restoreResult.issues = validation.issues;
    }

    if (!restoreResult.issues.empty())
    {
        currentSessionState.transientMetrics.integrationState = restoreResult.state;
        currentSessionState.transientMetrics.lastFailure = summarizeIssues(restoreResult.issues);
        refreshDiagnosticsSnapshot();
        markStateChanged();
        return restoreResult;
    }

    currentSessionState.presetId = presetState.presetId;
    currentSessionState.targetInstrumentId = presetState.targetInstrumentId;
    currentSessionState.targetInstrumentSchemaName = presetState.targetInstrumentSchemaName;
    currentSessionState.targetInstrumentSchemaVersion = presetState.targetInstrumentSchemaVersion;
    currentSessionState.loadProfileId = presetState.loadProfileId;
    currentSessionState.selectedArticulationId = presetState.selectedArticulationId;
    currentSessionState.macroValues = presetState.macroValues;
    currentSessionState.notes = presetState.notes;
    currentSessionState.transientMetrics.integrationState = "Project preset state restored";
    currentSessionState.transientMetrics.lastFailure.clear();

    // The authored draft has already been staged and reopened by the restore transaction.
    // Do not call initializeDraftPlaybackContract(): that is the legacy reference-preset path
    // and would replace the project-aware playback context we are restoring.
    referenceInstrumentActive = true;
    previewPlaybackSnapshot = {};
    syncPreviewSnapshotFromDraftPlayback();
    previewPlaybackSnapshot.articulationId = currentSessionState.selectedArticulationId;
    refreshDiagnosticsSnapshot();
    markStateChanged();

    restoreResult.restored = true;
    restoreResult.state = "Project preset state restored";
    return restoreResult;
}

EnginePresetStateRestoreResult EngineFacade::restorePresetStateJson(const std::string& presetStateJson)
{
    EnginePresetStateRestoreResult restoreResult;
    restoreResult.state = "Preset state restore failed";

    const auto parsedState = parseRuntimePresetState(presetStateJson);
    if (!parsedState.loaded)
    {
        restoreResult.issues = parsedState.issues;
        currentSessionState.transientMetrics.integrationState = "Preset state restore failed";
        currentSessionState.transientMetrics.lastFailure = summarizeIssues(parsedState.issues);
        refreshDiagnosticsSnapshot();
        markStateChanged();
        return restoreResult;
    }

    if (!referenceManifest.loaded)
    {
        restoreResult.issues.push_back("Reference instrument manifest is unavailable, so preset state cannot be restored.");
        currentSessionState.transientMetrics.integrationState = "Preset state restore failed";
        currentSessionState.transientMetrics.lastFailure = restoreResult.issues.front();
        refreshDiagnosticsSnapshot();
        markStateChanged();
        return restoreResult;
    }

    const auto validation = validateRuntimePresetState(parsedState.preset, referenceManifest.instrument);
    if (!validation.valid)
    {
        restoreResult.issues = validation.issues;
        currentSessionState.transientMetrics.integrationState = "Preset state restore failed";
        currentSessionState.transientMetrics.lastFailure = summarizeIssues(validation.issues);
        refreshDiagnosticsSnapshot();
        markStateChanged();
        return restoreResult;
    }

    currentSessionState.presetId = parsedState.preset.presetId;
    currentSessionState.targetInstrumentId = parsedState.preset.targetInstrumentId;
    currentSessionState.targetInstrumentSchemaName = parsedState.preset.targetInstrumentSchemaName;
    currentSessionState.targetInstrumentSchemaVersion = parsedState.preset.targetInstrumentSchemaVersion;
    currentSessionState.loadProfileId = parsedState.preset.loadProfileId;
    currentSessionState.selectedArticulationId = parsedState.preset.selectedArticulationId;
    currentSessionState.macroValues = parsedState.preset.macroValues;
    currentSessionState.notes = parsedState.preset.notes;
    currentSessionState.transientMetrics.integrationState = "Preset state restored";
    currentSessionState.transientMetrics.lastFailure.clear();
    referenceInstrumentActive = true;
    previewPlaybackSnapshot = {};
    initializeDraftPlaybackContract(false, false);
    previewPlaybackSnapshot.articulationId = currentSessionState.selectedArticulationId;
    refreshDiagnosticsSnapshot();
    markStateChanged();

    restoreResult.restored = true;
    restoreResult.state = "Preset state restored";
    return restoreResult;
}

EnginePresetStateRestoreResult EngineFacade::restorePresetStateFile(const std::string& presetStatePath)
{
    return restorePresetStateJson(readTextFile(presetStatePath));
}

EngineContentFailureProbeResult EngineFacade::probeContentFailure(const EngineContentFailureCategory category)
{
    EngineContentFailureProbeResult probeResult;
    probeResult.attempted = true;
    probeResult.categoryId = getFailureCategoryId(category);
    probeResult.state = "Content failure probe did not run";

    switch (category)
    {
    case EngineContentFailureCategory::missingContent:
    case EngineContentFailureCategory::schemaMismatch:
    case EngineContentFailureCategory::partialCompiledArtifact:
    {
        const auto manifestResult = loadRuntimeInstrumentManifest(getFailureFixturePath(category).generic_string());
        probeResult.failedGracefully = !manifestResult.loaded && !manifestResult.issues.empty();
        probeResult.state = manifestResult.state;
        probeResult.issues = manifestResult.issues;
        break;
    }
    case EngineContentFailureCategory::badChecksum:
    {
        const auto corruptStreamPath = buildChecksumMismatchFixture();
        const auto streamResult = loadRuntimeStreamContainer(corruptStreamPath.generic_string());
        probeResult.failedGracefully = !streamResult.loaded && !streamResult.issues.empty();
        probeResult.state = streamResult.state;
        probeResult.issues = streamResult.issues;
        break;
    }
    }

    lastContentFailureProbe = probeResult;
    refreshDiagnosticsSnapshot();
    markStateChanged();
    return probeResult;
}

void EngineFacade::clearContentFailureProbe()
{
    lastContentFailureProbe = {};
    refreshDiagnosticsSnapshot();
    markStateChanged();
}

void EngineFacade::resetSessionStateToDefault()
{
    referenceManifest = bundledReferenceManifest;
    referenceStream = bundledReferenceStream;
    packagePerformanceActivationPayload.reset();
    packagePerformanceRenderModel.reset();
    packageBackgroundArtworkPayloadId.clear();
    packageBackgroundArtworkJpgBytes.reset();
    preparedPlaybackService.setBackgroundWorkerStream(referenceStream);

    if (!referenceManifest.loaded)
    {
        referenceInstrumentActive = false;
        currentSessionState = {};
        currentSessionState.transientMetrics.integrationState = "Reference manifest unavailable";
        currentSessionState.transientMetrics.lastFailure = referenceManifest.state;
        lastContentFailureProbe = {};
        refreshDiagnosticsSnapshot();
        markStateChanged();
        return;
    }

    referenceInstrumentActive = true;
    currentSessionState = buildDefaultRuntimeSessionState(referenceManifest);
    currentSessionState.transientMetrics.integrationState = "Default preset state loaded";
    currentSessionState.transientMetrics.lastFailure.clear();
    previewPlaybackSnapshot = {};

    // An unloaded authoring shell still needs a real immutable bootstrap for the explicit
    // bundled-session reset. Snapshot the checked-in reference project for this build only;
    // do not bind it as the user's authoring document.
    const auto usedTemporaryBootstrapProject = !authoringProject.loaded;
    if (usedTemporaryBootstrapProject)
    {
        auto bootstrapProject = loadPhase2ReferenceProjectManifest();
        if (bootstrapProject.loaded)
            authoringProject = std::move(bootstrapProject);
    }
    initializeDraftPlaybackContract(true);
    if (usedTemporaryBootstrapProject)
        authoringProject = {};
    previewPlaybackSnapshot.articulationId = currentSessionState.selectedArticulationId;
    lastContentFailureProbe = {};
    refreshDiagnosticsSnapshot();
    markStateChanged();
}

void EngineFacade::syncPreviewSnapshotFromDraftPlayback()
{
    const auto& draftStatus = draftPlaybackContract.getStatus();
    previewPlaybackSnapshot.ready = draftStatus.preview.available;
    previewPlaybackSnapshot.draftRevision = draftStatus.draftRevision;
    previewPlaybackSnapshot.preparedRevision = draftStatus.preview.revision;
    previewPlaybackSnapshot.pendingBuild = draftStatus.pendingPreview.active;
    previewPlaybackSnapshot.revisionState = draftStatus.preview.state;

    if (previewPlaybackSnapshot.articulationId.empty())
        previewPlaybackSnapshot.articulationId = currentSessionState.selectedArticulationId;

    if (previewPlaybackSnapshot.state.empty())
    {
        previewPlaybackSnapshot.state = draftStatus.preview.state.empty()
            ? buildLoadIndicator(referenceManifest, referenceStream, currentSessionState)
            : draftStatus.preview.state;
    }

    if (previewPlaybackSnapshot.errorMessage.empty() && !draftStatus.preview.findings.empty())
        previewPlaybackSnapshot.errorMessage = summarizeSnapshotFindings(draftStatus.preview.findings);
}

void EngineFacade::initializeDraftPlaybackContract(bool activatePerformanceRevision,
                                                   bool bootstrapPreparedPlayback)
{
    clearPendingPreparedCompletions();
    if (activatePerformanceRevision)
    {
        ++performancePublishProjectGeneration;
        performancePublishController.reset(true, true);
    }
    preparedPlaybackService.setBackgroundWorkerStream(referenceStream);
    draftPlaybackContract.reopenProject(0);

    if (!referenceManifest.loaded || !referenceStream.loaded)
    {
        previewPlaybackSnapshot = {};
        syncPreviewSnapshotFromDraftPlayback();
        return;
    }

    if (authoringProject.loaded)
    {
        if (bootstrapPreparedPlayback)
        {
            if (const auto previewRequest = draftPlaybackContract.requestPreviewBuild(); previewRequest.accepted)
            {
                const auto previewBuild = buildCurrentPlaybackSnapshot(false);
                if (!previewBuild.built || !previewBuild.activationEligible)
                {
                    const auto preparedPreview = buildRejectedPreparedPlayback(previewBuild);
                    draftPlaybackContract.completePreviewBuild(previewRequest.requestId, previewBuild, preparedPreview);
                }
                else if (!enqueuePreparedPlaybackBuild(previewRequest.requestId,
                                                       previewBuild,
                                                       PreparedPlaybackWorkLane::preview))
                {
                    PreparedPlaybackBuildResult queueRejected;
                    queueRejected.snapshotBuildId = previewBuild.buildId;
                    queueRejected.requestedDraftRevision = previewBuild.requestedDraftRevision;
                    queueRejected.activationRequested = previewBuild.activationRequested;
                    queueRejected.lifecycleState = PlaybackSnapshotLifecycleState::failed;
                    queueRejected.state = "Prepared playback queue is full";
                    queueRejected.metrics.failureCount = 1;
                    queueRejected.findings.push_back({
                        PlaybackSnapshotFindingSeverity::error,
                        "prepared-queue-full",
                        "preparedWorker",
                        "Prepared playback queue is full."
                    });
                    draftPlaybackContract.completePreviewBuild(previewRequest.requestId,
                                                              previewBuild,
                                                              queueRejected);
                }
            }

            if (activatePerformanceRevision)
            {
                if (const auto publishRequest = draftPlaybackContract.requestPerformanceBuild();
                    publishRequest.accepted)
                {
                    const auto publishBuild = buildCurrentPlaybackSnapshot(true);
                    if (!publishBuild.built || !publishBuild.activationEligible)
                    {
                        const auto preparedPublish = buildRejectedPreparedPlayback(publishBuild);
                        draftPlaybackContract.completePerformanceBuild(publishRequest.requestId,
                                                                      publishBuild,
                                                                      preparedPublish);
                    }
                    else if (!enqueuePreparedPlaybackBuild(publishRequest.requestId,
                                                           publishBuild,
                                                           PreparedPlaybackWorkLane::performance,
                                                           true))
                    {
                        PreparedPlaybackBuildResult queueRejected;
                        queueRejected.snapshotBuildId = publishBuild.buildId;
                        queueRejected.requestedDraftRevision = publishBuild.requestedDraftRevision;
                        queueRejected.activationRequested = publishBuild.activationRequested;
                        queueRejected.lifecycleState = PlaybackSnapshotLifecycleState::failed;
                        queueRejected.state = "Prepared playback queue is full";
                        queueRejected.metrics.failureCount = 1;
                        queueRejected.findings.push_back({
                            PlaybackSnapshotFindingSeverity::error,
                            "prepared-queue-full",
                            "preparedWorker",
                            "Prepared playback queue is full."
                        });
                        draftPlaybackContract.completePerformanceBuild(publishRequest.requestId,
                                                                      publishBuild,
                                                                      queueRejected);
                    }
                }
            }

            waitForPreparedPlaybackIdle(std::chrono::milliseconds(1000));
        }
    }
    else
    {
        if (const auto previewRequest = draftPlaybackContract.requestPreviewBuild(); previewRequest.accepted)
            draftPlaybackContract.completePreviewBuild(previewRequest.requestId);

        if (activatePerformanceRevision)
        {
            if (const auto publishRequest = draftPlaybackContract.requestPerformanceBuild(); publishRequest.accepted)
                draftPlaybackContract.completePerformanceBuild(publishRequest.requestId);
        }
    }

    previewPlaybackSnapshot = {};
    syncPreviewSnapshotFromDraftPlayback();
}

void EngineFacade::refreshDiagnosticsSnapshot()
{
    diagnosticsSnapshot = {};
    syncSessionSelectionsIntoDiagnostics(currentSessionState, diagnosticsSnapshot);
    syncDraftPlaybackIntoDiagnostics(draftPlaybackContract.getStatus(), diagnosticsSnapshot);
    syncPreparedPlaybackWorkerIntoDiagnostics(preparedPlaybackService.getWorkerStatus(), diagnosticsSnapshot);
    auto publishPresentation = std::make_shared<const PerformancePublishPresentationSnapshot>(
        buildPerformancePublishPresentationSnapshot(
            draftPlaybackContract.getStatus(),
            performancePublishController.getSnapshot(),
            preparedPlaybackService.getWorkerStatus(),
            nextPerformancePublishPresentationSequence++));
    std::atomic_store_explicit(&performancePublishPresentation,
                               std::move(publishPresentation),
                               std::memory_order_release);
    if (const auto presentation = getPerformancePublishPresentationSnapshot())
        diagnosticsSnapshot.publishedPresentationState = presentation->state;
    applyPreparedCachePressurePolicy(diagnosticsSnapshot);
    diagnosticsSnapshot.rendererMode = resolveRendererMode(referenceInstrumentActive);

    if (!referenceInstrumentActive)
    {
        diagnosticsSnapshot.headline = "No instrument loaded";
        diagnosticsSnapshot.failureState = "Reference-backed renderer is available but inactive.";
        return;
    }

    if (!referenceManifest.loaded)
    {
        diagnosticsSnapshot.headline = "Reference manifest unavailable";
        diagnosticsSnapshot.hasFailure = true;
        diagnosticsSnapshot.failureState = referenceManifest.state;
        diagnosticsSnapshot.issues = referenceManifest.issues;
        return;
    }

    if (!referenceStream.loaded)
    {
        diagnosticsSnapshot.headline = "Reference stream unavailable";
        diagnosticsSnapshot.hasFailure = true;
        diagnosticsSnapshot.failureState = referenceStream.state;
        diagnosticsSnapshot.issues = referenceStream.issues;
        return;
    }

    const auto loadProfile = findPhase1RuntimeLoadProfile(currentSessionState.loadProfileId);
    if (!loadProfile.has_value())
    {
        diagnosticsSnapshot.headline = "Diagnostics unavailable";
        diagnosticsSnapshot.hasFailure = true;
        diagnosticsSnapshot.failureState = "Unknown current load profile '" + currentSessionState.loadProfileId + "'.";
        diagnosticsSnapshot.issues.push_back(diagnosticsSnapshot.failureState);
        return;
    }

    diagnosticsSnapshot.available = true;
    diagnosticsSnapshot.headline = "Reference-backed runtime diagnostics ready";

    for (const auto& finding : draftPlaybackContract.getStatus().preview.findings)
        diagnosticsSnapshot.issues.push_back("Preview snapshot: " + finding.message);

    for (const auto& finding : draftPlaybackContract.getStatus().performance.findings)
        diagnosticsSnapshot.issues.push_back("Publish snapshot: " + finding.message);
    diagnosticsSnapshot.configuredMaxCachedPages = loadProfile->maxCachedPages;
    diagnosticsSnapshot.maxPrefetchBytesPerVoice = loadProfile->maxPrefetchBytesPerVoice;

    if (packagePerformanceActivationPayload != nullptr)
    {
        const auto worker = preparedPlaybackService.getWorkerStatus();
        diagnosticsSnapshot.headline = "Package v2 bounded-stream diagnostics ready";
        diagnosticsSnapshot.cacheMissCount = worker.pageIntentCount;
        diagnosticsSnapshot.pageMissCount = worker.pagePrepareFailureCount;
        diagnosticsSnapshot.backgroundReadCount = worker.pagePrepareCount;
        diagnosticsSnapshot.pendingPageCount = worker.pendingWorkCount;
        diagnosticsSnapshot.lastContentProbeCategory = lastContentFailureProbe.categoryId;
        diagnosticsSnapshot.lastContentProbeFailedGracefully
            = lastContentFailureProbe.failedGracefully;
        diagnosticsSnapshot.lastContentProbeState = lastContentFailureProbe.state;
        diagnosticsSnapshot.lastContentProbeIssues = lastContentFailureProbe.issues;
        diagnosticsSnapshot.hasFailure = worker.pagePrepareFailureCount != 0
            || !currentSessionState.transientMetrics.lastFailure.empty();
        diagnosticsSnapshot.failureState = currentSessionState.transientMetrics.lastFailure;
        return;
    }

    try
    {
        RuntimeStreamingService service(
            referenceStream.container,
            buildRuntimeStreamingServiceOptions(*loadProfile, 1500));

        const auto macroValues = toVoiceMacroValues(currentSessionState);
        RuntimeVoice sustainVoiceA;
        RuntimeVoice sustainVoiceB;
        RuntimeVoice leadVoice;
        std::string errorMessage;

        if (!sustainVoiceA.allocate(referenceManifest.instrument,
                                    referenceStream.container,
                                    { 2101, "", 57, 64, macroValues, "" },
                                    errorMessage))
        {
            throw std::runtime_error("Diagnostics sustain voice A failed to allocate: " + errorMessage);
        }

        if (!sustainVoiceB.allocate(referenceManifest.instrument,
                                    referenceStream.container,
                                    { 2102, "", 57, 120, macroValues, "" },
                                    errorMessage))
        {
            throw std::runtime_error("Diagnostics sustain voice B failed to allocate: " + errorMessage);
        }

        if (!leadVoice.allocate(referenceManifest.instrument,
                                referenceStream.container,
                                { 2103, "", 69, 120, macroValues, "lead" },
                                errorMessage))
        {
            throw std::runtime_error("Diagnostics lead voice failed to allocate: " + errorMessage);
        }

        diagnosticsSnapshot.routedZones.push_back(sustainVoiceA.getSnapshot().zoneId);
        diagnosticsSnapshot.routedZones.push_back(sustainVoiceB.getSnapshot().zoneId);
        diagnosticsSnapshot.routedZones.push_back(leadVoice.getSnapshot().zoneId);

        sustainVoiceA.advanceFrames(4096, service);
        sustainVoiceB.advanceFrames(4096, service);
        leadVoice.advanceFrames(2048, service);
        sustainVoiceA.advanceFrames(64, service);
        sustainVoiceB.advanceFrames(64, service);
        leadVoice.advanceFrames(64, service);

        const auto firstPagesReady = waitUntil(
            [&]
            {
                return service.isPageReady({ "sine-a3", 0 })
                    && service.isPageReady({ "triangle-a4", 0 });
            },
            std::chrono::milliseconds(500));

        if (!firstPagesReady)
            throw std::runtime_error("Diagnostics service did not make the first streamed pages ready in time.");

        sustainVoiceA.advanceFrames(64, service);
        sustainVoiceB.advanceFrames(64, service);
        leadVoice.advanceFrames(64, service);

        std::vector<RuntimeStreamPageRequest> followUpRequests;
        for (const auto& sample : referenceStream.container.samples)
        {
            for (const auto& page : sample.pages)
            {
                if (page.pageIndex == 0)
                    continue;

                followUpRequests.push_back({ sample.sampleId, page.pageIndex });
            }
        }

        for (const auto& request : followUpRequests)
            service.enqueuePageRead(request);

        const auto expectedBackgroundReads = static_cast<std::size_t>(2 + followUpRequests.size());
        const auto queuedReadsReady = waitUntil(
            [&]
            {
                return service.getMetrics().backgroundReadCount >= expectedBackgroundReads;
            },
            std::chrono::milliseconds(1000));

        if (!queuedReadsReady)
            throw std::runtime_error("Diagnostics service did not finish the queued background reads in time.");

        sustainVoiceA.beginRelease();
        sustainVoiceB.beginRelease();
        leadVoice.beginRelease();
        drainVoice(sustainVoiceA, service);
        drainVoice(sustainVoiceB, service);
        drainVoice(leadVoice, service);
        service.purgeDormantPages();

        const auto metrics = service.getMetrics();
        diagnosticsSnapshot.cacheHitCount = metrics.cacheHitCount;
        diagnosticsSnapshot.cacheMissCount = metrics.cacheMissCount;
        diagnosticsSnapshot.pageMissCount = metrics.pageMissCount;
        diagnosticsSnapshot.backgroundReadCount = metrics.backgroundReadCount;
        diagnosticsSnapshot.residentPageCount = metrics.residentPageCount;
        diagnosticsSnapshot.pendingPageCount = metrics.pendingPageCount;
        diagnosticsSnapshot.activeVoiceCount = metrics.activeVoiceCount;
        diagnosticsSnapshot.peakActiveVoiceCount = metrics.peakActiveVoiceCount;
        diagnosticsSnapshot.purgePassCount = metrics.purgePassCount;
        diagnosticsSnapshot.dormantPurgeCount = metrics.dormantPurgeCount;
        diagnosticsSnapshot.evictedPageCount = metrics.evictedPageCount;
        diagnosticsSnapshot.lastPurgeEvictedPageCount = metrics.lastPurgeEvictedPageCount;
        diagnosticsSnapshot.averageReadLatencyMicros = metrics.averageReadLatencyMicros;
        diagnosticsSnapshot.maxReadLatencyMicros = metrics.maxReadLatencyMicros;
        diagnosticsSnapshot.headFramesRead = metrics.headFramesRead;
        diagnosticsSnapshot.headBytesRead = metrics.headBytesRead;
    }
    catch (const std::exception& exception)
    {
        diagnosticsSnapshot.hasFailure = true;
        diagnosticsSnapshot.failureState = exception.what();
        diagnosticsSnapshot.issues.push_back(exception.what());
        return;
    }

    diagnosticsSnapshot.lastContentProbeCategory = lastContentFailureProbe.categoryId;
    diagnosticsSnapshot.lastContentProbeFailedGracefully = lastContentFailureProbe.failedGracefully;
    diagnosticsSnapshot.lastContentProbeState = lastContentFailureProbe.state;
    diagnosticsSnapshot.lastContentProbeIssues = lastContentFailureProbe.issues;

    if (lastContentFailureProbe.attempted && !lastContentFailureProbe.state.empty())
        diagnosticsSnapshot.failureState = lastContentFailureProbe.state;
    else if (!currentSessionState.transientMetrics.lastFailure.empty())
        diagnosticsSnapshot.failureState = currentSessionState.transientMetrics.lastFailure;

    diagnosticsSnapshot.hasFailure = !diagnosticsSnapshot.failureState.empty() || !diagnosticsSnapshot.issues.empty();
}

PreparedPerformancePackageActivationResult preparePerformancePackageActivation(
    const PerformancePackageLoadResult& packageLoad,
    const PerformancePackagePreparationTimings& priorTimings)
{
    return buildPreparedPerformancePackageActivation(packageLoad, priorTimings);
}

PreparedPerformancePackageActivationResult preparePerformancePackageV2Activation(
    const PerformancePackageLoadResult& packageLoad,
    std::shared_ptr<const PackageV2OpenResult> package,
    const std::vector<SampleDataSourceDescriptor>& sampleDescriptors,
    const PerformancePackagePreparationTimings& priorTimings)
{
    PreparedPerformancePackageActivationResult result;
    result.failureCategory = PerformancePackageFailureCategory::playbackCompatibilityFailure;
    result.state = "Performance package v2 activation preparation failed";
    result.packageLoad = packageLoad;
    result.timings = priorTimings;
    if (!packageLoad.loaded || package == nullptr || !package->opened
        || sampleDescriptors.empty())
    {
        result.issues.push_back("Package v2 metadata, TOC, and sample descriptors are required.");
        return result;
    }

    const auto snapshotStarted = Clock::now();
    result.snapshotResult = buildPerformancePackagePlaybackSnapshot(packageLoad);
    result.timings.snapshotBuildMicros = static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(Clock::now() - snapshotStarted).count());
    if (!result.snapshotResult.built || !result.snapshotResult.activationEligible)
    {
        for (const auto& finding : result.snapshotResult.findings)
            if (finding.severity == PlaybackSnapshotFindingSeverity::error)
                result.issues.push_back(finding.message);
        return result;
    }

    const auto preparedStarted = Clock::now();
    auto& preparedResult = result.preparedResult;
    preparedResult.buildId = 1;
    preparedResult.snapshotBuildId = result.snapshotResult.buildId;
    preparedResult.requestedDraftRevision = result.snapshotResult.requestedDraftRevision;
    preparedResult.lane = PreparedPlaybackWorkLane::performance;
    preparedResult.priority = PreparedPlaybackJobPriority::performance;
    preparedResult.lifecycleState = PlaybackSnapshotLifecycleState::ready;
    auto& prepared = preparedResult.prepared;
    prepared.snapshotBuildId = result.snapshotResult.buildId;
    prepared.snapshotContentDigest = result.snapshotResult.snapshot.contentDigest;
    prepared.snapshotDspGraphDigest = result.snapshotResult.snapshot.dspGraphDigest;
    prepared.dspGraphDigest = result.snapshotResult.snapshot.dspGraphDigest;
    prepared.compilerVersion = "package-v2-bounded-records";
    prepared.draftRevision = result.snapshotResult.snapshot.draftRevision;
    prepared.selectedGroupId = result.snapshotResult.snapshot.selectedGroupId;
    prepared.masterGainDb = result.snapshotResult.snapshot.masterGainDb;
    prepared.containerId = packageLoad.stream.container.containerId;
    prepared.containerPath = package->packagePath;
    prepared.payloadEncoding = "package-v2-paged-float32";
    prepared.pageSizeBytes = packageLoad.stream.container.pageSizeBytes;

    std::unordered_map<std::string, std::size_t> sampleIndices;
    std::unordered_map<std::string, std::size_t> streamIndices;
    for (const auto& identity : result.snapshotResult.snapshot.sampleIdentities)
    {
        const auto descriptor = std::find_if(sampleDescriptors.begin(), sampleDescriptors.end(),
            [&](const auto& value) { return value.sourceId == identity.sampleSourceId; });
        const auto streamSample = std::find_if(packageLoad.stream.container.samples.begin(),
                                               packageLoad.stream.container.samples.end(),
            [&](const auto& value) { return value.sampleId == identity.sampleSourceId; });
        if (descriptor == sampleDescriptors.end()
            || streamSample == packageLoad.stream.container.samples.end())
        {
            result.issues.push_back("Package v2 sample metadata is missing: "
                                    + identity.sampleSourceId + ".");
            return result;
        }
        auto source = std::make_shared<PackagePagedSampleDataSource>(*descriptor, package);
        if (!source->prepareHead())
        {
            result.issues.push_back("Package v2 head preparation failed for '"
                                    + identity.sampleSourceId + "': " + source->lastFailure());
            return result;
        }
        PreparedPlaybackSampleHandle sample;
        sample.sampleSourceId = identity.sampleSourceId;
        sample.streamSampleId = streamSample->sampleId;
        sample.sourcePath = streamSample->sourcePath;
        sample.canonicalSourcePath = descriptor->canonicalSourceIdentity;
        sample.canonicalSourceIdentity = descriptor->canonicalSourceIdentity;
        sample.sourceFingerprintHex = descriptor->provenanceIdentity;
        sample.formatName = descriptor->formatName;
        sample.role = identity.role;
        sample.channelLayout = descriptor->channelLayout;
        sample.sampleRate = descriptor->sampleRate;
        sample.frameCount = descriptor->frameCount;
        sample.channelCount = descriptor->channelCount;
        sample.rootMidiNotePresent = streamSample->rootMidiNotePresent;
        sample.rootMidiNote = streamSample->rootMidiNote;
        sample.loopRangePresent = streamSample->loopRangePresent;
        sample.loopStartFrame = streamSample->loopStartFrame;
        sample.loopEndFrame = streamSample->loopEndFrame;
        sample.dataSource = source;
        sample.ownershipToken = "package-v2-generation:"
            + std::to_string(descriptor->generation);
        sample.cacheKey = descriptor->provenanceIdentity;

        PreparedPlaybackStreamHandle stream;
        stream.sampleSourceId = identity.sampleSourceId;
        stream.streamSampleId = streamSample->sampleId;
        stream.containerId = packageLoad.stream.container.containerId;
        stream.containerPath = package->packagePath;
        stream.payloadEncoding = "package-v2-paged-float32";
        stream.topologyKind = "package-v2-records";
        stream.compiledStreamTopologyAvailable = true;
        stream.pageSizeBytes = descriptor->pageSizeBytes;
        stream.payloadOffsetBytes = streamSample->payloadOffsetBytes;
        stream.payloadSizeBytes = streamSample->payloadSizeBytes;
        stream.prefetchBytes = descriptor->headSizeBytes;
        stream.streamedPayloadOffsetBytes = streamSample->payloadOffsetBytes
            + descriptor->headSizeBytes;
        stream.streamedPayloadBytes = streamSample->payloadSizeBytes > descriptor->headSizeBytes
            ? streamSample->payloadSizeBytes - descriptor->headSizeBytes : 0;
        stream.pageCount = streamSample->pages.size();
        if (!streamSample->pages.empty())
        {
            stream.pageRangePresent = true;
            stream.firstPageIndex = streamSample->pages.front().pageIndex;
            stream.lastPageIndex = streamSample->pages.back().pageIndex;
            stream.firstPageOffsetBytes = streamSample->pages.front().offsetBytes;
            stream.lastPageOffsetBytes = streamSample->pages.back().offsetBytes;
            stream.lastPageSizeBytes = streamSample->pages.back().sizeBytes;
        }
        for (const auto& page : streamSample->pages)
            stream.pages.push_back({ page.pageIndex, page.offsetBytes, page.sizeBytes });
        stream.ownershipToken = sample.ownershipToken;
        stream.cacheKey = sample.cacheKey;

        PreparedPlaybackOwnershipRecord ownership;
        ownership.ownershipToken = sample.ownershipToken;
        ownership.cacheKey = sample.cacheKey;
        ownership.sampleSourceId = sample.sampleSourceId;
        ownership.streamSampleId = sample.streamSampleId;
        ownership.lifetimeState = "active-package-v2-generation";
        ownership.retainedBytes = source->metrics().publishedHeadBytes;
        ownership.preparedBuildId = preparedResult.buildId;
        sample.ownershipRecordIndex = prepared.ownershipRecords.size();
        stream.ownershipRecordIndex = sample.ownershipRecordIndex;
        prepared.ownershipRecords.push_back(std::move(ownership));
        sampleIndices.emplace(identity.sampleSourceId, prepared.samples.size());
        streamIndices.emplace(identity.sampleSourceId, prepared.streams.size());
        prepared.samples.push_back(std::move(sample));
        prepared.streams.push_back(std::move(stream));
    }

    for (const auto& zone : result.snapshotResult.snapshot.zones)
    {
        const auto sample = sampleIndices.find(zone.sampleSourceId);
        const auto stream = streamIndices.find(zone.sampleSourceId);
        if (sample == sampleIndices.end() || stream == streamIndices.end())
        {
            result.issues.push_back("Package v2 zone sample binding failed: " + zone.id + ".");
            return result;
        }
        prepared.zones.push_back({ zone.id, zone.sampleSourceId,
            prepared.samples[sample->second].streamSampleId, sample->second, stream->second,
            zone.rootKey, zone.keyLow, zone.keyHigh, zone.velocityLow, zone.velocityHigh,
            zone.velocityCrossfade, zone.velocityCrossfadeRuntime, zone.gainDb, zone.pan,
            zone.sampleStartFrame, zone.loopEnabled, zone.loopStartFrame, zone.loopEndFrame,
            zone.releaseSeconds, zone.roundRobin, zone.roundRobinLength,
            zone.roundRobinPosition, zone.triggerMode });
    }
    for (const auto& group : result.snapshotResult.snapshot.groupRoutes)
    {
        prepared.groupRoutes.push_back({ group.groupId, group.articulationIds, group.zoneIds,
            group.displayName, group.displayOrder, group.routingSourceId, group.workspaceVisible,
            group.gainDb, group.pan, group.routingBusId, group.auditionAnchorZoneId });
    }
    prepared.performanceProgram = result.snapshotResult.snapshot.performanceProgram;
    prepared.routeDigest = computePreparedPlaybackRouteDigest(result.snapshotResult.snapshot, prepared);
    prepared.sourceProvenanceDigest = computePreparedPlaybackSourceProvenanceDigest(prepared);
    prepared.macroSchemaDigest = computePlaybackSnapshotMacroSchemaDigest(result.snapshotResult.snapshot);
    prepared.preparedContentDigest = computePreparedPlaybackContentDigest(prepared);
    preparedResult.metrics.preparedSampleCount = prepared.samples.size();
    preparedResult.metrics.preparedStreamCount = prepared.streams.size();
    preparedResult.metrics.preparedZoneCount = prepared.zones.size();
    preparedResult.metrics.preparedOwnershipRecordCount = prepared.ownershipRecords.size();
    for (const auto& ownership : prepared.ownershipRecords)
        preparedResult.metrics.preparedOwnershipBytes += ownership.retainedBytes;
    preparedResult.metrics.preparedBytes = preparedResult.metrics.preparedOwnershipBytes
        + prepared.performanceProgram.retainedBytes;
    preparedResult.metrics.preparedPerformanceProgramBytes = prepared.performanceProgram.retainedBytes;
    preparedResult.admission.metadataAvailable = true;
    preparedResult.admission.sampleCount = prepared.samples.size();
    preparedResult.admission.readiness = PreparedPlaybackReadinessState::playable;
    preparedResult.built = true;
    preparedResult.activationEligible = true;
    preparedResult.completionDisposition = PreparedPlaybackCompletionDisposition::completed;
    preparedResult.state = "Package v2 prepared playback ready";
    result.timings.preparedBuildMicros = static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(Clock::now() - preparedStarted).count());

    const auto payloadStarted = Clock::now();
    result.activationPayload = buildPlaybackActivationPayload(
        PlaybackActivationLane::performance,
        result.snapshotResult.requestedDraftRevision,
        &result.snapshotResult,
        &preparedResult);
    result.timings.activationPayloadMicros = static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(Clock::now() - payloadStarted).count());
    result.timings.totalMicros = result.timings.packageLoadMicros
        + result.timings.snapshotBuildMicros + result.timings.preparedBuildMicros
        + result.timings.activationPayloadMicros;
    if (result.activationPayload == nullptr)
    {
        result.issues.push_back("Package v2 activation payload could not be built.");
        return result;
    }
    const auto renderModelStarted = Clock::now();
    const auto renderModel = buildPerformancePackageRenderModel(
        packageLoad, result.activationPayload);
    result.timings.renderModelBuildMicros = static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(
            Clock::now() - renderModelStarted).count());
    result.timings.totalMicros += result.timings.renderModelBuildMicros;
    if (!renderModel.built || renderModel.model == nullptr)
    {
        result.issues.push_back(renderModel.findings.empty()
            ? std::string("Package v2 render model could not be built.")
            : renderModel.findings.front().message);
        return result;
    }
    result.renderModel = renderModel.model;
    // Keep corpus-scale reclamation on the preparation worker rather than the
    // UI activation hand-off; the immutable payload/model retain the live data.
    result.snapshotResult = {};
    result.preparedResult = {};
    result.prepared = true;
    result.failureCategory = PerformancePackageFailureCategory::none;
    result.state = "Performance package v2 activation prepared";
    return result;
}
} // namespace drs::engine
