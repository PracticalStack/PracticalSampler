#include "shared/AuthoringPanel.h"

#ifndef DRS_ENABLE_INSTRUMENT_CONTROLS_UI
#define DRS_ENABLE_INSTRUMENT_CONTROLS_UI 0
#endif

#include "shared/MessageThreadMetrics.h"
#include "shared/authoring/AuthoringWorkspaceLayout.h"
#include "shared/authoring/OpenWorkbenchVisualSystem.h"
#include "drs/engine/ControlLaw.h"
#include "drs/engine/CuratedDspCatalog.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <charconv>
#include <cmath>
#include <initializer_list>
#include <limits>
#include <unordered_map>
#include <unordered_set>
#include <tuple>
#include <utility>

namespace drs::app
{
namespace
{
constexpr int statusTimerId = 1;
constexpr int previewReleaseTimerId = 2;
constexpr int keySwitchMidiLearnTimerId = 3;
const auto authoringPanelBackground = authoring::visual::shell;
const auto authoringPanelCard = authoring::visual::surface;
const auto authoringPanelAccent = authoring::visual::selection;
const auto authoringPanelMuted = authoring::visual::textMuted;
const auto authoringControlSurface = authoring::visual::surfaceRaised;
const auto authoringControlSurfaceHover = authoring::visual::surfaceHover;
const auto authoringControlOutline = authoring::visual::border;
const auto authoringFocusRing = authoring::visual::focus;
const auto authoringButtonFill = authoring::visual::surfaceRaised;
const auto authoringButtonFillPressed = authoring::visual::selectionHover;
const auto authoringToggleTick = authoring::visual::modulation;

juce::String formatMidiNoteName(const int midiNote)
{
    static constexpr std::array<const char*, 12> noteNames {
        "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
    };
    const auto clamped = std::clamp(midiNote, 0, 127);
    return juce::String(noteNames[static_cast<std::size_t>(clamped % 12)])
        + juce::String((clamped / 12) - 1) + " · MIDI " + juce::String(clamped);
}

struct CuratedMacroAssignment
{
    const char* parameterId;
    const char* parameterPath;
    const char* defaultRole;
    const char* label;
};

constexpr std::array<CuratedMacroAssignment, 4> curatedMacroAssignments
{
    CuratedMacroAssignment{"filter-cutoff", "engine.filter.main.cutoff", "timbre", "Filter cutoff"},
    CuratedMacroAssignment{"voice-pitch", "engine.pitch.main.semitones", "motion", "Voice pitch"},
    CuratedMacroAssignment{"zone-gain", "authoring.zone.gainDb", "mix", "Zone gain"},
    CuratedMacroAssignment{"zone-pan", "authoring.zone.pan", "placement", "Zone pan"}
};

constexpr std::array<const char*, 5> curatedMacroRoles
{
    "timbre",
    "motion",
    "mix",
    "space",
    "placement"
};

constexpr std::array<const char*, 6> curatedFxTypes
{
    "drs.gain",
    "drs.saturator",
    "drs.stereoDelay",
    "drs.algorithmicReverb",
    "drs.compactEq",
    "drs.chorus"
};

constexpr std::array<const char*, 3> curatedTriggerEvents
{
    "phrase-trigger",
    "key-switch",
    "phrase-latch"
};

constexpr std::array<const char*, 3> curatedChordModes
{
    "off",
    "follow-root",
    "preserve-intervals"
};

constexpr int unassignedMacroAssignmentId = 1;
constexpr int curatedMacroAssignmentBase = 2;
constexpr int curatedDspMacroAssignmentBase = 1000;

juce::String findZoneDisplayName(const drs::engine::RuntimeProjectModel& project,
                                 const std::string& zoneId);
juce::String findGroupDisplayName(const drs::engine::RuntimeProjectModel& project,
                                  const std::string& groupId);
juce::String findRoutingBusDisplayName(const drs::engine::RuntimeProjectModel& project,
                                       const std::string& routingBusId);
juce::String formatRoutingInputSourceLabel(const drs::engine::RuntimeProjectModel& project,
                                           const std::string& inputSourceId);

struct CuratedDspMacroAssignment
{
    const drs::engine::RuntimeProjectFxSlotDefinition* slot = nullptr;
    const drs::engine::CuratedDspParameterDescriptor* parameter = nullptr;
};

const drs::engine::RuntimeProjectRoutingBusDefinition* findRoutingBusById(
    const drs::engine::RuntimeProjectModel& project,
    const std::string& routingBusId)
{
    const auto iterator = std::find_if(project.authoring.routingBuses.begin(),
                                       project.authoring.routingBuses.end(),
                                       [&](const auto& routingBus)
                                       {
                                           return routingBus.id == routingBusId;
                                       });
    return iterator == project.authoring.routingBuses.end() ? nullptr : &*iterator;
}

const drs::engine::RuntimeProjectRoutingBusDefinition* findOwnerBusForFxSlot(
    const drs::engine::RuntimeProjectModel& project,
    const std::string& fxSlotId)
{
    const auto iterator = std::find_if(project.authoring.routingBuses.begin(),
                                       project.authoring.routingBuses.end(),
                                       [&](const auto& routingBus)
                                       {
                                           return std::find(routingBus.fxSlotIds.begin(),
                                                            routingBus.fxSlotIds.end(),
                                                            fxSlotId) != routingBus.fxSlotIds.end();
                                       });
    return iterator == project.authoring.routingBuses.end() ? nullptr : &*iterator;
}

const drs::engine::RuntimeProjectFxSlotDefinition::ParameterValue* findAuthoredFxParameterValue(
    const drs::engine::RuntimeProjectFxSlotDefinition& slot,
    std::string_view parameterId)
{
    const auto iterator = std::find_if(slot.parameters.begin(),
                                       slot.parameters.end(),
                                       [&](const auto& parameter)
                                       {
                                           return parameter.id == parameterId;
                                       });
    return iterator == slot.parameters.end() ? nullptr : &*iterator;
}

std::optional<std::size_t> findMacroIndexForDspTarget(
    const drs::engine::RuntimeProjectModel& project,
    std::string_view dspSlotId,
    std::string_view dspParameterId)
{
    for (std::size_t index = 0; index < project.authoring.macros.size(); ++index)
    {
        const auto& macro = project.authoring.macros[index];
        if (macro.targets.empty())
            continue;

        const auto& target = macro.targets.front();
        if (target.dspSlotId == dspSlotId && target.dspParameterId == dspParameterId)
            return index;
    }

    return std::nullopt;
}

juce::String formatDspParameterName(std::string_view parameterId)
{
    std::string text;
    text.reserve(parameterId.size() * 2);

    auto previous = '\0';
    for (const auto character : parameterId)
    {
        if (character == '-' || character == '_' || character == '.' || character == '/')
        {
            if (!text.empty() && text.back() != ' ')
                text.push_back(' ');
            previous = ' ';
            continue;
        }

        const auto current = static_cast<unsigned char>(character);
        const auto prior = static_cast<unsigned char>(previous);
        const auto needsSeparator = !text.empty() && previous != ' '
            && ((std::islower(prior) != 0 && std::isupper(current) != 0)
                || (std::isalpha(prior) != 0 && std::isdigit(current) != 0)
                || (std::isdigit(prior) != 0 && std::isalpha(current) != 0));
        if (needsSeparator)
            text.push_back(' ');

        text.push_back(character);
        previous = character;
    }

    auto formatted = juce::String::fromUTF8(text.c_str()).trim();
    formatted = formatted.replace(" Db", " dB");
    if (formatted.isNotEmpty())
        formatted = formatted.substring(0, 1).toUpperCase() + formatted.substring(1);
    return formatted;
}

int scoreCuratedDspMacroAssignment(const drs::engine::RuntimeProjectModel& project,
                                   const CuratedDspMacroAssignment& assignment,
                                   const std::string& preferredBusId,
                                   const std::string& preferredInputSourceId)
{
    auto score = 0;
    if (const auto* ownerBus = findOwnerBusForFxSlot(project, assignment.slot->id))
    {
        if (!preferredBusId.empty() && ownerBus->id == preferredBusId)
            score += 1000;
        else if (!preferredInputSourceId.empty() && ownerBus->inputSourceId == preferredInputSourceId)
            score += 500;
    }

    if (assignment.slot->effectType == "drs.gain" && assignment.parameter->id == "gainDb")
        score += 50;

    return score;
}

std::vector<CuratedDspMacroAssignment> buildCuratedDspMacroAssignments(
    const drs::engine::RuntimeProjectModel& project,
    const std::string& preferredBusId = {},
    const std::string& preferredInputSourceId = {})
{
    std::vector<CuratedDspMacroAssignment> assignments;
    for (const auto& slot : project.authoring.fxSlots)
    {
        const auto* effect = drs::engine::findCuratedDspEffect(slot.effectType, slot.effectVersion);
        if (effect == nullptr || slot.unavailable || slot.legacyInert)
            continue;
        for (const auto& parameter : effect->parameters)
            assignments.push_back({ &slot, &parameter });
    }

    std::stable_sort(assignments.begin(),
                     assignments.end(),
                     [&](const auto& left, const auto& right)
                     {
                         const auto leftScore = scoreCuratedDspMacroAssignment(project,
                                                                              left,
                                                                              preferredBusId,
                                                                              preferredInputSourceId);
                         const auto rightScore = scoreCuratedDspMacroAssignment(project,
                                                                                right,
                                                                                preferredBusId,
                                                                                preferredInputSourceId);
                         if (leftScore != rightScore)
                             return leftScore > rightScore;

                         return std::tie(left.slot->displayName, left.parameter->id, left.slot->id)
                             < std::tie(right.slot->displayName, right.parameter->id, right.slot->id);
                     });

    return assignments;
}

juce::String formatCuratedDspMacroAssignment(const drs::engine::RuntimeProjectModel& project,
                                             const CuratedDspMacroAssignment& assignment,
                                             const std::string& preferredBusId)
{
    auto scopeLabel = juce::String("FX");
    if (const auto* ownerBus = findOwnerBusForFxSlot(project, assignment.slot->id))
    {
        scopeLabel = ownerBus->id == preferredBusId
            ? "Current Scope"
            : formatRoutingInputSourceLabel(project, ownerBus->inputSourceId);
    }

    return scopeLabel + " | "
        + juce::String::fromUTF8(assignment.slot->displayName.c_str())
        + " / " + formatDspParameterName(assignment.parameter->id);
}

juce::String buildMacroControllerSummaryForBus(const drs::engine::RuntimeProjectModel& project,
                                               const std::string& routingBusId)
{
    if (routingBusId.empty())
        return "macros none";

    juce::String summary = "macros ";
    auto appendedAny = false;
    for (const auto& macro : project.authoring.macros)
    {
        if (macro.targets.empty() || macro.targets.front().dspSlotId.empty())
            continue;

        const auto* ownerBus = findOwnerBusForFxSlot(project, macro.targets.front().dspSlotId);
        if (ownerBus == nullptr || ownerBus->id != routingBusId)
            continue;

        if (appendedAny)
            summary << ", ";
        summary << juce::String::fromUTF8(macro.name.c_str());
        appendedAny = true;
    }

    return appendedAny ? summary : "macros none";
}

void configureEditorSlider(juce::Slider& slider,
                           double minValue,
                           double maxValue,
                           double interval)
{
    slider.setSliderStyle(juce::Slider::LinearHorizontal);
    slider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 56, 24);
    slider.setRange(minValue, maxValue, interval);
}

void configureSectionLabel(juce::Label& label, const char* text)
{
    label.setText(text, juce::dontSendNotification);
    label.setColour(juce::Label::textColourId, authoring::visual::text);
    label.setFont(juce::FontOptions(authoring::visual::sectionTypeSize, juce::Font::bold));
}

void configureFieldLabel(juce::Label& label, const char* text)
{
    label.setText(text, juce::dontSendNotification);
    label.setColour(juce::Label::textColourId, authoring::visual::text);
    label.setFont(juce::FontOptions(authoring::visual::fieldTypeSize, juce::Font::bold));
}

void configureMetadataLabel(juce::Label& label)
{
    label.setColour(juce::Label::textColourId, authoringPanelMuted);
    label.setFont(juce::FontOptions(authoring::visual::bodyTypeSize));
    label.setJustificationType(juce::Justification::centredLeft);
}

void drawAuthoringFocusRing(juce::Graphics& g,
                            juce::Rectangle<float> bounds,
                            float cornerSize,
                            const juce::Colour& outlineColour)
{
    juce::ignoreUnused(outlineColour);
    authoring::visual::drawFocusRing(g, bounds, cornerSize);
}

juce::String formatZoneRange(const drs::engine::AuthoringZoneSummary& zone)
{
    auto text = "Keys " + juce::String(zone.keyLow) + "-" + juce::String(zone.keyHigh)
        + " | Vel " + juce::String(zone.velocityLow) + "-" + juce::String(zone.velocityHigh);

    if (drs::engine::hasAnyVelocityCrossfadeValue(zone.velocityCrossfade))
    {
        text += " | Xfade";

        if (drs::engine::hasCompleteFadeIn(zone.velocityCrossfade))
        {
            text += " in " + juce::String(zone.velocityCrossfade.fadeInLowVelocity)
                + "-" + juce::String(zone.velocityCrossfade.fadeInHighVelocity);
        }

        if (drs::engine::hasCompleteFadeOut(zone.velocityCrossfade))
        {
            text += " out " + juce::String(zone.velocityCrossfade.fadeOutLowVelocity)
                + "-" + juce::String(zone.velocityCrossfade.fadeOutHighVelocity);
        }
    }

    return text;
}

juce::String buildIssueSummary(const std::vector<std::string>& issues)
{
    if (issues.empty())
        return "The requested action could not be completed.";

    juce::StringArray lines;
    for (const auto& issue : issues)
        lines.add(juce::String::fromUTF8(issue.c_str()));
    return lines.joinIntoString("\n");
}

juce::String formatMicros(std::uint64_t micros);

juce::String formatCuratedDspUnit(const drs::engine::CuratedDspParameterUnit unit)
{
    switch (unit)
    {
        case drs::engine::CuratedDspParameterUnit::decibels: return "dB";
        case drs::engine::CuratedDspParameterUnit::normalized: return "normalized";
        case drs::engine::CuratedDspParameterUnit::boolean: return "on/off";
        case drs::engine::CuratedDspParameterUnit::milliseconds: return "ms";
        case drs::engine::CuratedDspParameterUnit::seconds: return "s";
        case drs::engine::CuratedDspParameterUnit::hertz: return "Hz";
        case drs::engine::CuratedDspParameterUnit::ratio: return "ratio";
        case drs::engine::CuratedDspParameterUnit::semitones: return "st";
    }
    return {};
}

juce::String formatAuthoringPreviewStatus(const drs::app::AuthoringPreviewStatusSnapshot& status)
{
    if (!status.available)
        return "Preview status unavailable";

    auto text = "Preview " + juce::String::fromUTF8(status.stateLabel.empty() ? "Unknown"
                                                                              : status.stateLabel.c_str())
        + " | draft r" + juce::String(static_cast<int>(status.draftRevision));

    if (status.activeRevision > 0)
        text += " | active r" + juce::String(static_cast<int>(status.activeRevision));

    if (status.usingLastKnownGood)
        text += " | auditioning last good r" + juce::String(static_cast<int>(status.audibleRevision));

    if (status.failedRevision > 0 && status.failedRevision != status.activeRevision)
        text += " | failed r" + juce::String(static_cast<int>(status.failedRevision));

    if (status.pendingRevision > 0 && status.pendingRevision != status.activeRevision)
        text += " | pending r" + juce::String(static_cast<int>(status.pendingRevision));

    if (!status.blockingPrerequisite.empty())
        text += " | Fix: " + juce::String::fromUTF8(status.blockingPrerequisite.c_str());

    if (!status.failureState.empty())
        text += " | " + juce::String::fromUTF8(status.failureState.c_str());

    if (status.lastRequestToAudibleMicros > 0)
        text += " | audible " + formatMicros(status.lastRequestToAudibleMicros);

    return text;
}

juce::String formatMicros(std::uint64_t micros)
{
    if (micros >= 1000)
        return juce::String(static_cast<double>(micros) / 1000.0, 2) + " ms";

    return juce::String(static_cast<int>(micros)) + " us";
}

juce::String formatImportResponsivenessState(const std::string& state)
{
    if (state.empty())
        return "unknown";

    juce::String formatted = juce::String::fromUTF8(state.c_str()).replaceCharacter('-', ' ');
    if (formatted.isEmpty())
        return "unknown";

    const auto words = juce::StringArray::fromTokens(formatted, " ", {});
    juce::String result;
    for (int index = 0; index < words.size(); ++index)
    {
        auto word = words[index].trim();
        if (word.isEmpty())
            continue;

        word = word.substring(0, 1).toUpperCase() + word.substring(1).toLowerCase();
        if (!result.isEmpty())
            result << ' ';
        result << word;
    }

    return result.isEmpty() ? "unknown" : result;
}

bool isSourceValidationActive(const drs::app::AuthoringSourceValidationSnapshot& snapshot) noexcept
{
    return snapshot.available && snapshot.state == "active";
}

juce::String formatSourceValidationStatus(const drs::app::AuthoringSourceValidationSnapshot& snapshot)
{
    if (!snapshot.available)
        return "Source validation unavailable";

    juce::String text = "Validation " + formatImportResponsivenessState(snapshot.state);

    if (snapshot.totalItemCount == 0)
        return text + " | no linked project sources";

    text += " " + juce::String(static_cast<int>(snapshot.processedCount))
        + "/" + juce::String(static_cast<int>(snapshot.totalItemCount));
    text += " | warn " + juce::String(static_cast<int>(snapshot.warningItemCount));
    text += " | fail " + juce::String(static_cast<int>(snapshot.failedItemCount));
    text += " | canceled " + juce::String(static_cast<int>(snapshot.canceledItemCount));

    if (snapshot.totalBytesExpected > 0)
    {
        text += " | "
            + juce::File::descriptionOfSizeInBytes(static_cast<juce::int64>(snapshot.totalBytesProcessed))
            + "/"
            + juce::File::descriptionOfSizeInBytes(static_cast<juce::int64>(snapshot.totalBytesExpected));
    }

    if (snapshot.totalDurationMicros > 0)
        text += " | " + formatMicros(snapshot.totalDurationMicros);

    if (!snapshot.currentSourcePath.empty())
        text += " | current " + juce::File(juce::String::fromUTF8(snapshot.currentSourcePath.c_str())).getFileName();

    return text;
}

juce::String joinIdList(const std::vector<std::string>& values)
{
    if (values.empty())
        return "(none)";

    juce::String result;
    for (std::size_t index = 0; index < values.size(); ++index)
    {
        if (index > 0)
            result << " -> ";
        result << juce::String::fromUTF8(values[index].c_str());
    }

    return result;
}

std::vector<std::string> buildArticulationIds(const drs::engine::RuntimeProjectModel& project)
{
    std::vector<std::string> articulationIds;

    if (project.schemaVersion >= 6 && project.authoring.schemaVersion >= 5)
    {
        articulationIds.reserve(project.authoring.articulations.size());
        for (const auto& articulation : project.authoring.articulations)
            articulationIds.push_back(articulation.id);
        return articulationIds;
    }

    // Compatibility display only. Loaded projects are migrated before editing;
    // Sprint 1 no longer derives the authoritative articulation model from zones.
    for (const auto& zone : project.authoring.zones)
    {
        if (std::find(articulationIds.begin(), articulationIds.end(), zone.articulationId) == articulationIds.end())
            articulationIds.push_back(zone.articulationId);
    }

    return articulationIds;
}

const CuratedMacroAssignment* findCuratedMacroAssignment(const std::string& parameterId)
{
    for (const auto& assignment : curatedMacroAssignments)
    {
        if (parameterId == assignment.parameterId)
            return &assignment;
    }

    return nullptr;
}

int findAssignmentIndex(const std::string& parameterId)
{
    for (std::size_t index = 0; index < curatedMacroAssignments.size(); ++index)
    {
        if (parameterId == curatedMacroAssignments[index].parameterId)
            return static_cast<int>(index);
    }

    return -1;
}

bool sameVelocityCrossfadeDescriptor(const drs::engine::VelocityCrossfadeDescriptor& left,
                                     const drs::engine::VelocityCrossfadeDescriptor& right) noexcept
{
    return left.fadeInLowVelocity == right.fadeInLowVelocity
        && left.fadeInHighVelocity == right.fadeInHighVelocity
        && left.fadeOutLowVelocity == right.fadeOutLowVelocity
        && left.fadeOutHighVelocity == right.fadeOutHighVelocity
        && left.curve == right.curve;
}

bool isRoundRobinGroupingCompatible(const drs::engine::RuntimeProjectZoneDefinition& anchor,
                                    const drs::engine::RuntimeProjectZoneDefinition& candidate) noexcept
{
    return anchor.groupId == candidate.groupId
        && anchor.articulationId == candidate.articulationId
        && anchor.rootKey == candidate.rootKey
        && anchor.keyLow == candidate.keyLow
        && anchor.keyHigh == candidate.keyHigh
        && anchor.velocityLow == candidate.velocityLow
        && anchor.velocityHigh == candidate.velocityHigh
        && sameVelocityCrossfadeDescriptor(anchor.velocityCrossfade, candidate.velocityCrossfade)
        && anchor.triggerMode == candidate.triggerMode;
}

std::size_t countRoundRobinPoolMembers(const drs::engine::RuntimeProjectModel& project,
                                       const drs::engine::RoundRobinDescriptor& roundRobin)
{
    return static_cast<std::size_t>(std::count_if(project.authoring.zones.begin(),
                                                  project.authoring.zones.end(),
                                                  [&](const auto& zone)
                                                  {
                                                      return zone.roundRobin.has_value()
                                                          && zone.roundRobin->poolId == roundRobin.poolId;
                                                  }));
}

std::size_t countCompatibleUnpooledRoundRobinZones(const drs::engine::RuntimeProjectModel& project,
                                                   const drs::engine::RuntimeProjectZoneDefinition& anchor)
{
    return static_cast<std::size_t>(std::count_if(project.authoring.zones.begin(),
                                                  project.authoring.zones.end(),
                                                  [&](const auto& zone)
                                                  {
                                                      return zone.id != anchor.id
                                                          && !zone.roundRobin.has_value()
                                                          && isRoundRobinGroupingCompatible(anchor, zone);
                                                  }));
}

bool isExpandedLayout(AuthoringPanel::LayoutMode layoutMode)
{
    return layoutMode == AuthoringPanel::LayoutMode::expanded;
}

void configureAccessibleMetadata(juce::Component& component,
                                 const juce::String& title,
                                 const juce::String& description,
                                 const juce::String& helpText = {})
{
    component.setTitle(title);
    component.setDescription(description);

    if (helpText.isNotEmpty())
        component.setHelpText(helpText);
}

void updateDynamicAccessibleText(juce::Component& component,
                                 const juce::String& text,
                                 const juce::String& descriptionPrefix)
{
    component.setTitle(text);
    component.setDescription(descriptionPrefix + text);
}

void updateAccessibleDescriptionAndHelpText(juce::Component& component,
                                            const juce::String& description,
                                            const juce::String& helpText)
{
    component.setDescription(description);
    component.setHelpText(helpText);
}

void setVisibleAndAccessible(juce::Component& component, bool shouldShow)
{
    component.setVisible(shouldShow);
    component.setAccessible(shouldShow);
}

bool isComponentFocusedWithin(const juce::Component* focusedComponent, const juce::Component& ancestor)
{
    for (auto* current = focusedComponent; current != nullptr; current = current->getParentComponent())
    {
        if (current == &ancestor)
            return true;
    }

    return false;
}

juce::String buildMacroListStatusText(const drs::engine::RuntimeProjectMacroDefinition& macro)
{
    juce::String status = macro.exposedInPerformance ? "Perform | " : "Hidden | ";

    if (macro.targets.empty())
        return status + "Unassigned";

    const auto& target = macro.targets.front();
    if (!target.role.empty())
        status << juce::String::fromUTF8(target.role.c_str()) << " | ";

    if (const auto* assignment = findCuratedMacroAssignment(target.parameterId))
        status << assignment->label;
    else if (!target.parameterPath.empty())
        status << juce::String::fromUTF8(target.parameterPath.c_str());
    else if (!target.parameterId.empty())
        status << juce::String::fromUTF8(target.parameterId.c_str());
    else
        status << "Unassigned";

    return status;
}

juce::String macroTargetFamily(const drs::engine::RuntimeProjectMacroTargetDefinition& target)
{
    if (!target.dspSlotId.empty() && !target.dspParameterId.empty())
        return "DSP";
    if (target.parameterPath.rfind("authoring.", 0) == 0)
        return "Authoring";
    if (target.parameterPath.rfind("engine.", 0) == 0)
        return "Engine";
    return "Custom";
}

juce::String macroTargetName(const drs::engine::RuntimeProjectMacroTargetDefinition& target)
{
    if (const auto* assignment = findCuratedMacroAssignment(target.parameterId))
        return assignment->label;
    if (!target.dspSlotId.empty() && !target.dspParameterId.empty())
        return juce::String::fromUTF8(target.dspSlotId.c_str()) + " / "
            + juce::String::fromUTF8(target.dspParameterId.c_str());
    if (!target.parameterPath.empty())
        return juce::String::fromUTF8(target.parameterPath.c_str());
    if (!target.parameterId.empty())
        return juce::String::fromUTF8(target.parameterId.c_str());
    return "Unassigned target";
}

juce::String macroTargetMappingSummary(
    const drs::engine::RuntimeProjectMacroTargetDefinition& target)
{
    auto summary = juce::String::fromUTF8(target.role.empty() ? "no role" : target.role.c_str());
    summary << " | " << juce::String(target.sourceMinimum, 2)
            << "-" << juce::String(target.sourceMaximum, 2);
    if (!target.dspSlotId.empty())
        summary << " -> " << juce::String(target.destinationMinimum, 2)
                << "-" << juce::String(target.destinationMaximum, 2);
    return summary;
}

juce::String macroTargetDetail(const drs::engine::RuntimeProjectMacroTargetDefinition& target,
                               const int targetIndex,
                               const int targetCount)
{
    auto detail = "Target " + juce::String(targetIndex + 1) + " of "
        + juce::String(targetCount) + " | " + macroTargetFamily(target)
        + " | " + macroTargetName(target);
    if (!target.controlLaw.id.empty())
        detail << " | law " << juce::String::fromUTF8(target.controlLaw.id.c_str());
    else if (!target.curve.empty())
        detail << " | " << juce::String::fromUTF8(target.curve.c_str());
    return detail;
}

std::size_t countZonesInGroup(const drs::engine::RuntimeProjectModel& project,
                              const std::string& groupId)
{
    return static_cast<std::size_t>(std::count_if(project.authoring.zones.begin(),
                                                  project.authoring.zones.end(),
                                                  [&](const auto& zone)
                                                  {
                                                      return zone.groupId == groupId;
                                                  }));
}

juce::String findZoneDisplayName(const drs::engine::RuntimeProjectModel& project,
                                 const std::string& zoneId)
{
    const auto iterator = std::find_if(project.authoring.zones.begin(),
                                       project.authoring.zones.end(),
                                       [&](const auto& zone)
                                       {
                                           return zone.id == zoneId;
                                       });
    return iterator != project.authoring.zones.end()
        ? juce::String::fromUTF8(iterator->displayName.c_str())
        : juce::String::fromUTF8(zoneId.c_str());
}

juce::String findGroupDisplayName(const drs::engine::RuntimeProjectModel& project,
                                  const std::string& groupId)
{
    const auto iterator = std::find_if(project.authoring.groups.begin(),
                                       project.authoring.groups.end(),
                                       [&](const auto& group)
                                       {
                                           return group.id == groupId;
                                       });
    return iterator != project.authoring.groups.end()
        ? juce::String::fromUTF8(iterator->displayName.c_str())
        : juce::String::fromUTF8(groupId.c_str());
}

juce::String findRoutingBusDisplayName(const drs::engine::RuntimeProjectModel& project,
                                       const std::string& routingBusId)
{
    const auto iterator = std::find_if(project.authoring.routingBuses.begin(),
                                       project.authoring.routingBuses.end(),
                                       [&](const auto& routingBus)
                                       {
                                           return routingBus.id == routingBusId;
                                       });
    return iterator != project.authoring.routingBuses.end()
        ? juce::String::fromUTF8(iterator->displayName.c_str())
        : juce::String::fromUTF8(routingBusId.c_str());
}

juce::String formatRoutingInputSourceLabel(const drs::engine::RuntimeProjectModel& project,
                                           const std::string& inputSourceId)
{
    if (inputSourceId.empty())
        return "(none)";

    if (inputSourceId == "master")
        return "Master";

    constexpr auto groupPrefix = "groups/";
    if (inputSourceId.rfind(groupPrefix, 0) == 0)
        return "Group: " + findGroupDisplayName(project, inputSourceId.substr(std::char_traits<char>::length(groupPrefix)));

    constexpr auto layerPrefix = "layers/";
    if (inputSourceId.rfind(layerPrefix, 0) == 0)
    {
        const auto layerId = inputSourceId.substr(std::char_traits<char>::length(layerPrefix));
        const auto iterator = std::find_if(project.authoring.layers.begin(), project.authoring.layers.end(),
                                           [&](const auto& layer) { return layer.id == layerId; });
        return "Layer: " + (iterator == project.authoring.layers.end()
            ? juce::String::fromUTF8(layerId.c_str())
            : juce::String::fromUTF8(iterator->displayName.c_str()));
    }

    return "Zone: " + findZoneDisplayName(project, inputSourceId);
}

juce::String buildGroupListStatusText(const drs::engine::RuntimeProjectModel& project,
                                      const drs::engine::RuntimeProjectGroupDefinition& group)
{
    juce::String status = group.workspaceVisible ? "Shown" : "Hidden";
    status << " | " << static_cast<int>(countZonesInGroup(project, group.id)) << " zones";
    if (!group.layerId.empty())
        status << " | layer " << juce::String::fromUTF8(group.layerId.c_str());

    if (!group.routingBusId.empty())
        status << " | " << findRoutingBusDisplayName(project, group.routingBusId);

    return status;
}

struct DraftPlaybackGuidance
{
    std::string statusText;
    bool canPrepareDraftPlayback = false;
    bool canPublishDraftPlayback = false;
};

bool hasFindingCode(const std::vector<drs::engine::PlaybackSnapshotFinding>& findings,
                    const std::string& code)
{
    return std::any_of(findings.begin(),
                       findings.end(),
                       [&](const drs::engine::PlaybackSnapshotFinding& finding)
                       {
                           return finding.code == code;
                       });
}

const drs::engine::PlaybackSnapshotFinding* findBlockingPlaybackFinding(
    const std::vector<drs::engine::PlaybackSnapshotFinding>& findings)
{
    const auto error = std::find_if(findings.begin(), findings.end(), [](const auto& finding)
    {
        return finding.severity == drs::engine::PlaybackSnapshotFindingSeverity::error;
    });
    if (error != findings.end())
        return &*error;
    return findings.empty() ? nullptr : &findings.front();
}

DraftPlaybackGuidance buildDraftPlaybackGuidance(const drs::engine::AuthoringSession& authoringSession,
                                                 const drs::engine::DraftPlaybackStatus& playbackStatus)
{
    DraftPlaybackGuidance guidance;
    const auto hasZones = !authoringSession.getProject().authoring.zones.empty();
    const auto previewReadyForCurrentDraft = playbackStatus.preview.revision == playbackStatus.draftRevision
        && playbackStatus.preview.state == "Ready";

    guidance.canPrepareDraftPlayback = playbackStatus.projectOpen
        && !playbackStatus.deviceRestartInProgress
        && !playbackStatus.pendingPreview.active
        && hasZones;
    guidance.canPublishDraftPlayback = playbackStatus.projectOpen
        && !playbackStatus.deviceRestartInProgress
        && !playbackStatus.pendingPerformance.active
        && previewReadyForCurrentDraft
        && (playbackStatus.performance.revision != playbackStatus.draftRevision
            || playbackStatus.performance.state != "Active");

    if (!playbackStatus.projectOpen)
    {
        guidance.statusText = "playback blocked: Open a project before preparing draft playback.";
        return guidance;
    }

    if (!hasZones
        || hasFindingCode(playbackStatus.preview.findings, "no-playable-zones")
        || hasFindingCode(playbackStatus.performance.findings, "no-playable-zones"))
    {
        guidance.statusText = "playback blocked: Import a sample and create at least one playable zone.";
        return guidance;
    }

    if (playbackStatus.deviceRestartInProgress)
    {
        guidance.statusText = "playback paused: Wait for the device restart to finish before preparing or publishing.";
        return guidance;
    }

    if (playbackStatus.pendingPreview.active || playbackStatus.pendingPerformance.active)
    {
        const auto& pending = playbackStatus.pendingPerformance.active
            ? playbackStatus.pendingPerformance : playbackStatus.pendingPreview;
        if (!pending.progressPhase.empty() && pending.progressTotal != 0)
        {
            guidance.statusText = "playback busy: " + pending.progressPhase + " "
                + std::to_string(pending.progressOrdinal) + "/"
                + std::to_string(pending.progressTotal) + ".";
        }
        else if (!pending.progressPhase.empty())
        {
            guidance.statusText = "playback busy: " + pending.progressPhase + ".";
        }
        else
        {
            guidance.statusText = "playback busy: Wait for the current playback build to finish applying.";
        }
        return guidance;
    }

    const auto previewFailed = playbackStatus.preview.state == "Failed"
        || playbackStatus.preview.lifecycleState == drs::engine::PlaybackSnapshotLifecycleState::failed;
    if (previewFailed)
    {
        if (const auto* finding = findBlockingPlaybackFinding(playbackStatus.preview.findings))
        {
            guidance.statusText = "playback blocked: " + finding->code + ": " + finding->message;
            return guidance;
        }
    }

    const auto performanceFailed = playbackStatus.performance.state == "Failed"
        || playbackStatus.performance.lifecycleState == drs::engine::PlaybackSnapshotLifecycleState::failed;
    if (performanceFailed)
    {
        if (const auto* finding = findBlockingPlaybackFinding(playbackStatus.performance.findings))
        {
            guidance.statusText = "playback blocked: " + finding->code + ": " + finding->message;
            return guidance;
        }
    }

    if (playbackStatus.preview.revision != playbackStatus.draftRevision
        || playbackStatus.preview.state == "Stale")
    {
        guidance.statusText = "playback action: Prepare the latest draft for preview.";
        return guidance;
    }

    if (previewReadyForCurrentDraft
        && (playbackStatus.performance.revision != playbackStatus.draftRevision
            || playbackStatus.performance.state != "Active"))
    {
        guidance.statusText = "playback action: Publish the ready draft to the performance path.";
        return guidance;
    }

    if (playbackStatus.performance.revision == playbackStatus.draftRevision
        && playbackStatus.performance.state == "Active")
    {
        guidance.statusText = "playback ready: The latest draft is active on the performance path.";
    }

    return guidance;
}

} // namespace

void AuthoringPanel::requestWaveformPreviewLoad(const bool refreshImmediately)
{
    if (waveformPreviewRequestCallback)
        waveformPreviewRequestCallback();
    if (refreshImmediately)
        refreshWaveformWorkbenchContent();
}

AuthoringPanel::AuthoringControlLookAndFeel::AuthoringControlLookAndFeel()
{
    setColour(juce::TextButton::buttonColourId, authoringButtonFill);
    setColour(juce::TextButton::buttonOnColourId, authoringPanelAccent);
    setColour(juce::TextButton::textColourOffId, authoring::visual::text);
    setColour(juce::TextButton::textColourOnId, authoring::visual::textOnAccent);

    setColour(juce::ToggleButton::textColourId, authoring::visual::text);
    setColour(juce::ToggleButton::tickColourId, authoringToggleTick);
    setColour(juce::ToggleButton::tickDisabledColourId, authoringControlOutline);

    setColour(juce::ComboBox::backgroundColourId, authoringControlSurface);
    setColour(juce::ComboBox::textColourId, authoring::visual::text);
    setColour(juce::ComboBox::arrowColourId, authoring::visual::text.withAlpha(0.82f));
    setColour(juce::ComboBox::outlineColourId, authoringControlOutline);
    setColour(juce::ComboBox::focusedOutlineColourId, authoringFocusRing);

    setColour(juce::TextEditor::backgroundColourId, authoringControlSurface);
    setColour(juce::TextEditor::textColourId, authoring::visual::text);
    setColour(juce::TextEditor::outlineColourId, authoringControlOutline);
    setColour(juce::TextEditor::focusedOutlineColourId, authoringFocusRing);
    setColour(juce::TextEditor::highlightColourId, authoringToggleTick.withAlpha(0.18f));
    setColour(juce::TextEditor::highlightedTextColourId, authoring::visual::text);

    setColour(juce::Label::textColourId, authoring::visual::text);
    setColour(juce::Slider::thumbColourId, authoringPanelAccent);
    setColour(juce::Slider::trackColourId, authoringToggleTick.withAlpha(0.76f));
    setColour(juce::Slider::backgroundColourId, authoringControlSurfaceHover);
    setColour(juce::Slider::textBoxTextColourId, authoring::visual::text);
    setColour(juce::Slider::textBoxBackgroundColourId, authoringControlSurface);
    setColour(juce::Slider::textBoxOutlineColourId, authoringControlOutline);

    setColour(juce::ListBox::backgroundColourId, authoringControlSurfaceHover);
    setColour(juce::ListBox::outlineColourId, authoringControlOutline);
    setColour(juce::ScrollBar::backgroundColourId, authoring::visual::surfaceSubtle);
    setColour(juce::ScrollBar::thumbColourId, authoring::visual::borderStrong);
    setColour(juce::PopupMenu::backgroundColourId, authoring::visual::surfaceRaised);
    setColour(juce::PopupMenu::textColourId, authoring::visual::text);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, authoring::visual::selection);
    setColour(juce::PopupMenu::highlightedTextColourId, authoring::visual::textOnAccent);
    setColour(juce::TooltipWindow::backgroundColourId, authoring::visual::surfaceRaised);
    setColour(juce::TooltipWindow::textColourId, authoring::visual::text);
    setColour(juce::TooltipWindow::outlineColourId, authoring::visual::borderStrong);
}

void AuthoringPanel::AuthoringControlLookAndFeel::drawButtonBackground(juce::Graphics& g,
                                                                       juce::Button& button,
                                                                       const juce::Colour& backgroundColour,
                                                                       bool shouldDrawButtonAsHighlighted,
                                                                       bool shouldDrawButtonAsDown)
{
    auto bounds = button.getLocalBounds().toFloat().reduced(0.5f);
    const auto cornerSize = authoring::visual::controlRadius;
    const auto hasFocus = button.hasKeyboardFocus(true);

    if (hasFocus)
    {
        drawAuthoringFocusRing(g, bounds.reduced(1.0f), cornerSize, findColour(juce::TextEditor::focusedOutlineColourId));
        bounds = bounds.reduced(3.0f);
    }

    auto fillColour = button.isEnabled() ? backgroundColour : authoring::visual::disabled(backgroundColour);
    if (shouldDrawButtonAsDown)
        fillColour = fillColour.interpolatedWith(authoringButtonFillPressed, 0.45f);
    else if (shouldDrawButtonAsHighlighted)
        fillColour = fillColour.interpolatedWith(authoringControlSurfaceHover, 0.72f);

    g.setColour(fillColour);
    g.fillRoundedRectangle(bounds, cornerSize);
    g.setColour(button.findColour(juce::ComboBox::outlineColourId));
    g.drawRoundedRectangle(bounds, cornerSize, hasFocus ? 1.2f : 1.0f);
}

void AuthoringPanel::AuthoringControlLookAndFeel::drawToggleButton(juce::Graphics& g,
                                                                   juce::ToggleButton& button,
                                                                   bool shouldDrawButtonAsHighlighted,
                                                                   bool shouldDrawButtonAsDown)
{
    if (button.hasKeyboardFocus(true))
        drawAuthoringFocusRing(g,
                               button.getLocalBounds().toFloat().reduced(1.0f),
                               authoring::visual::controlRadius,
                               findColour(juce::TextEditor::focusedOutlineColourId));

    juce::LookAndFeel_V4::drawToggleButton(g, button, shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);
}

void AuthoringPanel::AuthoringControlLookAndFeel::drawComboBox(juce::Graphics& g,
                                                               int width,
                                                               int height,
                                                               bool,
                                                               int,
                                                               int,
                                                               int,
                                                               int,
                                                               juce::ComboBox& box)
{
    const auto cornerSize = authoring::visual::controlRadius;
    auto bounds = juce::Rectangle<float>(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height)).reduced(0.5f);
    const auto hasFocus = box.hasKeyboardFocus(true);

    if (hasFocus)
    {
        drawAuthoringFocusRing(g, bounds.reduced(1.0f), cornerSize, box.findColour(juce::ComboBox::focusedOutlineColourId));
        bounds = bounds.reduced(3.0f);
    }

    g.setColour(box.findColour(juce::ComboBox::backgroundColourId));
    g.fillRoundedRectangle(bounds, cornerSize);
    g.setColour(box.findColour(hasFocus ? juce::ComboBox::focusedOutlineColourId
                                        : juce::ComboBox::outlineColourId));
    g.drawRoundedRectangle(bounds, cornerSize, hasFocus ? 1.6f : 1.0f);

    const auto arrowZone = juce::Rectangle<float>(bounds.getRight() - 24.0f, bounds.getY(), 16.0f, bounds.getHeight());
    juce::Path path;
    path.startNewSubPath(arrowZone.getX() + 1.5f, arrowZone.getCentreY() - 2.0f);
    path.lineTo(arrowZone.getCentreX(), arrowZone.getCentreY() + 2.5f);
    path.lineTo(arrowZone.getRight() - 1.5f, arrowZone.getCentreY() - 2.0f);
    g.setColour(box.findColour(juce::ComboBox::arrowColourId).withAlpha(box.isEnabled() ? 0.95f : 0.28f));
    g.strokePath(path, juce::PathStrokeType(2.0f));
}

void AuthoringPanel::AuthoringControlLookAndFeel::drawLinearSliderOutline(juce::Graphics& g,
                                                                          int x,
                                                                          int y,
                                                                          int width,
                                                                          int height,
                                                                          const juce::Slider::SliderStyle style,
                                                                          juce::Slider& slider)
{
    juce::ignoreUnused(x, y, width, height, style);

    if (slider.hasKeyboardFocus(true))
    {
        drawAuthoringFocusRing(g,
                               slider.getLocalBounds().toFloat().reduced(1.0f),
                               authoring::visual::controlRadius,
                               findColour(juce::TextEditor::focusedOutlineColourId));
    }
    else
    {
        juce::LookAndFeel_V4::drawLinearSliderOutline(g,
                                                      x,
                                                      y,
                                                      width,
                                                      height,
                                                      style,
                                                      slider);
    }
}

AuthoringPanel::AuthoringPanel(drs::engine::AuthoringSession& session,
                               WaveformPreviewProvider previewProvider,
                               AuthoringPreviewStatusProvider nextAuthoringPreviewStatusProvider,
                               ImportResponsivenessProvider responsivenessProvider,
                               LayoutMode nextLayoutMode,
                               RestoreRootKeyCallback restoreRootKeyRequested,
                               DraftPlaybackStatusProvider nextDraftPlaybackStatusProvider,
                               DraftPlaybackActionCallback prepareDraftPlaybackRequested,
                               DraftPlaybackActionCallback publishDraftPlaybackRequested,
                               PreviewCommandCallback nextPreviewCommandCallback,
                               SampleFilesDroppedCallback nextSampleFilesDroppedCallback,
                               WaveformPreviewRequestCallback nextWaveformPreviewRequestCallback,
                               SourceValidationStatusProvider nextSourceValidationStatusProvider,
                               DraftPlaybackActionCallback nextRequestSourceValidation,
                               DraftPlaybackActionCallback nextCancelSourceValidation,
                               WaveformDetailRequestCallback nextWaveformDetailRequestCallback)
    : authoringSession(session),
      waveformPreviewProvider(std::move(previewProvider)),
      waveformPreviewRequestCallback(std::move(nextWaveformPreviewRequestCallback)),
      waveformDetailRequestCallback(std::move(nextWaveformDetailRequestCallback)),
      authoringPreviewStatusProvider(std::move(nextAuthoringPreviewStatusProvider)),
      importResponsivenessProvider(std::move(responsivenessProvider)),
      sourceValidationStatusProvider(std::move(nextSourceValidationStatusProvider)),
      layoutMode(nextLayoutMode),
      onRestoreRootKeyRequested(std::move(restoreRootKeyRequested)),
      draftPlaybackStatusProvider(std::move(nextDraftPlaybackStatusProvider)),
      onPrepareDraftPlaybackRequested(std::move(prepareDraftPlaybackRequested)),
      onPublishDraftPlaybackRequested(std::move(publishDraftPlaybackRequested)),
      onRequestSourceValidation(std::move(nextRequestSourceValidation)),
      onCancelSourceValidation(std::move(nextCancelSourceValidation)),
      previewCommandCallback(std::move(nextPreviewCommandCallback)),
      sampleFilesDroppedCallback(std::move(nextSampleFilesDroppedCallback)),
      layerList("authoringLayerList",
                "authoringLayerListBox",
                "authoringLayerListEmptyState"),
      groupList("authoringGroupList",
                "authoringGroupListBox",
                "authoringGroupListEmptyState"),
      macroList("authoringMacroList",
                "authoringMacroListBox",
                "authoringMacroListEmptyState"),
      macroAssignmentList("authoringMacroAssignmentList",
                          "authoringMacroAssignmentListBox",
                          "authoringMacroAssignmentEmptyState"),
      articulationList("authoringArticulationList",
                       "authoringArticulationListBox",
                       "authoringArticulationListEmptyState")
{
    setLookAndFeel(&authoringLookAndFeel);
    setComponentID("authoringWorkspace");
    // The primary authoring surface opens on the hierarchy and Map. The
    // workbench remains collapsed by default and opens below the Map.
    workbenchState.open = false;
    workbenchState.activeTab = authoring::WorkbenchTab::waveform;
    workbenchLayoutState.setOpen(workbenchState.open);
    workbenchLayoutState.suggestHeightForTab(workbenchState.activeTab);

    configureMetadataLabel(waveformScopeLabel);
    configureMetadataLabel(workbenchBreadcrumbLabel);
    configureMetadataLabel(waveformStatusLabel);
    configureMetadataLabel(waveformInfoLabel);
    configureMetadataLabel(loopInfoLabel);
    configureMetadataLabel(importMetricsLabel);
    configureMetadataLabel(waveformLoopGuidanceLabel);
    configureMetadataLabel(sourceValidationLabel);
    macroSummaryLabel.setColour(juce::Label::textColourId, authoringPanelMuted);
    fxParameterValueLabel.setColour(juce::Label::textColourId, authoringPanelMuted);
    fxSummaryLabel.setColour(juce::Label::textColourId, authoringPanelMuted);
    fxDiagnosticsLabel.setColour(juce::Label::textColourId, authoringPanelMuted);
    routingSummaryLabel.setColour(juce::Label::textColourId, authoringPanelMuted);
    groupSummaryLabel.setColour(juce::Label::textColourId, authoringPanelMuted);
    groupVisibilityHintLabel.setColour(juce::Label::textColourId, authoringPanelMuted);
    groupRoundRobinLabel.setColour(juce::Label::textColourId, authoringPanelMuted);
    groupRoundRobinHintLabel.setColour(juce::Label::textColourId, authoringPanelMuted);
    performanceSummaryLabel.setColour(juce::Label::textColourId, authoringPanelMuted);
    phraseSummaryLabel.setColour(juce::Label::textColourId, authoringPanelMuted);
    articulationSwitchNoteValueLabel.setColour(juce::Label::textColourId, authoringPanelMuted);
    articulationStatusLabel.setColour(juce::Label::textColourId, authoringPanelMuted);

    configureSectionLabel(waveformLabel, "Waveform Detail");
    configureSectionLabel(zoneLabel, "Selected Zone");
    configureSectionLabel(groupSectionLabel, "Zone Groups");
    configureSectionLabel(layerSectionLabel, "Layers");
    configureSectionLabel(fxSectionLabel, "Selected Insert");
    configureSectionLabel(routingSectionLabel, "Bus & Signal Path");

    configureFieldLabel(groupNameLabel, "Group Name");
    configureFieldLabel(layerNameLabel, "Layer Name");
    configureFieldLabel(macroNameLabel, "Macro Name");
    configureFieldLabel(macroExposeLabel, "Perform");
    configureFieldLabel(macroAssignmentLabel, "Parameter");
    configureFieldLabel(macroRoleLabel, "Role");
    configureFieldLabel(macroDefaultLabel, "Default");
    configureFieldLabel(macroMinLabel, "Min");
    configureFieldLabel(macroMaxLabel, "Max");
    configureFieldLabel(fxTypeLabel, "Type");
    configureFieldLabel(fxScopeLabel, "Scope");
    configureFieldLabel(masterGainLabel, "Master Gain");
    configureFieldLabel(groupVisibilityLabel, "Visibility");
    configureFieldLabel(groupGainLabel, "Gain");
    configureFieldLabel(groupPanLabel, "Pan");
    configureFieldLabel(groupRoutingLabel, "Routing Bus");
    configureFieldLabel(groupAnchorLabel, "Audition Anchor");
    configureFieldLabel(routingInputLabel, "Input Source");
    configureFieldLabel(routingInsertOneLabel, "Insert A");
    configureFieldLabel(routingInsertTwoLabel, "Insert B");
    configureFieldLabel(triggerEventLabel, "Trigger");
    configureFieldLabel(targetArticulationLabel, "Articulation");
    configureFieldLabel(phraseAssetLabel, "Phrase");
    configureFieldLabel(chordModeLabel, "Chord Rule");
    configureFieldLabel(phraseImportPathLabel, "MIDI Path");
    configureFieldLabel(articulationNameLabel, "Name");
    configureFieldLabel(articulationSwitchNoteLabel, "Key Switch");
    configureFieldLabel(articulationDeleteReassignLabel, "Reassign zones to");
    configureFieldLabel(waveformPlaybackStartLabel, "PLAYBACK START");
    configureFieldLabel(waveformPlaybackEndLabel, "END");
    configureFieldLabel(waveformLoopStartLabel, "LOOP START");
    configureFieldLabel(waveformLoopEndLabel, "END");
    configureFieldLabel(waveformLoopCrossfadeLabel, "CROSSFADE");

    configureEditorSlider(macroDefaultSlider, 0.0, 1.0, 0.01);
    configureEditorSlider(macroMinSlider, 0.0, 1.0, 0.01);
    configureEditorSlider(macroMaxSlider, 0.0, 1.0, 0.01);
    configureEditorSlider(fxParameterSlider, 0.0, 1.0, 0.01);
    configureMetadataLabel(groupVisibilityHintLabel);
    configureMetadataLabel(groupSummaryLabel);
    configureMetadataLabel(groupRoundRobinLabel);
    configureMetadataLabel(groupRoundRobinHintLabel);
    configureMetadataLabel(fxParameterValueLabel);
    configureMetadataLabel(fxSummaryLabel);
    configureMetadataLabel(fxDiagnosticsLabel);

    zoneSelector.setComponentID("authoringZoneSelector");
    previewEnabledToggle.setComponentID("authoringPreviewEnabledToggle");
    previewStopButton.setComponentID("authoringPreviewStopButton");
    previewEnabledToggle.setButtonText("Preview On");
    previewEnabledToggle.setToggleState(true, juce::dontSendNotification);
    previewStopButton.setButtonText("Stop");
    zoneMap.setComponentID("authoringZoneMap");
    showMapButton.setComponentID("authoringShowMapButton");
    showMapButton.setButtonText("Show Map");
    showMapButton.setClickingTogglesState(false);
    structureSearchLabel.setText("Find", juce::dontSendNotification);
    structureSearchLabel.setComponentID("authoringStructureSearchLabel");
    structureSearchEditor.setComponentID("authoringStructureSearchEditor");
    structureSearchEditor.setTextToShowWhenEmpty("Search layers, groups, zones", authoringPanelMuted);
    structureSortSelector.setComponentID("authoringStructureSortSelector");
    structureSortSelector.addItem("Authored order", 1);
    structureSortSelector.addItem("Name", 2);
    structureSortSelector.addItem("Key low", 3);
    structureSortSelector.addItem("Root key", 4);
    structureSortSelector.addItem("Velocity low", 5);
    structureSortSelector.addItem("Diagnostics", 6);
    structureSortSelector.setSelectedId(1, juce::dontSendNotification);
    structureDiagnosticFilterSelector.setComponentID("authoringStructureDiagnosticFilterSelector");
    structureDiagnosticFilterSelector.addItem("All diagnostics", 1);
    structureDiagnosticFilterSelector.addItem("Overlaps", 2);
    structureDiagnosticFilterSelector.addItem("Potential collisions", 3);
    structureDiagnosticFilterSelector.addItem("Exact stacks", 4);
    structureDiagnosticFilterSelector.addItem("Visible only", 5);
    structureDiagnosticFilterSelector.setSelectedId(1, juce::dontSendNotification);
    structureContextFilterSelector.setComponentID("authoringStructureContextFilterSelector");
    structureContextFilterSelector.addItem("All context", 1);
    structureContextFilterSelector.addItem("Current articulation", 2);
    structureContextFilterSelector.addItem("Note-on", 3);
    structureContextFilterSelector.addItem("Note-off", 4);
    structureContextFilterSelector.addItem("Release", 5);
    structureContextFilterSelector.setSelectedId(1, juce::dontSendNotification);
    groupSectionLabel.setComponentID("authoringGroupSectionLabel");
    groupNameLabel.setComponentID("authoringGroupNameLabel");
    groupNameEditor.setComponentID("authoringGroupNameEditor");
    groupCreateButton.setComponentID("authoringGroupCreateButton");
    groupAssignZonesButton.setComponentID("authoringGroupAssignZonesButton");
    groupPreviewAnchorButton.setComponentID("authoringGroupPreviewAnchorButton");
    groupMoveUpButton.setComponentID("authoringGroupMoveUpButton");
    groupMoveDownButton.setComponentID("authoringGroupMoveDownButton");
    groupVisibilityButton.setComponentID("authoringGroupVisibilityButton");
    groupVisibilityHintLabel.setComponentID("authoringGroupVisibilityHintLabel");
    workbenchRegion.setComponentID("authoringWorkbench");
    workbenchTabStrip.setComponentID("authoringWorkbenchTabStrip");
    workbenchContentHost.setComponentID("authoringWorkbenchContentHost");
    macroWorkbenchContent.setComponentID("authoringMacroContent");
    macroWorkbenchViewport.setComponentID("authoringMacroViewport");
    routingWorkbenchContent.setComponentID("authoringRoutingContent");
    routingWorkbenchViewport.setComponentID("authoringRoutingViewport");
    workbenchToggleButton.setComponentID("authoringWorkbenchToggleButton");
    workbenchWaveformTabButton.setComponentID("authoringWorkbenchWaveformTab");
    workbenchGroupsTabButton.setComponentID("authoringWorkbenchGroupsTab");
    workbenchMacrosTabButton.setComponentID("authoringWorkbenchMacrosTab");
    workbenchRoutingTabButton.setComponentID("authoringWorkbenchRoutingTab");
    workbenchPerformanceTabButton.setComponentID("authoringWorkbenchPerformanceTab");
    waveformLabel.setComponentID("authoringWorkbenchTitleLabel");
    waveformScopeLabel.setComponentID("authoringWorkbenchScopeLabel");
    workbenchBreadcrumbLabel.setComponentID("authoringWorkbenchBreadcrumbLabel");
    waveformStatusLabel.setComponentID("authoringWaveformStatusLabel");
    waveformInfoLabel.setComponentID("authoringWaveformInfoLabel");
    loopInfoLabel.setComponentID("authoringWaveformLoopLabel");
    importMetricsLabel.setComponentID("authoringWaveformImportLabel");
    sourceValidationLabel.setComponentID("authoringWaveformValidationLabel");
    sourceValidationButton.setComponentID("authoringWaveformValidationButton");
    sourceValidationButton.setButtonText("Validate Sources");
    waveformPreview.setComponentID("authoringWaveformPreview");
    waveformPreview.setDetailRequestCallback(waveformDetailRequestCallback);
    waveformPreview.setLoopRegionCommitCallback(
        [this](const std::uint64_t startFrame,
               const std::uint64_t endFrameExclusive,
               const std::string& label)
        {
            commitWaveformLoopRegion(startFrame, endFrameExclusive, label);
        });
    waveformPreview.setPlaybackRegionCommitCallback(
        [this](const std::uint64_t startFrame,
               const std::uint64_t endFrameExclusive,
               const std::string& label)
        {
            commitWaveformPlaybackRegion(startFrame, endFrameExclusive, label);
        });
    waveformPreview.setSelectionChangedCallback([this](const bool hasSelection)
    {
        AuthoringWaveformPreview preview;
        if (waveformPreviewProvider)
            preview = waveformPreviewProvider();
        AuthoringPreviewStatusSnapshot previewStatus;
        if (authoringPreviewStatusProvider)
            previewStatus = authoringPreviewStatusProvider();
        const auto hasEditableSource = preview.available
            && preview.frameCount > preview.playbackStartFrame;
        waveformSetPlaybackSelectionButton.setEnabled(hasEditableSource && hasSelection);
        waveformSetLoopSelectionButton.setEnabled(hasEditableSource && hasSelection);
        waveformSelectionAuditionButton.setEnabled(
            hasEditableSource && hasSelection && previewStatus.auditionAvailable);
    });
    waveformPlaybackStartEditor.setComponentID("authoringWaveformPlaybackStart");
    waveformPlaybackStartLabel.setComponentID("authoringWaveformPlaybackStartLabel");
    waveformPlaybackStartEditor.setTextToShowWhenEmpty("frame or 0.25s", authoringPanelMuted);
    waveformPlaybackEndEditor.setComponentID("authoringWaveformPlaybackEnd");
    waveformPlaybackEndLabel.setComponentID("authoringWaveformPlaybackEndLabel");
    waveformPlaybackEndEditor.setTextToShowWhenEmpty("frame or 0.25s", authoringPanelMuted);
    waveformPlaybackResetButton.setComponentID("authoringWaveformPlaybackReset");
    waveformPlaybackResetButton.setButtonText("Reset");
    waveformSetPlaybackSelectionButton.setComponentID("authoringWaveformSetPlaybackSelection");
    waveformSetPlaybackSelectionButton.setButtonText("Use for Playback");
    waveformPlaybackAuditionButton.setComponentID("authoringWaveformPlaybackAudition");
    waveformPlaybackAuditionButton.setButtonText("Play Region");
    waveformSelectionAuditionButton.setComponentID("authoringWaveformSelectionAudition");
    waveformSelectionAuditionButton.setButtonText("Play Selection");
    waveformSnapToggle.setComponentID("authoringWaveformSnapToggle");
    waveformSnapToggle.setButtonText("Snap to Zero");
    waveformLoopModeSelector.setComponentID("authoringWaveformLoopMode");
    waveformLoopModeSelector.addItem("Loop Off", 1);
    waveformLoopModeSelector.addItem("While Held", 4);
    waveformLoopModeSelector.addItem("Always", 3);
    waveformLoopStartEditor.setComponentID("authoringWaveformLoopStart");
    waveformLoopStartLabel.setComponentID("authoringWaveformLoopStartLabel");
    waveformLoopStartEditor.setTextToShowWhenEmpty("frame or 0.25s", authoringPanelMuted);
    waveformLoopEndEditor.setComponentID("authoringWaveformLoopEnd");
    waveformLoopEndLabel.setComponentID("authoringWaveformLoopEndLabel");
    waveformLoopEndEditor.setTextToShowWhenEmpty("frame or 0.25s", authoringPanelMuted);
    waveformLoopCrossfadeEditor.setComponentID("authoringWaveformLoopCrossfade");
    waveformLoopCrossfadeLabel.setComponentID("authoringWaveformLoopCrossfadeLabel");
    waveformLoopCrossfadeEditor.setTextToShowWhenEmpty("frames or 0.01s", authoringPanelMuted);
    waveformSetLoopSelectionButton.setComponentID("authoringWaveformSetLoopSelection");
    waveformSetLoopSelectionButton.setButtonText("Use for Loop");
    waveformLoopAuditionButton.setComponentID("authoringWaveformLoopAudition");
    waveformLoopAuditionButton.setButtonText("Play Loop");
    waveformLoopGuidanceLabel.setComponentID("authoringWaveformLoopGuidance");
    macroAssignmentSelector.setComponentID("authoringMacroAssignmentSelector");
    macroRoleSelector.setComponentID("authoringMacroRoleSelector");
    macroDefaultSlider.setComponentID("authoringMacroDefaultSlider");
    macroMinSlider.setComponentID("authoringMacroMinSlider");
    macroMaxSlider.setComponentID("authoringMacroMaxSlider");
    macroCreateButton.setComponentID("authoringMacroCreateButton");
    macroDuplicateButton.setComponentID("authoringMacroDuplicateButton");
    macroDeleteButton.setComponentID("authoringMacroDeleteButton");
    macroAssignmentAddButton.setComponentID("authoringMacroAssignmentAddButton");
    macroAssignmentRemoveButton.setComponentID("authoringMacroAssignmentRemoveButton");
    macroNameLabel.setComponentID("authoringMacroNameLabel");
    macroNameEditor.setComponentID("authoringMacroNameEditor");
    macroExposeLabel.setComponentID("authoringMacroExposeLabel");
    macroExposeToggle.setComponentID("authoringMacroExposeToggle");
    macroMoveUpButton.setComponentID("authoringMacroMoveUpButton");
    macroMoveDownButton.setComponentID("authoringMacroMoveDownButton");
    macroSummaryLabel.setComponentID("authoringMacroAssignmentSummaryLabel");
    fxSectionLabel.setComponentID("authoringFxSectionLabel");
    routingSectionLabel.setComponentID("authoringRoutingSectionLabel");
    fxSelector.setComponentID("authoringFxSelector");
    fxScopeSelector.setComponentID("authoringDspScopeSelector");
    fxScopeBreadcrumbLabel.setComponentID("authoringDspScopeBreadcrumb");
    fxNameEditor.setComponentID("authoringFxNameEditor");
    fxTypeSelector.setComponentID("authoringFxTypeSelector");
    fxBypassedToggle.setComponentID("authoringFxBypassedToggle");
    fxAddButton.setComponentID("authoringFxAddButton");
    fxDuplicateButton.setComponentID("authoringFxDuplicateButton");
    fxMoveUpButton.setComponentID("authoringFxMoveUpButton");
    fxMoveDownButton.setComponentID("authoringFxMoveDownButton");
    fxDeleteButton.setComponentID("authoringFxDeleteButton");
    fxOwnerSelector.setComponentID("authoringFxOwnerSelector");
    fxMoveOwnerButton.setComponentID("authoringFxMoveOwnerButton");
    fxParameterSelector.setComponentID("authoringFxParameterSelector");
    fxParameterSlider.setComponentID("authoringFxParameterSlider");
    fxParameterResetButton.setComponentID("authoringFxParameterResetButton");
    fxAssignMacroButton.setComponentID("authoringFxAssignMacroButton");
    fxParameterValueLabel.setComponentID("authoringFxParameterValueLabel");
    fxSummaryLabel.setComponentID("authoringFxSummaryLabel");
    fxDiagnosticsLabel.setComponentID("authoringFxDiagnosticsLabel");
    routingBusSelector.setComponentID("authoringRoutingSelector");
    routingInputSelector.setComponentID("authoringRoutingInputSelector");
    routingInsertOneSelector.setComponentID("authoringRoutingInsertOneSelector");
    routingInsertTwoSelector.setComponentID("authoringRoutingInsertTwoSelector");
    routingSummaryLabel.setComponentID("authoringRoutingSummaryLabel");
    groupSummaryLabel.setComponentID("authoringGroupSummaryLabel");
    masterGainLabel.setComponentID("authoringMasterGainLabel");
    masterGainSlider.setComponentID("authoringMasterGainSlider");
    groupVisibilityLabel.setComponentID("authoringGroupVisibilityLabel");
    groupVisibilityToggle.setComponentID("authoringGroupVisibilityToggle");
    groupGainLabel.setComponentID("authoringGroupGainLabel");
    groupGainSlider.setComponentID("authoringGroupGainSlider");
    groupPanLabel.setComponentID("authoringGroupPanLabel");
    groupPanSlider.setComponentID("authoringGroupPanSlider");
    groupRoutingLabel.setComponentID("authoringGroupRoutingLabel");
    groupRoutingSelector.setComponentID("authoringGroupRoutingSelector");
    groupAnchorLabel.setComponentID("authoringGroupAnchorLabel");
    groupAnchorSelector.setComponentID("authoringGroupAnchorSelector");
    groupDeleteButton.setComponentID("authoringGroupDeleteButton");
    groupRoundRobinLabel.setComponentID("authoringGroupRoundRobinLabel");
    groupRoundRobinHintLabel.setComponentID("authoringGroupRoundRobinHintLabel");
    groupRoundRobinToggle.setComponentID("authoringGroupRoundRobinToggle");
    groupRoundRobinModeSelector.setComponentID("authoringGroupRoundRobinModeSelector");
    performanceBankSelector.setComponentID("authoringPerformanceBankSelector");
    triggerSlotSelector.setComponentID("authoringTriggerSlotSelector");
    triggerEventSelector.setComponentID("authoringTriggerEventSelector");
    targetArticulationSelector.setComponentID("authoringTargetArticulationSelector");
    phraseAssetSelector.setComponentID("authoringPhraseAssetSelector");
    chordModeSelector.setComponentID("authoringChordModeSelector");
    performanceSummaryLabel.setComponentID("authoringPerformanceSummaryLabel");
    phraseSummaryLabel.setComponentID("authoringPhraseSummaryLabel");
    roundRobinResetLabel.setComponentID("authoringRoundRobinResetLabel");
    roundRobinResetSelector.setComponentID("authoringRoundRobinResetSelector");
    roundRobinResetEventSelector.setComponentID("authoringRoundRobinResetEventSelector");
    roundRobinResetTargetSelector.setComponentID("authoringRoundRobinResetTargetSelector");
    roundRobinResetAddButton.setComponentID("authoringRoundRobinResetAddButton");
    roundRobinResetDeleteButton.setComponentID("authoringRoundRobinResetDeleteButton");
    roundRobinResetSummaryLabel.setComponentID("authoringRoundRobinResetSummaryLabel");
    phraseImportPathEditor.setComponentID("authoringPhraseImportPath");
    workbenchArticulationsTabButton.setComponentID("authoringWorkbenchArticulationsTab");
    articulationList.setComponentID("authoringArticulationList");
    articulationWorkbenchViewport.setComponentID("authoringArticulationWorkbenchViewport");
    articulationCreateButton.setComponentID("authoringArticulationCreateButton");
    articulationDuplicateButton.setComponentID("authoringArticulationDuplicateButton");
    articulationDefaultButton.setComponentID("authoringArticulationDefaultButton");
    articulationMoveUpButton.setComponentID("authoringArticulationMoveUpButton");
    articulationMoveDownButton.setComponentID("authoringArticulationMoveDownButton");
    articulationDeleteButton.setComponentID("authoringArticulationDeleteButton");
    articulationNameEditor.setComponentID("authoringArticulationNameEditor");
    articulationSwitchNoteSlider.setComponentID("authoringArticulationKeySwitchPicker");
    articulationSwitchNoteValueLabel.setComponentID("authoringArticulationKeySwitchValue");
    articulationClearSwitchButton.setComponentID("authoringArticulationKeySwitchClearButton");
    articulationMidiLearnButton.setComponentID("authoringArticulationMidiLearnButton");
    articulationDeleteReassignSelector.setComponentID("authoringArticulationDeleteReassignSelector");
    articulationStatusLabel.setComponentID("authoringArticulationStatusLabel");

    workbenchToggleButton.onClick = [this]
    {
        setWorkbenchOpen(!workbenchState.open);
    };
    workbenchWaveformTabButton.setButtonText("Waveform");
    workbenchGroupsTabButton.setButtonText("Groups");
    workbenchMacrosTabButton.setButtonText("Macros");
    workbenchRoutingTabButton.setButtonText("Routing");
    workbenchPerformanceTabButton.setButtonText("Performance");
    workbenchArticulationsTabButton.setButtonText("Articulations");
    workbenchWaveformTabButton.onClick = [this] { setActiveWorkbenchTab(authoring::WorkbenchTab::waveform); };
    waveformLoopModeSelector.onChange = [this]
    {
        if (!isRefreshing)
            commitWaveformLoopControls("Change loop behavior");
    };
    waveformSetLoopSelectionButton.onClick = [this] { setWaveformLoopToSelection(); };
    waveformLoopAuditionButton.onClick = [this] { auditionWaveformLoop(); };
    const auto commitLoopRegion = [this]
    {
        commitWaveformLoopControls("Set loop region");
    };
    waveformLoopStartEditor.onReturnKey = commitLoopRegion;
    waveformLoopStartEditor.onFocusLost = commitLoopRegion;
    waveformLoopEndEditor.onReturnKey = commitLoopRegion;
    waveformLoopEndEditor.onFocusLost = commitLoopRegion;
    const auto commitLoopCrossfade = [this]
    {
        commitWaveformLoopControls("Set loop crossfade");
    };
    waveformLoopCrossfadeEditor.onReturnKey = commitLoopCrossfade;
    waveformLoopCrossfadeEditor.onFocusLost = commitLoopCrossfade;
    const auto commitPlaybackRegion = [this]
    {
        commitWaveformPlaybackControls("Set playback region");
    };
    waveformPlaybackStartEditor.onReturnKey = commitPlaybackRegion;
    waveformPlaybackStartEditor.onFocusLost = commitPlaybackRegion;
    waveformPlaybackEndEditor.onReturnKey = commitPlaybackRegion;
    waveformPlaybackEndEditor.onFocusLost = commitPlaybackRegion;
    waveformPlaybackResetButton.onClick = [this] { resetWaveformPlaybackRegion(); };
    waveformSetPlaybackSelectionButton.onClick = [this] { setWaveformPlaybackToSelection(); };
    waveformPlaybackAuditionButton.onClick = [this]
    {
        auditionWaveformRegion(drs::engine::WaveformAuditionMode::playbackRegion);
    };
    waveformSelectionAuditionButton.onClick = [this]
    {
        auditionWaveformRegion(drs::engine::WaveformAuditionMode::selection);
    };
    waveformSnapToggle.onClick = [this]
    {
        waveformPreview.setZeroCrossingSnapEnabled(waveformSnapToggle.getToggleState());
        refreshWaveformWorkbenchContent();
    };
    workbenchGroupsTabButton.onClick = [this] { setActiveWorkbenchTab(authoring::WorkbenchTab::groups); };
    workbenchMacrosTabButton.onClick = [this] { setActiveWorkbenchTab(authoring::WorkbenchTab::macros); };
    workbenchRoutingTabButton.onClick = [this] { setActiveWorkbenchTab(authoring::WorkbenchTab::routing); };
    workbenchPerformanceTabButton.onClick = [this] { setActiveWorkbenchTab(authoring::WorkbenchTab::performance); };
    workbenchArticulationsTabButton.onClick = [this] { setActiveWorkbenchTab(authoring::WorkbenchTab::articulations); };
    workbenchSplitter.setOnHeightRequested([this](const int height)
    {
        workbenchLayoutState.setUserHeight(height);
        workbenchState.open = true;
        refreshWorkbenchVisibility();
        resized();
    });
    workbenchSplitter.setOnSizeToggleRequested([this]
    {
        if (workbenchLayoutState.getSizeMode() == authoring::WorkbenchSizeMode::focused)
            workbenchLayoutState.requestStandard();
        else
            workbenchLayoutState.requestFocused();
        workbenchState.open = true;
        refreshWorkbenchVisibility();
        resized();
        workbenchSplitter.grabKeyboardFocus();
    });
    structureMapSplitter.setOnWidthRequested([this](const int width)
    {
        userStructureBrowserWidth = width;
        resized();
    });
    structureMapSplitter.setOnResetRequested([this]
    {
        userStructureBrowserWidth.reset();
        resized();
        structureMapSplitter.grabKeyboardFocus();
    });
    phraseImportPathEditor.onTextChange = [this]
    {
        refreshContextualAccessibility();
    };
    configureAccessibilityAndFocus();

    authoring::SelectionSummaryCallbacks summaryCallbacks;
    summaryCallbacks.onPreviewRequested = [this] { previewSelectedZone(); };
    summaryCallbacks.onPrepareDraftPlaybackRequested = [this] { prepareDraftPlaybackPreview(); };
    summaryCallbacks.onPublishDraftPlaybackRequested = [this] { publishDraftPlayback(); };
    summaryCallbacks.onUndoRequested = [this] { undoLastEdit(); };
    summaryCallbacks.onRedoRequested = [this] { redoLastEdit(); };
    summaryStrip.setCallbacks(std::move(summaryCallbacks));

    authoring::ZoneFieldCallbacks zoneCallbacks;
    zoneCallbacks.onCommitRequested = [this](const authoring::ZoneFieldValuesViewModel& values,
                                             const std::string& label)
    {
        applySelectedZoneEdit(values, juce::String::fromUTF8(label.c_str()));
    };
    zoneCallbacks.onArticulationCommitRequested = [this](const std::string& articulationId)
    {
        auto zoneIds = zoneMapSelectedZoneIds;
        if (zoneIds.empty())
        {
            if (const auto selectedZone = authoringSession.getSelectedZone(); selectedZone.has_value())
                zoneIds.push_back(selectedZone->id);
        }
        const auto result = authoringSession.reassignZonesToArticulation(
            zoneIds, articulationId, "Assign selected zones to articulation");
        if (result.applied)
            refreshFromSession();
    };
    zoneCallbacks.onCreateVelocityCrossfadeRequested = [this](const std::string& lowerZoneId,
                                                               const std::string& upperZoneId,
                                                               const int overlapLow,
                                                               const int overlapHigh)
    {
        const auto result = authoringSession.createVelocityCrossfadePair(
            { lowerZoneId, upperZoneId, overlapLow, overlapHigh }, "Create velocity crossfade");
        if (result.applied)
        {
            refreshFromSession();
            return;
        }
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                               "Crossfade Unavailable",
                                               buildIssueSummary(result.issues));
    };
    zoneCallbacks.onUpdateVelocityCrossfadeRequested = [this](const std::string& lowerZoneId,
                                                               const std::string& upperZoneId,
                                                               const int overlapLow,
                                                               const int overlapHigh)
    {
        const auto result = authoringSession.updateVelocityCrossfadePair(
            { lowerZoneId, upperZoneId, overlapLow, overlapHigh }, "Update velocity crossfade");
        if (result.applied)
        {
            refreshFromSession();
            return;
        }
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                               "Crossfade Update Unavailable",
                                               buildIssueSummary(result.issues));
    };
    zoneCallbacks.onRemoveVelocityCrossfadeRequested = [this](const std::string& lowerZoneId,
                                                               const std::string& upperZoneId)
    {
        const auto result = authoringSession.removeVelocityCrossfadePair(
            lowerZoneId, upperZoneId, "Remove velocity crossfade");
        if (result.applied)
        {
            refreshFromSession();
            return;
        }
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                               "Crossfade Removal Unavailable",
                                               buildIssueSummary(result.issues));
    };
    zoneCallbacks.onCreateVelocityCrossfadeStackRequested = [this](const std::vector<std::string>& zoneIds,
                                                                    const int overlapWidth)
    {
        const auto result = authoringSession.createVelocityCrossfadeStack(
            { zoneIds, overlapWidth }, "Create velocity crossfade stack");
        if (result.applied)
        {
            refreshFromSession();
            return;
        }
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                               "Crossfade Stack Unavailable",
                                               buildIssueSummary(result.issues));
    };
    zoneCallbacks.onRemoveVelocityCrossfadeStackRequested = [this](const std::vector<std::string>& zoneIds)
    {
        const auto result = authoringSession.removeVelocityCrossfadeStack(
            zoneIds, "Remove velocity crossfade stack");
        if (result.applied)
        {
            refreshFromSession();
            return;
        }
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                               "Crossfade Stack Removal Unavailable",
                                               buildIssueSummary(result.issues));
    };
    zoneCallbacks.onCreateChokeGroupRequested = [this]
    {
        const auto selectedZone = authoringSession.getSelectedZone();
        if (!selectedZone.has_value())
            return;
        auto zone = *selectedZone;
        const auto& zones = authoringSession.getProject().authoring.zones;
        int suffix = 1;
        auto groupId = std::string("choke-group-") + std::to_string(suffix);
        const auto groupExists = [&](const std::string& candidate)
        {
            return std::any_of(zones.begin(), zones.end(), [&](const auto& existing)
            {
                return existing.exclusiveGroupId == candidate;
            });
        };
        while (groupExists(groupId))
            groupId = std::string("choke-group-") + std::to_string(++suffix);
        zone.exclusiveGroupId = groupId;
        zone.exclusiveTargetGroupIds.clear();
        authoringSession.updateSelectedZone(zone, "Create choke group");
        refreshFromSession();
    };
    zoneCallbacks.onRestoreRootKeyRequested = [this]
    {
        if (onRestoreRootKeyRequested)
            onRestoreRootKeyRequested();
    };
    zoneCallbacks.onRevealInStructureRequested = [this]
    {
        auto ids = zoneMapSelectedZoneIds;
        if (ids.empty())
        {
            if (const auto zone = authoringSession.getSelectedZone(); zone.has_value())
                ids.push_back(zone->id);
        }
        if (ids.empty())
            return;
        const auto primary = authoringSession.getSelectedZone();
        showStructureForSelection(std::move(ids), primary.has_value() ? primary->id : std::string {});
    };
    zoneCallbacks.onOpenWaveformRequested = [this]
    {
        setWorkbenchOpen(true);
        setActiveWorkbenchTab(authoring::WorkbenchTab::waveform);
        requestWaveformPreviewLoad(true);
    };
    zoneCallbacks.onPreviewRequested = [this]
    {
        previewSelectedZone(drs::engine::AuthoringPreviewAuditionSource::inspector);
    };
    zoneCallbacks.onAuditionVelocityCrossfadeRequested = [this](const std::vector<int>& velocities)
    {
        auditionVelocityCrossfade(velocities);
    };
    zoneMappingEditor.setCallbacks(std::move(zoneCallbacks));
    zoneMap.setOnZoneSelectionStateRequested([this](const authoring::ZoneMapCanvas::SelectionState& selectionState)
    {
        if (isRefreshing)
            return;

        if (applyZoneMapSelectionState(selectionState))
        {
            requestWaveformPreviewLoad(workbenchState.activeTab == authoring::WorkbenchTab::waveform);
            refreshSelectionFromSession();
        }
    });
    zoneMap.setOnZoneAuditionRequested([this](const std::string& zoneId,
                                               int midiNote,
                                               int velocity)
    {
        previewSelectedZone(drs::engine::AuthoringPreviewAuditionSource::zoneMap,
                            midiNote, velocity, zoneId);
    });
    zoneMap.setOnOverlappingZonesRequested([this](const std::vector<std::string>& zoneIds)
    {
        if (zoneIds.size() < 2)
            return;
        juce::PopupMenu menu;
        const auto& project = authoringSession.getProject();
        for (std::size_t index = 0; index < zoneIds.size(); ++index)
        {
            const auto zone = std::find_if(project.authoring.zones.begin(), project.authoring.zones.end(),
                                           [&](const auto& candidate) { return candidate.id == zoneIds[index]; });
            const auto label = zone == project.authoring.zones.end()
                ? juce::String::fromUTF8(zoneIds[index].c_str())
                : juce::String::fromUTF8((zone->displayName.empty() ? zone->id : zone->displayName).c_str())
                    + "  ·  key " + juce::String(zone->keyLow) + "–" + juce::String(zone->keyHigh)
                    + "  ·  vel " + juce::String(zone->velocityLow) + "–" + juce::String(zone->velocityHigh);
            menu.addItem(static_cast<int>(index + 1), label);
        }
        menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&zoneMap),
                           [this, zoneIds](const int result)
                           {
                               if (result <= 0 || static_cast<std::size_t>(result) > zoneIds.size()) return;
                               const auto zoneId = zoneIds[static_cast<std::size_t>(result - 1)];
                               if (authoringSession.selectZone(zoneId).applied)
                                   refreshFromSession();
                           });
    });
    zoneMap.setOnShowInStructureRequested([this](std::vector<std::string> zoneIds,
                                                  std::string primaryZoneId)
    {
        showStructureForSelection(std::move(zoneIds), std::move(primaryZoneId));
    });
    zoneMap.setOnSampleFilesDropped([this](std::vector<juce::File> files)
    {
        if (sampleFilesDroppedCallback)
            sampleFilesDroppedCallback(std::move(files));
    });
    zoneMap.setOnDeleteSelectedSampleRequested([this]
    {
        if (isRefreshing)
            return;

        const auto result = authoringSession.deleteZones(zoneMapSelectedZoneIds,
                                                         zoneMapSelectedZoneIds.size() > 1
                                                             ? "Delete selected zones"
                                                             : "Delete selected sample");
        if (result.applied)
            refreshFromSession();
    });
    zoneMap.setOnZoneRangeCommitRequested([this](
        const std::vector<drs::engine::AuthoringZoneSummary>& zones,
        const std::string& label)
    {
        const auto result = authoringSession.updateZoneRanges(zones, label);
        if (result.applied)
            refreshFromSession();
    });
    zoneMap.setOnVelocityCrossfadeCommitRequested([this](const std::string& lowerZoneId,
                                                          const std::string& upperZoneId,
                                                          const int overlapLow,
                                                          const int overlapHigh)
    {
        const auto result = authoringSession.updateVelocityCrossfadePair(
            { lowerZoneId, upperZoneId, overlapLow, overlapHigh }, "Drag velocity crossfade overlap");
        if (result.applied)
            refreshFromSession();
    });
    showMapButton.onClick = [this]
    {
        structureViewState.setMapPaneVisible(!structureViewState.isMapPaneVisible());
        refreshInspectorVisibility();
        resized();
    };
    structureBrowser.setOnSelectionChanged([this](const authoring::StructureSelectionKind kind,
                                                   std::vector<std::string> ids,
                                                   std::string primaryId)
    {
        applyStructureSelection(kind, std::move(ids), std::move(primaryId));
    });
    structureBrowser.setOnScopeRequested([this](authoring::StructureScope scope)
    {
        structureViewState.setScope(std::move(scope));
        refreshStructureInspector();
    });
    structureBrowser.setOnShowZonesRequested([this]
    {
        showZonesForCurrentSelection();
    });
    structureBrowser.setOnNewLayerRequested([this]
    {
        createLayer();
    });
    structureBrowser.setOnNewGroupRequested([this]
    {
        createGroup();
    });
    structureBrowser.setOnDeleteRequested([this]
    {
        if (structureSelection.getKind() == authoring::StructureSelectionKind::group
            && !structureSelection.getPrimaryId().empty())
            deleteSelectedGroup();
        else if (structureSelection.getKind() == authoring::StructureSelectionKind::zone
                 && !structureSelection.getIds().empty())
        {
            if (authoringSession.deleteZones(structureSelection.getIds(), "Delete selected zones").applied)
                refreshFromSession();
        }
    });
    const auto selectStructureChildren = [this](const bool visibleOnly)
    {
        const auto& project = authoringSession.getProject();
        std::vector<std::string> childIds;
        if (structureSelection.getKind() == authoring::StructureSelectionKind::instrument)
        {
            for (const auto& layer : project.authoring.layers)
                if (!visibleOnly || layer.workspaceVisible)
                    childIds.push_back(layer.id);
            if (!childIds.empty())
            {
                const auto primaryId = childIds.front();
                applyStructureSelection(authoring::StructureSelectionKind::layer, std::move(childIds), primaryId);
            }
        }
        else if (structureSelection.getKind() == authoring::StructureSelectionKind::layer)
        {
            for (const auto& group : project.authoring.groups)
                if (structureSelection.contains(group.layerId) && (!visibleOnly || group.workspaceVisible))
                    childIds.push_back(group.id);
            if (!childIds.empty())
            {
                const auto primaryId = childIds.front();
                applyStructureSelection(authoring::StructureSelectionKind::group, std::move(childIds), primaryId);
            }
        }
        else if (structureSelection.getKind() == authoring::StructureSelectionKind::group)
        {
            for (const auto& zone : project.authoring.zones)
            {
                if (!structureSelection.contains(zone.groupId)) continue;
                if (visibleOnly)
                {
                    const auto group = std::find_if(project.authoring.groups.begin(), project.authoring.groups.end(),
                                                    [&](const auto& candidate) { return candidate.id == zone.groupId; });
                    const auto layer = group == project.authoring.groups.end()
                        ? project.authoring.layers.end()
                        : std::find_if(project.authoring.layers.begin(), project.authoring.layers.end(),
                                       [&](const auto& candidate) { return candidate.id == group->layerId; });
                    if (group == project.authoring.groups.end() || layer == project.authoring.layers.end()
                        || !group->workspaceVisible || !layer->workspaceVisible)
                        continue;
                }
                childIds.push_back(zone.id);
            }
            if (!childIds.empty())
            {
                const auto primaryId = childIds.front();
                applyStructureSelection(authoring::StructureSelectionKind::zone, std::move(childIds), primaryId);
            }
        }
    };
    structureBrowser.setOnSelectChildrenRequested([selectStructureChildren] { selectStructureChildren(false); });
    structureBrowser.setOnSelectVisibleChildrenRequested([selectStructureChildren] { selectStructureChildren(true); });
    structureBrowser.setOnDisclosureChanged([this](std::string id, const bool disclosed)
    {
        structureViewState.setDisclosed(id, disclosed);
        structureViewState.setCollapsed(id, !disclosed);
        refreshStructureBrowser();
    });
    structureSearchEditor.onTextChange = [this]
    {
        structureViewState.setSearchText(structureSearchEditor.getText().toStdString());
        refreshStructureBrowser();
    };
    structureSortSelector.onChange = [this]
    {
        const auto sortId = structureSortSelector.getSelectedId();
        structureViewState.setSortMode(sortId == 2 ? authoring::StructureSortMode::name
                                           : sortId == 3 ? authoring::StructureSortMode::keyLow
                                           : sortId == 4 ? authoring::StructureSortMode::rootKey
                                           : sortId == 5 ? authoring::StructureSortMode::velocityLow
                                           : sortId == 6 ? authoring::StructureSortMode::diagnostic
                                                         : authoring::StructureSortMode::authoredOrder);
        refreshStructureBrowser();
    };
    structureDiagnosticFilterSelector.onChange = [this]
    {
        const auto filterId = structureDiagnosticFilterSelector.getSelectedId();
        structureViewState.setShowOverlapsOnly(filterId == 2 || filterId == 3 || filterId == 4);
        structureViewState.setShowPotentialCollisionsOnly(filterId == 3);
        structureViewState.setShowExactStacksOnly(filterId == 4);
        structureViewState.setVisibleOnly(filterId == 5);
        refreshStructureBrowser();
    };
    structureContextFilterSelector.onChange = [this]
    {
        const auto filterId = structureContextFilterSelector.getSelectedId();
        structureViewState.setArticulationFilter({});
        if (filterId == 2)
        {
            if (const auto zone = authoringSession.getSelectedZone(); zone.has_value())
                structureViewState.setArticulationFilter(zone->articulationId);
        }
        if (filterId == 3)
            structureViewState.setPerformanceEventFilter(drs::engine::PerformanceEventKind::noteOn);
        else if (filterId == 4)
            structureViewState.setPerformanceEventFilter(drs::engine::PerformanceEventKind::noteOff);
        else if (filterId == 5)
            structureViewState.setPerformanceEventFilter(drs::engine::PerformanceEventKind::release);
        else
            structureViewState.setPerformanceEventFilter(std::nullopt);
        refreshStructureBrowser();
    };
    structureInspector.setOnPatchRequested([this](const authoring::StructureSelectionKind kind,
                                                  drs::engine::AuthoringStructureBatchPatch patch)
    {
        if (kind == authoring::StructureSelectionKind::instrument)
        {
            if (!patch.releaseSeconds.has_value())
                return;
            std::vector<std::string> zoneIds;
            zoneIds.reserve(authoringSession.getProject().authoring.zones.size());
            for (const auto& zone : authoringSession.getProject().authoring.zones)
                zoneIds.push_back(zone.id);
            const auto result = authoringSession.applyStructureBatchPatch(
                drs::engine::AuthoringStructureEntityKind::zone,
                zoneIds, patch, "Edit instrument descendant zones");
            if (result.applied)
                refreshFromSession();
            return;
        }
        const auto entityKind = kind == authoring::StructureSelectionKind::layer
            ? drs::engine::AuthoringStructureEntityKind::layer
            : kind == authoring::StructureSelectionKind::group
                ? drs::engine::AuthoringStructureEntityKind::group
                : drs::engine::AuthoringStructureEntityKind::zone;
        const auto result = authoringSession.applyStructureBatchPatch(
            entityKind, structureSelection.getIds(), patch, "Edit structure selection");
        if (result.applied)
            refreshFromSession();
    });
    structureInspector.setOnActionRequested([this](const authoring::StructureInspectorAction action)
    {
        if (structureSelection.getKind() == authoring::StructureSelectionKind::none)
            return;

        if (action == authoring::StructureInspectorAction::showZones)
        {
            showZonesForCurrentSelection();
            return;
        }

        if (action == authoring::StructureInspectorAction::openWaveform)
        {
            setWorkbenchOpen(true);
            setActiveWorkbenchTab(authoring::WorkbenchTab::waveform);
            requestWaveformPreviewLoad(true);
            return;
        }

        if (action == authoring::StructureInspectorAction::audition)
        {
            if (structureSelection.getKind() == authoring::StructureSelectionKind::zone)
                previewSelectedZone(drs::engine::AuthoringPreviewAuditionSource::inspector,
                                    -1, 0, structureSelection.getPrimaryId());
            else if (structureSelection.getKind() == authoring::StructureSelectionKind::group)
                previewSelectedGroupAnchor();
            else if (const auto layer = authoringSession.getSelectedLayer(); layer.has_value()
                     && !layer->auditionAnchorGroupId.empty()
                     && authoringSession.selectGroup(layer->auditionAnchorGroupId).applied)
            {
                refreshSelectionFromSession();
                previewSelectedGroupAnchor();
            }
            return;
        }

        const auto selectVisible = action == authoring::StructureInspectorAction::selectVisibleChildren;
        std::vector<std::string> childIds;
        const auto& project = authoringSession.getProject();
        if (structureSelection.getKind() == authoring::StructureSelectionKind::layer)
        {
            for (const auto& group : project.authoring.groups)
                if (structureSelection.contains(group.layerId)
                    && (!selectVisible || group.workspaceVisible))
                    childIds.push_back(group.id);
        }
        else if (structureSelection.getKind() == authoring::StructureSelectionKind::group)
        {
            for (const auto& zone : project.authoring.zones)
            {
                if (!structureSelection.contains(zone.groupId))
                    continue;
                if (selectVisible)
                {
                    const auto group = std::find_if(project.authoring.groups.begin(), project.authoring.groups.end(),
                                                   [&](const auto& candidate) { return candidate.id == zone.groupId; });
                    const auto layer = group == project.authoring.groups.end()
                        ? project.authoring.layers.end()
                        : std::find_if(project.authoring.layers.begin(), project.authoring.layers.end(),
                                       [&](const auto& candidate) { return candidate.id == group->layerId; });
                    if (group == project.authoring.groups.end()
                        || layer == project.authoring.layers.end()
                        || !group->workspaceVisible || !layer->workspaceVisible)
                        continue;
                }
                childIds.push_back(zone.id);
            }
        }
        if (childIds.empty())
            return;
        structureSelection.replace(structureSelection.getKind() == authoring::StructureSelectionKind::layer
                                       ? authoring::StructureSelectionKind::group
                                       : authoring::StructureSelectionKind::zone,
                                   childIds,
                                   childIds.front());
        if (structureSelection.getKind() == authoring::StructureSelectionKind::zone)
        {
            zoneMapSelectedZoneIds = structureSelection.getIds();
            authoringSession.selectZone(structureSelection.getPrimaryId());
        }
        else
            authoringSession.selectGroup(structureSelection.getPrimaryId());
        refreshFromSession();
        refreshStructureBrowser();
    });
    groupList.setOnSelectionChanged([this](int nextIndex)
    {
        const ScopedMessageThreadSpan timing(MessageThreadSpanKind::zoneSelection);
        if (isRefreshing)
            return;

        const auto& groups = authoringSession.getProject().authoring.groups;
        if (nextIndex < 0 || static_cast<std::size_t>(nextIndex) >= groups.size())
            return;

        selectedGroupIndex = nextIndex;
        const auto result = authoringSession.selectGroup(groups[static_cast<std::size_t>(nextIndex)].id);
        if (!result.applied)
            return;

        structureSelection.replace(authoring::StructureSelectionKind::group,
                                   { groups[static_cast<std::size_t>(nextIndex)].id },
                                   groups[static_cast<std::size_t>(nextIndex)].id);

        setActiveWorkbenchTab(authoring::WorkbenchTab::groups);
        requestWaveformPreviewLoad(workbenchState.activeTab == authoring::WorkbenchTab::waveform);
        refreshSelectionFromSession();
    });

    groupCreateButton.setButtonText("New Group");
    groupCreateButton.onClick = [this] { createGroup(); };
    groupAssignZonesButton.setButtonText("Add Selected Zones");
    groupAssignZonesButton.onClick = [this] { assignSelectedZonesToSelectedGroup(); };
    groupPreviewAnchorButton.setButtonText("Preview Anchor");
    groupPreviewAnchorButton.onClick = [this] { previewSelectedGroupAnchor(); };
    groupMoveUpButton.setButtonText("Move Up");
    groupMoveUpButton.onClick = [this] { moveSelectedGroup(-1); };
    groupMoveDownButton.setButtonText("Move Down");
    groupMoveDownButton.onClick = [this] { moveSelectedGroup(1); };
    groupVisibilityButton.onClick = [this] { toggleSelectedGroupVisibility(); };

    layerCreateButton.setButtonText("New Layer");
    layerCreateButton.onClick = [this] { createLayer(); };
    layerAssignGroupsButton.setButtonText("Assign Group");
    layerAssignGroupsButton.onClick = [this] { assignSelectedGroupsToSelectedLayer(); };
    layerMoveUpButton.setButtonText("Move Up");
    layerMoveUpButton.onClick = [this] { moveSelectedLayer(-1); };
    layerMoveDownButton.setButtonText("Move Down");
    layerMoveDownButton.onClick = [this] { moveSelectedLayer(1); };
    layerVisibilityToggle.setButtonText("Visible In Workspace");
    layerNameEditor.setMultiLine(false);
    layerNameEditor.setReturnKeyStartsNewLine(false);
    layerNameEditor.onReturnKey = [this] { applySelectedLayerEdit("Rename layer"); };
    layerNameEditor.onFocusLost = [this] { applySelectedLayerEdit("Rename layer"); };
    configureEditorSlider(layerGainSlider, -24.0, 24.0, 0.1);
    configureEditorSlider(layerPanSlider, -1.0, 1.0, 0.01);
    configureEditorSlider(layerCrossfadeControllerSlider, 0.0, 127.0, 1.0);
    configureEditorSlider(layerCrossfadeLowSlider, 0.0, 127.0, 1.0);
    configureEditorSlider(layerCrossfadeHighSlider, 0.0, 127.0, 1.0);
    layerGainSlider.onDragEnd = [this] { applySelectedLayerEdit("Update layer gain"); };
    layerPanSlider.onDragEnd = [this] { applySelectedLayerEdit("Update layer pan"); };
    layerVisibilityToggle.onClick = [this] { applySelectedLayerEdit("Toggle layer visibility"); };
    layerCrossfadeSourceSelector.addItem("None", 1);
    layerCrossfadeSourceSelector.addItem("Velocity", 2);
    layerCrossfadeSourceSelector.addItem("Controller", 3);
    layerCrossfadeSourceSelector.onChange = [this] { applySelectedLayerEdit("Update layer crossfade"); };
    layerCrossfadeLowSlider.onDragEnd = [this] { applySelectedLayerEdit("Update layer crossfade low"); };
    layerCrossfadeHighSlider.onDragEnd = [this] { applySelectedLayerEdit("Update layer crossfade high"); };
    layerCrossfadeDirectionSelector.addItem("Fade In", 1);
    layerCrossfadeDirectionSelector.addItem("Fade Out", 2);
    layerCrossfadeDirectionSelector.onChange = [this] { applySelectedLayerEdit("Update layer crossfade direction"); };
    layerRoutingSelector.onChange = [this] { applySelectedLayerEdit("Update layer routing"); };
    layerAnchorSelector.onChange = [this] { applySelectedLayerEdit("Update layer audition anchor"); };

    layerList.setOnSelectionChanged([this](int nextIndex)
    {
        if (isRefreshing)
            return;
        const auto& layers = authoringSession.getProject().authoring.layers;
        if (nextIndex < 0 || static_cast<std::size_t>(nextIndex) >= layers.size())
            return;
        selectedLayerIndex = nextIndex;
        if (authoringSession.selectLayer(layers[static_cast<std::size_t>(nextIndex)].id).applied)
        {
            structureSelection.replace(authoring::StructureSelectionKind::layer,
                                       { layers[static_cast<std::size_t>(nextIndex)].id },
                                       layers[static_cast<std::size_t>(nextIndex)].id);
            refreshSelectionFromSession();
        }
    });

    groupNameEditor.setMultiLine(false);
    groupNameEditor.setReturnKeyStartsNewLine(false);
    groupNameEditor.onReturnKey = [this] { applySelectedGroupNameEdit(); };
    groupNameEditor.onFocusLost = [this] { applySelectedGroupNameEdit(); };

    configureEditorSlider(masterGainSlider, -24.0, 24.0, 0.1);
    configureEditorSlider(groupGainSlider, -24.0, 24.0, 0.1);
    configureEditorSlider(groupPanSlider, -1.0, 1.0, 0.01);
    masterGainSlider.onDragEnd = [this] { applyProjectMasterGainEdit("Update master gain"); };
    groupGainSlider.onDragEnd = [this] { applySelectedGroupMixEdit("Update group gain"); };
    groupPanSlider.onDragEnd = [this] { applySelectedGroupMixEdit("Update group pan"); };
    groupVisibilityToggle.setButtonText("Visible In Workspace");
    groupVisibilityToggle.onClick = [this]
    {
        if (isRefreshing)
            return;

        applySelectedGroupMixEdit("Toggle group visibility");
    };
    groupRoutingSelector.onChange = [this]
    {
        if (isRefreshing)
            return;

        applySelectedGroupMixEdit("Update group routing");
    };
    groupAnchorSelector.onChange = [this]
    {
        if (isRefreshing)
            return;

        applySelectedGroupMixEdit("Update group audition anchor");
    };
    groupDeleteButton.setButtonText("Delete Group");
    groupDeleteButton.onClick = [this] { deleteSelectedGroup(); };
    groupRoundRobinToggle.setButtonText("Round Robin");
    groupRoundRobinToggle.onClick = [this]
    {
        if (isRefreshing)
            return;

        const auto shouldEnable = groupRoundRobinToggle.getToggleState();
        const auto mode = groupRoundRobinModeSelector.getSelectedId() == 2
            ? drs::engine::RoundRobinMode::random
            : drs::engine::RoundRobinMode::sequential;
        const auto result = authoringSession.setSelectedGroupRoundRobinEnabled(
            shouldEnable,
            mode,
            shouldEnable ? "Enable selected-group Round Robin" : "Disable selected-group Round Robin");
        if (result.applied)
        {
            refreshFromSession();
            return;
        }

        groupRoundRobinToggle.setToggleState(!shouldEnable, juce::dontSendNotification);
        juce::AlertWindow::showMessageBoxAsync(
            juce::AlertWindow::WarningIcon,
            "Round Robin Unavailable",
            buildIssueSummary(result.issues));
    };
    groupRoundRobinModeSelector.addItem("Cycle", 1);
    groupRoundRobinModeSelector.addItem("Random", 2);
    groupRoundRobinModeSelector.setSelectedId(1, juce::dontSendNotification);
    groupRoundRobinModeSelector.onChange = [this]
    {
        if (isRefreshing)
            return;

        const auto status = authoringSession.getSelectedGroupRoundRobinStatus();
        if (!status.enabled)
            return;

        const auto mode = groupRoundRobinModeSelector.getSelectedId() == 2
            ? drs::engine::RoundRobinMode::random
            : drs::engine::RoundRobinMode::sequential;
        const auto result = authoringSession.setSelectedGroupRoundRobinMode(
            mode, "Change selected-group Round Robin mode");
        if (result.applied)
            refreshFromSession();
    };

    zoneSelector.onChange = [this]
    {
        const ScopedMessageThreadSpan timing(MessageThreadSpanKind::zoneSelection);
        if (isRefreshing)
            return;

        const auto zoneIndex = zoneSelector.getSelectedId() - 1;
        const auto zones = authoringSession.getZoneSummaries();
        if (zoneIndex < 0 || static_cast<std::size_t>(zoneIndex) >= zones.size())
            return;

        authoringSession.selectZone(zones[static_cast<std::size_t>(zoneIndex)].id);
        structureSelection.replace(authoring::StructureSelectionKind::zone,
                                   { zones[static_cast<std::size_t>(zoneIndex)].id },
                                   zones[static_cast<std::size_t>(zoneIndex)].id);
        requestWaveformPreviewLoad(workbenchState.activeTab == authoring::WorkbenchTab::waveform);
        refreshSelectionFromSession();
    };

    previewEnabledToggle.onClick = [this]
    {
        if (!previewEnabledToggle.getToggleState())
        {
            crossfadeAuditionSequence.active = false;
            waveformAuditionCueActive = false;
            waveformAuditionNoteOffMillis = 0.0;
            waveformAuditionInitialFrame = 0;
            waveformAuditionRegion = {};
        }
        if (!previewEnabledToggle.getToggleState() && previewCommandCallback)
        {
            drs::engine::AuthoringPreviewCommand command;
            command.type = drs::engine::AuthoringPreviewCommandType::stopAll;
            command.source = drs::engine::AuthoringPreviewAuditionSource::summaryPreview;
            previewCommandCallback(command);
        }
        refreshWaveformWorkbenchContent();
    };
    previewStopButton.onClick = [this]
    {
        crossfadeAuditionSequence.active = false;
        waveformAuditionCueActive = false;
        waveformAuditionNoteOffMillis = 0.0;
        waveformAuditionInitialFrame = 0;
        waveformAuditionRegion = {};
        for (auto& timedNote : timedPreviewNotes)
            timedNote = {};
        stopTimer(previewReleaseTimerId);
        if (previewCommandCallback)
        {
            drs::engine::AuthoringPreviewCommand command;
            command.type = drs::engine::AuthoringPreviewCommandType::stopAll;
            command.source = drs::engine::AuthoringPreviewAuditionSource::summaryPreview;
            previewCommandCallback(command);
        }
        refreshWaveformWorkbenchContent();
    };
    sourceValidationButton.onClick = [this]
    {
        updateSourceValidationAction();
        refreshWaveformWorkbenchContent();
    };

    macroList.setOnSelectionChanged([this](int nextIndex)
    {
        if (isRefreshing)
            return;

        const auto& macros = authoringSession.getProject().authoring.macros;
        if (nextIndex < 0 || static_cast<std::size_t>(nextIndex) >= macros.size())
            return;

        selectedMacroIndex = std::max(0, nextIndex);
        selectedMacroTargetIndex = 0;
        authoringSession.selectMacro(macros[static_cast<std::size_t>(nextIndex)].id);
        refreshFromSession();
    });

    macroAssignmentList.setOnSelectionChanged([this](int nextIndex)
    {
        if (isRefreshing)
            return;

        const auto selectedMacro = authoringSession.getSelectedMacro();
        if (!selectedMacro.has_value() || nextIndex < 0
            || static_cast<std::size_t>(nextIndex) >= selectedMacro->targets.size())
            return;

        selectedMacroTargetIndex = nextIndex;
        refreshFromSession();
    });

    macroCreateButton.setButtonText("Create");
    macroCreateButton.onClick = [this] { createMacro(); };
    macroDuplicateButton.setButtonText("Duplicate");
    macroDuplicateButton.onClick = [this] { duplicateSelectedMacro(); };
    macroDeleteButton.setButtonText("Delete");
    macroDeleteButton.setColour(juce::TextButton::textColourOffId,
                                authoring::visual::error);
    macroDeleteButton.onClick = [this] { deleteSelectedMacro(); };
    macroAssignmentAddButton.setButtonText("Add Target");
    macroAssignmentAddButton.onClick = [this] { addMacroAssignment(); };
    macroAssignmentRemoveButton.setButtonText("Remove Target");
    macroAssignmentRemoveButton.setColour(juce::TextButton::textColourOffId,
                                          authoring::visual::error);
    macroAssignmentRemoveButton.onClick = [this] { removeSelectedMacroAssignment(); };
    macroNameEditor.setMultiLine(false);
    macroNameEditor.setReturnKeyStartsNewLine(false);
    macroNameEditor.onReturnKey = [this] { applySelectedMacroEdit("Rename macro", MacroEditField::name); };
    macroNameEditor.onFocusLost = [this] { applySelectedMacroEdit("Rename macro", MacroEditField::name); };
    macroExposeToggle.setButtonText("Expose In Perform");
    macroExposeToggle.onClick = [this]
    {
        if (isRefreshing)
            return;

        applySelectedMacroEdit("Toggle macro exposure", MacroEditField::exposure);
    };

    fxSelector.onChange = [this]
    {
        if (isRefreshing)
            return;

        selectedFxSlotIndex = std::max(0, fxSelector.getSelectedId() - 1);
        refreshFromSession();
    };

    fxScopeSelector.onChange = [this]
    {
        if (isRefreshing)
            return;
        selectedDspScopeIndex = std::clamp(fxScopeSelector.getSelectedId() - 1, 0, 2);
        if (const auto scopedBusId = selectedDspScopeRoutingBusId(); !scopedBusId.empty())
        {
            const auto& routingBuses = authoringSession.getProject().authoring.routingBuses;
            const auto iterator = std::find_if(routingBuses.begin(),
                                               routingBuses.end(),
                                               [&](const auto& routingBus)
                                               {
                                                   return routingBus.id == scopedBusId;
                                               });
            if (iterator != routingBuses.end())
                selectedRoutingBusIndex = static_cast<int>(std::distance(routingBuses.begin(), iterator));
        }
        refreshFromSession();
    };

    routingBusSelector.onChange = [this]
    {
        if (isRefreshing)
            return;

        selectedRoutingBusIndex = std::max(0, routingBusSelector.getSelectedId() - 1);
        refreshFromSession();
    };

    performanceBankSelector.onChange = [this]
    {
        if (isRefreshing)
            return;

        selectedPerformanceBankIndex = std::max(0, performanceBankSelector.getSelectedId() - 1);
        const auto& performanceBanks = authoringSession.getProject().authoring.performanceBanks;
        if (selectedPerformanceBankIndex >= 0
            && static_cast<std::size_t>(selectedPerformanceBankIndex) < performanceBanks.size())
        {
            authoringSession.selectPerformanceBank(
                performanceBanks[static_cast<std::size_t>(selectedPerformanceBankIndex)].id);
        }

        selectedTriggerSlotIndex = 0;
        refreshFromSession();
    };

    triggerSlotSelector.onChange = [this]
    {
        if (isRefreshing)
            return;

        selectedTriggerSlotIndex = std::max(0, triggerSlotSelector.getSelectedId() - 1);
        refreshFromSession();
    };

    auto bindCommitOnDragEnd = [this](juce::Slider& slider, const juce::String& label, auto&& callback)
    {
        slider.onDragEnd = [this, label, callback]
        {
            callback(label);
        };
    };

    bindCommitOnDragEnd(macroDefaultSlider, "Update macro default", [this](const juce::String& label) { applySelectedMacroEdit(label, MacroEditField::defaultValue); });
    bindCommitOnDragEnd(macroMinSlider, "Update macro range", [this](const juce::String& label) { applySelectedMacroEdit(label, MacroEditField::range); });
    bindCommitOnDragEnd(macroMaxSlider, "Update macro range", [this](const juce::String& label) { applySelectedMacroEdit(label, MacroEditField::range); });

    macroAssignmentSelector.onChange = [this]
    {
        if (isRefreshing)
            return;

        applySelectedMacroEdit("Update macro assignment", MacroEditField::assignment);
    };

    macroRoleSelector.onChange = [this]
    {
        if (isRefreshing)
            return;

        applySelectedMacroEdit("Update macro role", MacroEditField::role);
    };

    macroMoveUpButton.setButtonText("Move Up");
    macroMoveUpButton.onClick = [this] { moveSelectedMacro(-1); };
    macroMoveDownButton.setButtonText("Move Down");
    macroMoveDownButton.onClick = [this] { moveSelectedMacro(1); };

    fxTypeSelector.onChange = [this]
    {
        if (isRefreshing)
            return;

        applySelectedFxSlotEdit("Update FX type");
    };

    fxBypassedToggle.setButtonText("Bypassed");
    fxBypassedToggle.onClick = [this]
    {
        if (isRefreshing)
            return;

        applySelectedFxSlotEdit("Toggle FX bypass");
    };
    fxNameEditor.onReturnKey = [this] { applySelectedFxSlotEdit("Rename FX slot"); };
    fxAddButton.setButtonText("Add Insert");
    fxAddButton.onClick = [this] { createScopedFxSlot(); };
    fxDuplicateButton.setButtonText("Duplicate");
    fxDuplicateButton.onClick = [this] { duplicateSelectedFxSlot(); };
    fxMoveUpButton.setButtonText("Move Up");
    fxMoveUpButton.onClick = [this] { moveSelectedFxSlot(-1); };
    fxMoveDownButton.setButtonText("Move Down");
    fxMoveDownButton.onClick = [this] { moveSelectedFxSlot(1); };
    fxDeleteButton.setButtonText("Delete");
    fxDeleteButton.setColour(juce::TextButton::textColourOffId,
                             authoring::visual::error);
    fxDeleteButton.onClick = [this] { deleteSelectedFxSlot(); };
    fxMoveOwnerButton.setButtonText("Move to Scope");
    fxMoveOwnerButton.onClick = [this] { moveSelectedFxSlotToSelectedOwner(); };
    fxParameterSelector.onChange = [this]
    {
        if (isRefreshing) return;
        selectedFxParameterIndex = std::max(0, fxParameterSelector.getSelectedId() - 1);
        refreshFromSession();
    };
    fxParameterSlider.onDragEnd = [this] { applySelectedFxParameterEdit("Update FX parameter"); };
    fxParameterResetButton.setButtonText("Reset");
    fxParameterResetButton.onClick = [this] { resetSelectedFxParameter(); };
    fxAssignMacroButton.setButtonText("Assign Macro");
    fxAssignMacroButton.onClick = [this]
    {
        assignSelectedFxParameterToMacro();
    };

    routingInputSelector.onChange = [this]
    {
        if (isRefreshing)
            return;

        applySelectedRoutingBusEdit("Update routing input");
    };

    routingInsertOneSelector.onChange = [this]
    {
        if (isRefreshing)
            return;

        applySelectedRoutingBusEdit("Update routing insert chain");
    };

    routingInsertTwoSelector.onChange = [this]
    {
        if (isRefreshing)
            return;

        applySelectedRoutingBusEdit("Update routing insert chain");
    };

    triggerEventSelector.onChange = [this]
    {
        if (isRefreshing)
            return;

        applySelectedTriggerSlotEdit("Update trigger event");
    };

    targetArticulationSelector.onChange = [this]
    {
        if (isRefreshing)
            return;

        applySelectedTriggerSlotEdit("Update trigger articulation");
    };

    articulationList.setOnSelectionChanged([this](int nextIndex)
    {
        if (isRefreshing)
            return;
        const auto articulations = authoringSession.getArticulations();
        if (nextIndex < 0 || static_cast<std::size_t>(nextIndex) >= articulations.size())
            return;
        selectedArticulationIndex = nextIndex;
        refreshFromSession();
    });
    articulationCreateButton.setButtonText("Create");
    articulationCreateButton.onClick = [this] { createArticulation(); };
    articulationDuplicateButton.setButtonText("Duplicate");
    articulationDuplicateButton.onClick = [this] { duplicateSelectedArticulation(); };
    articulationDefaultButton.setButtonText("Make Default");
    articulationDefaultButton.onClick = [this] { setSelectedArticulationDefault(); };
    articulationMoveUpButton.setButtonText("Move Up");
    articulationMoveUpButton.onClick = [this] { moveSelectedArticulation(-1); };
    articulationMoveDownButton.setButtonText("Move Down");
    articulationMoveDownButton.onClick = [this] { moveSelectedArticulation(1); };
    articulationDeleteButton.setButtonText("Delete / Reassign");
    articulationDeleteButton.onClick = [this] { deleteSelectedArticulation(); };
    articulationNameEditor.setMultiLine(false);
    articulationNameEditor.setReturnKeyStartsNewLine(false);
    articulationNameEditor.onReturnKey = [this] { applySelectedArticulationEdit("Rename articulation"); };
    articulationNameEditor.onFocusLost = [this] { applySelectedArticulationEdit("Rename articulation"); };
    configureEditorSlider(articulationSwitchNoteSlider, 0.0, 127.0, 1.0);
    articulationSwitchNoteSlider.onDragEnd = [this] { applySelectedArticulationEdit("Set key switch"); };
    articulationClearSwitchButton.setButtonText("Clear Switch");
    articulationClearSwitchButton.onClick = [this] { clearSelectedArticulationKeySwitch(); };
    articulationMidiLearnButton.onClick = [this] { toggleKeySwitchMidiLearn(); };

    phraseAssetSelector.onChange = [this]
    {
        if (isRefreshing)
            return;

        applySelectedTriggerSlotEdit("Update trigger phrase asset");
    };

    chordModeSelector.onChange = [this]
    {
        if (isRefreshing)
            return;

        applySelectedTriggerSlotEdit("Update trigger chord rule");
    };

    phraseImportButton.setButtonText("Import MIDI Phrase");
    phraseImportButton.setComponentID("authoringPhraseImportButton");
    phraseImportButton.onClick = [this]
    {
        importPhraseForSelectedBank();
    };
    roundRobinResetEventSelector.addItem("Program activation", 1);
    roundRobinResetEventSelector.addItem("Articulation change", 2);
    roundRobinResetEventSelector.addItem("All notes off", 3);
    roundRobinResetEventSelector.addItem("Pedal down", 4);
    roundRobinResetEventSelector.addItem("Pedal up", 5);
    roundRobinResetAddButton.setButtonText("Add Reset");
    roundRobinResetAddButton.onClick = [this] { addRoundRobinResetRule(); };
    roundRobinResetDeleteButton.setButtonText("Remove Reset");
    roundRobinResetDeleteButton.onClick = [this] { deleteRoundRobinResetRule(); };
    roundRobinResetSelector.onChange = [this]
    {
        if (!isRefreshing)
        {
            selectedRoundRobinResetIndex = roundRobinResetSelector.getSelectedId() - 1;
            refreshFromSession();
        }
    };
    roundRobinResetEventSelector.onChange = [this]
    {
        if (!isRefreshing) updateSelectedRoundRobinResetRule();
    };
    roundRobinResetTargetSelector.onChange = [this]
    {
        if (!isRefreshing) updateSelectedRoundRobinResetRule();
    };

    for (auto* component : {
             static_cast<juce::Component*>(&summaryStrip),
             static_cast<juce::Component*>(&workbenchRegion),
             static_cast<juce::Component*>(&workbenchTabStrip),
             static_cast<juce::Component*>(&workbenchContentHost),
             static_cast<juce::Component*>(&waveformLabel),
             static_cast<juce::Component*>(&waveformScopeLabel),
             static_cast<juce::Component*>(&workbenchBreadcrumbLabel),
             static_cast<juce::Component*>(&waveformStatusLabel),
             static_cast<juce::Component*>(&waveformInfoLabel),
             static_cast<juce::Component*>(&loopInfoLabel),
             static_cast<juce::Component*>(&importMetricsLabel),
             static_cast<juce::Component*>(&waveformPlaybackStartLabel),
             static_cast<juce::Component*>(&waveformPlaybackEndLabel),
             static_cast<juce::Component*>(&waveformLoopStartLabel),
             static_cast<juce::Component*>(&waveformLoopEndLabel),
             static_cast<juce::Component*>(&waveformLoopCrossfadeLabel),
             static_cast<juce::Component*>(&waveformPlaybackStartEditor),
             static_cast<juce::Component*>(&waveformPlaybackEndEditor),
             static_cast<juce::Component*>(&waveformPlaybackResetButton),
             static_cast<juce::Component*>(&waveformSetPlaybackSelectionButton),
             static_cast<juce::Component*>(&waveformPlaybackAuditionButton),
             static_cast<juce::Component*>(&waveformSelectionAuditionButton),
             static_cast<juce::Component*>(&waveformSnapToggle),
             static_cast<juce::Component*>(&waveformLoopModeSelector),
             static_cast<juce::Component*>(&waveformLoopStartEditor),
             static_cast<juce::Component*>(&waveformLoopEndEditor),
             static_cast<juce::Component*>(&waveformLoopCrossfadeEditor),
             static_cast<juce::Component*>(&waveformSetLoopSelectionButton),
             static_cast<juce::Component*>(&waveformLoopAuditionButton),
             static_cast<juce::Component*>(&waveformLoopGuidanceLabel),
             static_cast<juce::Component*>(&sourceValidationLabel),
             static_cast<juce::Component*>(&sourceValidationButton),
             static_cast<juce::Component*>(&workbenchToggleButton),
             static_cast<juce::Component*>(&workbenchWaveformTabButton),
             static_cast<juce::Component*>(&workbenchMacrosTabButton),
             static_cast<juce::Component*>(&workbenchRoutingTabButton),
             static_cast<juce::Component*>(&workbenchPerformanceTabButton),
             static_cast<juce::Component*>(&workbenchArticulationsTabButton),
             static_cast<juce::Component*>(&zoneLabel),
             static_cast<juce::Component*>(&zoneSelector),
             static_cast<juce::Component*>(&previewEnabledToggle),
             static_cast<juce::Component*>(&previewStopButton),
             static_cast<juce::Component*>(&showMapButton),
             static_cast<juce::Component*>(&zoneMap),
             static_cast<juce::Component*>(&structureBrowser),
             static_cast<juce::Component*>(&structureInspector),
             static_cast<juce::Component*>(&structureSearchLabel),
             static_cast<juce::Component*>(&structureSearchEditor),
             static_cast<juce::Component*>(&structureSortSelector),
             static_cast<juce::Component*>(&structureDiagnosticFilterSelector),
             static_cast<juce::Component*>(&structureContextFilterSelector),
             static_cast<juce::Component*>(&layerSectionLabel),
             static_cast<juce::Component*>(&layerList),
             static_cast<juce::Component*>(&layerCreateButton),
             static_cast<juce::Component*>(&layerAssignGroupsButton),
             static_cast<juce::Component*>(&layerMoveUpButton),
             static_cast<juce::Component*>(&layerMoveDownButton),
             static_cast<juce::Component*>(&layerNameLabel),
             static_cast<juce::Component*>(&layerNameEditor),
             static_cast<juce::Component*>(&layerVisibilityToggle),
             static_cast<juce::Component*>(&layerGainSlider),
             static_cast<juce::Component*>(&layerPanSlider),
             static_cast<juce::Component*>(&layerCrossfadeSourceSelector),
             static_cast<juce::Component*>(&layerCrossfadeControllerSlider),
             static_cast<juce::Component*>(&layerCrossfadeLowSlider),
             static_cast<juce::Component*>(&layerCrossfadeHighSlider),
             static_cast<juce::Component*>(&layerCrossfadeDirectionSelector),
             static_cast<juce::Component*>(&layerRoutingSelector),
             static_cast<juce::Component*>(&layerAnchorSelector),
             static_cast<juce::Component*>(&groupSectionLabel),
             static_cast<juce::Component*>(&groupNameLabel),
             static_cast<juce::Component*>(&groupNameEditor),
             static_cast<juce::Component*>(&groupCreateButton),
             static_cast<juce::Component*>(&groupAssignZonesButton),
             static_cast<juce::Component*>(&groupPreviewAnchorButton),
             static_cast<juce::Component*>(&groupList),
             static_cast<juce::Component*>(&groupMoveUpButton),
             static_cast<juce::Component*>(&groupMoveDownButton),
             static_cast<juce::Component*>(&groupVisibilityButton),
             static_cast<juce::Component*>(&groupVisibilityHintLabel),
             static_cast<juce::Component*>(&zoneMappingEditor),
             static_cast<juce::Component*>(&waveformPreview),
             static_cast<juce::Component*>(&workbenchGroupsTabButton),
             static_cast<juce::Component*>(&macroWorkbenchViewport),
             static_cast<juce::Component*>(&macroList),
             static_cast<juce::Component*>(&macroCreateButton),
             static_cast<juce::Component*>(&macroDuplicateButton),
             static_cast<juce::Component*>(&macroDeleteButton),
             static_cast<juce::Component*>(&macroNameLabel),
             static_cast<juce::Component*>(&macroNameEditor),
             static_cast<juce::Component*>(&macroExposeLabel),
             static_cast<juce::Component*>(&macroExposeToggle),
             static_cast<juce::Component*>(&macroAssignmentLabel),
             static_cast<juce::Component*>(&macroAssignmentSelector),
             static_cast<juce::Component*>(&macroRoleLabel),
             static_cast<juce::Component*>(&macroRoleSelector),
             static_cast<juce::Component*>(&macroDefaultLabel),
             static_cast<juce::Component*>(&macroDefaultSlider),
             static_cast<juce::Component*>(&macroMinLabel),
             static_cast<juce::Component*>(&macroMinSlider),
             static_cast<juce::Component*>(&macroMaxLabel),
             static_cast<juce::Component*>(&macroMaxSlider),
             static_cast<juce::Component*>(&macroSummaryLabel),
             static_cast<juce::Component*>(&macroMoveUpButton),
             static_cast<juce::Component*>(&macroMoveDownButton),
             static_cast<juce::Component*>(&routingWorkbenchViewport),
             static_cast<juce::Component*>(&fxSectionLabel),
             static_cast<juce::Component*>(&fxScopeLabel),
             static_cast<juce::Component*>(&fxScopeSelector),
             static_cast<juce::Component*>(&fxScopeBreadcrumbLabel),
             static_cast<juce::Component*>(&fxSelector),
             static_cast<juce::Component*>(&fxNameEditor),
             static_cast<juce::Component*>(&fxTypeLabel),
             static_cast<juce::Component*>(&fxTypeSelector),
             static_cast<juce::Component*>(&fxBypassedToggle),
             static_cast<juce::Component*>(&fxAddButton),
             static_cast<juce::Component*>(&fxDuplicateButton),
             static_cast<juce::Component*>(&fxMoveUpButton),
             static_cast<juce::Component*>(&fxMoveDownButton),
             static_cast<juce::Component*>(&fxDeleteButton),
             static_cast<juce::Component*>(&fxOwnerSelector),
             static_cast<juce::Component*>(&fxMoveOwnerButton),
             static_cast<juce::Component*>(&fxParameterSelector),
             static_cast<juce::Component*>(&fxParameterSlider),
             static_cast<juce::Component*>(&fxParameterResetButton),
             static_cast<juce::Component*>(&fxAssignMacroButton),
             static_cast<juce::Component*>(&fxParameterValueLabel),
             static_cast<juce::Component*>(&fxSummaryLabel),
             static_cast<juce::Component*>(&fxDiagnosticsLabel),
             static_cast<juce::Component*>(&routingSectionLabel),
             static_cast<juce::Component*>(&routingBusSelector),
             static_cast<juce::Component*>(&routingInputLabel),
             static_cast<juce::Component*>(&routingInputSelector),
             static_cast<juce::Component*>(&routingInsertOneLabel),
             static_cast<juce::Component*>(&routingInsertOneSelector),
             static_cast<juce::Component*>(&routingInsertTwoLabel),
             static_cast<juce::Component*>(&routingInsertTwoSelector),
             static_cast<juce::Component*>(&routingSummaryLabel),
             static_cast<juce::Component*>(&groupSummaryLabel),
             static_cast<juce::Component*>(&masterGainLabel),
             static_cast<juce::Component*>(&masterGainSlider),
             static_cast<juce::Component*>(&groupVisibilityLabel),
             static_cast<juce::Component*>(&groupVisibilityToggle),
             static_cast<juce::Component*>(&groupGainLabel),
             static_cast<juce::Component*>(&groupGainSlider),
             static_cast<juce::Component*>(&groupPanLabel),
             static_cast<juce::Component*>(&groupPanSlider),
             static_cast<juce::Component*>(&groupRoutingLabel),
             static_cast<juce::Component*>(&groupRoutingSelector),
             static_cast<juce::Component*>(&groupAnchorLabel),
             static_cast<juce::Component*>(&groupAnchorSelector),
             static_cast<juce::Component*>(&groupDeleteButton),
             static_cast<juce::Component*>(&groupRoundRobinLabel),
             static_cast<juce::Component*>(&groupRoundRobinHintLabel),
             static_cast<juce::Component*>(&groupRoundRobinToggle),
             static_cast<juce::Component*>(&groupRoundRobinModeSelector),
             static_cast<juce::Component*>(&performanceBankSelector),
             static_cast<juce::Component*>(&triggerSlotSelector),
             static_cast<juce::Component*>(&triggerEventLabel),
             static_cast<juce::Component*>(&triggerEventSelector),
             static_cast<juce::Component*>(&targetArticulationLabel),
             static_cast<juce::Component*>(&targetArticulationSelector),
             static_cast<juce::Component*>(&phraseAssetLabel),
             static_cast<juce::Component*>(&phraseAssetSelector),
             static_cast<juce::Component*>(&chordModeLabel),
             static_cast<juce::Component*>(&chordModeSelector),
             static_cast<juce::Component*>(&phraseImportPathLabel),
             static_cast<juce::Component*>(&phraseImportPathEditor),
             static_cast<juce::Component*>(&phraseImportButton),
             static_cast<juce::Component*>(&performanceSummaryLabel),
             static_cast<juce::Component*>(&phraseSummaryLabel),
             static_cast<juce::Component*>(&roundRobinResetLabel),
             static_cast<juce::Component*>(&roundRobinResetSelector),
             static_cast<juce::Component*>(&roundRobinResetEventSelector),
             static_cast<juce::Component*>(&roundRobinResetTargetSelector),
             static_cast<juce::Component*>(&roundRobinResetAddButton),
             static_cast<juce::Component*>(&roundRobinResetDeleteButton),
             static_cast<juce::Component*>(&roundRobinResetSummaryLabel),
             static_cast<juce::Component*>(&articulationWorkbenchViewport)
         })
    {
        addAndMakeVisible(component);
    }
    addAndMakeVisible(workbenchSplitter);
    addAndMakeVisible(structureMapSplitter);
    structureBrowser.toFront(false);

    macroWorkbenchViewport.setViewedComponent(&macroWorkbenchContent, false);
    macroWorkbenchViewport.setScrollBarsShown(true, false);
    macroWorkbenchViewport.setScrollBarThickness(12);
    macroWorkbenchViewport.setWantsKeyboardFocus(false);
    macroWorkbenchContent.setSize(1, 1);
    macroWorkbenchContent.setBindings({
        &macroList,
        &macroCreateButton,
        &macroDuplicateButton,
        &macroDeleteButton,
        &macroMoveUpButton,
        &macroMoveDownButton,
        &macroNameLabel,
        &macroNameEditor,
        &macroExposeLabel,
        &macroExposeToggle,
        &macroRoleLabel,
        &macroRoleSelector,
        &macroDefaultLabel,
        &macroDefaultSlider,
        &macroMinLabel,
        &macroMinSlider,
        &macroMaxLabel,
        &macroMaxSlider,
        &macroAssignmentList,
        &macroAssignmentLabel,
        &macroAssignmentSelector,
        &macroAssignmentAddButton,
        &macroAssignmentRemoveButton,
        &macroSummaryLabel
    });

    routingWorkbenchViewport.setViewedComponent(&routingWorkbenchContent, false);
    routingWorkbenchViewport.setScrollBarsShown(true, false);
    routingWorkbenchViewport.setScrollBarThickness(12);
    routingWorkbenchViewport.setWantsKeyboardFocus(false);
    routingWorkbenchContent.setSize(1, 1);
    routingWorkbenchContent.setBindings({
        &fxSectionLabel,
        &fxScopeLabel,
        &fxScopeSelector,
        &fxScopeBreadcrumbLabel,
        &fxSelector,
        &fxNameEditor,
        &fxTypeLabel,
        &fxTypeSelector,
        &fxBypassedToggle,
        &fxAddButton,
        &fxDuplicateButton,
        &fxMoveUpButton,
        &fxMoveDownButton,
        &fxDeleteButton,
        &fxOwnerSelector,
        &fxMoveOwnerButton,
        &fxParameterSelector,
        &fxParameterSlider,
        &fxParameterResetButton,
        &fxAssignMacroButton,
        &fxParameterValueLabel,
        &fxSummaryLabel,
        &fxDiagnosticsLabel,
        &routingSectionLabel,
        &routingBusSelector,
        &routingInputLabel,
        &routingInputSelector,
        &routingInsertOneLabel,
        &routingInsertOneSelector,
        &routingInsertTwoLabel,
        &routingInsertTwoSelector,
        &routingSummaryLabel
    });

    articulationWorkbenchViewport.setViewedComponent(&articulationWorkbenchContent, false);
    articulationWorkbenchViewport.setScrollBarsShown(true, false);
    articulationWorkbenchViewport.setScrollBarThickness(12);
    articulationWorkbenchViewport.setWantsKeyboardFocus(false);
    articulationWorkbenchContent.setSize(1, 1);
    articulationWorkbenchContent.setVisible(true);
    for (auto* component : {
             static_cast<juce::Component*>(&articulationList),
             static_cast<juce::Component*>(&articulationCreateButton),
             static_cast<juce::Component*>(&articulationDuplicateButton),
             static_cast<juce::Component*>(&articulationDefaultButton),
             static_cast<juce::Component*>(&articulationMoveUpButton),
             static_cast<juce::Component*>(&articulationMoveDownButton),
             static_cast<juce::Component*>(&articulationDeleteButton),
             static_cast<juce::Component*>(&articulationNameLabel),
             static_cast<juce::Component*>(&articulationNameEditor),
             static_cast<juce::Component*>(&articulationSwitchNoteLabel),
             static_cast<juce::Component*>(&articulationSwitchNoteSlider),
             static_cast<juce::Component*>(&articulationSwitchNoteValueLabel),
             static_cast<juce::Component*>(&articulationClearSwitchButton),
             static_cast<juce::Component*>(&articulationMidiLearnButton),
             static_cast<juce::Component*>(&articulationDeleteReassignLabel),
             static_cast<juce::Component*>(&articulationDeleteReassignSelector),
             static_cast<juce::Component*>(&articulationStatusLabel)
         })
    {
        articulationWorkbenchContent.addAndMakeVisible(component);
    }
    for (int midiNote = 0; midiNote < 36; ++midiNote)
    {
        auto key = std::make_unique<juce::TextButton>(formatMidiNoteName(midiNote));
        key->setComponentID("authoringArticulationKeyboardNote" + juce::String(midiNote));
        key->setHelpText("Assign " + formatMidiNoteName(midiNote)
                         + " as the selected articulation key switch.");
        key->onClick = [this, midiNote]
        {
            articulationSwitchNoteSlider.setValue(midiNote, juce::dontSendNotification);
            applySelectedArticulationEdit("Assign key switch from keyboard");
        };
        articulationWorkbenchContent.addAndMakeVisible(*key);
        articulationKeyButtons.push_back(std::move(key));
    }

    refreshFromSession();
    if (waveformPreviewProvider || authoringPreviewStatusProvider || importResponsivenessProvider
        || sourceValidationStatusProvider || draftPlaybackStatusProvider)
    {
        startTimer(statusTimerId, 250);
    }
}

AuthoringPanel::~AuthoringPanel()
{
    crossfadeAuditionSequence.active = false;
    for (std::size_t source = 0; source < timedPreviewNotes.size(); ++source)
        releaseTimedPreview(source);
    stopTimer(statusTimerId);
    stopTimer(previewReleaseTimerId);
    stopTimer(keySwitchMidiLearnTimerId);
    setLookAndFeel(nullptr);
}

void AuthoringPanel::configureAccessibilityAndFocus()
{
    configureAccessibleMetadata(*this,
                                "Authoring workspace",
                                "Open Workbench authoring workspace for zone mapping, resizable editors, routing, and performance setup.");
    configureAccessibleMetadata(zoneLabel,
                                "Selected zone label",
                                "Labels the selected zone chooser.");
    configureAccessibleMetadata(zoneSelector,
                                "Zone selector",
                                "Chooses the active zone for map and inspector editing.",
                                "Open the list or use arrow keys to change the selected zone.");
    zoneSelector.setExplicitFocusOrder(24);
    configureAccessibleMetadata(groupSectionLabel,
                                "Zone group manager",
                                "Shows the persistent zone-group manager above the map workspace.");
    configureAccessibleMetadata(groupNameEditor,
                                "Group name",
                                "Renames the selected group from the persistent group manager.",
                                "Type a new group name and press Enter to rename the selected group.");
    configureAccessibleMetadata(groupCreateButton,
                                "Create group",
                                "Creates a new empty group in the authoring workspace.",
                                "Press to create a new group.");
    configureAccessibleMetadata(groupPreviewAnchorButton,
                                "Preview group anchor",
                                "Auditions the selected group's anchor zone as a group-preview entry point.",
                                "Press to preview the selected group's anchor zone.");
    configureAccessibleMetadata(groupList,
                                "Zone group list",
                                "Lists authored zone groups and their workspace visibility state.");
    configureAccessibleMetadata(layerList,
                                "Layer list",
                                "Lists authored layers with their group and zone counts.",
                                "Select a layer to make it the active parent for group authoring.");
    configureAccessibleMetadata(layerCreateButton,
                                "Create layer",
                                "Creates a new parent layer for groups.",
                                "Press to create a new layer.");
    configureAccessibleMetadata(layerAssignGroupsButton,
                                "Assign group to layer",
                                "Assigns the selected group to the active layer.",
                                "Press to move the selected group into the active layer.");
    configureAccessibleMetadata(layerMoveUpButton,
                                "Move layer up",
                                "Moves the selected layer earlier in layer order.",
                                "Press to move the selected layer up.");
    configureAccessibleMetadata(layerMoveDownButton,
                                "Move layer down",
                                "Moves the selected layer later in layer order.",
                                "Press to move the selected layer down.");
    configureAccessibleMetadata(layerNameEditor,
                                "Layer name",
                                "Renames the selected layer.",
                                "Type a new layer name and press Enter to commit it.");
    configureAccessibleMetadata(layerVisibilityToggle,
                                "Layer visibility",
                                "Controls whether the selected layer is visible in the map workspace.");
    configureAccessibleMetadata(layerCrossfadeSourceSelector,
                                "Layer crossfade source",
                                "Selects none, velocity, or controller input for the selected layer crossfade.");
    configureAccessibleMetadata(layerRoutingSelector,
                                "Layer routing",
                                "Selects the layer-owned routing bus when one is authored.");
    configureAccessibleMetadata(layerAnchorSelector,
                                "Layer audition anchor",
                                "Selects the group used as the layer audition anchor.");
    configureAccessibleMetadata(layerCrossfadeLowSlider,
                                "Layer crossfade low",
                                "Sets the lower bound of the selected layer crossfade window.");
    configureAccessibleMetadata(layerCrossfadeHighSlider,
                                "Layer crossfade high",
                                "Sets the upper bound of the selected layer crossfade window.");
    configureAccessibleMetadata(layerCrossfadeDirectionSelector,
                                "Layer crossfade direction",
                                "Selects whether the layer fades in or fades out across its window.");
    configureAccessibleMetadata(groupVisibilityButton,
                                "Toggle group visibility",
                                "Shows or hides the selected group on the workspace map without changing audio.",
                                "Press to toggle whether the selected group is visible on the workspace map.");
    configureAccessibleMetadata(groupMoveUpButton,
                                "Move group up",
                                "Moves the selected group earlier in group order.",
                                "Press to move the selected group up.");
    configureAccessibleMetadata(groupMoveDownButton,
                                "Move group down",
                                "Moves the selected group later in group order.",
                                "Press to move the selected group down.");
    groupNameEditor.setExplicitFocusOrder(27);
    groupCreateButton.setExplicitFocusOrder(28);
    groupPreviewAnchorButton.setExplicitFocusOrder(29);
    groupList.getListBox().setExplicitFocusOrder(30);
    groupVisibilityButton.setExplicitFocusOrder(31);
    groupMoveUpButton.setExplicitFocusOrder(32);
    groupMoveDownButton.setExplicitFocusOrder(33);
    configureAccessibleMetadata(previewEnabledToggle,
                                "Preview enabled",
                                "Enables or disables authoring-only audition commands.",
                                "Turn Preview off to release all authoring Preview notes without affecting performance playback.");
    configureAccessibleMetadata(previewStopButton,
                                "Stop Preview",
                                "Releases all notes owned by the authoring Preview path.",
                                "Press to stop authoring Preview audio without affecting performance playback.");
    previewEnabledToggle.setExplicitFocusOrder(25);
    previewStopButton.setExplicitFocusOrder(26);

    configureAccessibleMetadata(zoneMap,
                                "Zone map",
                                "Displays project zones across key and velocity ranges.",
                                "Use the mouse wheel or Control-scroll to zoom around the pointer. Use a smooth trackpad gesture to pan, Shift-scroll to pan horizontally, or middle-drag to pan. Hold Space while dragging for the temporary hand. Use Fit All or Fit Selected in the map toolbar, arrow keys to move the primary selection, Control to toggle extra zones, or drag a box to multi-select.");
    configureAccessibleMetadata(zoneMap.getScopeSummaryLabel(),
                                "Map scope summary",
                                "Describes the instrument, layer, or group currently projected into the Map.",
                                "Use Show Zones from the hierarchy or inspector to change this scope.");
    zoneMap.setExplicitFocusOrder(30);
    configureAccessibleMetadata(showMapButton,
                                "Show Map",
                                "Toggles the existing key and velocity Zone Map without changing selection or scope.",
                                "Press to show or hide the Map pane.");
    configureAccessibleMetadata(structureBrowser,
                                "Instrument structure browser",
                                "Shows the current instrument as one persistent hierarchy of layers, groups, and zones.",
                                "Select a row to edit its context. Double-click a container to disclose or hide children. Use Show Zones to filter the Map.");
    for (const auto& action : {
             std::pair<const char*, const char*> { "authoringStructureNewLayerButton", "New layer" },
             std::pair<const char*, const char*> { "authoringStructureNewGroupButton", "New group" },
             std::pair<const char*, const char*> { "authoringStructureDeleteButton", "Delete structure item" },
             std::pair<const char*, const char*> { "authoringStructureMoreButton", "More structure actions" }
         })
    {
        if (auto* actionComponent = structureBrowser.findChildWithID(action.first))
            configureAccessibleMetadata(*actionComponent, action.second,
                                        "Runs an authored hierarchy action.",
                                        "The action uses the current hierarchy selection and creates one undoable transaction.");
    }
    configureAccessibleMetadata(structureInspector,
                                "Structure inspector",
                                "Shows context-sensitive values for the active layer, group, or zone selection.",
                                "Review the selected object's shared attributes. Mixed values indicate that the multi-selection contains different values.");
    if (auto* parentEditor = structureInspector.findChildWithID("authoringStructureInspectorParentEditor"))
        configureAccessibleMetadata(*parentEditor,
                                    "Structure parent ID",
                                    "Changes the parent layer for groups or parent group for zones.",
                                    "Enter a valid stable parent ID and apply the edit atomically to the selection.");
    if (auto* releaseEditor = structureInspector.findChildWithID("authoringStructureInspectorZoneReleaseEditor"))
        configureAccessibleMetadata(*releaseEditor,
                                    "Zone release seconds",
                                    "Edits release seconds for selected zones or every descendant zone of the selected instrument, layers, and groups.",
                                    "Enter a non-negative duration in seconds. The complete hierarchy edit is committed as one undoable transaction.");
    for (const auto& action : {
             std::pair<const char*, const char*> { "authoringStructureInspectorPrimaryAction", "Primary structure action" },
             std::pair<const char*, const char*> { "authoringStructureInspectorSecondaryAction", "Secondary structure action" },
             std::pair<const char*, const char*> { "authoringStructureInspectorTertiaryAction", "Tertiary structure action" }
         })
    {
        if (auto* actionComponent = structureInspector.findChildWithID(action.first))
            configureAccessibleMetadata(*actionComponent,
                                        action.second,
                                        "Runs the context-sensitive structure inspector command.",
                                        "The command text identifies whether it shows the scoped Map, selects children, opens waveform, or auditions.");
    }
    configureAccessibleMetadata(structureInspector.findChildWithID("authoringStructureInspectorApplyButton") != nullptr
                                    ? *structureInspector.findChildWithID("authoringStructureInspectorApplyButton")
                                    : structureInspector,
                                "Apply structure edit",
                                "Applies shared values to every selected entity; release seconds also applies to descendant zones of the instrument, layers, and groups.",
                                "Press to commit one atomic hierarchy edit.");
    showMapButton.setExplicitFocusOrder(34);
    structureBrowser.setExplicitFocusOrder(35);
    structureInspector.setExplicitFocusOrder(36);
    configureAccessibleMetadata(structureSearchEditor,
                                "Structure search",
                                "Filters layer, group, and zone rows by name.",
                                "Type text to narrow the hierarchy to matching rows.");
    configureAccessibleMetadata(structureSortSelector,
                                "Structure sort",
                                "Chooses the ordering for structure rows.",
                                "Open the list and choose authored order, name, key, root, velocity, or diagnostics.");
    structureSearchEditor.setExplicitFocusOrder(38);
    structureSortSelector.setExplicitFocusOrder(39);
    configureAccessibleMetadata(structureDiagnosticFilterSelector,
                                "Structure diagnostic filter",
                                "Filters overlap and visibility diagnostics without changing authored data.",
                                "Choose all diagnostics, overlaps, potential collisions, exact stacks, or visible objects.");
    configureAccessibleMetadata(structureContextFilterSelector,
                                "Structure context filter",
                                "Filters zones by current articulation or performance event.",
                                "Choose an articulation or event context to narrow the zone rows.");
    structureDiagnosticFilterSelector.setExplicitFocusOrder(40);
    structureContextFilterSelector.setExplicitFocusOrder(41);

    configureAccessibleMetadata(workbenchRegion,
                                "Authoring workbench",
                                "Hosts waveform, groups, macros, routing, performance, and articulation editors.");
    configureAccessibleMetadata(workbenchTabStrip,
                                "Workbench tab strip",
                                "Contains the workbench visibility control and editor tab buttons.");
    configureAccessibleMetadata(workbenchContentHost,
                                "Workbench content",
                                "Displays the active workbench editor when expanded.");
    structureMapSplitter.setExplicitFocusOrder(42);
    workbenchSplitter.setExplicitFocusOrder(59);
    configureAccessibleMetadata(macroWorkbenchViewport,
                                "Macro editor",
                                "Provides access to project macro creation, assignment, range, and ordering controls.",
                                "All macro controls are visible at standard workspace heights. Scroll vertically in unusually short host windows.");
    configureAccessibleMetadata(routingWorkbenchViewport,
                                "Routing signal-path editor",
                                "Provides access to the selected bus signal path, insert ownership, FX parameters, and Macro controls.",
                                "Bus and selected-insert regions are side by side at normal widths and stack vertically in compact or short hosts.");

    configureAccessibleMetadata(workbenchToggleButton,
                                "Workbench visibility",
                                "Shows or hides the active workbench content.",
                                "Press to collapse or expand the workbench.");
    workbenchToggleButton.setExplicitFocusOrder(60);

    configureAccessibleMetadata(workbenchWaveformTabButton,
                                "Waveform workbench tab",
                                "Shows zone-scoped waveform detail.",
                                "Press to switch the workbench to waveform detail.");
    configureAccessibleMetadata(workbenchGroupsTabButton,
                                "Groups workbench tab",
                                "Shows group-scoped mixing, routing, and visibility detail.",
                                "Press to switch the workbench to group detail.");
    configureAccessibleMetadata(workbenchMacrosTabButton,
                                "Macros workbench tab",
                                "Shows project-scoped macro assignments.",
                                "Press to switch the workbench to macro editing.");
    configureAccessibleMetadata(workbenchRoutingTabButton,
                                "Routing workbench tab",
                                "Shows project-scoped FX and bus routing detail.",
                                "Press to switch the workbench to routing detail.");
    configureAccessibleMetadata(workbenchPerformanceTabButton,
                                "Performance workbench tab",
                                "Shows bank-scoped performance and trigger detail.",
                                "Press to switch the workbench to performance detail.");
    configureAccessibleMetadata(workbenchArticulationsTabButton,
                                "Articulations workbench tab",
                                "Shows project articulations and key-switch assignment.",
                                "Press to switch the workbench to articulation management.");
    workbenchWaveformTabButton.setExplicitFocusOrder(61);
    workbenchGroupsTabButton.setExplicitFocusOrder(62);
    workbenchMacrosTabButton.setExplicitFocusOrder(63);
    workbenchRoutingTabButton.setExplicitFocusOrder(64);
    workbenchPerformanceTabButton.setExplicitFocusOrder(65);
    workbenchArticulationsTabButton.setExplicitFocusOrder(66);

    configureAccessibleMetadata(waveformLabel,
                                "Workbench title",
                                "Names the active workbench surface.");
    configureAccessibleMetadata(waveformScopeLabel,
                                "Workbench scope",
                                "Shows whether the active workbench is zone-, project-, bank-, or trigger-scoped.");
    configureAccessibleMetadata(workbenchBreadcrumbLabel,
                                "Workbench breadcrumb",
                                "Shows the selection path for the active workbench.");
    configureAccessibleMetadata(waveformPreview,
                                "Waveform preview",
                                "Displays editable playback and loop regions for the selected zone.",
                                "Drag to select audio, drag a boundary handle to edit it, or hold Space or use the middle mouse button to pan. Hold Alt to bypass zero-crossing snap.");
    configureAccessibleMetadata(waveformPlaybackStartEditor,
                                "Playback start",
                                "Playback start in source frames, or a time value ending in s. Press Return or leave the field to commit.");
    configureAccessibleMetadata(waveformPlaybackEndEditor,
                                "Playback end",
                                "Exclusive playback end in source frames, or a time value ending in s. Press Return or leave the field to commit.");
    configureAccessibleMetadata(waveformPlaybackResetButton,
                                "Reset playback to source",
                                "Restores offset zero and source-end playback without changing the source file.");
    configureAccessibleMetadata(waveformSetPlaybackSelectionButton,
                                "Set playback region to selection",
                                "Commits the temporary selection as the playback region.");
    configureAccessibleMetadata(waveformPlaybackAuditionButton,
                                "Play playback region",
                                "Plays the current non-destructive playback region.");
    configureAccessibleMetadata(waveformSelectionAuditionButton,
                                "Audition waveform selection",
                                "Auditions only the temporary waveform selection without authoring it.");
    configureAccessibleMetadata(waveformSnapToggle,
                                "Zero-crossing snap",
                                "Searches a bounded window on a worker thread; hold Alt while dragging for direct frame editing.");
    configureAccessibleMetadata(waveformLoopModeSelector,
                                "Loop behavior",
                                "Turns looping off, repeats while a note is held, or repeats through release.");
    configureAccessibleMetadata(waveformLoopStartEditor,
                                "Loop start",
                                "Accepts a source frame or a time value ending in s.");
    configureAccessibleMetadata(waveformLoopEndEditor,
                                "Loop end",
                                "Accepts an exclusive source frame or a time value ending in s.");
    configureAccessibleMetadata(waveformLoopCrossfadeEditor,
                                "Loop crossfade",
                                "Equal-power loop smoothing in source frames or seconds; zero disables it. Press Return or leave the field to commit.");
    configureAccessibleMetadata(waveformSetLoopSelectionButton,
                                "Set loop to selection",
                                "Commits the temporary waveform selection as the loop region.");
    configureAccessibleMetadata(waveformLoopAuditionButton,
                                "Audition loop",
                                "Auditions the selected zone through the authoring Preview path.");
    configureAccessibleMetadata(waveformStatusLabel,
                                "Waveform preview status",
                                "Shows the current authoring preview revision state for the selected zone.");
    configureAccessibleMetadata(waveformInfoLabel,
                                "Waveform metadata",
                                "Shows source and format information for the selected waveform.");
    configureAccessibleMetadata(loopInfoLabel,
                                "Loop metadata",
                                "Shows loop state information for the selected waveform.");
    configureAccessibleMetadata(importMetricsLabel,
                                "Import responsiveness",
                                "Shows import responsiveness metrics for the current project.");
    configureAccessibleMetadata(sourceValidationLabel,
                                "Source validation",
                                "Shows project source validation status for the current project.");
    configureAccessibleMetadata(sourceValidationButton,
                                "Validate project sources",
                                "Starts or cancels project source validation in the background.",
                                "Press to validate the current project sources or cancel an active validation.");
    waveformPlaybackStartEditor.setExplicitFocusOrder(67);
    waveformPlaybackEndEditor.setExplicitFocusOrder(68);
    waveformPlaybackResetButton.setExplicitFocusOrder(69);
    waveformPlaybackAuditionButton.setExplicitFocusOrder(70);
    waveformLoopModeSelector.setExplicitFocusOrder(71);
    waveformLoopStartEditor.setExplicitFocusOrder(72);
    waveformLoopEndEditor.setExplicitFocusOrder(73);
    waveformLoopCrossfadeEditor.setExplicitFocusOrder(74);
    waveformLoopAuditionButton.setExplicitFocusOrder(75);
    waveformSnapToggle.setExplicitFocusOrder(76);
    waveformSetPlaybackSelectionButton.setExplicitFocusOrder(77);
    waveformSetLoopSelectionButton.setExplicitFocusOrder(78);
    waveformSelectionAuditionButton.setExplicitFocusOrder(79);
    sourceValidationButton.setExplicitFocusOrder(80);

    configureAccessibleMetadata(macroList,
                                "Macro list",
                                "Lists project macros in authored order with role, host exposure, and target metadata.",
                                "Use the up and down arrow keys to change the selected macro.");
    macroList.getListBox().setExplicitFocusOrder(70);
    configureAccessibleMetadata(macroAssignmentList,
                                "Assigned macro targets",
                                "Lists every target assigned to the selected macro with family, path, role, and mapping metadata.",
                                "Use the up and down arrow keys to choose the target edited by the Selected Target controls.");
    configureAccessibleMetadata(macroCreateButton,
                                "Create macro",
                                "Creates a new authored macro with sensible defaults.",
                                "Press to create a new macro in the project.");
    configureAccessibleMetadata(macroDuplicateButton,
                                "Duplicate macro",
                                "Duplicates the selected macro.",
                                "Press to duplicate the selected macro.");
    configureAccessibleMetadata(macroDeleteButton,
                                "Delete macro",
                                "Deletes the selected macro.",
                                "Press to delete the selected macro.");
    configureAccessibleMetadata(macroAssignmentAddButton,
                                "Add macro target",
                                "Adds the next supported unassigned target to the selected macro.",
                                "Press to add an available curated target through the existing macro transaction.");
    configureAccessibleMetadata(macroAssignmentRemoveButton,
                                "Remove selected macro target",
                                "Removes the selected target from the current macro.",
                                "Press to remove the selected target through the existing macro transaction.");
    configureAccessibleMetadata(macroNameEditor,
                                "Macro name",
                                "Renames the selected macro.",
                                "Type a new macro name and press Enter.");
    configureAccessibleMetadata(macroExposeToggle,
                                "Expose macro in Perform",
                                "Chooses whether the selected macro appears in the player-facing Perform surface.",
                                "Press to expose or hide the selected macro in Perform.");
    configureAccessibleMetadata(macroAssignmentSelector,
                                "Macro parameter",
                                "Chooses the parameter assigned to the selected macro.",
                                "Open the list to choose a parameter target.");
    configureAccessibleMetadata(macroRoleSelector,
                                "Macro role",
                                "Chooses the semantic role for the selected macro.",
                                "Open the list to choose a macro role.");
    configureAccessibleMetadata(macroDefaultSlider,
                                "Macro default",
                                "Adjusts the selected macro default value.",
                                "Drag the slider or enter a numeric value.");
    configureAccessibleMetadata(macroMinSlider,
                                "Macro minimum",
                                "Adjusts the selected macro minimum value.",
                                "Drag the slider or enter a numeric value.");
    configureAccessibleMetadata(macroMaxSlider,
                                "Macro maximum",
                                "Adjusts the selected macro maximum value.",
                                "Drag the slider or enter a numeric value.");
    configureAccessibleMetadata(macroMoveUpButton,
                                "Move macro up",
                                "Moves the selected macro earlier in the list.",
                                "Press to move the selected macro up.");
    configureAccessibleMetadata(macroMoveDownButton,
                                "Move macro down",
                                "Moves the selected macro later in the list.",
                                "Press to move the selected macro down.");
    macroNameEditor.setExplicitFocusOrder(71);
    macroRoleSelector.setExplicitFocusOrder(72);
    macroExposeToggle.setExplicitFocusOrder(73);
    macroDefaultSlider.setExplicitFocusOrder(74);
    macroMinSlider.setExplicitFocusOrder(75);
    macroMaxSlider.setExplicitFocusOrder(76);
    macroAssignmentList.getListBox().setExplicitFocusOrder(77);
    macroAssignmentSelector.setExplicitFocusOrder(78);
    macroAssignmentAddButton.setExplicitFocusOrder(79);
    macroAssignmentRemoveButton.setExplicitFocusOrder(80);
    macroCreateButton.setExplicitFocusOrder(81);
    macroDuplicateButton.setExplicitFocusOrder(82);
    macroMoveUpButton.setExplicitFocusOrder(83);
    macroMoveDownButton.setExplicitFocusOrder(84);
    macroDeleteButton.setExplicitFocusOrder(85);

    configureAccessibleMetadata(fxSelector,
                                "FX selector",
                                "Chooses the active FX slot for routing detail.",
                                "Open the list to choose an FX slot.");
    configureAccessibleMetadata(fxScopeSelector,
                                "DSP processing scope",
                                "Chooses whether the insert chain belongs to the current zone, current group, or instrument master.",
                                "Open the list to change scope. Changing scope never moves an existing chain.");
    configureAccessibleMetadata(fxScopeBreadcrumbLabel,
                                "DSP scope breadcrumb",
                                "Shows the canonical audio owner and current insert chain for the chosen scope.");
    configureAccessibleMetadata(fxNameEditor,
                                "FX name",
                                "Renames the selected FX slot.",
                                "Type a new name and press Enter to rename the selected insert.");
    configureAccessibleMetadata(fxTypeSelector,
                                "FX type",
                                "Chooses the effect type for the selected FX slot.",
                                "Open the list to choose an effect type.");
    configureAccessibleMetadata(fxBypassedToggle,
                                "FX bypass",
                                "Toggles bypass for the selected FX slot.",
                                "Press to toggle FX bypass.");
    configureAccessibleMetadata(fxAddButton, "Add insert", "Creates a curated Gain insert at the selected scope.",
                                "Press to create an explicit scoped insert chain when needed.");
    configureAccessibleMetadata(fxDuplicateButton, "Duplicate insert", "Duplicates the selected insert in its owner chain.",
                                "Press to duplicate the selected insert.");
    configureAccessibleMetadata(fxMoveUpButton, "Move insert up", "Moves the selected insert earlier in its chain.",
                                "Press to move the insert earlier.");
    configureAccessibleMetadata(fxMoveDownButton, "Move insert down", "Moves the selected insert later in its chain.",
                                "Press to move the insert later.");
    configureAccessibleMetadata(fxDeleteButton, "Delete insert", "Deletes the selected insert from its owner chain.",
                                "Press to delete the selected insert.");
    configureAccessibleMetadata(fxOwnerSelector, "Insert owner", "Chooses a destination owner chain for the selected insert.",
                                "Open the list to choose another owner chain.");
    configureAccessibleMetadata(fxMoveOwnerButton, "Move insert to owner", "Moves the selected insert to the chosen owner chain.",
                                "Press to move the insert to the selected owner.");
    configureAccessibleMetadata(fxParameterSelector, "FX parameter", "Chooses a descriptor-defined parameter for the selected effect.",
                                "Open the list to choose a catalog parameter.");
    configureAccessibleMetadata(fxParameterSlider, "FX parameter value", "Adjusts the selected descriptor-defined parameter.",
                                "Drag the slider or enter a numeric value.");
    configureAccessibleMetadata(fxParameterResetButton, "Reset FX parameter", "Resets the selected parameter to its catalog default.",
                                "Press to restore the descriptor default.");
    configureAccessibleMetadata(fxAssignMacroButton, "Assign FX parameter to macro", "Opens the macro assignment workflow for the selected parameter.",
                                "Press to open Macro assignments.");
    configureAccessibleMetadata(fxParameterValueLabel, "FX parameter value and default",
                                "Shows the selected parameter value, unit, and descriptor default.");
    configureAccessibleMetadata(fxSummaryLabel, "FX state summary", "Shows the selected insert's state, cost, and legacy review guidance.");
    configureAccessibleMetadata(fxDiagnosticsLabel, "DSP preview diagnostics",
                                "Shows immutable preview state, chain and graph cost, budget status, and tail capability.");
    configureAccessibleMetadata(routingBusSelector,
                                "Routing bus selector",
                                "Chooses the active routing bus.",
                                "Open the list to choose a routing bus.");
    configureAccessibleMetadata(routingInputSelector,
                                "Routing input source",
                                "Chooses the input source for the selected routing bus.",
                                "Open the list to choose an input source.");
    configureAccessibleMetadata(routingInsertOneSelector,
                                "Routing insert A",
                                "Chooses the first insert effect for the selected routing bus.",
                                "Open the list to choose the first insert.");
    configureAccessibleMetadata(routingInsertTwoSelector,
                                "Routing insert B",
                                "Chooses the second insert effect for the selected routing bus.",
                                "Open the list to choose the second insert.");
    fxScopeSelector.setExplicitFocusOrder(80);
    routingBusSelector.setExplicitFocusOrder(81);
    routingInputSelector.setExplicitFocusOrder(82);
    routingInsertOneSelector.setExplicitFocusOrder(83);
    routingInsertTwoSelector.setExplicitFocusOrder(84);
    fxSelector.setExplicitFocusOrder(85);
    fxNameEditor.setExplicitFocusOrder(86);
    fxTypeSelector.setExplicitFocusOrder(87);
    fxBypassedToggle.setExplicitFocusOrder(88);
    fxOwnerSelector.setExplicitFocusOrder(89);
    fxMoveOwnerButton.setExplicitFocusOrder(90);
    fxParameterSelector.setExplicitFocusOrder(91);
    fxParameterSlider.setExplicitFocusOrder(92);
    fxParameterResetButton.setExplicitFocusOrder(93);
    fxAssignMacroButton.setExplicitFocusOrder(94);
    fxAddButton.setExplicitFocusOrder(95);
    fxDuplicateButton.setExplicitFocusOrder(96);
    fxMoveUpButton.setExplicitFocusOrder(97);
    fxMoveDownButton.setExplicitFocusOrder(98);
    fxDeleteButton.setExplicitFocusOrder(99);
    configureAccessibleMetadata(masterGainSlider,
                                "Project master gain",
                                "Adjusts the project's top-level gain before group and zone gain are combined.",
                                "Drag the slider or enter a numeric master-gain value.");
    configureAccessibleMetadata(groupVisibilityToggle,
                                "Group workspace visibility",
                                "Toggles whether the selected group is visible on the workspace map.",
                                "Press to toggle selected-group visibility without changing audio.");
    configureAccessibleMetadata(groupGainSlider,
                                "Group gain",
                                "Adjusts the selected group's gain.",
                                "Drag the slider or enter a numeric gain value.");
    configureAccessibleMetadata(groupPanSlider,
                                "Group pan",
                                "Adjusts the selected group's pan.",
                                "Drag the slider or enter a numeric pan value.");
    configureAccessibleMetadata(groupRoutingSelector,
                                "Group routing bus",
                                "Chooses the routing bus fed by the selected group.",
                                "Open the list to choose a routing bus for the selected group.");
    configureAccessibleMetadata(groupAnchorSelector,
                                "Group audition anchor",
                                "Chooses the zone used as the selected group's audition anchor.",
                                "Open the list to choose an audition anchor zone.");
    configureAccessibleMetadata(groupDeleteButton,
                                "Delete group",
                                "Deletes the selected group when it has no remaining member zones.",
                                "Press to delete the selected empty group.");
    configureAccessibleMetadata(groupAssignZonesButton,
                                "Add selected zones to group",
                                "Assigns the current selected zone or zone-map multi-selection into the selected group.",
                                "Press to add the selected zone or selected zones to the current group.");
    configureAccessibleMetadata(groupSummaryLabel,
                                "Group summary",
                                "Summarizes the current selected-group state.");
    configureAccessibleMetadata(groupRoundRobinLabel,
                                "Group round robin summary",
                                "Summarizes round-robin state for the selected group's member zones.");
    configureAccessibleMetadata(groupRoundRobinHintLabel,
                                "Group round robin guidance",
                                "Guides how round-robin edits currently relate to the selected group.");
    configureAccessibleMetadata(groupRoundRobinToggle,
                                "Group round robin",
                                "Enables or disables all-or-nothing Round Robin for the selected group.",
                                "Every mapping in the group must have at least two exact-match zones.");
    configureAccessibleMetadata(groupRoundRobinModeSelector,
                                "Group round robin mode",
                                "Chooses cycle or random selection for the selected group's Round Robin pools.",
                                "Enable Round Robin before changing its mode.");
    masterGainSlider.setExplicitFocusOrder(71);
    groupVisibilityToggle.setExplicitFocusOrder(72);
    groupGainSlider.setExplicitFocusOrder(73);
    groupPanSlider.setExplicitFocusOrder(74);
    groupRoutingSelector.setExplicitFocusOrder(75);
    groupAnchorSelector.setExplicitFocusOrder(76);
    groupDeleteButton.setExplicitFocusOrder(77);
    groupAssignZonesButton.setExplicitFocusOrder(78);
    groupRoundRobinToggle.setExplicitFocusOrder(79);
    groupRoundRobinModeSelector.setExplicitFocusOrder(80);

    configureAccessibleMetadata(performanceBankSelector,
                                "Performance bank selector",
                                "Chooses the active performance bank.",
                                "Open the list to choose a performance bank.");
    configureAccessibleMetadata(triggerSlotSelector,
                                "Trigger slot selector",
                                "Chooses the active trigger slot within the selected bank.",
                                "Open the list to choose a trigger slot.");
    configureAccessibleMetadata(triggerEventSelector,
                                "Trigger event",
                                "Chooses the event that activates the selected trigger slot.",
                                "Open the list to choose a trigger event.");
    configureAccessibleMetadata(targetArticulationSelector,
                                "Target articulation",
                                "Chooses the articulation targeted by the selected trigger slot.",
                                "Open the list to choose an articulation.");
    configureAccessibleMetadata(phraseAssetSelector,
                                "Phrase asset",
                                "Chooses the phrase asset for the selected trigger slot.",
                                "Open the list to choose a phrase.");
    configureAccessibleMetadata(chordModeSelector,
                                "Chord rule",
                                "Chooses the chord-follow behavior for the selected phrase.",
                                "Open the list to choose a chord rule.");
    configureAccessibleMetadata(phraseImportPathEditor,
                                "MIDI phrase path",
                                "Edits the import path used for phrase import.",
                                "Type a MIDI file path for phrase import.");
    configureAccessibleMetadata(phraseImportButton,
                                "Import MIDI phrase",
                                "Imports the MIDI phrase at the current path into the selected bank.",
                                "Press to import the specified MIDI phrase.");
    configureAccessibleMetadata(performanceSummaryLabel,
                                "Performance summary",
                                "Summarizes the active performance trigger state.");
    configureAccessibleMetadata(phraseSummaryLabel,
                                "Phrase summary",
                                "Summarizes the active phrase library or phrase import state.");
    performanceBankSelector.setExplicitFocusOrder(90);
    triggerSlotSelector.setExplicitFocusOrder(91);
    triggerEventSelector.setExplicitFocusOrder(92);
    targetArticulationSelector.setExplicitFocusOrder(93);
    phraseAssetSelector.setExplicitFocusOrder(94);
    chordModeSelector.setExplicitFocusOrder(95);
    phraseImportPathEditor.setExplicitFocusOrder(96);
    phraseImportButton.setExplicitFocusOrder(97);

    configureAccessibleMetadata(articulationWorkbenchViewport,
                                "Articulation editor",
                                "Creates, reorders, and assigns articulations and their key switches.",
                                "Use the list to choose an articulation, then edit its name or key switch.");
    configureAccessibleMetadata(articulationList,
                                "Articulation list",
                                "Lists project articulations, their default state, and key switches.");
    configureAccessibleMetadata(articulationNameEditor, "Articulation name",
                                "Renames the selected articulation.", "Type a name and press Enter.");
    configureAccessibleMetadata(articulationSwitchNoteSlider, "Key-switch note picker",
                                "Chooses a MIDI note for the selected articulation key switch.",
                                "Use arrow keys or drag to choose a note from MIDI 0 through 127.");
    configureAccessibleMetadata(articulationMidiLearnButton, "Key-switch MIDI Learn",
                                "Waits for the next MIDI note to assign as the selected key switch.",
                                "Press again to cancel MIDI Learn.");
    articulationList.getListBox().setExplicitFocusOrder(100);
    articulationCreateButton.setExplicitFocusOrder(101);
    articulationDuplicateButton.setExplicitFocusOrder(102);
    articulationDefaultButton.setExplicitFocusOrder(103);
    articulationMoveUpButton.setExplicitFocusOrder(104);
    articulationMoveDownButton.setExplicitFocusOrder(105);
    articulationDeleteButton.setExplicitFocusOrder(106);
    articulationNameEditor.setExplicitFocusOrder(107);
    articulationSwitchNoteSlider.setExplicitFocusOrder(108);
    articulationClearSwitchButton.setExplicitFocusOrder(109);
    articulationMidiLearnButton.setExplicitFocusOrder(110);
    articulationDeleteReassignSelector.setExplicitFocusOrder(111);
}

void AuthoringPanel::paint(juce::Graphics& g)
{
    g.fillAll(authoringPanelBackground);

    auto bounds = getLocalBounds().toFloat().reduced(14.0f);
    g.setColour(authoringPanelCard);
    g.fillRoundedRectangle(bounds, authoring::visual::panelRadius);
    g.setColour(authoringControlOutline);
    g.drawRoundedRectangle(bounds.reduced(0.5f), authoring::visual::panelRadius,
                           authoring::visual::borderWidth);

}

void AuthoringPanel::resized()
{
    auto area = getLocalBounds().reduced(28);
    const auto shortHeightLayout = getHeight() < authoring::shortHeightBreakpoint;

    auto summaryArea = getLocalBounds().reduced(14, 28);
    summaryStrip.setBounds(summaryArea.removeFromTop(authoring::heroHeight));
    area.removeFromTop(authoring::heroHeight);

    area.removeFromTop(shortHeightLayout ? 8 : 12);
    auto toolbarRow = area.removeFromTop(28);
    zoneLabel.setBounds(toolbarRow.removeFromLeft(96));
    toolbarRow.removeFromLeft(8);
    auto previewControls = toolbarRow.removeFromRight(std::min(190, toolbarRow.getWidth()));
    previewStopButton.setBounds(previewControls.removeFromRight(std::min(72, previewControls.getWidth())));
    previewControls.removeFromRight(std::min(8, previewControls.getWidth()));
    previewEnabledToggle.setBounds(previewControls);
    toolbarRow.removeFromRight(std::min(8, toolbarRow.getWidth()));
    zoneSelector.setBounds(toolbarRow.removeFromLeft(std::min(360, toolbarRow.getWidth())));
    toolbarRow.removeFromLeft(std::min(8, toolbarRow.getWidth()));
    const auto mappingButtonWidth = toolbarRow.getWidth() < 150 ? 52 : 68;
    showMapButton.setBounds(toolbarRow.removeFromLeft(mappingButtonWidth));
    toolbarRow.removeFromLeft(4);
    // Show Zones is owned by the persistent hierarchy browser; the former
    // Map/Structure switch is no longer part of the toolbar composition.
    // The structure browser is persistent in the unified workspace, so its
    // search and diagnostics controls remain available beside the Map.
    const auto showStructureControls = true;
    structureSearchLabel.setVisible(showStructureControls);
    structureSearchEditor.setVisible(showStructureControls);
    structureSortSelector.setVisible(showStructureControls);
    structureDiagnosticFilterSelector.setVisible(showStructureControls);
    structureContextFilterSelector.setVisible(showStructureControls);
    if (showStructureControls)
    {
        toolbarRow.removeFromLeft(8);
        structureSearchLabel.setBounds(toolbarRow.removeFromLeft(34));
        toolbarRow.removeFromLeft(4);
        structureSearchEditor.setBounds(toolbarRow.removeFromLeft(std::min(190, toolbarRow.getWidth())));
        toolbarRow.removeFromLeft(6);
        structureSortSelector.setBounds(toolbarRow.removeFromLeft(std::min(132, toolbarRow.getWidth())));
        toolbarRow.removeFromLeft(6);
        structureDiagnosticFilterSelector.setBounds(toolbarRow.removeFromLeft(std::min(132, toolbarRow.getWidth())));
        toolbarRow.removeFromLeft(6);
        structureContextFilterSelector.setBounds(toolbarRow.removeFromLeft(std::min(132, toolbarRow.getWidth())));
    }

    auto layoutLabelAndField = [](juce::Rectangle<int> row,
                                  juce::Label& label,
                                  juce::Component& field,
                                  int labelWidth)
    {
        label.setBounds(row.removeFromLeft(labelWidth));
        row.removeFromLeft(6);
        field.setBounds(row);
    };

    auto layoutDualLabelAndFieldRow = [&](juce::Rectangle<int> row,
                                          juce::Label& leftLabel,
                                          juce::Component& leftField,
                                          int leftLabelWidth,
                                          juce::Label& rightLabel,
                                          juce::Component& rightField,
                                          int rightLabelWidth)
    {
        auto left = row.removeFromLeft((row.getWidth() - 12) / 2);
        row.removeFromLeft(12);
        auto right = row;
        layoutLabelAndField(left, leftLabel, leftField, leftLabelWidth);
        layoutLabelAndField(right, rightLabel, rightField, rightLabelWidth);
    };

    area.removeFromTop(shortHeightLayout ? 8 : 12);
    const auto expanded = isExpandedLayout(layoutMode);
    const auto groupWorkbenchInShortLayout = shortHeightLayout
        && workbenchState.activeTab == authoring::WorkbenchTab::groups;
    const auto routingWorkbenchInShortLayout = shortHeightLayout
        && workbenchState.activeTab == authoring::WorkbenchTab::routing;
    const auto inspectorWorkbenchInShortLayout = groupWorkbenchInShortLayout || routingWorkbenchInShortLayout;
    const auto mapGap = inspectorWorkbenchInShortLayout ? 4 : 8;

    // The tab strip is persistent chrome. The open workbench shares the main
    // surface with the Map instead of replacing it.
    auto workbenchTabArea = area.removeFromBottom(
        std::min(authoring::workbenchTabStripHeight, area.getHeight()));
    workbenchTabStrip.setBounds(workbenchTabArea);

    area.removeFromTop(std::min(mapGap, area.getHeight()));
    auto shellArea = area;
    juce::Rectangle<int> workbenchSurfaceArea;
    juce::Rectangle<int> workbenchSplitterArea;
    if (workbenchState.open)
    {
        const auto workbenchHeight = workbenchLayoutState.resolveHeight(
            shellArea.getHeight(), authoring::minimumMapVisibleHeight,
            authoring::WorkbenchLayoutState::splitterHeight);
        workbenchSurfaceArea = shellArea.removeFromBottom(
            std::min(workbenchHeight, shellArea.getHeight()));
        workbenchSplitterArea = shellArea.removeFromBottom(
            std::min(authoring::WorkbenchLayoutState::splitterHeight,
                     shellArea.getHeight()));
        workbenchSplitter.setCurrentHeight(workbenchSurfaceArea.getHeight());
    }
    workbenchRegion.setBounds(workbenchSurfaceArea);
    workbenchContentHost.setBounds(workbenchSurfaceArea);
    workbenchSplitter.setBounds(workbenchSplitterArea);

    const auto desiredInspectorWidth = expanded ? authoring::expandedInspectorPreferredWidth
                                                : authoring::compactInspectorPreferredWidth;
    const auto minimumInspectorWidth = expanded ? authoring::expandedInspectorMinWidth
                                                : authoring::compactInspectorMinWidth;
    const auto maximumInspectorWidth = expanded ? authoring::expandedInspectorMaxWidth
                                                : authoring::compactInspectorMaxWidth;
    const auto inspectorWidth = juce::jlimit(minimumInspectorWidth,
                                             std::min(maximumInspectorWidth,
                                                      std::max(minimumInspectorWidth,
                                                               shellArea.getWidth() / 2)),
                                             desiredInspectorWidth);

    auto inspector = shellArea.removeFromRight(inspectorWidth);
    shellArea.removeFromRight(std::min(14, shellArea.getWidth()));
    juce::Rectangle<int> structureBrowserArea;
    juce::Rectangle<int> structureMapSplitterArea;
    const auto baseStructureViewerHeight = expanded ? 252 : 264;
    const auto desiredGroupManagerHeight = juce::roundToInt(
        static_cast<float>(baseStructureViewerHeight) * 0.95f);
    constexpr auto minimumStackedStructureHeight = 180;
    const auto availableStackedStructureHeight = shellArea.getHeight()
        - authoring::minimumMapVisibleHeight - 8;
    const auto canStackGroupManager = availableStackedStructureHeight
        >= minimumStackedStructureHeight;
    if (canStackGroupManager)
    {
        structureBrowserArea = shellArea.removeFromTop(
            std::min(desiredGroupManagerHeight, availableStackedStructureHeight));
    }
    else
    {
        const auto defaultStructureWidth = shortHeightLayout
            ? std::min(200, std::max(188, shellArea.getWidth() - 280))
            : std::min(expanded ? 248 : 224,
                       std::max(188, shellArea.getWidth() / 3));
        const auto maximumStructureWidth = std::max(
            authoring::minimumStructureBrowserWidth,
            shellArea.getWidth() - authoring::structureMapSplitterWidth
                - authoring::minimumSideBySideMapWidth);
        const auto structureWidth = juce::jlimit(
            authoring::minimumStructureBrowserWidth,
            maximumStructureWidth,
            userStructureBrowserWidth.value_or(defaultStructureWidth));
        structureBrowserArea = shellArea.removeFromLeft(structureWidth);
        structureMapSplitterArea = shellArea.removeFromLeft(
            std::min(authoring::structureMapSplitterWidth, shellArea.getWidth()));
        structureMapSplitter.setCurrentStructureWidth(structureWidth);
    }
    auto mapSurfaceArea = shellArea;

    const auto groupWorkbenchInCompactSurface = workbenchState.activeTab
            == authoring::WorkbenchTab::groups
        && (shortHeightLayout || workbenchContentHost.getHeight() < 220);
    const auto groupWorkbenchInVeryCompactSurface = groupWorkbenchInCompactSurface
        && workbenchContentHost.getHeight() < 184;
    const auto performanceWorkbenchInCompactSurface = workbenchState.activeTab
            == authoring::WorkbenchTab::performance
        && workbenchContentHost.getHeight() < 220;

    auto toggleArea = workbenchTabStrip.getBounds().reduced(0, 4);
    workbenchToggleButton.setBounds(toggleArea.removeFromRight(110));

    auto tabArea = workbenchTabStrip.getBounds().reduced(0, 4);
    constexpr auto tabGap = 8;
    constexpr auto tabCount = 6;
    const auto desiredTabWidth = expanded ? 104 : 96;
    const auto tabWidth = juce::jlimit(54,
                                       desiredTabWidth,
                                       (tabArea.getWidth() - (tabGap * (tabCount - 1)) - 114) / tabCount);
    workbenchWaveformTabButton.setBounds(tabArea.removeFromLeft(tabWidth));
    tabArea.removeFromLeft(tabGap);
    workbenchGroupsTabButton.setBounds(tabArea.removeFromLeft(tabWidth));
    tabArea.removeFromLeft(tabGap);
    workbenchMacrosTabButton.setBounds(tabArea.removeFromLeft(tabWidth));
    tabArea.removeFromLeft(tabGap);
    workbenchRoutingTabButton.setBounds(tabArea.removeFromLeft(tabWidth));
    tabArea.removeFromLeft(tabGap);
    workbenchPerformanceTabButton.setBounds(tabArea.removeFromLeft(tabWidth));
    tabArea.removeFromLeft(tabGap);
    workbenchArticulationsTabButton.setBounds(tabArea.removeFromLeft(tabWidth + (expanded ? 8 : 2)));

    const auto waveformWorkbenchInShortLayout = workbenchState.activeTab
            == authoring::WorkbenchTab::waveform
        && (shortHeightLayout || workbenchContentHost.getHeight() < 220);
    auto workbenchEditorArea = workbenchContentHost.getBounds().reduced(
        12, (inspectorWorkbenchInShortLayout || groupWorkbenchInCompactSurface
             || waveformWorkbenchInShortLayout || performanceWorkbenchInCompactSurface) ? 6 : 10);
    if (workbenchState.activeTab == authoring::WorkbenchTab::waveform)
    {
        auto headingRow = workbenchEditorArea.removeFromTop(22);
        waveformLabel.setBounds(headingRow.removeFromLeft(std::min(148, headingRow.getWidth())));
        headingRow.removeFromLeft(std::min(8, headingRow.getWidth()));
        waveformScopeLabel.setBounds(headingRow.removeFromLeft(std::min(196, headingRow.getWidth() / 2)));
        headingRow.removeFromLeft(std::min(8, headingRow.getWidth()));
        workbenchBreadcrumbLabel.setBounds(headingRow);
        workbenchEditorArea.removeFromTop(std::min(4, workbenchEditorArea.getHeight()));
    }
    else if (workbenchState.activeTab == authoring::WorkbenchTab::groups)
    {
        auto headingRow = workbenchEditorArea.removeFromTop(groupWorkbenchInCompactSurface ? 20 : 22);
        waveformLabel.setBounds(headingRow.removeFromLeft(std::min(160, headingRow.getWidth())));
        waveformScopeLabel.setBounds(headingRow);
        workbenchEditorArea.removeFromTop(groupWorkbenchInCompactSurface ? 0 : 1);
        workbenchBreadcrumbLabel.setBounds(workbenchEditorArea.removeFromTop(14));
        workbenchEditorArea.removeFromTop(groupWorkbenchInCompactSurface ? 2 : 3);
    }
    else if (performanceWorkbenchInCompactSurface)
    {
        waveformLabel.setBounds(workbenchEditorArea.removeFromTop(20));
        waveformScopeLabel.setBounds(workbenchEditorArea.removeFromTop(12));
        workbenchBreadcrumbLabel.setBounds(workbenchEditorArea.removeFromTop(12));
    }
    else
    {
        waveformLabel.setBounds(workbenchEditorArea.removeFromTop(22));
        workbenchEditorArea.removeFromTop(2);
        waveformScopeLabel.setBounds(workbenchEditorArea.removeFromTop(14));
        workbenchEditorArea.removeFromTop(1);
        workbenchBreadcrumbLabel.setBounds(workbenchEditorArea.removeFromTop(14));
        workbenchEditorArea.removeFromTop(3);
    }

    if (workbenchState.activeTab == authoring::WorkbenchTab::waveform)
    {
        constexpr auto paneGap = 10;
        constexpr auto rowGap = 3;
        const auto footerHeight = waveformWorkbenchInShortLayout ? 20 : 34;

        auto footer = workbenchEditorArea.removeFromBottom(
            std::min(footerHeight, workbenchEditorArea.getHeight()));
        workbenchEditorArea.removeFromBottom(std::min(waveformWorkbenchInShortLayout ? 2 : 4,
                                                      workbenchEditorArea.getHeight()));
        auto body = workbenchEditorArea;

        const auto minimumWaveformWidth = body.getWidth() >= 650 ? 200 : 40;
        const auto maximumControlPaneWidth = std::max(
            0, body.getWidth() - minimumWaveformWidth - paneGap);
        const auto controlPaneWidth = std::min(
            maximumControlPaneWidth,
            std::min(560, std::max(360, maximumControlPaneWidth)));
        auto controlPane = body.removeFromRight(controlPaneWidth);
        body.removeFromRight(std::min(paneGap, body.getWidth()));
        waveformPreview.setBounds(body);

        auto layoutWeightedRow = [](juce::Rectangle<int> row,
                                    const std::initializer_list<std::pair<juce::Component*, int>> items)
        {
            constexpr auto itemGap = 4;
            const auto gapPixels = itemGap * std::max(0, static_cast<int>(items.size()) - 1);
            const auto availableWidth = std::max(0, row.getWidth() - gapPixels);
            auto totalWeight = 0;
            for (const auto& item : items)
                totalWeight += item.second;

            auto remainingWeight = totalWeight;
            auto remainingWidth = availableWidth;
            auto index = 0;
            for (const auto& item : items)
            {
                const auto isLast = index == static_cast<int>(items.size()) - 1;
                const auto width = isLast || remainingWeight <= 0
                    ? remainingWidth
                    : std::max(0, remainingWidth * item.second / remainingWeight);
                item.first->setBounds(row.removeFromLeft(width));
                remainingWidth -= width;
                remainingWeight -= item.second;
                if (!isLast)
                    row.removeFromLeft(std::min(itemGap, row.getWidth()));
                ++index;
            }
        };

        auto guidanceRow = controlPane.removeFromBottom(std::min(13, controlPane.getHeight()));
        controlPane.removeFromBottom(std::min(3, controlPane.getHeight()));
        waveformLoopGuidanceLabel.setBounds(guidanceRow);
        const auto controlRowHeight = std::max(
            1, (controlPane.getHeight() - (rowGap * 3)) / 4);
        auto takeControlRow = [&](const bool addGap)
        {
            auto row = controlPane.removeFromTop(
                std::min(controlRowHeight, controlPane.getHeight()));
            if (addGap)
                controlPane.removeFromTop(std::min(rowGap, controlPane.getHeight()));
            return row;
        };

        layoutWeightedRow(takeControlRow(true),
        {
            { &waveformPlaybackStartLabel, waveformWorkbenchInShortLayout ? 48 : 76 },
            { &waveformPlaybackStartEditor, 72 },
            { &waveformPlaybackEndLabel, 28 },
            { &waveformPlaybackEndEditor, 72 },
            { &waveformPlaybackResetButton, 70 },
            { &waveformPlaybackAuditionButton, 88 }
        });
        layoutWeightedRow(takeControlRow(true),
        {
            { &waveformLoopModeSelector, 112 },
            { &waveformLoopStartLabel, waveformWorkbenchInShortLayout ? 42 : 64 },
            { &waveformLoopStartEditor, 68 },
            { &waveformLoopEndLabel, 28 },
            { &waveformLoopEndEditor, 68 }
        });
        layoutWeightedRow(takeControlRow(true),
        {
            { &waveformLoopCrossfadeLabel, 74 },
            { &waveformLoopCrossfadeEditor, 86 },
            { &waveformLoopAuditionButton, 104 },
            { &waveformSnapToggle, 104 }
        });
        layoutWeightedRow(takeControlRow(false),
        {
            { &waveformSetPlaybackSelectionButton, 150 },
            { &waveformSetLoopSelectionButton, 128 },
            { &waveformSelectionAuditionButton, 122 }
        });

        auto footerControls = footer.removeFromRight(controlPaneWidth);
        footer.removeFromRight(std::min(paneGap, footer.getWidth()));
        auto waveformFooter = footer;
        waveformStatusLabel.setBounds(waveformFooter.removeFromTop(
            std::min(17, waveformFooter.getHeight())));
        waveformInfoLabel.setBounds(waveformFooter);

        auto contextRow = footerControls.removeFromTop(
            std::min(17, footerControls.getHeight()));
        loopInfoLabel.setBounds(contextRow.removeFromLeft(
            std::min(contextRow.getWidth() * 3 / 5, contextRow.getWidth())));
        contextRow.removeFromLeft(std::min(6, contextRow.getWidth()));
        importMetricsLabel.setBounds(contextRow);
        auto validationRow = footerControls;
        sourceValidationButton.setBounds(validationRow.removeFromRight(
            std::min(122, validationRow.getWidth())));
        validationRow.removeFromRight(std::min(6, validationRow.getWidth()));
        sourceValidationLabel.setBounds(validationRow);
    }

    if (workbenchState.activeTab == authoring::WorkbenchTab::groups)
    {
        const auto fieldRowHeight = groupWorkbenchInVeryCompactSurface
            ? 18 : (groupWorkbenchInCompactSurface ? 21 : (expanded ? 26 : 24));
        const auto summaryRowHeight = groupWorkbenchInVeryCompactSurface
            ? 12 : (groupWorkbenchInCompactSurface ? 16 : (expanded ? 20 : 18));
        const auto actionRowHeight = groupWorkbenchInVeryCompactSurface
            ? 20 : (groupWorkbenchInCompactSurface ? 24 : (expanded ? 30 : 28));
        const auto fieldGap = groupWorkbenchInVeryCompactSurface ? 1 : 2;

        auto row = workbenchEditorArea.removeFromTop(fieldRowHeight);
        layoutLabelAndField(row, groupNameLabel, groupNameEditor, 92);
        workbenchEditorArea.removeFromTop(fieldGap);

        row = workbenchEditorArea.removeFromTop(fieldRowHeight);
        layoutDualLabelAndFieldRow(row,
                                   masterGainLabel,
                                   masterGainSlider,
                                   92,
                                   groupVisibilityLabel,
                                   groupVisibilityToggle,
                                   74);
        workbenchEditorArea.removeFromTop(fieldGap);

        row = workbenchEditorArea.removeFromTop(fieldRowHeight);
        layoutDualLabelAndFieldRow(row,
                                   groupGainLabel,
                                   groupGainSlider,
                                   42,
                                   groupPanLabel,
                                   groupPanSlider,
                                   36);
        workbenchEditorArea.removeFromTop(fieldGap);

        row = workbenchEditorArea.removeFromTop(fieldRowHeight);
        layoutDualLabelAndFieldRow(row,
                                   groupRoutingLabel,
                                   groupRoutingSelector,
                                   84,
                                   groupAnchorLabel,
                                   groupAnchorSelector,
                                   96);
        workbenchEditorArea.removeFromTop(fieldGap);
        auto summaryRow = workbenchEditorArea.removeFromTop(summaryRowHeight);
        auto groupSummaryArea = summaryRow.removeFromLeft((summaryRow.getWidth() - 12) / 2);
        summaryRow.removeFromLeft(12);
        groupSummaryLabel.setBounds(groupSummaryArea);
        groupRoundRobinLabel.setBounds(summaryRow);
        groupRoundRobinHintLabel.setBounds({});
        workbenchEditorArea.removeFromTop(groupWorkbenchInVeryCompactSurface
                                              ? 2
                                              : (groupWorkbenchInCompactSurface ? 3 : (expanded ? 6 : 4)));

        auto actionRow = workbenchEditorArea.removeFromTop(actionRowHeight);
        constexpr auto actionGap = 8;
        const auto deleteButtonWidth = expanded ? 136 : 128;
        auto deleteArea = actionRow.removeFromRight(std::min(deleteButtonWidth, actionRow.getWidth()));
        actionRow.removeFromRight(std::min(actionGap, actionRow.getWidth()));
        auto toggleArea = actionRow.removeFromLeft(std::max(1, (actionRow.getWidth() - actionGap) / 2));
        actionRow.removeFromLeft(std::min(actionGap, actionRow.getWidth()));
        groupRoundRobinToggle.setBounds(toggleArea);
        groupRoundRobinModeSelector.setBounds(actionRow);
        groupDeleteButton.setBounds(deleteArea);
    }
    else if (workbenchState.activeTab == authoring::WorkbenchTab::macros)
    {
        macroWorkbenchViewport.setBounds(workbenchEditorArea);
        const auto macroContentWidth = std::max(420,
                                                macroWorkbenchViewport.getWidth()
                                                    - macroWorkbenchViewport.getScrollBarThickness());
        const auto macroContentHeight = authoring::MacroWorkbenchView::preferredContentHeight(
            macroContentWidth, macroWorkbenchViewport.getHeight(), shortHeightLayout);
        macroWorkbenchContent.setSize(macroContentWidth, macroContentHeight);
    }
    else if (workbenchState.activeTab == authoring::WorkbenchTab::routing)
    {
        routingWorkbenchViewport.setBounds(workbenchEditorArea);
        const auto routingContentWidth = std::max(320,
                                                  routingWorkbenchViewport.getWidth()
                                                      - routingWorkbenchViewport.getScrollBarThickness());
        const auto routingContentHeight = authoring::RoutingWorkbenchView::preferredContentHeight(
            routingContentWidth, routingWorkbenchViewport.getHeight(), shortHeightLayout);
        routingWorkbenchContent.setSize(routingContentWidth, routingContentHeight);
    }
    else if (workbenchState.activeTab == authoring::WorkbenchTab::performance)
    {
        if (workbenchEditorArea.getWidth() < 420)
        {
            performanceBankSelector.setBounds(workbenchEditorArea.removeFromTop(28));
            workbenchEditorArea.removeFromTop(4);
            triggerSlotSelector.setBounds(workbenchEditorArea.removeFromTop(28));
        }
        else
        {
            auto selectorRow = workbenchEditorArea.removeFromTop(28);
            performanceBankSelector.setBounds(selectorRow.removeFromLeft(280));
            selectorRow.removeFromLeft(10);
            triggerSlotSelector.setBounds(selectorRow.removeFromLeft(280));
        }

        workbenchEditorArea.removeFromTop(4);
        auto row = workbenchEditorArea.removeFromTop(28);
        layoutDualLabelAndFieldRow(row,
                                   triggerEventLabel,
                                   triggerEventSelector,
                                   52,
                                   targetArticulationLabel,
                                   targetArticulationSelector,
                                   72);
        workbenchEditorArea.removeFromTop(4);

        row = workbenchEditorArea.removeFromTop(28);
        layoutDualLabelAndFieldRow(row,
                                   phraseAssetLabel,
                                   phraseAssetSelector,
                                   48,
                                   chordModeLabel,
                                   chordModeSelector,
                                   72);
        workbenchEditorArea.removeFromTop(4);

        row = workbenchEditorArea.removeFromTop(28);
        auto buttonArea = row.removeFromRight(180);
        row.removeFromRight(10);
        layoutLabelAndField(row, phraseImportPathLabel, phraseImportPathEditor, 56);
        phraseImportButton.setBounds(buttonArea);
        workbenchEditorArea.removeFromTop(4);
        row = workbenchEditorArea.removeFromTop(26);
        roundRobinResetLabel.setBounds(row.removeFromLeft(58));
        roundRobinResetSelector.setBounds(row.removeFromLeft(std::max(96, row.getWidth() / 4)));
        row.removeFromLeft(6);
        roundRobinResetEventSelector.setBounds(row.removeFromLeft(std::max(110, row.getWidth() / 3)));
        row.removeFromLeft(6);
        roundRobinResetTargetSelector.setBounds(row.removeFromLeft(std::max(90, row.getWidth() / 3)));
        row.removeFromLeft(6);
        roundRobinResetAddButton.setBounds(row.removeFromLeft(std::min(88, row.getWidth())));
        row.removeFromLeft(4);
        roundRobinResetDeleteButton.setBounds(row);
        if (expanded)
        {
            workbenchEditorArea.removeFromTop(6);
            performanceSummaryLabel.setBounds(workbenchEditorArea.removeFromTop(20));
            workbenchEditorArea.removeFromTop(4);
            phraseSummaryLabel.setBounds(workbenchEditorArea.removeFromTop(24));
            workbenchEditorArea.removeFromTop(2);
            roundRobinResetSummaryLabel.setBounds(workbenchEditorArea.removeFromTop(18));
        }
    }
    else if (workbenchState.activeTab == authoring::WorkbenchTab::articulations)
    {
        articulationWorkbenchViewport.setBounds(workbenchEditorArea);
        const auto articulationContentWidth = std::max(420,
            articulationWorkbenchViewport.getWidth() - articulationWorkbenchViewport.getScrollBarThickness());
        articulationWorkbenchContent.setSize(articulationContentWidth, expanded ? 308 : 286);
        auto editorArea = articulationWorkbenchContent.getLocalBounds();
        auto actionRow = editorArea.removeFromTop(26);
        constexpr auto actionGap = 5;
        const auto actionWidth = std::max(48, (actionRow.getWidth() - actionGap * 5) / 6);
        articulationCreateButton.setBounds(actionRow.removeFromLeft(actionWidth)); actionRow.removeFromLeft(actionGap);
        articulationDuplicateButton.setBounds(actionRow.removeFromLeft(actionWidth)); actionRow.removeFromLeft(actionGap);
        articulationDefaultButton.setBounds(actionRow.removeFromLeft(actionWidth)); actionRow.removeFromLeft(actionGap);
        articulationMoveUpButton.setBounds(actionRow.removeFromLeft(actionWidth)); actionRow.removeFromLeft(actionGap);
        articulationMoveDownButton.setBounds(actionRow.removeFromLeft(actionWidth)); actionRow.removeFromLeft(actionGap);
        articulationDeleteButton.setBounds(actionRow);
        editorArea.removeFromTop(4);
        articulationList.setBounds(editorArea.removeFromTop(42));
        editorArea.removeFromTop(4);
        auto row = editorArea.removeFromTop(26);
        layoutLabelAndField(row, articulationNameLabel, articulationNameEditor, 48);
        editorArea.removeFromTop(4);
        row = editorArea.removeFromTop(28);
        auto noteLabelArea = row.removeFromLeft(76);
        articulationSwitchNoteLabel.setBounds(noteLabelArea.removeFromLeft(70));
        articulationSwitchNoteSlider.setBounds(row.removeFromLeft(std::max(80, row.getWidth() / 2)));
        row.removeFromLeft(6);
        articulationSwitchNoteValueLabel.setBounds(row);
        editorArea.removeFromTop(4);
        row = editorArea.removeFromTop(26);
        articulationClearSwitchButton.setBounds(row.removeFromLeft(std::max(110, row.getWidth() / 3)));
        row.removeFromLeft(6);
        articulationMidiLearnButton.setBounds(row.removeFromLeft(std::max(110, row.getWidth() / 3)));
        row.removeFromLeft(6);
        articulationStatusLabel.setBounds(row);
        editorArea.removeFromTop(4);
        row = editorArea.removeFromTop(26);
        layoutLabelAndField(row, articulationDeleteReassignLabel, articulationDeleteReassignSelector, 128);
        editorArea.removeFromTop(4);
        const auto keyboardColumns = 18;
        const auto keyGap = 3;
        const auto keyWidth = std::max(20, (editorArea.getWidth() - (keyboardColumns - 1) * keyGap) / keyboardColumns);
        for (std::size_t index = 0; index < articulationKeyButtons.size(); ++index)
        {
            const auto column = static_cast<int>(index) % keyboardColumns;
            const auto rowIndex = static_cast<int>(index) / keyboardColumns;
            auto keyBounds = juce::Rectangle<int>(editorArea.getX() + column * (keyWidth + keyGap),
                                                  editorArea.getY() + rowIndex * 28,
                                                  keyWidth,
                                                  24);
            articulationKeyButtons[index]->setBounds(keyBounds);
        }
    }

    auto layoutGroupManager = [&](juce::Rectangle<int> groupManagerArea)
    {
        const auto groupManagerOrigin = groupManagerArea;
        layerSectionLabel.setBounds(groupManagerArea.removeFromTop(20));
        groupManagerArea.removeFromTop(2);
        auto layerRow = groupManagerArea.removeFromTop(34);
        auto layerButtons = layerRow.removeFromRight(std::min(250, layerRow.getWidth()));
        layerMoveDownButton.setBounds(layerButtons.removeFromRight(58));
        layerButtons.removeFromRight(4);
        layerMoveUpButton.setBounds(layerButtons.removeFromRight(58));
        layerButtons.removeFromRight(4);
        layerAssignGroupsButton.setBounds(layerButtons.removeFromRight(92));
        layerButtons.removeFromRight(4);
        layerCreateButton.setBounds(layerButtons);
        layerList.setBounds(layerRow);
        groupManagerArea.removeFromTop(4);
        auto layerEditorRow = groupManagerArea.removeFromTop(28);
        layerNameLabel.setBounds(layerEditorRow.removeFromLeft(72));
        layerEditorRow.removeFromLeft(4);
        layerNameEditor.setBounds(layerEditorRow.removeFromLeft(150));
        layerEditorRow.removeFromLeft(6);
        layerGainSlider.setBounds(layerEditorRow.removeFromLeft(120));
        layerEditorRow.removeFromLeft(6);
        layerPanSlider.setBounds(layerEditorRow.removeFromLeft(120));
        layerEditorRow.removeFromLeft(6);
        layerVisibilityToggle.setBounds(layerEditorRow);
        auto layerCrossfadeRow = groupManagerArea.removeFromTop(28);
        layerCrossfadeSourceSelector.setBounds(layerCrossfadeRow.removeFromLeft(120));
        layerCrossfadeRow.removeFromLeft(6);
        layerCrossfadeControllerSlider.setBounds(layerCrossfadeRow.removeFromLeft(120));
        layerCrossfadeRow.removeFromLeft(6);
        layerRoutingSelector.setBounds(layerCrossfadeRow.removeFromLeft(120));
        layerCrossfadeRow.removeFromLeft(6);
        layerAnchorSelector.setBounds(layerCrossfadeRow);
        auto layerCrossfadeRangeRow = groupManagerArea.removeFromTop(28);
        layerCrossfadeLowSlider.setBounds(layerCrossfadeRangeRow.removeFromLeft(120));
        layerCrossfadeRangeRow.removeFromLeft(6);
        layerCrossfadeHighSlider.setBounds(layerCrossfadeRangeRow.removeFromLeft(120));
        layerCrossfadeRangeRow.removeFromLeft(6);
        layerCrossfadeDirectionSelector.setBounds(layerCrossfadeRangeRow);
        groupManagerArea.removeFromTop(4);
        groupSectionLabel.setBounds(groupManagerArea.removeFromTop(22));
        groupManagerArea.removeFromTop(4);
        const auto wrapGroupManagerButtons = groupManagerArea.getWidth() < 360;
        auto managerTopArea = groupManagerArea.removeFromTop(wrapGroupManagerButtons ? 60 : 28);
        auto managerTopRow = managerTopArea.removeFromTop(28);
        managerTopArea.removeFromTop(wrapGroupManagerButtons ? 4 : 0);
        auto managerSecondaryRow = wrapGroupManagerButtons
            ? managerTopArea.removeFromTop(28)
            : juce::Rectangle<int> {};
        const auto showVisibilityHint = !shortHeightLayout && groupManagerArea.getHeight() >= 70;
        auto managerActionRow = groupManagerArea.removeFromTop(showVisibilityHint
                                                                   ? std::max(40, groupManagerArea.getHeight() - 18)
                                                                   : groupManagerArea.getHeight());
        groupVisibilityHintLabel.setBounds(showVisibilityHint
                                               ? groupManagerArea.removeFromTop(18)
                                               : juce::Rectangle<int> {});

        if (wrapGroupManagerButtons)
        {
            auto primaryButtons = managerTopRow;
            const auto halfWidth = std::max(1, (primaryButtons.getWidth() - 8) / 2);
            groupCreateButton.setBounds(primaryButtons.removeFromLeft(halfWidth));
            primaryButtons.removeFromLeft(std::min(8, primaryButtons.getWidth()));
            groupPreviewAnchorButton.setBounds(primaryButtons);
            groupAssignZonesButton.setBounds(managerSecondaryRow);
        }
        else
        {
            auto managerButtons = managerTopRow.removeFromRight(std::min(364, managerTopRow.getWidth()));
            groupPreviewAnchorButton.setBounds(managerButtons.removeFromRight(118));
            managerButtons.removeFromRight(std::min(8, managerButtons.getWidth()));
            groupAssignZonesButton.setBounds(managerButtons.removeFromRight(138));
            managerButtons.removeFromRight(std::min(8, managerButtons.getWidth()));
            groupCreateButton.setBounds(managerButtons);
        }

        auto listArea = managerActionRow.removeFromLeft(std::max(160, managerActionRow.getWidth() - 116));
        groupList.setBounds(listArea);
        managerActionRow.removeFromLeft(8);
        auto managerButtonColumn = managerActionRow.removeFromLeft(108);
        groupVisibilityButton.setBounds(managerButtonColumn.removeFromTop(20));
        managerButtonColumn.removeFromTop(8);
        groupMoveUpButton.setBounds(managerButtonColumn.removeFromTop(20));
        managerButtonColumn.removeFromTop(8);
        groupMoveDownButton.setBounds(managerButtonColumn.removeFromTop(20));

        // In a narrow, short host the layer editor can consume the entire
        // vertical budget before the group action row is reached. Keep the
        // primary group actions reachable rather than allowing zero-sized
        // controls to disappear from the authoring surface.
        if (groupCreateButton.getBounds().isEmpty() && !groupManagerOrigin.isEmpty())
        {
            auto rescueArea = groupManagerOrigin;
            auto rescueRow = rescueArea.removeFromTop(std::min(28, rescueArea.getHeight()));
            auto rescueButtons = rescueRow;
            const auto rescueWidth = std::max(1, (rescueButtons.getWidth() - 8) / 2);
            groupCreateButton.setBounds(rescueButtons.removeFromLeft(rescueWidth));
            rescueButtons.removeFromLeft(std::min(8, rescueButtons.getWidth()));
            groupPreviewAnchorButton.setBounds(rescueButtons);
        }
        if (groupList.getBounds().isEmpty() && !groupManagerOrigin.isEmpty())
        {
            auto rescueArea = groupManagerOrigin;
            rescueArea.removeFromTop(std::min(32, rescueArea.getHeight()));
            auto rescueList = rescueArea.removeFromTop(std::min(64, rescueArea.getHeight()));
            groupList.setBounds(rescueList);
            auto rescueControls = rescueArea;
            groupVisibilityButton.setBounds(rescueControls.removeFromTop(std::min(20, rescueControls.getHeight())));
            rescueControls.removeFromTop(std::min(8, rescueControls.getHeight()));
            groupMoveUpButton.setBounds(rescueControls.removeFromTop(std::min(20, rescueControls.getHeight())));
            rescueControls.removeFromTop(std::min(8, rescueControls.getHeight()));
            groupMoveDownButton.setBounds(rescueControls.removeFromTop(std::min(20, rescueControls.getHeight())));
        }
        if (groupAssignZonesButton.getBounds().isEmpty() && !groupManagerOrigin.isEmpty())
        {
            auto rescueArea = groupManagerOrigin;
            rescueArea.removeFromTop(std::min(28, rescueArea.getHeight()));
            groupAssignZonesButton.setBounds(rescueArea.removeFromTop(std::min(28, rescueArea.getHeight())));
        }
    };

    layoutGroupManager(structureBrowserArea);

    zoneMap.setBounds(mapSurfaceArea);
    structureBrowser.setBounds(structureBrowserArea);
    const auto showingMap = structureViewState.isMapPaneVisible();
    const auto showingStructureMapSplitter = !canStackGroupManager && showingMap;
    structureMapSplitter.setBounds(showingStructureMapSplitter
                                       ? structureMapSplitterArea
                                       : juce::Rectangle<int> {});
    structureMapSplitter.setVisible(showingStructureMapSplitter);
    if (!showingMap)
    {
        // When Show Map is off, reclaim the Map rectangle instead of leaving
        // an empty placeholder. The inspector becomes the wide authoring
        // surface while the hierarchy remains visible for navigation.
        structureInspector.setBounds(mapSurfaceArea.getUnion(inspector));
        zoneMap.setBounds({});
    }
    else
    {
        const auto showingZoneEditor = structureSelection.getKind() == authoring::StructureSelectionKind::zone;
        structureInspector.setBounds(showingZoneEditor ? juce::Rectangle<int> {} : inspector);
        zoneMappingEditor.setBounds(showingZoneEditor ? inspector : juce::Rectangle<int> {});
    }
}

void AuthoringPanel::reloadFromSession()
{
    refreshFromSession();
}

void AuthoringPanel::restoreDefaultView()
{
    userStructureBrowserWidth.reset();
    workbenchState = {};
    workbenchLayoutState = {};
    structureViewState.setMapPaneVisible(true);
    zoneMap.fitAllVisible();
    refreshInspectorVisibility();
    resized();
}

void AuthoringPanel::refreshNow()
{
    const auto documentRevision = authoringSession.getDocumentState().revision;
    const auto selectionRevision = authoringSession.getWorkspaceSelectionRevision();
    if (!hasObservedSessionRevisions || documentRevision != observedDocumentRevision)
    {
        refreshFromSession();
        return;
    }
    if (selectionRevision != observedWorkspaceSelectionRevision)
    {
        refreshSelectionFromSession();
        return;
    }

    selectionSummaryViewModel = buildSelectionSummaryViewModel();
    summaryStrip.setViewModel(selectionSummaryViewModel);
    refreshWaveformWorkbenchContent();
}

authoring::SelectionSummaryViewModel AuthoringPanel::buildSelectionSummaryViewModel() const
{
    authoring::SelectionSummaryViewModel viewModel;
    const auto& documentState = authoringSession.getDocumentState();
    viewModel.statusText = std::string(documentState.dirty ? "Dirty" : "Clean")
        + " | Revision " + std::to_string(documentState.revision);
    viewModel.sourceText = "Sample source: none";
    viewModel.articulationText = "Articulation: none";
    viewModel.playbackText = "Preview: unavailable | Draft: unavailable | Publish: unavailable";
    viewModel.canUndo = documentState.undoDepth > 0;
    viewModel.canRedo = documentState.redoDepth > 0;

    if (draftPlaybackStatusProvider)
    {
        const auto playbackStatus = draftPlaybackStatusProvider();
        const auto playbackGuidance = buildDraftPlaybackGuidance(authoringSession, playbackStatus);
        viewModel.playbackText = "Preview: " + playbackStatus.preview.state
            + " | Draft r" + std::to_string(playbackStatus.draftRevision)
            + " | Publish: " + playbackStatus.performance.state;
        viewModel.canPrepareDraftPlayback = playbackGuidance.canPrepareDraftPlayback;
        viewModel.canPublishDraftPlayback = playbackGuidance.canPublishDraftPlayback;
        if (!playbackGuidance.statusText.empty())
            viewModel.statusText += " | " + playbackGuidance.statusText;
    }

    if (authoringPreviewStatusProvider)
    {
        const auto previewStatus = authoringPreviewStatusProvider();
        if (previewStatus.available)
        {
            viewModel.playbackText += " | authoring preview r" + std::to_string(previewStatus.draftRevision)
                + " (" + previewStatus.stateLabel + ")";

            if (!previewStatus.failureState.empty())
            {
                viewModel.statusText += " | preview blocked: " + previewStatus.failureState;

                if (!previewStatus.blockingPrerequisite.empty())
                    viewModel.statusText += " | fix: " + previewStatus.blockingPrerequisite;
            }
        }
    }

    if (const auto zone = authoringSession.getSelectedZone(); zone.has_value())
    {
        viewModel.sourceText = "Sample source: " + zone->sampleSourceId;
        viewModel.articulationText = "Articulation: " + zone->articulationId;
        viewModel.canPreview = true;
        viewModel.canRestoreRootKey = true;
    }

    const auto mapSelectionCount = getZoneMapSelectionCount();
    if (mapSelectionCount > 1)
    {
        viewModel.statusText += " | map selection=" + std::to_string(mapSelectionCount);
        viewModel.playbackText += " | inspector edits primary zone";
    }

    return viewModel;
}

authoring::ZoneFieldValuesViewModel AuthoringPanel::buildZoneFieldValuesViewModel() const
{
    authoring::ZoneFieldValuesViewModel viewModel;
    viewModel.emptyStateText = "Select a zone to edit mapping values.";

    const auto& project = authoringSession.getProject();
    if (const auto zone = authoringSession.getSelectedZone(); zone.has_value())
    {
        viewModel.hasSelection = true;
        viewModel.rootKey = zone->rootKey;
        viewModel.keyLow = zone->keyLow;
        viewModel.keyHigh = zone->keyHigh;
        viewModel.velocityLow = zone->velocityLow;
        viewModel.velocityHigh = zone->velocityHigh;
        viewModel.gainDb = zone->gainDb;
        viewModel.pan = zone->pan;
        viewModel.loopEnabled = zone->loopEnabled;
        viewModel.loopMode = zone->loopMode;
        viewModel.sampleEndFrame = zone->sampleEndFrame;
        viewModel.releaseSeconds = zone->releaseSeconds;
        viewModel.releaseShape = zone->releaseShape;
        viewModel.triggerMode = zone->triggerMode;
        viewModel.performanceEvent = zone->performance.event;
        viewModel.performanceSustain = zone->performance.sustain;
        viewModel.performancePitchSource = zone->performance.pitchSource;
        viewModel.exclusiveGroupId = zone->exclusiveGroupId;
        viewModel.exclusiveTargetGroupId = zone->exclusiveTargetGroupIds.empty()
            ? std::string {} : zone->exclusiveTargetGroupIds.front();
        viewModel.chokeReleaseSeconds = zone->chokeReleaseSeconds.value_or(0.0);
        for (const auto& candidate : project.authoring.zones)
        {
            if (candidate.exclusiveGroupId.empty()
                || std::find(viewModel.exclusiveGroupIds.begin(), viewModel.exclusiveGroupIds.end(),
                             candidate.exclusiveGroupId) != viewModel.exclusiveGroupIds.end())
                continue;
            viewModel.exclusiveGroupIds.push_back(candidate.exclusiveGroupId);
        }
        viewModel.articulationId = zone->articulationId;
        viewModel.hasMultipleZoneSelection = zoneMapSelectedZoneIds.size() > 1;
        viewModel.articulationIds.reserve(project.authoring.articulations.size());
        for (const auto& articulation : project.authoring.articulations)
            viewModel.articulationIds.push_back(articulation.id);

        const auto compatibleUnpooledZoneCount = countCompatibleUnpooledRoundRobinZones(project, *zone);
        viewModel.roundRobinEnabled = zone->roundRobin.has_value();
        viewModel.canCreateRoundRobinPool = true;
        viewModel.canAddCompatibleZonesToRoundRobinPool = compatibleUnpooledZoneCount > 0;
        viewModel.canNormalizeRoundRobinPool = zone->roundRobin.has_value();
        viewModel.canRemoveZoneFromRoundRobinPool = zone->roundRobin.has_value();

        if (zone->roundRobin.has_value())
        {
            const auto poolMemberCount = countRoundRobinPoolMembers(project, *zone->roundRobin);
            viewModel.roundRobinPoolText = "Pool: " + zone->roundRobin->poolId;
            viewModel.roundRobinSlotText = "Slot: "
                + std::to_string(zone->roundRobin->slotIndex)
                + " of " + std::to_string(zone->roundRobin->slotCount)
                + " | Mode: sequential";
            viewModel.roundRobinHintText = compatibleUnpooledZoneCount > 0
                ? "Pool members: " + std::to_string(poolMemberCount)
                    + " | Unpooled matches: " + std::to_string(compatibleUnpooledZoneCount)
                : "Pool members: " + std::to_string(poolMemberCount)
                    + " | No unpooled matching zones";
            viewModel.previewAdvancesRoundRobin = poolMemberCount > 1;
        }
        else
        {
            viewModel.roundRobinPoolText = "Pool: none";
            viewModel.roundRobinSlotText = compatibleUnpooledZoneCount > 0
                ? "Slot: standalone | Mode: sequential when grouped"
                : "Slot: standalone";
            viewModel.roundRobinHintText = compatibleUnpooledZoneCount > 0
                ? "Compatible unpooled zones: " + std::to_string(compatibleUnpooledZoneCount)
                : "No compatible unpooled zones available for grouping";
        }

        const auto displayNameForZoneId = [&](const std::string& zoneId)
        {
            const auto iterator = std::find_if(project.authoring.zones.begin(), project.authoring.zones.end(),
                                               [&](const auto& candidate) { return candidate.id == zoneId; });
            return iterator == project.authoring.zones.end() ? zoneId : iterator->displayName;
        };
        viewModel.crossfadeFadeInText = "Fade In: none";
        viewModel.crossfadeFadeOutText = "Fade Out: none";
        viewModel.crossfadeGuidanceText =
            "Select two compatible velocity layers to create a shared linear overlap.";

        const auto setRelationship = [&](const std::string& lowerZoneId,
                                         const std::string& upperZoneId,
                                         int overlapLow,
                                         int overlapHigh,
                                         bool isFadeIn)
        {
            const auto text = std::string(isFadeIn ? "Fade In: Linear " : "Fade Out: Linear ")
                + std::to_string(overlapLow) + "-" + std::to_string(overlapHigh)
                + " with " + displayNameForZoneId(isFadeIn ? lowerZoneId : upperZoneId);
            if (isFadeIn)
            {
                viewModel.crossfadeHasFadeIn = true;
                viewModel.crossfadeFadeInText = text;
                viewModel.crossfadeFadeInLowerZoneId = lowerZoneId;
                viewModel.crossfadeFadeInUpperZoneId = upperZoneId;
            }
            else
            {
                viewModel.crossfadeHasFadeOut = true;
                viewModel.crossfadeFadeOutText = text;
                viewModel.crossfadeFadeOutLowerZoneId = lowerZoneId;
                viewModel.crossfadeFadeOutUpperZoneId = upperZoneId;
            }
        };

        if (drs::engine::hasCompleteFadeIn(zone->velocityCrossfade))
        {
            const auto discovery = drs::engine::discoverVelocityCrossfadePartner(
                project, zone->id, drs::engine::VelocityCrossfadeDirection::fadeIn);
            if (discovery.eligible())
            {
                setRelationship(discovery.partnerZoneIds.front(), zone->id,
                                zone->velocityCrossfade.fadeInLowVelocity,
                                zone->velocityCrossfade.fadeInHighVelocity, true);
            }
            else
            {
                viewModel.crossfadeFadeInText = "Fade In: needs review — "
                    + (discovery.blockingIssues.empty() ? std::string("partner unavailable")
                                                        : discovery.blockingIssues.front());
            }
        }
        if (drs::engine::hasCompleteFadeOut(zone->velocityCrossfade))
        {
            const auto discovery = drs::engine::discoverVelocityCrossfadePartner(
                project, zone->id, drs::engine::VelocityCrossfadeDirection::fadeOut);
            if (discovery.eligible())
            {
                setRelationship(zone->id, discovery.partnerZoneIds.front(),
                                zone->velocityCrossfade.fadeOutLowVelocity,
                                zone->velocityCrossfade.fadeOutHighVelocity, false);
            }
            else
            {
                viewModel.crossfadeFadeOutText = "Fade Out: needs review — "
                    + (discovery.blockingIssues.empty() ? std::string("partner unavailable")
                                                        : discovery.blockingIssues.front());
            }
        }

        if (viewModel.crossfadeHasFadeIn)
        {
            viewModel.crossfadeLowerZoneId = viewModel.crossfadeFadeInLowerZoneId;
            viewModel.crossfadeUpperZoneId = viewModel.crossfadeFadeInUpperZoneId;
            viewModel.crossfadeOverlapLow = zone->velocityCrossfade.fadeInLowVelocity;
            viewModel.crossfadeOverlapHigh = zone->velocityCrossfade.fadeInHighVelocity;
            viewModel.crossfadeCanEdit = true;
            viewModel.crossfadeCanRemove = true;
            viewModel.crossfadeGuidanceText = "Edit or remove the complete Fade In relationship with its lower layer.";
        }
        else if (viewModel.crossfadeHasFadeOut)
        {
            viewModel.crossfadeLowerZoneId = viewModel.crossfadeFadeOutLowerZoneId;
            viewModel.crossfadeUpperZoneId = viewModel.crossfadeFadeOutUpperZoneId;
            viewModel.crossfadeOverlapLow = zone->velocityCrossfade.fadeOutLowVelocity;
            viewModel.crossfadeOverlapHigh = zone->velocityCrossfade.fadeOutHighVelocity;
            viewModel.crossfadeCanEdit = true;
            viewModel.crossfadeCanRemove = true;
            viewModel.crossfadeGuidanceText = "Edit or remove the complete Fade Out relationship with its upper layer.";
        }

        if (!viewModel.crossfadeLowerZoneId.empty() && !viewModel.crossfadeUpperZoneId.empty())
        {
            const auto audition = drs::engine::planVelocityCrossfadeAudition(
                project, viewModel.crossfadeLowerZoneId, viewModel.crossfadeUpperZoneId);
            if (audition.valid())
            {
                viewModel.crossfadeCanAudition = true;
                std::string text;
                for (const auto& step : audition.steps)
                {
                    viewModel.crossfadeAuditionVelocities.push_back(step.velocity);
                    if (!text.empty()) text += " | ";
                    text += step.label + " " + std::to_string(step.velocity) + ": L "
                        + juce::String(step.lowerGain, 2).toStdString() + ", U "
                        + juce::String(step.upperGain, 2).toStdString();
                }
                viewModel.crossfadeAuditionText = text;
            }
        }

        auto selectedZoneIds = zoneMapSelectedZoneIds;
        if (selectedZoneIds.empty())
            selectedZoneIds.push_back(zone->id);
        std::sort(selectedZoneIds.begin(), selectedZoneIds.end());
        selectedZoneIds.erase(std::unique(selectedZoneIds.begin(), selectedZoneIds.end()), selectedZoneIds.end());
        if (selectedZoneIds.size() == 2)
        {
            const auto first = std::find_if(project.authoring.zones.begin(), project.authoring.zones.end(),
                                            [&](const auto& candidate) { return candidate.id == selectedZoneIds[0]; });
            const auto second = std::find_if(project.authoring.zones.begin(), project.authoring.zones.end(),
                                             [&](const auto& candidate) { return candidate.id == selectedZoneIds[1]; });
            if (first != project.authoring.zones.end() && second != project.authoring.zones.end())
            {
                const auto* lower = &*first;
                const auto* upper = &*second;
                if (lower->velocityLow > upper->velocityLow
                    || (lower->velocityLow == upper->velocityLow && lower->id > upper->id))
                    std::swap(lower, upper);

                const auto seam = (lower->velocityHigh + upper->velocityLow) / 2;
                const auto overlapLow = std::clamp(seam - 7, 1, 126);
                const auto overlapHigh = std::clamp(seam + 8, overlapLow + 1, 127);
                const drs::engine::VelocityCrossfadePairRequest request {
                    lower->id, upper->id, overlapLow, overlapHigh
                };
                const auto plan = drs::engine::planVelocityCrossfadePair(project, request);
                viewModel.crossfadeLowerZoneId = lower->id;
                viewModel.crossfadeUpperZoneId = upper->id;
                viewModel.crossfadeOverlapLow = overlapLow;
                viewModel.crossfadeOverlapHigh = overlapHigh;
                viewModel.crossfadeCanCreate = plan.changesProject();
                if (plan.changesProject())
                    viewModel.crossfadeGuidanceText = "Create a Linear " + std::to_string(overlapLow)
                        + "-" + std::to_string(overlapHigh) + " overlap for the two selected layers.";
                else if (!plan.blockingIssues.empty())
                    viewModel.crossfadeGuidanceText = plan.blockingIssues.front();
            }
        }
        else if (selectedZoneIds.size() >= 3)
        {
            const drs::engine::VelocityCrossfadeStackRequest request { selectedZoneIds, 16 };
            const auto plan = drs::engine::planVelocityCrossfadeStack(project, request);
            viewModel.crossfadeStackZoneIds = selectedZoneIds;
            viewModel.crossfadeCanCreateStack = plan.changesProject();
            if (!plan.stackOverlaps.empty())
            {
                viewModel.crossfadeOverlapLow = plan.stackOverlaps.front().lowVelocity;
                viewModel.crossfadeOverlapHigh = plan.stackOverlaps.front().highVelocity;
            }
            if (plan.valid())
            {
                std::string preview = std::to_string(plan.orderedLayerZoneIds.size()) + " layers, ";
                preview += std::to_string(plan.stackOverlaps.size()) + " adjacent overlaps: ";
                for (std::size_t index = 0; index < plan.stackOverlaps.size(); ++index)
                {
                    if (index > 0) preview += ", ";
                    preview += std::to_string(plan.stackOverlaps[index].lowVelocity) + "-"
                        + std::to_string(plan.stackOverlaps[index].highVelocity);
                    if (plan.stackOverlaps[index].widthClamped) preview += " (clamped)";
                }
                viewModel.crossfadeStackPreviewText = preview;
                viewModel.crossfadeGuidanceText = "Preview: " + preview
                    + ". Create applies every relationship in one undo step.";

                const auto removal = drs::engine::planVelocityCrossfadeStackRemoval(project, selectedZoneIds);
                viewModel.crossfadeCanRemoveStack = removal.changesProject();
            }
            else if (!plan.blockingIssues.empty())
            {
                viewModel.crossfadeGuidanceText = plan.blockingIssues.front();
            }
        }
    }

    return viewModel;
}

void AuthoringPanel::rebuildZoneSelector()
{
    const auto zones = authoringSession.getZoneSummaries();
    zoneSelector.clear(juce::dontSendNotification);

    int itemId = 1;
    int selectedItemId = 0;
    for (const auto& zone : zones)
    {
        zoneSelector.addItem(juce::String::fromUTF8(zone.displayName.c_str())
                                 + "  ["
                                 + formatZoneRange(zone)
                                 + "]",
                             itemId);
        if (zone.selected)
            selectedItemId = itemId;
        ++itemId;
    }

    zoneSelector.setSelectedId(selectedItemId, juce::dontSendNotification);
}

void AuthoringPanel::rebuildLayerList()
{
    const auto& project = authoringSession.getProject();
    const auto selectedLayer = authoringSession.getSelectedLayer();
    authoring::RepeatedStructureListViewModel viewModel;
    viewModel.emptyStateText = "No layers are authored in this project yet.";
    selectedLayerIndex = -1;
    viewModel.rows.reserve(project.authoring.layers.size());
    for (std::size_t index = 0; index < project.authoring.layers.size(); ++index)
    {
        const auto& layer = project.authoring.layers[index];
        auto row = authoring::RepeatedStructureRowViewModel {};
        row.key = layer.id;
        row.title = layer.displayName;
        const auto groupCount = static_cast<int>(std::count_if(
            project.authoring.groups.begin(), project.authoring.groups.end(),
            [&](const auto& group) { return group.layerId == layer.id; }));
        const auto zoneCount = static_cast<int>(std::count_if(
            project.authoring.zones.begin(), project.authoring.zones.end(),
            [&](const auto& zone)
            {
                return std::any_of(project.authoring.groups.begin(), project.authoring.groups.end(),
                                   [&](const auto& group)
                                   {
                                       return group.id == zone.groupId && group.layerId == layer.id;
                                   });
            }));
        row.statusText = std::to_string(groupCount) + " groups · " + std::to_string(zoneCount) + " zones"
            + (layer.workspaceVisible ? " · visible" : " · hidden");
        viewModel.rows.push_back(std::move(row));
        if (selectedLayer.has_value() && selectedLayer->id == layer.id)
            selectedLayerIndex = static_cast<int>(index);
    }
    if (selectedLayerIndex < 0 && !project.authoring.layers.empty())
        selectedLayerIndex = 0;
    viewModel.selectedIndex = selectedLayerIndex;
    layerList.setViewModel(std::move(viewModel));
}

void AuthoringPanel::rebuildGroupList()
{
    const auto& project = authoringSession.getProject();
    const auto selectedGroup = authoringSession.getSelectedGroup();
    authoring::RepeatedStructureListViewModel viewModel;
    viewModel.emptyStateText = "No groups are authored in this project yet.";

    if (project.authoring.groups.empty())
    {
        selectedGroupIndex = -1;
        groupList.setViewModel(std::move(viewModel));
        return;
    }

    selectedGroupIndex = 0;
    viewModel.rows.reserve(project.authoring.groups.size());
    for (std::size_t index = 0; index < project.authoring.groups.size(); ++index)
    {
        const auto& group = project.authoring.groups[index];
        auto row = authoring::RepeatedStructureRowViewModel {};
        row.key = group.id;
        row.title = group.displayName;
        row.statusText = buildGroupListStatusText(project, group).toStdString();
        viewModel.rows.push_back(std::move(row));

        if (selectedGroup.has_value() && selectedGroup->id == group.id)
            selectedGroupIndex = static_cast<int>(index);
    }

    viewModel.selectedIndex = selectedGroupIndex;
    groupList.setViewModel(std::move(viewModel));
}

void AuthoringPanel::rebuildMacroList()
{
    const auto& macros = authoringSession.getProject().authoring.macros;
    authoring::RepeatedStructureListViewModel viewModel;
    viewModel.emptyStateText = "No macros are authored in this project yet. Use Create to add one.";

    if (macros.empty())
    {
        selectedMacroIndex = -1;
        macroList.setViewModel(std::move(viewModel));
        return;
    }

    if (const auto sessionSelectedMacroIndex = authoringSession.getSelectedMacroIndex();
        sessionSelectedMacroIndex.has_value())
    {
        selectedMacroIndex = std::clamp(static_cast<int>(*sessionSelectedMacroIndex),
                                        0,
                                        static_cast<int>(macros.size()) - 1);
    }
    else
    {
        selectedMacroIndex = std::clamp(selectedMacroIndex, 0, static_cast<int>(macros.size()) - 1);
    }
    viewModel.selectedIndex = selectedMacroIndex;
    viewModel.rows.reserve(macros.size());

    for (std::size_t index = 0; index < macros.size(); ++index)
    {
        auto row = authoring::RepeatedStructureRowViewModel{};
        row.key = macros[index].id;
        row.title = macros[index].name;
        row.statusText = buildMacroListStatusText(macros[index]).toStdString();
        viewModel.rows.push_back(std::move(row));
    }

    macroList.setViewModel(std::move(viewModel));
}

void AuthoringPanel::rebuildMacroAssignmentList()
{
    authoring::RepeatedStructureListViewModel viewModel;
    const auto selectedMacro = authoringSession.getSelectedMacro();
    if (!selectedMacro.has_value())
    {
        viewModel.emptyStateText = "Create or select a macro before assigning a target.";
        selectedMacroTargetIndex = -1;
        macroAssignmentList.setViewModel(std::move(viewModel));
        return;
    }

    if (selectedMacro->targets.empty())
    {
        viewModel.emptyStateText = "No targets assigned. Choose a supported target below, or use Add Target.";
        selectedMacroTargetIndex = -1;
        macroAssignmentList.setViewModel(std::move(viewModel));
        return;
    }

    selectedMacroTargetIndex = std::clamp(selectedMacroTargetIndex, 0,
                                          static_cast<int>(selectedMacro->targets.size()) - 1);
    viewModel.selectedIndex = selectedMacroTargetIndex;
    viewModel.rows.reserve(selectedMacro->targets.size());
    for (std::size_t index = 0; index < selectedMacro->targets.size(); ++index)
    {
        const auto& target = selectedMacro->targets[index];
        authoring::RepeatedStructureRowViewModel row;
        row.key = !target.parameterId.empty()
            ? target.parameterId : "target-" + std::to_string(index);
        row.title = (macroTargetFamily(target) + " · " + macroTargetName(target)).toStdString();
        row.statusText = macroTargetMappingSummary(target).toStdString();
        viewModel.rows.push_back(std::move(row));
    }
    macroAssignmentList.setViewModel(std::move(viewModel));
}

void AuthoringPanel::rebuildArticulationList()
{
    const auto articulations = authoringSession.getArticulations();
    authoring::RepeatedStructureListViewModel viewModel;
    viewModel.emptyStateText = "No articulations are available in this project.";
    if (articulations.empty())
    {
        selectedArticulationIndex = -1;
        articulationList.setViewModel(std::move(viewModel));
        return;
    }

    selectedArticulationIndex = std::clamp(selectedArticulationIndex, 0,
                                            static_cast<int>(articulations.size()) - 1);
    viewModel.selectedIndex = selectedArticulationIndex;
    viewModel.rows.reserve(articulations.size());
    for (const auto& articulation : articulations)
    {
        auto row = authoring::RepeatedStructureRowViewModel {};
        row.key = articulation.id;
        row.title = articulation.displayName;
        row.statusText = articulation.isDefault ? "Default" : "";
        if (articulation.activation.has_value())
        {
            if (!row.statusText.empty())
                row.statusText += " | ";
            row.statusText += formatMidiNoteName(articulation.activation->midiNote).toStdString();
        }
        else if (!row.statusText.empty())
            row.statusText += " | no switch";
        else
            row.statusText = "no switch";
        viewModel.rows.push_back(std::move(row));
    }
    articulationList.setViewModel(std::move(viewModel));
}

std::string AuthoringPanel::selectedDspScopeInputSource() const
{
    if (selectedDspScopeIndex == 0)
    {
        if (const auto zone = authoringSession.getSelectedZone(); zone.has_value())
            return "zones/" + zone->id;
    }
    else if (selectedDspScopeIndex == 1)
    {
        if (const auto group = authoringSession.getSelectedGroup(); group.has_value())
            return "groups/" + group->id;
    }
    return "master";
}

std::string AuthoringPanel::selectedDspScopeRoutingBusId() const
{
    const auto input = selectedDspScopeInputSource();
    for (const auto& bus : authoringSession.getProject().authoring.routingBuses)
    {
        if (bus.inputSourceId == input)
            return bus.id;
        // Older authored zone buses used the raw zone id. Preserve them as the selected chain.
        if (selectedDspScopeIndex == 0 && input.rfind("zones/", 0) == 0
            && bus.inputSourceId == input.substr(6))
            return bus.id;
    }
    return {};
}

void AuthoringPanel::rebuildDspScopeSelector()
{
    fxScopeSelector.clear(juce::dontSendNotification);
    fxScopeSelector.addItem("Current Zone", 1);
    fxScopeSelector.addItem("Current Group", 2);
    fxScopeSelector.addItem("Instrument Master", 3);
    selectedDspScopeIndex = std::clamp(selectedDspScopeIndex, 0, 2);
    fxScopeSelector.setSelectedId(selectedDspScopeIndex + 1, juce::dontSendNotification);
    const auto input = selectedDspScopeInputSource();
    const auto bus = selectedDspScopeRoutingBusId();
    fxScopeBreadcrumbLabel.setText(
        (selectedDspScopeIndex == 0 ? "Zone" : selectedDspScopeIndex == 1 ? "Group" : "Instrument")
            + juce::String(" | ") + juce::String::fromUTF8(input.c_str())
            + (bus.empty() ? " | no insert chain" : " | chain " + juce::String::fromUTF8(bus.c_str())),
        juce::dontSendNotification);

    fxOwnerBusIds.clear();
    fxOwnerSelector.clear(juce::dontSendNotification);
    int selectedOwner = 0;
    for (std::size_t index = 0; index < authoringSession.getProject().authoring.routingBuses.size(); ++index)
    {
        const auto& owner = authoringSession.getProject().authoring.routingBuses[index];
        fxOwnerBusIds.push_back(owner.id);
        fxOwnerSelector.addItem(juce::String::fromUTF8(owner.displayName.c_str()), static_cast<int>(index) + 1);
        if (owner.id == bus) selectedOwner = static_cast<int>(index) + 1;
    }
    fxOwnerSelector.setSelectedId(selectedOwner > 0 ? selectedOwner : 1, juce::dontSendNotification);
}

void AuthoringPanel::rebuildFxSelector()
{
    const auto& project = authoringSession.getProject();
    fxSelector.clear(juce::dontSendNotification);
    scopedFxSlotIds.clear();
    const auto ownerBusId = selectedDspScopeRoutingBusId();
    const auto bus = std::find_if(project.authoring.routingBuses.begin(), project.authoring.routingBuses.end(),
                                  [&](const auto& candidate) { return candidate.id == ownerBusId; });
    if (bus == project.authoring.routingBuses.end())
    {
        selectedFxSlotIndex = -1;
        return;
    }
    for (const auto& slotId : bus->fxSlotIds)
    {
        const auto slot = std::find_if(project.authoring.fxSlots.begin(), project.authoring.fxSlots.end(),
                                       [&](const auto& candidate) { return candidate.id == slotId; });
        if (slot == project.authoring.fxSlots.end()) continue;
        const auto index = static_cast<int>(std::distance(project.authoring.fxSlots.begin(), slot));
        scopedFxSlotIds.push_back(slotId);
        fxSelector.addItem(juce::String::fromUTF8(slot->displayName.c_str()), index + 1);
    }
    if (scopedFxSlotIds.empty())
    {
        selectedFxSlotIndex = -1;
        return;
    }
    if (std::find(scopedFxSlotIds.begin(), scopedFxSlotIds.end(),
                  project.authoring.fxSlots[static_cast<std::size_t>(std::max(0, selectedFxSlotIndex))].id)
        == scopedFxSlotIds.end())
    {
        const auto slot = std::find_if(project.authoring.fxSlots.begin(), project.authoring.fxSlots.end(),
                                       [&](const auto& candidate) { return candidate.id == scopedFxSlotIds.front(); });
        selectedFxSlotIndex = static_cast<int>(std::distance(project.authoring.fxSlots.begin(), slot));
    }
    fxSelector.setSelectedId(selectedFxSlotIndex + 1, juce::dontSendNotification);
}

void AuthoringPanel::rebuildRoutingBusSelector()
{
    const auto& routingBuses = authoringSession.getProject().authoring.routingBuses;
    routingBusSelector.clear(juce::dontSendNotification);

    if (routingBuses.empty())
    {
        selectedRoutingBusIndex = 0;
        return;
    }

    selectedRoutingBusIndex = std::clamp(selectedRoutingBusIndex, 0, static_cast<int>(routingBuses.size()) - 1);
    for (std::size_t index = 0; index < routingBuses.size(); ++index)
    {
        routingBusSelector.addItem(juce::String::fromUTF8(routingBuses[index].displayName.c_str()),
                                   static_cast<int>(index) + 1);
    }

    routingBusSelector.setSelectedId(selectedRoutingBusIndex + 1, juce::dontSendNotification);
}

void AuthoringPanel::rebuildPerformanceBankSelector()
{
    const auto& project = authoringSession.getProject();
    const auto& performanceBanks = authoringSession.getProject().authoring.performanceBanks;
    performanceBankSelector.clear(juce::dontSendNotification);

    if (performanceBanks.empty())
    {
        selectedPerformanceBankIndex = 0;
        return;
    }

    selectedPerformanceBankIndex = std::clamp(selectedPerformanceBankIndex,
                                              0,
                                              static_cast<int>(performanceBanks.size()) - 1);
    for (std::size_t index = 0; index < performanceBanks.size(); ++index)
    {
        performanceBankSelector.addItem(juce::String::fromUTF8(performanceBanks[index].displayName.c_str()),
                                        static_cast<int>(index) + 1);
    }

    const auto& resetRules = project.authoring.roundRobinResetRules;
    selectedRoundRobinResetIndex = resetRules.empty() ? -1
        : std::clamp(selectedRoundRobinResetIndex, 0, static_cast<int>(resetRules.size()) - 1);
    roundRobinResetSelector.clear(juce::dontSendNotification);
    for (std::size_t index = 0; index < resetRules.size(); ++index)
        roundRobinResetSelector.addItem("Reset " + juce::String(static_cast<int>(index) + 1), static_cast<int>(index) + 1);
    roundRobinResetSelector.setSelectedId(selectedRoundRobinResetIndex + 1, juce::dontSendNotification);
    roundRobinResetTargetSelector.clear(juce::dontSendNotification);
    roundRobinResetTargetSelector.addItem("All pools", 1);
    std::vector<std::string> rrPoolIds;
    for (const auto& zone : project.authoring.zones)
        if (zone.roundRobin.has_value()
            && std::find(rrPoolIds.begin(), rrPoolIds.end(), zone.roundRobin->poolId) == rrPoolIds.end())
            rrPoolIds.push_back(zone.roundRobin->poolId);
    for (std::size_t index = 0; index < rrPoolIds.size(); ++index)
        roundRobinResetTargetSelector.addItem(juce::String::fromUTF8(rrPoolIds[index].c_str()), static_cast<int>(index) + 2);
    int resetEventId = 2;
    int resetTargetId = 1;
    if (selectedRoundRobinResetIndex >= 0)
    {
        const auto& rule = resetRules[static_cast<std::size_t>(selectedRoundRobinResetIndex)];
        resetEventId = rule.event == drs::engine::RoundRobinResetEvent::programActivation ? 1
            : rule.event == drs::engine::RoundRobinResetEvent::allNotesOff ? 3
            : rule.event == drs::engine::RoundRobinResetEvent::pedalDown ? 4
            : rule.event == drs::engine::RoundRobinResetEvent::pedalUp ? 5 : 2;
        if (!rule.targetAll)
            for (std::size_t index = 0; index < rrPoolIds.size(); ++index)
                if (rrPoolIds[index] == rule.targetPoolId) resetTargetId = static_cast<int>(index) + 2;
    }
    roundRobinResetEventSelector.setSelectedId(resetEventId, juce::dontSendNotification);
    roundRobinResetTargetSelector.setSelectedId(resetTargetId, juce::dontSendNotification);
    roundRobinResetSelector.setEnabled(!resetRules.empty());
    roundRobinResetDeleteButton.setEnabled(!resetRules.empty());
    roundRobinResetTargetSelector.setEnabled(!rrPoolIds.empty() || !resetRules.empty());
    roundRobinResetSummaryLabel.setText(resetRules.empty()
        ? "No RR reset rules. Add one for a predictable phrase boundary."
        : juce::String(static_cast<int>(resetRules.size())) + " RR reset rule(s); all-pools and named-pool targets validate before Publish.",
        juce::dontSendNotification);

    if (const auto selectedPerformanceBank = authoringSession.getSelectedPerformanceBank(); selectedPerformanceBank.has_value())
    {
        const auto iterator = std::find_if(performanceBanks.begin(),
                                           performanceBanks.end(),
                                           [&](const auto& performanceBank)
                                           {
                                               return performanceBank.id == selectedPerformanceBank->id;
                                           });
        if (iterator != performanceBanks.end())
            selectedPerformanceBankIndex = static_cast<int>(std::distance(performanceBanks.begin(), iterator));
    }

    performanceBankSelector.setSelectedId(selectedPerformanceBankIndex + 1, juce::dontSendNotification);
}

void AuthoringPanel::rebuildTriggerSlotSelector()
{
    triggerSlotSelector.clear(juce::dontSendNotification);

    const auto selectedPerformanceBank = authoringSession.getSelectedPerformanceBank();
    if (!selectedPerformanceBank.has_value() || selectedPerformanceBank->triggerSlots.empty())
    {
        selectedTriggerSlotIndex = 0;
        return;
    }

    selectedTriggerSlotIndex = std::clamp(selectedTriggerSlotIndex,
                                          0,
                                          static_cast<int>(selectedPerformanceBank->triggerSlots.size()) - 1);
    for (std::size_t index = 0; index < selectedPerformanceBank->triggerSlots.size(); ++index)
    {
        triggerSlotSelector.addItem(
            juce::String::fromUTF8(selectedPerformanceBank->triggerSlots[index].displayName.c_str()),
            static_cast<int>(index) + 1);
    }

    triggerSlotSelector.setSelectedId(selectedTriggerSlotIndex + 1, juce::dontSendNotification);
}

void AuthoringPanel::setWorkbenchOpen(bool shouldOpen)
{
    if (workbenchState.open == shouldOpen)
        return;

    workbenchState.open = shouldOpen;
    workbenchLayoutState.setOpen(shouldOpen);
    refreshInspectorVisibility();
    resized();
}

void AuthoringPanel::setActiveWorkbenchTab(authoring::WorkbenchTab nextTab)
{
    workbenchState.activeTab = nextTab;
    workbenchState.open = true;
    workbenchLayoutState.suggestHeightForTab(nextTab);
    workbenchLayoutState.setOpen(true);
    refreshInspectorVisibility();
    resized();
    if (workbenchState.activeTab == authoring::WorkbenchTab::waveform)
        requestWaveformPreviewLoad(true);
}

void AuthoringPanel::timerCallback(int timerId)
{
    if (timerId == statusTimerId)
    {
        refreshNow();
        return;
    }
    if (timerId == keySwitchMidiLearnTimerId)
    {
        if (keySwitchMidiLearnActive
            && juce::Time::getMillisecondCounterHiRes() >= keySwitchMidiLearnDeadlineMillis)
        {
            keySwitchMidiLearnActive = false;
            keySwitchMidiLearnDeadlineMillis = 0.0;
            stopTimer(keySwitchMidiLearnTimerId);
            refreshFromSession();
        }
        return;
    }
    if (timerId != previewReleaseTimerId)
        return;

    const auto now = juce::Time::getMillisecondCounterHiRes();
    auto anyPending = false;
    for (std::size_t source = 0; source < timedPreviewNotes.size(); ++source)
    {
        if (timedPreviewNotes[source].active
            && now >= timedPreviewNotes[source].releaseAtMillis)
            releaseTimedPreview(source);
        anyPending = anyPending || timedPreviewNotes[source].active;
    }
    if (crossfadeAuditionSequence.active && now >= crossfadeAuditionSequence.nextAtMillis)
        dispatchNextCrossfadeAuditionStep();
    anyPending = anyPending || crossfadeAuditionSequence.active;
    if (!anyPending)
        stopTimer(previewReleaseTimerId);
}

void AuthoringPanel::refreshWorkbenchVisibility()
{
    const auto waveformTab = workbenchState.activeTab == authoring::WorkbenchTab::waveform;
    const auto groupsTab = workbenchState.activeTab == authoring::WorkbenchTab::groups;
    const auto macrosTab = workbenchState.activeTab == authoring::WorkbenchTab::macros;
    const auto routingTab = workbenchState.activeTab == authoring::WorkbenchTab::routing;
    const auto performanceTab = workbenchState.activeTab == authoring::WorkbenchTab::performance;
    const auto articulationsTab = workbenchState.activeTab == authoring::WorkbenchTab::articulations;
    const auto workbenchContentVisible = workbenchState.open;
    const auto expanded = isExpandedLayout(layoutMode);
    const auto* focusedComponent = juce::Component::getCurrentlyFocusedComponent();
    const auto focusWithinWaveform = isComponentFocusedWithin(focusedComponent, waveformPreview)
        || isComponentFocusedWithin(focusedComponent, waveformStatusLabel)
        || isComponentFocusedWithin(focusedComponent, waveformInfoLabel)
        || isComponentFocusedWithin(focusedComponent, loopInfoLabel)
        || isComponentFocusedWithin(focusedComponent, importMetricsLabel)
        || isComponentFocusedWithin(focusedComponent, waveformPlaybackStartEditor)
        || isComponentFocusedWithin(focusedComponent, waveformPlaybackEndEditor)
        || isComponentFocusedWithin(focusedComponent, waveformPlaybackResetButton)
        || isComponentFocusedWithin(focusedComponent, waveformSetPlaybackSelectionButton)
        || isComponentFocusedWithin(focusedComponent, waveformPlaybackAuditionButton)
        || isComponentFocusedWithin(focusedComponent, waveformSelectionAuditionButton)
        || isComponentFocusedWithin(focusedComponent, waveformSnapToggle)
        || isComponentFocusedWithin(focusedComponent, waveformLoopModeSelector)
        || isComponentFocusedWithin(focusedComponent, waveformLoopStartEditor)
        || isComponentFocusedWithin(focusedComponent, waveformLoopEndEditor)
        || isComponentFocusedWithin(focusedComponent, waveformLoopCrossfadeEditor)
        || isComponentFocusedWithin(focusedComponent, waveformSetLoopSelectionButton)
        || isComponentFocusedWithin(focusedComponent, waveformLoopAuditionButton)
        || isComponentFocusedWithin(focusedComponent, sourceValidationLabel)
        || isComponentFocusedWithin(focusedComponent, sourceValidationButton);
    const auto focusWithinGroups = isComponentFocusedWithin(focusedComponent, groupNameEditor)
        || isComponentFocusedWithin(focusedComponent, groupVisibilityToggle)
        || isComponentFocusedWithin(focusedComponent, groupGainSlider)
        || isComponentFocusedWithin(focusedComponent, groupPanSlider)
        || isComponentFocusedWithin(focusedComponent, groupRoutingSelector)
        || isComponentFocusedWithin(focusedComponent, groupAnchorSelector)
        || isComponentFocusedWithin(focusedComponent, groupDeleteButton)
        || isComponentFocusedWithin(focusedComponent, groupRoundRobinToggle)
        || isComponentFocusedWithin(focusedComponent, groupRoundRobinModeSelector);
    const auto focusWithinMacros = isComponentFocusedWithin(focusedComponent, macroList)
        || isComponentFocusedWithin(focusedComponent, macroAssignmentList)
        || isComponentFocusedWithin(focusedComponent, macroCreateButton)
        || isComponentFocusedWithin(focusedComponent, macroDuplicateButton)
        || isComponentFocusedWithin(focusedComponent, macroDeleteButton)
        || isComponentFocusedWithin(focusedComponent, macroAssignmentAddButton)
        || isComponentFocusedWithin(focusedComponent, macroAssignmentRemoveButton)
        || isComponentFocusedWithin(focusedComponent, macroNameEditor)
        || isComponentFocusedWithin(focusedComponent, macroExposeToggle)
        || isComponentFocusedWithin(focusedComponent, macroAssignmentSelector)
        || isComponentFocusedWithin(focusedComponent, macroRoleSelector)
        || isComponentFocusedWithin(focusedComponent, macroDefaultSlider)
        || isComponentFocusedWithin(focusedComponent, macroMinSlider)
        || isComponentFocusedWithin(focusedComponent, macroMaxSlider)
        || isComponentFocusedWithin(focusedComponent, macroMoveUpButton)
        || isComponentFocusedWithin(focusedComponent, macroMoveDownButton);
    const auto focusWithinRouting = isComponentFocusedWithin(focusedComponent, fxScopeSelector)
        || isComponentFocusedWithin(focusedComponent, fxSelector)
        || isComponentFocusedWithin(focusedComponent, fxNameEditor)
        || isComponentFocusedWithin(focusedComponent, fxTypeSelector)
        || isComponentFocusedWithin(focusedComponent, fxBypassedToggle)
        || isComponentFocusedWithin(focusedComponent, fxAddButton)
        || isComponentFocusedWithin(focusedComponent, fxDuplicateButton)
        || isComponentFocusedWithin(focusedComponent, fxMoveUpButton)
        || isComponentFocusedWithin(focusedComponent, fxMoveDownButton)
        || isComponentFocusedWithin(focusedComponent, fxDeleteButton)
        || isComponentFocusedWithin(focusedComponent, fxOwnerSelector)
        || isComponentFocusedWithin(focusedComponent, fxMoveOwnerButton)
        || isComponentFocusedWithin(focusedComponent, fxParameterSelector)
        || isComponentFocusedWithin(focusedComponent, fxParameterSlider)
        || isComponentFocusedWithin(focusedComponent, fxParameterResetButton)
        || isComponentFocusedWithin(focusedComponent, fxAssignMacroButton)
        || isComponentFocusedWithin(focusedComponent, routingBusSelector)
        || isComponentFocusedWithin(focusedComponent, routingInputSelector)
        || isComponentFocusedWithin(focusedComponent, routingInsertOneSelector)
        || isComponentFocusedWithin(focusedComponent, routingInsertTwoSelector);
    const auto focusWithinPerformance = isComponentFocusedWithin(focusedComponent, performanceBankSelector)
        || isComponentFocusedWithin(focusedComponent, triggerSlotSelector)
        || isComponentFocusedWithin(focusedComponent, triggerEventSelector)
        || isComponentFocusedWithin(focusedComponent, targetArticulationSelector)
        || isComponentFocusedWithin(focusedComponent, phraseAssetSelector)
        || isComponentFocusedWithin(focusedComponent, chordModeSelector)
        || isComponentFocusedWithin(focusedComponent, phraseImportPathEditor)
        || isComponentFocusedWithin(focusedComponent, phraseImportButton);
    const auto focusWithinArticulations = isComponentFocusedWithin(focusedComponent, articulationWorkbenchViewport)
        || isComponentFocusedWithin(focusedComponent, articulationList)
        || isComponentFocusedWithin(focusedComponent, articulationNameEditor)
        || isComponentFocusedWithin(focusedComponent, articulationSwitchNoteSlider)
        || isComponentFocusedWithin(focusedComponent, articulationMidiLearnButton);

    refreshWorkbenchContextLabels();

    workbenchToggleButton.setButtonText(workbenchState.open ? "Hide Workbench" : "Show Workbench");
    workbenchToggleButton.setTitle(workbenchToggleButton.getButtonText());
    setVisibleAndAccessible(workbenchContentHost, workbenchContentVisible);
    setVisibleAndAccessible(workbenchSplitter, workbenchContentVisible);
    setVisibleAndAccessible(waveformLabel, workbenchContentVisible);
    setVisibleAndAccessible(waveformScopeLabel, workbenchContentVisible);
    setVisibleAndAccessible(workbenchBreadcrumbLabel, workbenchContentVisible);
    setVisibleAndAccessible(waveformPreview, workbenchContentVisible && waveformTab);
    setVisibleAndAccessible(waveformStatusLabel, workbenchContentVisible && waveformTab);
    setVisibleAndAccessible(waveformInfoLabel, workbenchContentVisible && waveformTab);
    setVisibleAndAccessible(loopInfoLabel, workbenchContentVisible && waveformTab);
    setVisibleAndAccessible(importMetricsLabel, workbenchContentVisible && waveformTab);
    setVisibleAndAccessible(waveformPlaybackStartLabel, workbenchContentVisible && waveformTab);
    setVisibleAndAccessible(waveformPlaybackEndLabel, workbenchContentVisible && waveformTab);
    setVisibleAndAccessible(waveformLoopStartLabel, workbenchContentVisible && waveformTab);
    setVisibleAndAccessible(waveformLoopEndLabel, workbenchContentVisible && waveformTab);
    setVisibleAndAccessible(waveformLoopCrossfadeLabel, workbenchContentVisible && waveformTab);
    setVisibleAndAccessible(waveformPlaybackStartEditor, workbenchContentVisible && waveformTab);
    setVisibleAndAccessible(waveformPlaybackEndEditor, workbenchContentVisible && waveformTab);
    setVisibleAndAccessible(waveformPlaybackResetButton, workbenchContentVisible && waveformTab);
    setVisibleAndAccessible(waveformSetPlaybackSelectionButton, workbenchContentVisible && waveformTab);
    setVisibleAndAccessible(waveformPlaybackAuditionButton, workbenchContentVisible && waveformTab);
    setVisibleAndAccessible(waveformSelectionAuditionButton, workbenchContentVisible && waveformTab);
    setVisibleAndAccessible(waveformSnapToggle, workbenchContentVisible && waveformTab);
    setVisibleAndAccessible(waveformLoopModeSelector, workbenchContentVisible && waveformTab);
    setVisibleAndAccessible(waveformLoopStartEditor, workbenchContentVisible && waveformTab);
    setVisibleAndAccessible(waveformLoopEndEditor, workbenchContentVisible && waveformTab);
    setVisibleAndAccessible(waveformLoopCrossfadeEditor, workbenchContentVisible && waveformTab);
    setVisibleAndAccessible(waveformSetLoopSelectionButton, workbenchContentVisible && waveformTab);
    setVisibleAndAccessible(waveformLoopAuditionButton, workbenchContentVisible && waveformTab);
    setVisibleAndAccessible(waveformLoopGuidanceLabel, workbenchContentVisible && waveformTab);
    setVisibleAndAccessible(sourceValidationLabel, workbenchContentVisible && waveformTab);
    setVisibleAndAccessible(sourceValidationButton, workbenchContentVisible && waveformTab);

    setVisibleAndAccessible(groupNameLabel, workbenchContentVisible && groupsTab);
    setVisibleAndAccessible(groupNameEditor, workbenchContentVisible && groupsTab);
    setVisibleAndAccessible(masterGainLabel, workbenchContentVisible && groupsTab);
    setVisibleAndAccessible(masterGainSlider, workbenchContentVisible && groupsTab);
    setVisibleAndAccessible(groupVisibilityLabel, workbenchContentVisible && groupsTab);
    setVisibleAndAccessible(groupVisibilityToggle, workbenchContentVisible && groupsTab);
    setVisibleAndAccessible(groupGainLabel, workbenchContentVisible && groupsTab);
    setVisibleAndAccessible(groupGainSlider, workbenchContentVisible && groupsTab);
    setVisibleAndAccessible(groupPanLabel, workbenchContentVisible && groupsTab);
    setVisibleAndAccessible(groupPanSlider, workbenchContentVisible && groupsTab);
    setVisibleAndAccessible(groupRoutingLabel, workbenchContentVisible && groupsTab);
    setVisibleAndAccessible(groupRoutingSelector, workbenchContentVisible && groupsTab);
    setVisibleAndAccessible(groupAnchorLabel, workbenchContentVisible && groupsTab);
    setVisibleAndAccessible(groupAnchorSelector, workbenchContentVisible && groupsTab);
    setVisibleAndAccessible(groupDeleteButton, workbenchContentVisible && groupsTab);
    setVisibleAndAccessible(groupAssignZonesButton, workbenchContentVisible && groupsTab);
    setVisibleAndAccessible(groupSummaryLabel, workbenchContentVisible && groupsTab);
    setVisibleAndAccessible(groupRoundRobinLabel, workbenchContentVisible && groupsTab);
    setVisibleAndAccessible(groupRoundRobinHintLabel, false);
    setVisibleAndAccessible(groupRoundRobinToggle, workbenchContentVisible && groupsTab);
    setVisibleAndAccessible(groupRoundRobinModeSelector, workbenchContentVisible && groupsTab);

    setVisibleAndAccessible(macroWorkbenchViewport, workbenchContentVisible && macrosTab);
    setVisibleAndAccessible(macroList, workbenchContentVisible && macrosTab);
    setVisibleAndAccessible(macroAssignmentList, workbenchContentVisible && macrosTab);
    setVisibleAndAccessible(macroCreateButton, workbenchContentVisible && macrosTab);
    setVisibleAndAccessible(macroDuplicateButton, workbenchContentVisible && macrosTab);
    setVisibleAndAccessible(macroDeleteButton, workbenchContentVisible && macrosTab);
    setVisibleAndAccessible(macroAssignmentAddButton, workbenchContentVisible && macrosTab);
    setVisibleAndAccessible(macroAssignmentRemoveButton, workbenchContentVisible && macrosTab);
    setVisibleAndAccessible(macroNameLabel, workbenchContentVisible && macrosTab);
    setVisibleAndAccessible(macroNameEditor, workbenchContentVisible && macrosTab);
    setVisibleAndAccessible(macroExposeLabel, workbenchContentVisible && macrosTab);
    setVisibleAndAccessible(macroExposeToggle, workbenchContentVisible && macrosTab);
    setVisibleAndAccessible(macroAssignmentLabel, workbenchContentVisible && macrosTab);
    setVisibleAndAccessible(macroAssignmentSelector, workbenchContentVisible && macrosTab);
    setVisibleAndAccessible(macroRoleLabel, workbenchContentVisible && macrosTab);
    setVisibleAndAccessible(macroRoleSelector, workbenchContentVisible && macrosTab);
    setVisibleAndAccessible(macroDefaultLabel, workbenchContentVisible && macrosTab);
    setVisibleAndAccessible(macroDefaultSlider, workbenchContentVisible && macrosTab);
    setVisibleAndAccessible(macroMinLabel, workbenchContentVisible && macrosTab);
    setVisibleAndAccessible(macroMinSlider, workbenchContentVisible && macrosTab);
    setVisibleAndAccessible(macroMaxLabel, workbenchContentVisible && macrosTab);
    setVisibleAndAccessible(macroMaxSlider, workbenchContentVisible && macrosTab);
    setVisibleAndAccessible(macroMoveUpButton, workbenchContentVisible && macrosTab);
    setVisibleAndAccessible(macroMoveDownButton, workbenchContentVisible && macrosTab);
    setVisibleAndAccessible(macroSummaryLabel, workbenchContentVisible && macrosTab);

    setVisibleAndAccessible(routingWorkbenchViewport, workbenchContentVisible && routingTab);
    setVisibleAndAccessible(fxSectionLabel, workbenchContentVisible && routingTab);
    setVisibleAndAccessible(fxScopeLabel, workbenchContentVisible && routingTab);
    setVisibleAndAccessible(fxScopeSelector, workbenchContentVisible && routingTab);
    setVisibleAndAccessible(fxScopeBreadcrumbLabel, workbenchContentVisible && routingTab);
    setVisibleAndAccessible(fxSelector, workbenchContentVisible && routingTab);
    setVisibleAndAccessible(fxNameEditor, workbenchContentVisible && routingTab);
    setVisibleAndAccessible(fxTypeLabel, workbenchContentVisible && routingTab);
    setVisibleAndAccessible(fxTypeSelector, workbenchContentVisible && routingTab);
    setVisibleAndAccessible(fxBypassedToggle, workbenchContentVisible && routingTab);
    setVisibleAndAccessible(fxAddButton, workbenchContentVisible && routingTab);
    setVisibleAndAccessible(fxDuplicateButton, workbenchContentVisible && routingTab);
    setVisibleAndAccessible(fxMoveUpButton, workbenchContentVisible && routingTab);
    setVisibleAndAccessible(fxMoveDownButton, workbenchContentVisible && routingTab);
    setVisibleAndAccessible(fxDeleteButton, workbenchContentVisible && routingTab);
    setVisibleAndAccessible(fxOwnerSelector, workbenchContentVisible && routingTab);
    setVisibleAndAccessible(fxMoveOwnerButton, workbenchContentVisible && routingTab);
    setVisibleAndAccessible(fxParameterSelector, workbenchContentVisible && routingTab);
    setVisibleAndAccessible(fxParameterSlider, workbenchContentVisible && routingTab);
    setVisibleAndAccessible(fxParameterResetButton, workbenchContentVisible && routingTab);
    setVisibleAndAccessible(fxAssignMacroButton, workbenchContentVisible && routingTab);
    setVisibleAndAccessible(fxParameterValueLabel, workbenchContentVisible && routingTab);
    setVisibleAndAccessible(fxSummaryLabel, workbenchContentVisible && routingTab);
    setVisibleAndAccessible(fxDiagnosticsLabel, workbenchContentVisible && routingTab);
    setVisibleAndAccessible(routingSectionLabel, workbenchContentVisible && routingTab);
    setVisibleAndAccessible(routingBusSelector, workbenchContentVisible && routingTab);
    setVisibleAndAccessible(routingInputLabel, workbenchContentVisible && routingTab);
    setVisibleAndAccessible(routingInputSelector, workbenchContentVisible && routingTab);
    setVisibleAndAccessible(routingInsertOneLabel, workbenchContentVisible && routingTab);
    setVisibleAndAccessible(routingInsertOneSelector, workbenchContentVisible && routingTab);
    setVisibleAndAccessible(routingInsertTwoLabel, workbenchContentVisible && routingTab);
    setVisibleAndAccessible(routingInsertTwoSelector, workbenchContentVisible && routingTab);
    setVisibleAndAccessible(routingSummaryLabel, workbenchContentVisible && routingTab);

    setVisibleAndAccessible(performanceBankSelector, workbenchContentVisible && performanceTab);
    setVisibleAndAccessible(triggerSlotSelector, workbenchContentVisible && performanceTab);
    setVisibleAndAccessible(triggerEventLabel, workbenchContentVisible && performanceTab);
    setVisibleAndAccessible(triggerEventSelector, workbenchContentVisible && performanceTab);
    setVisibleAndAccessible(targetArticulationLabel, workbenchContentVisible && performanceTab);
    setVisibleAndAccessible(targetArticulationSelector, workbenchContentVisible && performanceTab);
    setVisibleAndAccessible(phraseAssetLabel, workbenchContentVisible && performanceTab);
    setVisibleAndAccessible(phraseAssetSelector, workbenchContentVisible && performanceTab);
    setVisibleAndAccessible(chordModeLabel, workbenchContentVisible && performanceTab);
    setVisibleAndAccessible(chordModeSelector, workbenchContentVisible && performanceTab);
    setVisibleAndAccessible(phraseImportPathLabel, workbenchContentVisible && performanceTab);
    setVisibleAndAccessible(phraseImportPathEditor, workbenchContentVisible && performanceTab);
    setVisibleAndAccessible(phraseImportButton, workbenchContentVisible && performanceTab);
    setVisibleAndAccessible(performanceSummaryLabel, workbenchContentVisible && performanceTab && expanded);
    setVisibleAndAccessible(phraseSummaryLabel, workbenchContentVisible && performanceTab && expanded);
    setVisibleAndAccessible(roundRobinResetLabel, workbenchContentVisible && performanceTab);
    setVisibleAndAccessible(roundRobinResetSelector, workbenchContentVisible && performanceTab);
    setVisibleAndAccessible(roundRobinResetEventSelector, workbenchContentVisible && performanceTab);
    setVisibleAndAccessible(roundRobinResetTargetSelector, workbenchContentVisible && performanceTab);
    setVisibleAndAccessible(roundRobinResetAddButton, workbenchContentVisible && performanceTab);
    setVisibleAndAccessible(roundRobinResetDeleteButton, workbenchContentVisible && performanceTab);
    setVisibleAndAccessible(roundRobinResetSummaryLabel, workbenchContentVisible && performanceTab && expanded);

    setVisibleAndAccessible(articulationWorkbenchViewport, workbenchContentVisible && articulationsTab);
    setVisibleAndAccessible(articulationList, workbenchContentVisible && articulationsTab);
    setVisibleAndAccessible(articulationCreateButton, workbenchContentVisible && articulationsTab);
    setVisibleAndAccessible(articulationDuplicateButton, workbenchContentVisible && articulationsTab);
    setVisibleAndAccessible(articulationDefaultButton, workbenchContentVisible && articulationsTab);
    setVisibleAndAccessible(articulationMoveUpButton, workbenchContentVisible && articulationsTab);
    setVisibleAndAccessible(articulationMoveDownButton, workbenchContentVisible && articulationsTab);
    setVisibleAndAccessible(articulationDeleteButton, workbenchContentVisible && articulationsTab);
    setVisibleAndAccessible(articulationNameLabel, workbenchContentVisible && articulationsTab);
    setVisibleAndAccessible(articulationNameEditor, workbenchContentVisible && articulationsTab);
    setVisibleAndAccessible(articulationSwitchNoteLabel, workbenchContentVisible && articulationsTab);
    setVisibleAndAccessible(articulationSwitchNoteSlider, workbenchContentVisible && articulationsTab);
    setVisibleAndAccessible(articulationSwitchNoteValueLabel, workbenchContentVisible && articulationsTab);
    setVisibleAndAccessible(articulationClearSwitchButton, workbenchContentVisible && articulationsTab);
    setVisibleAndAccessible(articulationMidiLearnButton, workbenchContentVisible && articulationsTab);
    setVisibleAndAccessible(articulationDeleteReassignLabel, workbenchContentVisible && articulationsTab);
    setVisibleAndAccessible(articulationDeleteReassignSelector, workbenchContentVisible && articulationsTab);
    setVisibleAndAccessible(articulationStatusLabel, workbenchContentVisible && articulationsTab);
    workbenchWaveformTabButton.setToggleState(waveformTab, juce::dontSendNotification);
    workbenchGroupsTabButton.setToggleState(groupsTab, juce::dontSendNotification);
    workbenchMacrosTabButton.setToggleState(macrosTab, juce::dontSendNotification);
    workbenchRoutingTabButton.setToggleState(routingTab, juce::dontSendNotification);
    workbenchPerformanceTabButton.setToggleState(performanceTab, juce::dontSendNotification);
    workbenchArticulationsTabButton.setToggleState(articulationsTab, juce::dontSendNotification);
    const auto focusedWorkbenchContentBecameHidden = !workbenchContentVisible
        ? (focusWithinWaveform || focusWithinGroups || focusWithinMacros || focusWithinRouting || focusWithinPerformance || focusWithinArticulations)
        : (waveformTab ? (focusWithinGroups || focusWithinMacros || focusWithinRouting || focusWithinPerformance || focusWithinArticulations)
                       : groupsTab ? (focusWithinWaveform || focusWithinMacros || focusWithinRouting || focusWithinPerformance || focusWithinArticulations)
                                   : macrosTab ? (focusWithinWaveform || focusWithinGroups || focusWithinRouting || focusWithinPerformance || focusWithinArticulations)
                                               : routingTab ? (focusWithinWaveform || focusWithinGroups || focusWithinMacros || focusWithinPerformance || focusWithinArticulations)
                                                            : performanceTab ? (focusWithinWaveform || focusWithinGroups || focusWithinMacros || focusWithinRouting || focusWithinArticulations)
                                                                             : (focusWithinWaveform || focusWithinGroups || focusWithinMacros || focusWithinRouting || focusWithinPerformance));

    if (focusedWorkbenchContentBecameHidden)
    {
        if (!workbenchContentVisible)
            workbenchToggleButton.grabKeyboardFocus();
        else if (waveformTab)
            workbenchWaveformTabButton.grabKeyboardFocus();
        else if (groupsTab)
            workbenchGroupsTabButton.grabKeyboardFocus();
        else if (macrosTab)
            workbenchMacrosTabButton.grabKeyboardFocus();
        else if (routingTab)
            workbenchRoutingTabButton.grabKeyboardFocus();
        else if (performanceTab)
            workbenchPerformanceTabButton.grabKeyboardFocus();
        else
            workbenchArticulationsTabButton.grabKeyboardFocus();
    }
}

void AuthoringPanel::refreshContextualAccessibility()
{
    const auto& project = authoringSession.getProject();
    const auto describeCurrentValue = [](const juce::String& value, const juce::String& fallback)
    {
        const auto trimmed = value.trim();
        return trimmed.isNotEmpty() ? trimmed : fallback;
    };
    const auto selectedGroup = authoringSession.getSelectedGroup();
    const auto hasSelectedGroup = selectedGroup.has_value();
    const auto groupName = hasSelectedGroup
        ? juce::String::fromUTF8(selectedGroup->displayName.c_str())
        : juce::String("the selected group");
    const auto groupMemberCount = hasSelectedGroup
        ? static_cast<int>(countZonesInGroup(project, selectedGroup->id))
        : 0;
    const auto selectedZoneIdsForGrouping = collectSelectedZoneIdsForGrouping();
    const auto assignableZoneCount = hasSelectedGroup
        ? static_cast<int>(std::count_if(selectedZoneIdsForGrouping.begin(),
                                         selectedZoneIdsForGrouping.end(),
                                         [&](const std::string& zoneId)
                                         {
                                             const auto zoneIterator = std::find_if(project.authoring.zones.begin(),
                                                                                    project.authoring.zones.end(),
                                                                                    [&](const auto& zone)
                                                                                    {
                                                                                        return zone.id == zoneId;
                                                                                    });
                                             return zoneIterator != project.authoring.zones.end()
                                                 && zoneIterator->groupId != selectedGroup->id;
                                         }))
        : 0;

    updateAccessibleDescriptionAndHelpText(groupNameEditor,
                                           hasSelectedGroup
                                               ? "Renames " + groupName + "."
                                               : "Unavailable because no group is selected.",
                                           hasSelectedGroup
                                               ? "Type a new name for " + groupName + " and press Enter."
                                               : "Create or select a group before renaming it.");
    updateAccessibleDescriptionAndHelpText(groupCreateButton,
                                           "Creates a new empty group in the authored group list.",
                                           "Press to create a new empty group.");
    updateAccessibleDescriptionAndHelpText(groupPreviewAnchorButton,
                                           hasSelectedGroup
                                               ? "Auditions the anchor zone for " + groupName + "."
                                               : "Unavailable because no group is selected.",
                                           hasSelectedGroup
                                               ? "Press to preview the current audition anchor for " + groupName + "."
                                               : "Select a group before previewing its anchor zone.");
    updateAccessibleDescriptionAndHelpText(groupVisibilityButton,
                                           hasSelectedGroup
                                               ? (selectedGroup->workspaceVisible
                                                      ? "Hides " + groupName + " on the workspace map without changing audio."
                                                      : "Shows " + groupName + " on the workspace map without changing audio.")
                                               : "Unavailable because no group is selected.",
                                           hasSelectedGroup
                                               ? "Press to toggle the selected group's map visibility."
                                               : "Select a group before toggling visibility.");
    updateAccessibleDescriptionAndHelpText(groupMoveUpButton,
                                           hasSelectedGroup
                                               ? (groupMoveUpButton.isEnabled()
                                                      ? "Moves " + groupName + " earlier in group order."
                                                      : groupName + " is already the first group.")
                                               : "Unavailable because no group is selected.",
                                           hasSelectedGroup
                                               ? (groupMoveUpButton.isEnabled()
                                                      ? "Press to move " + groupName + " upward in group order."
                                                      : "Select a later group to move upward.")
                                               : "Select a group before reordering it.");
    updateAccessibleDescriptionAndHelpText(groupMoveDownButton,
                                           hasSelectedGroup
                                               ? (groupMoveDownButton.isEnabled()
                                                      ? "Moves " + groupName + " later in group order."
                                                      : groupName + " is already the last group.")
                                               : "Unavailable because no group is selected.",
                                           hasSelectedGroup
                                               ? (groupMoveDownButton.isEnabled()
                                                      ? "Press to move " + groupName + " downward in group order."
                                                      : "Select an earlier group to move downward.")
                                               : "Select a group before reordering it.");

    updateAccessibleDescriptionAndHelpText(groupVisibilityToggle,
                                           hasSelectedGroup
                                               ? "Toggles whether " + groupName + " is visible on the workspace map."
                                               : "Unavailable because no group is selected.",
                                           hasSelectedGroup
                                               ? "Press to toggle visibility for " + groupName + "."
                                               : "Select a group before changing workspace visibility.");
    updateAccessibleDescriptionAndHelpText(masterGainSlider,
                                           "Adjusts the top-level gain for "
                                               + juce::String::fromUTF8(project.displayName.c_str()) + ".",
                                           "Drag the slider or enter a master-gain value for the whole project.");
    updateAccessibleDescriptionAndHelpText(groupGainSlider,
                                           hasSelectedGroup
                                               ? "Adjusts gain for " + groupName + "."
                                               : "Unavailable because no group is selected.",
                                           hasSelectedGroup
                                               ? "Drag the slider or enter a gain value for " + groupName + "."
                                               : "Select a group before adjusting gain.");
    updateAccessibleDescriptionAndHelpText(groupPanSlider,
                                           hasSelectedGroup
                                               ? "Adjusts pan for " + groupName + "."
                                               : "Unavailable because no group is selected.",
                                           hasSelectedGroup
                                               ? "Drag the slider or enter a pan value for " + groupName + "."
                                               : "Select a group before adjusting pan.");
    updateAccessibleDescriptionAndHelpText(groupRoutingSelector,
                                           hasSelectedGroup
                                               ? "Chooses the routing bus fed by " + groupName + ". Current bus: "
                                                     + describeCurrentValue(groupRoutingSelector.getText(), "(direct)") + "."
                                               : "Unavailable because no group is selected.",
                                           hasSelectedGroup
                                               ? "Open the list to choose a routing bus for " + groupName + "."
                                               : "Select a group before choosing a routing bus.");
    updateAccessibleDescriptionAndHelpText(groupAnchorSelector,
                                           hasSelectedGroup
                                               ? "Chooses the audition anchor zone for " + groupName + ". Current anchor: "
                                                     + describeCurrentValue(groupAnchorSelector.getText(), "(none)") + "."
                                               : "Unavailable because no group is selected.",
                                           hasSelectedGroup
                                               ? "Open the list to choose an audition anchor for " + groupName + "."
                                               : "Select a group before choosing an audition anchor.");
    updateAccessibleDescriptionAndHelpText(groupDeleteButton,
                                           hasSelectedGroup
                                               ? (groupDeleteButton.isEnabled()
                                                      ? "Deletes " + groupName + " because it has no member zones."
                                                      : groupName + " cannot be deleted while it still owns member zones.")
                                               : "Unavailable because no group is selected.",
                                           hasSelectedGroup
                                               ? (groupDeleteButton.isEnabled()
                                                      ? "Press to delete the selected empty group."
                                                      : "Reassign every member zone before deleting this group.")
                                               : "Select a group before deleting it.");
    updateAccessibleDescriptionAndHelpText(groupAssignZonesButton,
                                           hasSelectedGroup
                                               ? (assignableZoneCount > 0
                                                      ? "Assigns " + juce::String(assignableZoneCount)
                                                            + " selected zone(s) into " + groupName + "."
                                                      : "No selected zones are waiting to be added to " + groupName + ".")
                                               : "Unavailable because no group is selected.",
                                           hasSelectedGroup
                                               ? (assignableZoneCount > 0
                                                      ? "Press to add the current zone-map selection into " + groupName + "."
                                                      : "Select one or more zones outside this group before adding them.")
                                               : "Select a group before assigning zones.");
    updateAccessibleDescriptionAndHelpText(groupSummaryLabel,
                                           hasSelectedGroup
                                               ? groupName + " currently owns " + juce::String(groupMemberCount) + " zones."
                                               : "No group is selected.",
                                           hasSelectedGroup
                                               ? "Review the selected group's summary before changing its mix or routing."
                                               : "Select a group to review its summary.");
    updateAccessibleDescriptionAndHelpText(groupRoundRobinLabel,
                                           hasSelectedGroup
                                               ? "Summarizes round-robin state for " + groupName + "."
                                               : "No group is selected.",
                                           hasSelectedGroup
                                               ? "Review the selected group's round-robin summary."
                                               : "Select a group to review round-robin state.");
    updateAccessibleDescriptionAndHelpText(groupRoundRobinHintLabel,
                                           hasSelectedGroup
                                               ? "Guides round-robin editing for " + groupName + "."
                                               : "No group is selected.",
                                           hasSelectedGroup
                                               ? "Read the selected group's round-robin guidance."
                                               : "Select a group to review round-robin guidance.");
    updateAccessibleDescriptionAndHelpText(groupRoundRobinToggle,
                                           hasSelectedGroup
                                               ? "Enables all-or-nothing Round Robin for " + groupName + "."
                                               : "Unavailable because no group is selected.",
                                           hasSelectedGroup
                                               ? "Every mapping must contain at least two exact-match zones."
                                               : "Select a group before enabling Round Robin.");
    updateAccessibleDescriptionAndHelpText(groupRoundRobinModeSelector,
                                           hasSelectedGroup
                                               ? "Chooses cycle or random Round Robin for " + groupName + "."
                                               : "Unavailable because no group is selected.",
                                           hasSelectedGroup
                                               ? "Enable Round Robin before changing its mode."
                                               : "Select a group before choosing a Round Robin mode.");

    const auto hasSelectedMacro = !project.authoring.macros.empty()
        && selectedMacroIndex >= 0
        && static_cast<std::size_t>(selectedMacroIndex) < project.authoring.macros.size();
    const auto macroName = hasSelectedMacro
        ? juce::String::fromUTF8(project.authoring.macros[static_cast<std::size_t>(selectedMacroIndex)].name.c_str())
        : juce::String("the selected macro");
    const auto macroTargetCount = hasSelectedMacro
        ? static_cast<int>(project.authoring.macros[static_cast<std::size_t>(selectedMacroIndex)].targets.size())
        : 0;
    const auto hasSelectedMacroTarget = hasSelectedMacro
        && selectedMacroTargetIndex >= 0 && selectedMacroTargetIndex < macroTargetCount;

    updateAccessibleDescriptionAndHelpText(macroCreateButton,
                                           "Creates a new authored macro with sensible defaults.",
                                           "Press to create a new macro in the project.");
    updateAccessibleDescriptionAndHelpText(macroDuplicateButton,
                                           hasSelectedMacro
                                               ? "Duplicates " + macroName + "."
                                               : "Unavailable because no macro is selected.",
                                           hasSelectedMacro
                                               ? "Press to duplicate " + macroName + "."
                                               : "Select a macro before duplicating it.");
    updateAccessibleDescriptionAndHelpText(macroDeleteButton,
                                           hasSelectedMacro
                                               ? "Deletes " + macroName + "."
                                               : "Unavailable because no macro is selected.",
                                           hasSelectedMacro
                                               ? "Press to delete " + macroName + "."
                                               : "Select a macro before deleting it.");
    updateAccessibleDescriptionAndHelpText(macroNameEditor,
                                           hasSelectedMacro
                                               ? "Renames " + macroName + "."
                                               : "Unavailable because no macro is selected.",
                                           hasSelectedMacro
                                               ? "Type a new name for " + macroName + " and press Enter."
                                               : "Create or select a macro before renaming it.");
    updateAccessibleDescriptionAndHelpText(macroExposeToggle,
                                           hasSelectedMacro
                                               ? "Chooses whether " + macroName + " appears in Perform."
                                               : "Unavailable because no macro is selected.",
                                           hasSelectedMacro
                                               ? "Press to expose or hide " + macroName + " in the Perform surface."
                                               : "Create or select a macro before changing its Perform visibility.");
    updateAccessibleDescriptionAndHelpText(macroAssignmentSelector,
                                           hasSelectedMacroTarget
                                               ? "Chooses the parameter assigned to the selected target on " + macroName + "."
                                               : hasSelectedMacro
                                                   ? "Chooses the parameter assigned to the first target on " + macroName + "."
                                                   : "Unavailable because no macro is selected.",
                                           hasSelectedMacro
                                               ? "Open the list to choose a supported parameter target for " + macroName + "."
                                               : "Author a macro before editing its parameter assignment.");
    updateAccessibleDescriptionAndHelpText(macroRoleSelector,
                                           hasSelectedMacroTarget
                                               ? "Chooses the semantic role for the selected target on " + macroName + "."
                                               : hasSelectedMacro
                                                   ? "Choose a target before assigning its semantic role on " + macroName + "."
                                               : "Unavailable because no macro is selected.",
                                           hasSelectedMacroTarget
                                               ? "Open the list to choose a role for the selected target."
                                               : "Author a macro before editing its role.");
    updateAccessibleDescriptionAndHelpText(macroAssignmentList,
                                           hasSelectedMacro
                                               ? macroName + " has " + juce::String(macroTargetCount)
                                                     + " assigned target(s)."
                                               : "Unavailable because no macro is selected.",
                                           hasSelectedMacroTarget
                                               ? "Use arrow keys to choose the target edited by the Selected Target controls."
                                               : "Add or choose a supported target for the selected macro.");
    updateAccessibleDescriptionAndHelpText(macroAssignmentAddButton,
                                           hasSelectedMacro
                                               ? "Adds the next supported unassigned target to " + macroName + "."
                                               : "Unavailable because no macro is selected.",
                                           hasSelectedMacro
                                               ? "Press to add a curated target through the existing macro transaction."
                                               : "Create or select a macro before adding a target.");
    updateAccessibleDescriptionAndHelpText(macroAssignmentRemoveButton,
                                           hasSelectedMacroTarget
                                               ? "Removes target " + juce::String(selectedMacroTargetIndex + 1)
                                                     + " from " + macroName + "."
                                               : "Unavailable because no macro target is selected.",
                                           hasSelectedMacroTarget
                                               ? "Press to remove the selected target through the existing macro transaction."
                                               : "Select an assigned target before removing it.");
    updateAccessibleDescriptionAndHelpText(macroDefaultSlider,
                                           hasSelectedMacro
                                               ? "Adjusts the default value for " + macroName + "."
                                               : "Unavailable because no macro is selected.",
                                           hasSelectedMacro
                                               ? "Drag the slider or enter a numeric default value for " + macroName + "."
                                               : "Author a macro before editing its default value.");
    updateAccessibleDescriptionAndHelpText(macroMinSlider,
                                           hasSelectedMacro
                                               ? "Adjusts the minimum value for " + macroName + "."
                                               : "Unavailable because no macro is selected.",
                                           hasSelectedMacro
                                               ? "Drag the slider or enter a numeric minimum value for " + macroName + "."
                                               : "Author a macro before editing its range.");
    updateAccessibleDescriptionAndHelpText(macroMaxSlider,
                                           hasSelectedMacro
                                               ? "Adjusts the maximum value for " + macroName + "."
                                               : "Unavailable because no macro is selected.",
                                           hasSelectedMacro
                                               ? "Drag the slider or enter a numeric maximum value for " + macroName + "."
                                               : "Author a macro before editing its range.");
    updateAccessibleDescriptionAndHelpText(macroMoveUpButton,
                                           hasSelectedMacro
                                               ? (macroMoveUpButton.isEnabled()
                                                      ? "Moves " + macroName + " earlier in the list."
                                                      : macroName + " is already the first macro.")
                                               : "Unavailable because no macros are authored.",
                                           hasSelectedMacro
                                               ? (macroMoveUpButton.isEnabled()
                                                      ? "Press to move " + macroName + " toward the start of the macro list."
                                                      : "Select a later macro to enable moving upward.")
                                               : "Author macros before changing their order.");
    updateAccessibleDescriptionAndHelpText(macroMoveDownButton,
                                           hasSelectedMacro
                                               ? (macroMoveDownButton.isEnabled()
                                                      ? "Moves " + macroName + " later in the list."
                                                      : macroName + " is already the last macro.")
                                               : "Unavailable because no macros are authored.",
                                           hasSelectedMacro
                                               ? (macroMoveDownButton.isEnabled()
                                                      ? "Press to move " + macroName + " toward the end of the macro list."
                                                      : "Select an earlier macro to enable moving downward.")
                                               : "Author macros before changing their order.");

    const auto hasSelectedFxSlot = !project.authoring.fxSlots.empty()
        && selectedFxSlotIndex >= 0
        && static_cast<std::size_t>(selectedFxSlotIndex) < project.authoring.fxSlots.size();
    const auto fxName = hasSelectedFxSlot
        ? juce::String::fromUTF8(project.authoring.fxSlots[static_cast<std::size_t>(selectedFxSlotIndex)].displayName.c_str())
        : juce::String("the selected FX slot");
    const auto fxType = hasSelectedFxSlot
        ? describeCurrentValue(fxTypeSelector.getText(), "(unspecified)")
        : juce::String{};
    const auto fxState = hasSelectedFxSlot
        ? juce::String(project.authoring.fxSlots[static_cast<std::size_t>(selectedFxSlotIndex)].bypassed ? "bypassed"
                                                                                                            : "active")
        : juce::String{};
    const auto hasSelectedFxParameter = hasSelectedFxSlot
        && selectedFxParameterIndex >= 0
        && static_cast<std::size_t>(selectedFxParameterIndex) < fxParameterIds.size();
    const auto fxParameterName = hasSelectedFxParameter
        ? formatDspParameterName(fxParameterIds[static_cast<std::size_t>(selectedFxParameterIndex)])
        : juce::String("the selected parameter");
    const auto existingMacroIndex = hasSelectedFxParameter
        ? findMacroIndexForDspTarget(project,
                                     project.authoring.fxSlots[static_cast<std::size_t>(selectedFxSlotIndex)].id,
                                     fxParameterIds[static_cast<std::size_t>(selectedFxParameterIndex)])
        : std::optional<std::size_t> {};

    updateAccessibleDescriptionAndHelpText(fxSelector,
                                           hasSelectedFxSlot
                                               ? "Chooses the active FX slot for routing detail. Current FX slot: " + fxName + "."
                                               : "Unavailable because no FX slots are authored.",
                                           hasSelectedFxSlot
                                               ? "Open the list to switch routing detail to another FX slot."
                                               : "Author an FX slot before editing routing FX detail.");
    updateAccessibleDescriptionAndHelpText(fxTypeSelector,
                                           hasSelectedFxSlot
                                               ? "Chooses the effect type for " + fxName + ". Current effect type: " + fxType + "."
                                               : "Unavailable because no FX slots are authored.",
                                           hasSelectedFxSlot
                                               ? "Open the list to choose a new effect type for " + fxName + "."
                                               : "Author an FX slot before choosing an effect type.");
    updateAccessibleDescriptionAndHelpText(fxBypassedToggle,
                                           hasSelectedFxSlot
                                               ? "Toggles bypass for " + fxName + ". Current state: " + fxState + "."
                                               : "Unavailable because no FX slots are authored.",
                                           hasSelectedFxSlot
                                               ? "Press to toggle whether " + fxName + " is bypassed."
                                               : "Author an FX slot before toggling bypass.");
    updateAccessibleDescriptionAndHelpText(fxAssignMacroButton,
                                           hasSelectedFxParameter
                                               ? (existingMacroIndex.has_value()
                                                      ? "Opens the authored control bound to " + fxParameterName
                                                            + " on " + fxName + "."
                                                      : "Creates a new performance control from " + fxParameterName
                                                            + " on " + fxName + ".")
                                               : "Unavailable because no FX parameter is selected.",
                                           hasSelectedFxParameter
                                               ? (existingMacroIndex.has_value()
                                                      ? "Press to jump to the macro that already controls this parameter."
                                                      : "Press to create a visible control from the selected routing parameter.")
                                               : "Select a curated FX parameter before creating a control.");

    const auto hasSelectedRoutingBus = !project.authoring.routingBuses.empty()
        && selectedRoutingBusIndex >= 0
        && static_cast<std::size_t>(selectedRoutingBusIndex) < project.authoring.routingBuses.size();
    const auto busName = hasSelectedRoutingBus
        ? juce::String::fromUTF8(project.authoring.routingBuses[static_cast<std::size_t>(selectedRoutingBusIndex)].displayName.c_str())
        : juce::String("the selected routing bus");
    const auto inputSource = hasSelectedRoutingBus
        ? describeCurrentValue(routingInputSelector.getText(), "(none)")
        : juce::String{};
    const auto insertOne = hasSelectedRoutingBus
        ? (project.authoring.routingBuses[static_cast<std::size_t>(selectedRoutingBusIndex)].fxSlotIds.empty()
               ? juce::String("(none)")
               : juce::String::fromUTF8(project.authoring.routingBuses[static_cast<std::size_t>(selectedRoutingBusIndex)].fxSlotIds.front().c_str()))
        : juce::String{};
    const auto insertTwo = hasSelectedRoutingBus
        ? (project.authoring.routingBuses[static_cast<std::size_t>(selectedRoutingBusIndex)].fxSlotIds.size() < 2
               ? juce::String("(none)")
               : juce::String::fromUTF8(project.authoring.routingBuses[static_cast<std::size_t>(selectedRoutingBusIndex)].fxSlotIds[1].c_str()))
        : juce::String{};

    updateAccessibleDescriptionAndHelpText(routingBusSelector,
                                           hasSelectedRoutingBus
                                               ? "Chooses the active routing bus. Current bus: " + busName + "."
                                               : "Unavailable because no routing buses are authored.",
                                           hasSelectedRoutingBus
                                               ? "Open the list to switch routing detail to another routing bus."
                                               : "Author a routing bus before editing its signal path.");
    updateAccessibleDescriptionAndHelpText(routingInputSelector,
                                           hasSelectedRoutingBus
                                               ? "Chooses the input source for " + busName + ". Current source: " + inputSource + "."
                                               : "Unavailable because no routing buses are authored.",
                                           hasSelectedRoutingBus
                                               ? "Open the list to choose a new input source for " + busName + "."
                                               : "Author a routing bus before choosing an input source.");
    updateAccessibleDescriptionAndHelpText(routingInsertOneSelector,
                                           hasSelectedRoutingBus
                                               ? "Chooses the first insert effect for " + busName + ". Current insert A: " + insertOne + "."
                                               : "Unavailable because no routing buses are authored.",
                                           hasSelectedRoutingBus
                                               ? "Open the list to choose the first insert effect for " + busName + "."
                                               : "Author a routing bus before assigning insert effects.");
    updateAccessibleDescriptionAndHelpText(routingInsertTwoSelector,
                                           hasSelectedRoutingBus
                                               ? "Chooses the second insert effect for " + busName + ". Current insert B: " + insertTwo + "."
                                               : "Unavailable because no routing buses are authored.",
                                           hasSelectedRoutingBus
                                               ? "Open the list to choose the second insert effect for " + busName + "."
                                               : "Author a routing bus before assigning insert effects.");

    if (const auto selectedPerformanceBank = authoringSession.getSelectedPerformanceBank(); selectedPerformanceBank.has_value())
    {
        const auto bankName = juce::String::fromUTF8(selectedPerformanceBank->displayName.c_str());
        const auto hasSelectedTriggerSlot = selectedTriggerSlotIndex >= 0
            && static_cast<std::size_t>(selectedTriggerSlotIndex) < selectedPerformanceBank->triggerSlots.size();
        const auto triggerName = hasSelectedTriggerSlot
            ? juce::String::fromUTF8(selectedPerformanceBank->triggerSlots[static_cast<std::size_t>(selectedTriggerSlotIndex)].displayName.c_str())
            : juce::String("the selected trigger slot");
        const auto triggerEvent = hasSelectedTriggerSlot
            ? describeCurrentValue(juce::String::fromUTF8(
                                       selectedPerformanceBank->triggerSlots[static_cast<std::size_t>(selectedTriggerSlotIndex)].triggerEvent.c_str()),
                                   "(unspecified)")
            : juce::String{};
        const auto targetArticulation = hasSelectedTriggerSlot
            ? describeCurrentValue(
                  juce::String::fromUTF8(
                      selectedPerformanceBank->triggerSlots[static_cast<std::size_t>(selectedTriggerSlotIndex)].targetArticulationId.c_str()),
                  "(none)")
            : juce::String{};
        const auto selectedPhraseText = describeCurrentValue(phraseAssetSelector.getText(), "(none)");
        const auto selectedChordMode = describeCurrentValue(chordModeSelector.getText(), "(unspecified)");
        const auto midiPath = phraseImportPathEditor.getText().trim();

        updateAccessibleDescriptionAndHelpText(performanceBankSelector,
                                               "Chooses the active performance bank. Current bank: " + bankName + ".",
                                               "Open the list to switch to another performance bank.");
        updateAccessibleDescriptionAndHelpText(triggerSlotSelector,
                                               hasSelectedTriggerSlot
                                                   ? "Chooses the active trigger slot within " + bankName + ". Current trigger slot: " + triggerName + "."
                                                   : "Unavailable because " + bankName + " has no trigger slots.",
                                               hasSelectedTriggerSlot
                                                   ? "Open the list to choose a different trigger slot in " + bankName + "."
                                                   : "Author a trigger slot in " + bankName + " before editing trigger detail.");
        updateAccessibleDescriptionAndHelpText(triggerEventSelector,
                                               hasSelectedTriggerSlot
                                                   ? "Chooses the event that activates " + triggerName + " in " + bankName
                                                         + ". Current event: " + triggerEvent + "."
                                                   : "Unavailable because no trigger slot is selected in " + bankName + ".",
                                               hasSelectedTriggerSlot
                                                   ? "Open the list to choose the trigger event for " + triggerName + "."
                                                   : "Select a trigger slot before changing its trigger event.");
        updateAccessibleDescriptionAndHelpText(targetArticulationSelector,
                                               hasSelectedTriggerSlot
                                                   ? "Chooses the articulation targeted by " + triggerName + " in " + bankName
                                                         + ". Current articulation: " + targetArticulation + "."
                                                   : "Unavailable because no trigger slot is selected in " + bankName + ".",
                                               hasSelectedTriggerSlot
                                                   ? "Open the list to choose the target articulation for " + triggerName + "."
                                                   : "Select a trigger slot before changing its target articulation.");
        updateAccessibleDescriptionAndHelpText(phraseAssetSelector,
                                               hasSelectedTriggerSlot
                                                   ? "Chooses the phrase asset for " + triggerName + " in " + bankName
                                                         + ". Current phrase asset: " + selectedPhraseText + "."
                                                   : "Unavailable because no trigger slot is selected in " + bankName + ".",
                                               hasSelectedTriggerSlot
                                                   ? "Open the list to choose the phrase asset used by " + triggerName + "."
                                                   : "Select a trigger slot before assigning a phrase asset.");
        updateAccessibleDescriptionAndHelpText(chordModeSelector,
                                               hasSelectedTriggerSlot
                                                   ? "Chooses the chord-follow behavior for " + triggerName + " in " + bankName
                                                         + ". Current chord rule: " + selectedChordMode + "."
                                                   : "Unavailable because no trigger slot is selected in " + bankName + ".",
                                               hasSelectedTriggerSlot
                                                   ? "Open the list to choose the chord-follow rule for " + triggerName + "."
                                                   : "Select a trigger slot before changing its chord-follow rule.");
        updateAccessibleDescriptionAndHelpText(phraseImportPathEditor,
                                               midiPath.isEmpty()
                                                   ? "Edits the MIDI import path for " + bankName + ". No MIDI file path is entered yet."
                                                   : "Edits the MIDI import path for " + bankName + ". Current path: " + midiPath,
                                               midiPath.isEmpty()
                                                   ? "Type a MIDI file path before pressing Import MIDI Phrase."
                                                   : "Edit the current MIDI file path before importing it into " + bankName + ".");
        updateAccessibleDescriptionAndHelpText(phraseImportButton,
                                               midiPath.isEmpty()
                                                   ? "Unavailable until a MIDI file path is entered for " + bankName + "."
                                                   : "Imports the MIDI phrase at " + midiPath + " into " + bankName + ".",
                                               midiPath.isEmpty()
                                                   ? "Enter a MIDI file path before importing."
                                                   : "Press to import the current MIDI file into " + bankName + ".");
    }
    else
    {
        updateAccessibleDescriptionAndHelpText(performanceBankSelector,
                                               "Unavailable because no performance bank is selected.",
                                               "Select a performance bank before editing performance trigger detail.");
        updateAccessibleDescriptionAndHelpText(triggerSlotSelector,
                                               "Unavailable because no performance bank is selected.",
                                               "Select a performance bank before choosing a trigger slot.");
        updateAccessibleDescriptionAndHelpText(triggerEventSelector,
                                               "Unavailable because no performance bank is selected.",
                                               "Select a performance bank and trigger slot before changing the trigger event.");
        updateAccessibleDescriptionAndHelpText(targetArticulationSelector,
                                               "Unavailable because no performance bank is selected.",
                                               "Select a performance bank and trigger slot before changing the target articulation.");
        updateAccessibleDescriptionAndHelpText(phraseAssetSelector,
                                               "Unavailable because no performance bank is selected.",
                                               "Select a performance bank and trigger slot before assigning a phrase asset.");
        updateAccessibleDescriptionAndHelpText(chordModeSelector,
                                               "Unavailable because no performance bank is selected.",
                                               "Select a performance bank and trigger slot before changing the chord-follow rule.");
        updateAccessibleDescriptionAndHelpText(phraseImportPathEditor,
                                               "Unavailable because no performance bank is selected.",
                                               "Select a performance bank before entering a MIDI file path.");
        updateAccessibleDescriptionAndHelpText(phraseImportButton,
                                               "Unavailable because no performance bank is selected.",
                                               "Select a performance bank before importing a phrase.");
    }
}

void AuthoringPanel::refreshInspectorVisibility()
{
    const auto showingMap = structureViewState.isMapPaneVisible();
    // The Map remains available while a workbench editor is open below it.
    zoneMap.setVisible(showingMap);
    structureBrowser.setVisible(true);
    // Layer/group contexts use the unified structure inspector. Zone context
    // keeps the mature Zone Mapping editor because it exposes the complete
    // editable mapping surface (root key, ranges, crossfades, release, etc.).
    const auto showingZoneEditor = showingMap
        && structureSelection.getKind() == authoring::StructureSelectionKind::zone;
    zoneMappingEditor.setVisible(showingZoneEditor);
    structureInspector.setVisible(!showingZoneEditor);

    showMapButton.setToggleState(showingMap, juce::dontSendNotification);

    refreshWorkbenchVisibility();
}

void AuthoringPanel::refreshWorkbenchContextLabels()
{
    const auto& project = authoringSession.getProject();

    switch (workbenchState.activeTab)
    {
        case authoring::WorkbenchTab::waveform:
        {
            waveformLabel.setText("Sample Region", juce::dontSendNotification);
            waveformScopeLabel.setText("Zone-scoped playback, loop, and selection",
                                       juce::dontSendNotification);

            if (const auto selectedZone = authoringSession.getSelectedZone(); selectedZone.has_value())
            {
                workbenchBreadcrumbLabel.setText("Project > Zones > "
                                                  + juce::String::fromUTF8(selectedZone->displayName.c_str()),
                                              juce::dontSendNotification);
            }
            else
            {
                workbenchBreadcrumbLabel.setText("Project > Zones", juce::dontSendNotification);
            }
            break;
        }
        case authoring::WorkbenchTab::macros:
        {
            waveformLabel.setText("Macro Assignment", juce::dontSendNotification);
            waveformScopeLabel.setText("Project-scoped automation detail", juce::dontSendNotification);

            juce::String breadcrumb = "Project > Macros";
            if (!project.authoring.macros.empty()
                && static_cast<std::size_t>(selectedMacroIndex) < project.authoring.macros.size())
            {
                breadcrumb << " > "
                           << juce::String::fromUTF8(project.authoring.macros[static_cast<std::size_t>(selectedMacroIndex)].name.c_str());
            }
            workbenchBreadcrumbLabel.setText(breadcrumb, juce::dontSendNotification);
            break;
        }
        case authoring::WorkbenchTab::groups:
        {
            waveformLabel.setText("Group Inspector", juce::dontSendNotification);
            waveformScopeLabel.setText("Group-scoped mix and visibility detail", juce::dontSendNotification);

            juce::String breadcrumb = "Project > Groups";
            if (const auto selectedGroup = authoringSession.getSelectedGroup(); selectedGroup.has_value())
                breadcrumb << " > " << juce::String::fromUTF8(selectedGroup->displayName.c_str());

            workbenchBreadcrumbLabel.setText(breadcrumb, juce::dontSendNotification);
            break;
        }
        case authoring::WorkbenchTab::routing:
        {
            waveformLabel.setText("Routing Detail", juce::dontSendNotification);
            waveformScopeLabel.setText("Project-scoped FX and bus detail", juce::dontSendNotification);

            juce::String fxName = "(none)";
            if (!project.authoring.fxSlots.empty()
                && static_cast<std::size_t>(selectedFxSlotIndex) < project.authoring.fxSlots.size())
            {
                fxName = juce::String::fromUTF8(project.authoring.fxSlots[static_cast<std::size_t>(selectedFxSlotIndex)].displayName.c_str());
            }

            juce::String busName = "(none)";
            if (!project.authoring.routingBuses.empty()
                && static_cast<std::size_t>(selectedRoutingBusIndex) < project.authoring.routingBuses.size())
            {
                busName = juce::String::fromUTF8(project.authoring.routingBuses[static_cast<std::size_t>(selectedRoutingBusIndex)].displayName.c_str());
            }

            workbenchBreadcrumbLabel.setText("Project > Routing > FX: " + fxName + " | Bus: " + busName,
                                          juce::dontSendNotification);
            break;
        }
        case authoring::WorkbenchTab::performance:
        {
            waveformLabel.setText("Performance Detail", juce::dontSendNotification);

            if (const auto selectedPerformanceBank = authoringSession.getSelectedPerformanceBank();
                selectedPerformanceBank.has_value())
            {
                juce::String breadcrumb = "Project > Performance > "
                    + juce::String::fromUTF8(selectedPerformanceBank->displayName.c_str());

                if (selectedTriggerSlotIndex >= 0
                    && static_cast<std::size_t>(selectedTriggerSlotIndex) < selectedPerformanceBank->triggerSlots.size())
                {
                    waveformScopeLabel.setText("Bank-scoped trigger detail", juce::dontSendNotification);
                    breadcrumb << " > "
                               << juce::String::fromUTF8(
                                      selectedPerformanceBank->triggerSlots[static_cast<std::size_t>(selectedTriggerSlotIndex)].displayName.c_str());
                }
                else
                {
                    waveformScopeLabel.setText("Bank-scoped performance detail", juce::dontSendNotification);
                }

                workbenchBreadcrumbLabel.setText(breadcrumb, juce::dontSendNotification);
            }
            else
            {
                waveformScopeLabel.setText("Bank-scoped performance detail", juce::dontSendNotification);
                workbenchBreadcrumbLabel.setText("Project > Performance", juce::dontSendNotification);
            }
            break;
        }
        case authoring::WorkbenchTab::articulations:
        {
            waveformLabel.setText("Articulation Detail", juce::dontSendNotification);
            waveformScopeLabel.setText("Project-scoped articulation and key-switch detail",
                                       juce::dontSendNotification);
            const auto articulations = authoringSession.getArticulations();
            const auto name = selectedArticulationIndex >= 0
                    && static_cast<std::size_t>(selectedArticulationIndex) < articulations.size()
                ? juce::String::fromUTF8(articulations[static_cast<std::size_t>(selectedArticulationIndex)].displayName.c_str())
                : juce::String("(none)");
            workbenchBreadcrumbLabel.setText("Project > Articulations > " + name,
                                          juce::dontSendNotification);
            break;
        }
        default:
            break;
    }

    updateDynamicAccessibleText(waveformLabel, waveformLabel.getText(), "Active workbench title: ");
    updateDynamicAccessibleText(waveformScopeLabel, waveformScopeLabel.getText(), "Active workbench scope: ");
    updateDynamicAccessibleText(workbenchBreadcrumbLabel, workbenchBreadcrumbLabel.getText(), "Active workbench breadcrumb: ");
}

void AuthoringPanel::refreshWaveformWorkbenchContent()
{
    AuthoringWaveformPreview preview;
    const auto structureKind = structureSelection.getKind();
    const auto hasZoneScopedSelection = structureKind == authoring::StructureSelectionKind::zone
        || structureKind == authoring::StructureSelectionKind::none;
    if (hasZoneScopedSelection && waveformPreviewProvider)
        preview = waveformPreviewProvider();
    else if (!hasZoneScopedSelection)
    {
        // Group and layer selections do not identify one sample source. Clear
        // the previous zone waveform instead of leaving stale peaks visible.
        waveformAuditionCueActive = false;
        waveformAuditionInitialFrame = 0;
        waveformAuditionRegion = {};
        preview.presentationState = AuthoringWaveformPresentationState::idle;
        preview.state = "Select a zone to view waveform";
    }

    AuthoringPreviewStatusSnapshot previewStatus;
    if (authoringPreviewStatusProvider)
        previewStatus = authoringPreviewStatusProvider();

    if (waveformAuditionCueActive && preview.available && preview.sampleRate > 0.0)
    {
        const auto audition = waveformAuditionRegion.valid
            ? waveformAuditionRegion
            : drs::engine::resolveWaveformAuditionRegion(
                drs::engine::WaveformAuditionMode::playbackRegion,
                { { preview.playbackStartFrame,
                    preview.playbackEndFrameExclusive == 0
                        ? preview.frameCount : preview.playbackEndFrameExclusive },
                  { preview.loopStartFrame, preview.loopEndFrame }, preview.loopEnabled },
                {}, preview.frameCount);
        const auto elapsedSeconds = std::max(
            0.0, (juce::Time::getMillisecondCounterHiRes() - waveformAuditionCueStartedMillis) / 1000.0);
        const auto noteOffSeconds = waveformAuditionNoteOffMillis > 0.0
            ? std::max(0.0, (waveformAuditionNoteOffMillis - waveformAuditionCueStartedMillis) / 1000.0)
            : elapsedSeconds;
        const auto noteStillHeld = audition.loopActive && waveformAuditionNoteOffMillis <= 0.0;
        const auto heldSeconds = audition.loopActive ? noteOffSeconds
            : static_cast<double>(audition.playback.length()) / preview.sampleRate;
        const auto compatibilityReleaseSeconds = 2048.0 / preview.sampleRate;
        const auto releaseSeconds = preview.releaseSeconds > 0.0
            ? preview.releaseSeconds : compatibilityReleaseSeconds;
        const auto cueDuration = noteStillHeld
            ? std::numeric_limits<double>::infinity()
            : heldSeconds + releaseSeconds;

        const auto linearFrameAt = [&](const double seconds)
        {
            const auto advanced = static_cast<long double>(seconds)
                * static_cast<long double>(preview.sampleRate);
            const auto initialFrame = std::clamp(waveformAuditionInitialFrame,
                                                 audition.playback.startFrame,
                                                 audition.playback.endFrameExclusive - 1);
            const auto maximumAdvance = static_cast<long double>(
                std::numeric_limits<std::uint64_t>::max() - initialFrame);
            return initialFrame
                + static_cast<std::uint64_t>(std::min(advanced, maximumAdvance));
        };
        const auto wrapFrame = [&](const std::uint64_t linearFrame)
        {
            if (!audition.loopActive || audition.loop.empty()
                || linearFrame < audition.loop.startFrame)
                return linearFrame;
            const auto loopLength = audition.loop.length();
            return audition.loop.startFrame + (linearFrame - audition.loop.startFrame) % loopLength;
        };

        auto playheadFrame = linearFrameAt(elapsedSeconds);
        if (audition.loopActive
            && waveformAuditionLoopMode == drs::engine::RegionLoopMode::loopContinuous)
            playheadFrame = wrapFrame(playheadFrame);
        else if (audition.loopActive
                 && waveformAuditionLoopMode == drs::engine::RegionLoopMode::loopSustain)
        {
            if (noteStillHeld || elapsedSeconds <= heldSeconds)
                playheadFrame = wrapFrame(playheadFrame);
            else
            {
                const auto noteOffFrame = wrapFrame(linearFrameAt(heldSeconds));
                const auto tailAdvance = static_cast<std::uint64_t>(
                    (elapsedSeconds - heldSeconds) * preview.sampleRate);
                playheadFrame = noteOffFrame > std::numeric_limits<std::uint64_t>::max() - tailAdvance
                    ? std::numeric_limits<std::uint64_t>::max()
                    : noteOffFrame + tailAdvance;
            }
        }

        if (elapsedSeconds <= cueDuration
            && playheadFrame < audition.playback.endFrameExclusive)
        {
            preview.playheadVisible = true;
            preview.playheadFrame = playheadFrame;
        }
        else
        {
            waveformAuditionCueActive = false;
            waveformAuditionNoteOffMillis = 0.0;
            waveformAuditionInitialFrame = 0;
            waveformAuditionRegion = {};
        }
    }

    waveformPreview.setPreview(preview);

    const auto modeSelectorId = [&]
    {
        switch (preview.loopMode)
        {
            case drs::engine::RegionLoopMode::noLoop: return 1;
            case drs::engine::RegionLoopMode::oneShot: return 1;
            case drs::engine::RegionLoopMode::loopContinuous: return 3;
            case drs::engine::RegionLoopMode::loopSustain: return 4;
        }
        return 1;
    }();
    waveformLoopModeSelector.setSelectedId(modeSelectorId, juce::dontSendNotification);
    const auto playbackEnd = preview.playbackEndFrameExclusive == 0
        ? preview.frameCount : preview.playbackEndFrameExclusive;
    const auto refreshFrameEditor = [](juce::TextEditor& editor, const std::uint64_t value)
    {
        // The status timer refreshes this panel four times per second. Preserve
        // in-progress numeric input until Return or focus loss commits it.
        if (!editor.hasKeyboardFocus(false))
            editor.setText(juce::String(value), juce::dontSendNotification);
    };
    refreshFrameEditor(waveformPlaybackStartEditor, preview.playbackStartFrame);
    refreshFrameEditor(waveformPlaybackEndEditor, playbackEnd);
    refreshFrameEditor(waveformLoopStartEditor, preview.loopStartFrame);
    refreshFrameEditor(waveformLoopEndEditor, preview.loopEndFrame);
    refreshFrameEditor(waveformLoopCrossfadeEditor, preview.loopCrossfadeFrames);
    const auto hasEditableSource = preview.available && preview.frameCount > preview.playbackStartFrame;
    waveformLoopModeSelector.setEnabled(hasEditableSource);
    waveformLoopStartEditor.setEnabled(hasEditableSource && preview.loopEnabled);
    waveformLoopEndEditor.setEnabled(hasEditableSource && preview.loopEnabled);
    waveformLoopCrossfadeEditor.setEnabled(hasEditableSource && preview.loopEnabled);
    waveformSetLoopSelectionButton.setEnabled(hasEditableSource && waveformPreview.hasSelection());
    constexpr auto inspectorPreviewSource = drs::engine::AuthoringPreviewAuditionSource::inspector;
    const auto inspectorPreviewIndex = static_cast<std::size_t>(inspectorPreviewSource);
    const auto loopAuditionHeld = waveformAuditionCueActive
        && waveformAuditionRegion.loopActive
        && inspectorPreviewIndex < timedPreviewNotes.size()
        && timedPreviewNotes[inspectorPreviewIndex].active;
    waveformLoopAuditionButton.setButtonText(loopAuditionHeld ? "Release Loop" : "Play Loop");
    waveformLoopAuditionButton.setEnabled(loopAuditionHeld
        || (hasEditableSource && preview.loopEnabled && previewStatus.auditionAvailable));
    const auto sustainLoopAudition = preview.loopMode == drs::engine::RegionLoopMode::loopSustain;
    updateAccessibleDescriptionAndHelpText(
        waveformLoopAuditionButton,
        loopAuditionHeld
            ? (sustainLoopAudition
                ? "Releases the held loop audition so playback enters its authored tail."
                : "Releases the held loop audition so its release fades while the loop continues.")
            : "Starts near the loop seam and keeps playing so the wrap and crossfade can be evaluated.",
        loopAuditionHeld
            ? (sustainLoopAudition
                ? "Press to leave the sustain loop and hear the post-loop tail."
                : "Press to hear the always-loop release behavior.")
            : "Press to hear the loop seam immediately, then press Release Loop when ready.");
    waveformPlaybackStartEditor.setEnabled(hasEditableSource);
    waveformPlaybackEndEditor.setEnabled(hasEditableSource);
    waveformPlaybackResetButton.setEnabled(hasEditableSource
        && (preview.playbackStartFrame != 0 || playbackEnd != preview.frameCount));
    waveformSetPlaybackSelectionButton.setEnabled(hasEditableSource && waveformPreview.hasSelection());
    waveformPlaybackAuditionButton.setEnabled(hasEditableSource && previewStatus.auditionAvailable);
    waveformSelectionAuditionButton.setEnabled(hasEditableSource
        && waveformPreview.hasSelection() && previewStatus.auditionAvailable);
    waveformSnapToggle.setEnabled(hasEditableSource && !preview.sourcePath.empty());
    waveformSnapToggle.setToggleState(waveformPreview.isZeroCrossingSnapEnabled(),
                                      juce::dontSendNotification);
    const auto loopGuidance = [&]() -> juce::String
    {
        switch (preview.loopMode)
        {
            case drs::engine::RegionLoopMode::noLoop:
                return "Loop off";
            case drs::engine::RegionLoopMode::oneShot:
                return "Loop off · one-shot trigger";
            case drs::engine::RegionLoopMode::loopContinuous:
                return "Always repeats through release";
            case drs::engine::RegionLoopMode::loopSustain:
                return "Repeats while held, then plays the tail";
        }
        return {};
    }();
    const auto activeAuditionGuidance = loopAuditionHeld
        ? (sustainLoopAudition
            ? " · Audition held; Release Loop to hear the tail"
            : " · Audition held; Release Loop to hear the fade")
        : juce::String {};
    waveformLoopGuidanceLabel.setText(
        loopGuidance + activeAuditionGuidance
            + " · Drag to select · Space/middle-drag to pan · "
            + juce::String::fromUTF8(waveformPreview.getSnapStatus().c_str()),
        juce::dontSendNotification);
    waveformStatusLabel.setText(formatAuthoringPreviewStatus(previewStatus), juce::dontSendNotification);
    previewStopButton.setEnabled(previewStatus.stopAvailable);
    previewStopButton.setDescription(previewStatus.stopAvailable
        ? "Releases all notes owned by the authoring Preview path."
        : "No authoring Preview notes are currently active.");
    previewStopButton.setHelpText(previewStatus.stopAvailable
        ? "Press to stop authoring Preview audio without affecting performance playback."
        : "Stop becomes available when authoring Preview owns an active note.");
    const auto previewGuidance = juce::String::fromUTF8(previewStatus.creatorGuidance.c_str());
    previewEnabledToggle.setDescription("Authoring Preview is "
        + juce::String(previewEnabledToggle.getToggleState() ? "enabled. " : "disabled. ")
        + previewGuidance);

    if (preview.available)
    {
        const auto sourceFile = preview.sourcePath.empty()
            ? juce::String("(source unavailable)")
            : juce::File(juce::String::fromUTF8(preview.sourcePath.c_str())).getFileName();
        waveformInfoLabel.setText(
            "File " + sourceFile
                + " | " + juce::String::fromUTF8(preview.formatName.c_str())
                + " | " + juce::String(static_cast<int>(preview.sampleRate)) + " Hz"
                + " | " + juce::String(static_cast<int>(preview.channelCount)) + " ch"
                + " | " + juce::String(preview.durationSeconds, 3) + " s",
            juce::dontSendNotification);
        loopInfoLabel.setText(
            loopGuidance
                + (preview.loopEnabled
                    ? " | frames " + juce::String(preview.loopStartFrame)
                        + " - " + juce::String(preview.loopEndFrame)
                    : " | inactive"),
            juce::dontSendNotification);
        if (preview.peakCacheEntryCount > 0)
        {
            loopInfoLabel.setText(
                loopInfoLabel.getText() + " | peaks "
                    + juce::String(static_cast<int>(preview.peakCacheEntryCount))
                    + " / " + juce::String(static_cast<int>((preview.peakCacheBytes + 1023) / 1024))
                    + " KiB" + (preview.detailCacheHit ? " hit" : ""),
                juce::dontSendNotification);
        }
    }
    else
    {
        waveformInfoLabel.setText(
            preview.state.empty() ? "Waveform metadata unavailable"
                                  : "Waveform: " + juce::String::fromUTF8(preview.state.c_str()),
            juce::dontSendNotification);
        loopInfoLabel.setText("Loop metadata unavailable", juce::dontSendNotification);
    }

    if (importResponsivenessProvider)
    {
        const auto metrics = importResponsivenessProvider();
        importMetricsLabel.setText(
            metrics.available
                ? "Import " + formatImportResponsivenessState(metrics.state)
                    + " " + juce::String(static_cast<int>(metrics.processedCount))
                    + "/" + juce::String(static_cast<int>(metrics.totalItemCount))
                    + " | pending " + juce::String(static_cast<int>(metrics.pendingCount))
                    + " | warn " + juce::String(static_cast<int>(metrics.warningItemCount))
                    + " | fail " + juce::String(static_cast<int>(metrics.failedItemCount))
                    + " | canceled " + juce::String(static_cast<int>(metrics.canceledItemCount))
                    + " | last " + formatMicros(metrics.lastProcessDurationMicros)
                    + " avg " + formatMicros(metrics.averageProcessDurationMicros)
                    + " max " + formatMicros(metrics.maxProcessDurationMicros)
                : "Import responsiveness unavailable",
            juce::dontSendNotification);
    }
    else
    {
        importMetricsLabel.setText("Import responsiveness unavailable", juce::dontSendNotification);
    }

    AuthoringSourceValidationSnapshot validation;
    if (sourceValidationStatusProvider)
        validation = sourceValidationStatusProvider();

    sourceValidationLabel.setText(formatSourceValidationStatus(validation), juce::dontSendNotification);
    const auto validationActive = isSourceValidationActive(validation);
    const auto canRequestValidation = validation.available
        && validation.totalItemCount > 0
        && !validationActive
        && static_cast<bool>(onRequestSourceValidation);
    const auto canCancelValidation = validationActive
        && static_cast<bool>(onCancelSourceValidation);
    sourceValidationButton.setButtonText(validationActive ? "Cancel Validation" : "Validate Sources");
    sourceValidationButton.setEnabled(canRequestValidation || canCancelValidation);
    sourceValidationButton.setTitle(sourceValidationButton.getButtonText());
    sourceValidationButton.setDescription(validationActive
        ? "Cancels the active background project source validation request."
        : "Starts background validation for the current project's linked sample sources.");
    sourceValidationButton.setHelpText(validationActive
        ? "Press to cancel background source validation without interrupting project editing."
        : (validation.totalItemCount > 0
               ? "Press to validate linked sample sources without blocking project load or restore."
               : "Validation becomes available after the project links one or more sample sources."));

    waveformStatusLabel.setTitle(waveformStatusLabel.getText());
    auto waveformStatusDescription = "Waveform preview status: " + waveformStatusLabel.getText().toStdString();
    if (!previewStatus.blockingGuidance.empty())
        waveformStatusDescription += " Next step: " + previewStatus.blockingGuidance;
    waveformStatusLabel.setDescription(waveformStatusDescription);
    updateDynamicAccessibleText(waveformInfoLabel, waveformInfoLabel.getText(), "Waveform metadata: ");
    updateDynamicAccessibleText(loopInfoLabel, loopInfoLabel.getText(), "Loop metadata: ");
    updateDynamicAccessibleText(importMetricsLabel, importMetricsLabel.getText(), "Import responsiveness: ");
    updateDynamicAccessibleText(sourceValidationLabel, sourceValidationLabel.getText(), "Source validation: ");
}

std::optional<std::uint64_t> AuthoringPanel::parseWaveformFrameText(
    const juce::String& text,
    const double sampleRate) const
{
    auto value = text.trim().toLowerCase();
    if (value.isEmpty())
        return std::nullopt;

    if (value.endsWithChar('s'))
    {
        const auto secondsText = value.dropLastCharacters(1).trim();
        if (secondsText.isEmpty() || !std::isfinite(sampleRate) || sampleRate <= 0.0)
            return std::nullopt;
        const auto secondsBytes = secondsText.toStdString();
        double seconds = 0.0;
        const auto parsed = std::from_chars(secondsBytes.data(),
                                            secondsBytes.data() + secondsBytes.size(),
                                            seconds);
        if (parsed.ec != std::errc {}
            || parsed.ptr != secondsBytes.data() + secondsBytes.size()
            || !std::isfinite(seconds) || seconds < 0.0)
            return std::nullopt;
        const auto frames = static_cast<long double>(seconds) * static_cast<long double>(sampleRate);
        if (frames > static_cast<long double>(std::numeric_limits<std::uint64_t>::max()))
            return std::nullopt;
        return static_cast<std::uint64_t>(std::llround(frames));
    }

    const auto bytes = value.toStdString();
    std::uint64_t frames = 0;
    const auto parsed = std::from_chars(bytes.data(), bytes.data() + bytes.size(), frames);
    if (parsed.ec != std::errc {} || parsed.ptr != bytes.data() + bytes.size())
        return std::nullopt;
    return frames;
}

void AuthoringPanel::commitWaveformPlaybackRegion(const std::uint64_t startFrame,
                                                  const std::uint64_t endFrameExclusive,
                                                  const std::string& label)
{
    const auto selectedZone = authoringSession.getSelectedZone();
    if (!selectedZone.has_value())
        return;
    AuthoringWaveformPreview preview;
    if (waveformPreviewProvider)
        preview = waveformPreviewProvider();
    if (!preview.available || preview.frameCount == 0)
        return;

    const auto playback = drs::engine::normalizePlaybackRegion(
        { startFrame, endFrameExclusive }, preview.frameCount);
    if (playback.empty())
        return;
    auto editedZone = *selectedZone;
    editedZone.sampleStartFrame = playback.startFrame;
    editedZone.sampleEndFrame = playback.endFrameExclusive == preview.frameCount
        ? 0 : playback.endFrameExclusive;
    if (editedZone.loopEnabled && editedZone.loopStartFrame < editedZone.loopEndFrame)
    {
        const auto loop = drs::engine::normalizeLoopRegion(
            { editedZone.loopStartFrame, editedZone.loopEndFrame }, playback);
        editedZone.loopStartFrame = loop.startFrame;
        editedZone.loopEndFrame = loop.endFrameExclusive;
        editedZone.loopCrossfadeFrames = std::min(
            editedZone.loopCrossfadeFrames,
            (loop.endFrameExclusive - loop.startFrame) / 2);
    }
    if (editedZone.sampleStartFrame == selectedZone->sampleStartFrame
        && editedZone.sampleEndFrame == selectedZone->sampleEndFrame
        && editedZone.loopStartFrame == selectedZone->loopStartFrame
        && editedZone.loopEndFrame == selectedZone->loopEndFrame)
        return;
    const auto result = authoringSession.updateSelectedZone(editedZone, label);
    if (!result.applied)
        return;
    refreshFromSession();
    if (onPrepareDraftPlaybackRequested)
        onPrepareDraftPlaybackRequested();
}

void AuthoringPanel::commitWaveformPlaybackControls(const std::string& label)
{
    if (isRefreshing)
        return;
    AuthoringWaveformPreview preview;
    if (waveformPreviewProvider)
        preview = waveformPreviewProvider();
    const auto start = parseWaveformFrameText(waveformPlaybackStartEditor.getText(),
                                               preview.sampleRate);
    const auto end = parseWaveformFrameText(waveformPlaybackEndEditor.getText(),
                                             preview.sampleRate);
    if (start.has_value() && end.has_value())
        commitWaveformPlaybackRegion(*start, *end, label);
}

void AuthoringPanel::resetWaveformPlaybackRegion()
{
    AuthoringWaveformPreview preview;
    if (waveformPreviewProvider)
        preview = waveformPreviewProvider();
    if (preview.frameCount > 0)
        commitWaveformPlaybackRegion(0, preview.frameCount,
                                     "Reset playback region to source");
}

void AuthoringPanel::setWaveformPlaybackToSelection()
{
    const auto selection = waveformPreview.getSelectionFrames();
    if (selection.empty())
        return;
    commitWaveformPlaybackRegion(selection.startFrame, selection.endFrameExclusive,
                                 "Use waveform selection for playback");
    waveformPreview.clearSelection();
}

void AuthoringPanel::commitWaveformLoopRegion(const std::uint64_t startFrame,
                                               const std::uint64_t endFrameExclusive,
                                               const std::string& label)
{
    const auto selectedZone = authoringSession.getSelectedZone();
    if (!selectedZone.has_value())
        return;

    AuthoringWaveformPreview preview;
    if (waveformPreviewProvider)
        preview = waveformPreviewProvider();
    const auto playbackEnd = preview.playbackEndFrameExclusive == 0
        ? preview.frameCount : preview.playbackEndFrameExclusive;
    if (!preview.available || playbackEnd <= preview.playbackStartFrame)
        return;

    const auto loop = drs::engine::normalizeLoopRegion(
        { startFrame, endFrameExclusive },
        { preview.playbackStartFrame, playbackEnd });
    if (loop.empty())
        return;

    auto editedZone = *selectedZone;
    editedZone.loopStartFrame = loop.startFrame;
    editedZone.loopEndFrame = loop.endFrameExclusive;
    editedZone.loopCrossfadeFrames = std::min(
        editedZone.loopCrossfadeFrames,
        (loop.endFrameExclusive - loop.startFrame) / 2);
    if (!drs::engine::regionLoopModeLoops(editedZone.loopMode))
        editedZone.loopMode = drs::engine::RegionLoopMode::loopContinuous;
    editedZone.loopEnabled = true;
    if (editedZone.loopMode == selectedZone->loopMode
        && editedZone.loopEnabled == selectedZone->loopEnabled
        && editedZone.loopStartFrame == selectedZone->loopStartFrame
        && editedZone.loopEndFrame == selectedZone->loopEndFrame)
        return;
    const auto result = authoringSession.updateSelectedZone(editedZone, label);
    if (!result.applied)
        return;

    refreshFromSession();
    if (onPrepareDraftPlaybackRequested)
        onPrepareDraftPlaybackRequested();
}

void AuthoringPanel::commitWaveformLoopControls(const std::string& label)
{
    if (isRefreshing)
        return;
    const auto selectedZone = authoringSession.getSelectedZone();
    if (!selectedZone.has_value())
        return;

    AuthoringWaveformPreview preview;
    if (waveformPreviewProvider)
        preview = waveformPreviewProvider();
    const auto playbackEnd = preview.playbackEndFrameExclusive == 0
        ? preview.frameCount : preview.playbackEndFrameExclusive;
    if (!preview.available || playbackEnd <= preview.playbackStartFrame)
        return;

    auto editedZone = *selectedZone;
    const auto previousLoopMode = editedZone.loopMode;
    switch (waveformLoopModeSelector.getSelectedId())
    {
        case 2: editedZone.loopMode = drs::engine::RegionLoopMode::oneShot; break;
        case 3: editedZone.loopMode = drs::engine::RegionLoopMode::loopContinuous; break;
        case 4: editedZone.loopMode = drs::engine::RegionLoopMode::loopSustain; break;
        default: editedZone.loopMode = drs::engine::RegionLoopMode::noLoop; break;
    }

    const auto start = parseWaveformFrameText(waveformLoopStartEditor.getText(), preview.sampleRate);
    const auto end = parseWaveformFrameText(waveformLoopEndEditor.getText(), preview.sampleRate);
    const auto crossfade = parseWaveformFrameText(waveformLoopCrossfadeEditor.getText(),
                                                   preview.sampleRate);
    if (!start.has_value() || !end.has_value() || !crossfade.has_value())
        return;
    const auto loop = drs::engine::normalizeLoopRegion(
        { *start, *end }, { preview.playbackStartFrame, playbackEnd });
    editedZone.loopStartFrame = loop.startFrame;
    editedZone.loopEndFrame = loop.endFrameExclusive;
    editedZone.loopEnabled = drs::engine::regionLoopModeLoops(editedZone.loopMode)
        && !loop.empty();
    const auto loopLength = loop.endFrameExclusive - loop.startFrame;
    editedZone.loopCrossfadeFrames = editedZone.loopEnabled
        ? std::min(*crossfade, loopLength / 2) : 0;
    if (editedZone.loopMode == drs::engine::RegionLoopMode::oneShot)
        editedZone.triggerMode = drs::engine::ZoneTriggerMode::oneShot;
    else if (previousLoopMode == drs::engine::RegionLoopMode::oneShot
             && editedZone.triggerMode == drs::engine::ZoneTriggerMode::oneShot)
        editedZone.triggerMode = drs::engine::ZoneTriggerMode::gated;

    if (editedZone.loopMode == selectedZone->loopMode
        && editedZone.loopEnabled == selectedZone->loopEnabled
        && editedZone.loopStartFrame == selectedZone->loopStartFrame
        && editedZone.loopEndFrame == selectedZone->loopEndFrame
        && editedZone.loopCrossfadeFrames == selectedZone->loopCrossfadeFrames
        && editedZone.triggerMode == selectedZone->triggerMode)
        return;

    const auto result = authoringSession.updateSelectedZone(editedZone, label);
    if (!result.applied)
        return;
    refreshFromSession();
    if (onPrepareDraftPlaybackRequested)
        onPrepareDraftPlaybackRequested();
}

void AuthoringPanel::setWaveformLoopToSelection()
{
    const auto selection = waveformPreview.getSelectionFrames();
    if (selection.empty())
        return;
    commitWaveformLoopRegion(selection.startFrame,
                             selection.endFrameExclusive,
                             "Use waveform selection for loop");
    waveformPreview.clearSelection();
}

void AuthoringPanel::auditionWaveformLoop()
{
    constexpr auto source = drs::engine::AuthoringPreviewAuditionSource::inspector;
    const auto sourceIndex = static_cast<std::size_t>(source);
    const auto loopAuditionHeld = waveformAuditionCueActive
        && waveformAuditionRegion.loopActive
        && sourceIndex < timedPreviewNotes.size()
        && timedPreviewNotes[sourceIndex].active;
    if (loopAuditionHeld)
    {
        releaseTimedPreview(sourceIndex);
        refreshWaveformWorkbenchContent();
        return;
    }

    auditionWaveformRegion(drs::engine::WaveformAuditionMode::loopRegion);
}

void AuthoringPanel::auditionWaveformRegion(const drs::engine::WaveformAuditionMode mode)
{
    if (!previewEnabledToggle.getToggleState() || !previewCommandCallback)
        return;
    AuthoringWaveformPreview preview;
    if (waveformPreviewProvider)
        preview = waveformPreviewProvider();
    const auto playbackEnd = preview.playbackEndFrameExclusive == 0
        ? preview.frameCount : preview.playbackEndFrameExclusive;
    auto audition = drs::engine::resolveWaveformAuditionRegion(
        mode,
        { { preview.playbackStartFrame, playbackEnd },
          { preview.loopStartFrame, preview.loopEndFrame }, preview.loopEnabled },
        waveformPreview.getSelectionFrames(), preview.frameCount);
    auto initialFrame = audition.playback.startFrame;
    if (mode == drs::engine::WaveformAuditionMode::loopRegion
        && audition.valid && audition.loopActive)
    {
        const auto loopLength = audition.loop.length();
        const auto contextFrames = preview.sampleRate > 0.0
            ? static_cast<std::uint64_t>(std::llround(std::max(1.0, preview.sampleRate * 0.050)))
            : std::max<std::uint64_t>(1, loopLength / 20);
        const auto minimumPreRollFrames = preview.sampleRate > 0.0
            ? static_cast<std::uint64_t>(std::llround(std::max(1.0, preview.sampleRate * 0.150)))
            : std::max<std::uint64_t>(1, loopLength / 4);
        const auto crossfadePreRollFrames = preview.loopCrossfadeFrames
            > std::numeric_limits<std::uint64_t>::max() - contextFrames
            ? std::numeric_limits<std::uint64_t>::max()
            : preview.loopCrossfadeFrames + contextFrames;
        const auto desiredPreRollFrames = std::max(minimumPreRollFrames,
                                                   crossfadePreRollFrames);
        const auto preRollFrames = std::min(loopLength, desiredPreRollFrames);
        initialFrame = audition.loop.endFrameExclusive - preRollFrames;
        audition.playback.startFrame = std::min(preview.playbackStartFrame,
                                                audition.loop.startFrame);
        audition.playback.endFrameExclusive = playbackEnd;
    }
    const auto request = authoringSession.buildSelectedZonePreviewRequest();
    if (!preview.available || !audition.valid || !request.available)
        return;

    constexpr auto source = drs::engine::AuthoringPreviewAuditionSource::inspector;
    const auto sourceIndex = static_cast<std::size_t>(source);
    releaseTimedPreview(sourceIndex);
    drs::engine::AuthoringPreviewCommand command;
    command.type = drs::engine::AuthoringPreviewCommandType::auditionSelectedZone;
    command.source = source;
    command.midiNote = request.midiNote;
    command.velocity = static_cast<float>(request.velocity) / 127.0f;
    command.selectedZoneId = authoringSession.getSelectedZone()->id;
    command.hasAuditionRegion = true;
    command.auditionStartFrame = audition.playback.startFrame;
    command.auditionEndFrameExclusive = audition.playback.endFrameExclusive;
    command.hasAuditionInitialFrame = initialFrame != audition.playback.startFrame;
    command.auditionInitialFrame = initialFrame;
    command.auditionLoopEnabled = audition.loopActive;
    command.auditionLoopStartFrame = audition.loop.startFrame;
    command.auditionLoopEndFrameExclusive = audition.loop.endFrameExclusive;
    previewCommandCallback(command);

    waveformAuditionRegion = audition;
    waveformAuditionInitialFrame = initialFrame;
    waveformAuditionCueActive = true;
    waveformAuditionCueStartedMillis = juce::Time::getMillisecondCounterHiRes();
    waveformAuditionNoteOffMillis = 0.0;
    waveformAuditionLoopMode = preview.loopMode;
    const auto durationMillis = preview.sampleRate > 0.0
        ? 1000.0 * static_cast<double>(audition.playback.length()) / preview.sampleRate
        : 180.0;
    constexpr auto loopAuditionSafetyTimeoutMillis = 12000.0;
    const auto releaseDelay = audition.loopActive ? loopAuditionSafetyTimeoutMillis
        : std::clamp(durationMillis + 20.0, 180.0, 3000.0);
    timedPreviewNotes[sourceIndex] = { true, request.midiNote,
                                       waveformAuditionCueStartedMillis + releaseDelay };
    startTimer(previewReleaseTimerId, 10);
    refreshWaveformWorkbenchContent();
}

void AuthoringPanel::updateSourceValidationAction()
{
    if (!sourceValidationStatusProvider)
        return;

    const auto validation = sourceValidationStatusProvider();
    if (isSourceValidationActive(validation))
    {
        if (onCancelSourceValidation)
            onCancelSourceValidation();
        return;
    }

    if (validation.available
        && validation.totalItemCount > 0
        && onRequestSourceValidation)
    {
        onRequestSourceValidation();
    }
}

void AuthoringPanel::showStructureForSelection(std::vector<std::string> zoneIds,
                                               std::string primaryZoneId)
{
    if (zoneIds.empty())
        return;

    structureSelection.replace(authoring::StructureSelectionKind::zone,
                               std::move(zoneIds),
                               std::move(primaryZoneId));
    if (structureSelection.getPrimaryId().empty())
        return;

    authoringSession.selectZone(structureSelection.getPrimaryId());
    structureViewState.setMapPaneVisible(true);
    refreshInspectorVisibility();
    resized();
    refreshSelectionFromSession();
    refreshStructureBrowser();
}

void AuthoringPanel::showZonesForCurrentSelection()
{
    const auto& project = authoringSession.getProject();
    const auto scope = authoring::deriveStructureScope(project, structureSelection);
    structureViewState.setScope(scope);
    structureViewState.setMapPaneVisible(true);

    authoring::ScopedZoneProjectionOptions options;
    options.searchText = structureViewState.getSearchText();
    options.articulationFilter = structureViewState.getArticulationFilter();
    options.performanceEventFilter = structureViewState.getPerformanceEventFilter();
    options.includeHiddenContainers = false;
    const auto projection = authoring::buildScopedZoneProjection(project, scope, structureSelection, options);
    scopedMapProjectionActive = true;
    zoneMap.setZoneSummaries(projection.zones);
    zoneMap.setScopeSummary("Showing zones: " + authoring::structureScopeName(project, scope)
                            + " · " + std::to_string(projection.zones.size()));
    zoneMap.fitAllVisible();
    refreshInspectorVisibility();
    resized();
}

void AuthoringPanel::applyStructureSelection(const authoring::StructureSelectionKind kind,
                                             std::vector<std::string> ids,
                                             std::string primaryId)
{
    if (isRefreshing || ids.empty() || kind == authoring::StructureSelectionKind::none)
        return;

    structureSelection.replace(kind, std::move(ids), std::move(primaryId));
    const auto& idsForSession = structureSelection.getIds();
    if (idsForSession.empty())
        return;

    const auto& primary = structureSelection.getPrimaryId();
    if (kind == authoring::StructureSelectionKind::instrument)
    {
        refreshStructureBrowser();
        refreshStructureInspector();
        return;
    }
    drs::engine::RuntimeProjectDocumentActionResult result;
    switch (kind)
    {
        case authoring::StructureSelectionKind::layer:
            result = authoringSession.selectLayer(primary);
            break;
        case authoring::StructureSelectionKind::group:
            result = authoringSession.selectGroup(primary);
            break;
        case authoring::StructureSelectionKind::zone:
            zoneMapSelectedZoneIds = idsForSession;
            result = authoringSession.selectZone(primary);
            break;
        case authoring::StructureSelectionKind::none:
            return;
    }

    if (result.applied)
    {
        // Instrument Structure Browser selection bypasses the zone selector
        // callback. Keep waveform preview authorization in sync with that
        // navigation path while the Waveform workbench is already open.
        if (kind == authoring::StructureSelectionKind::zone)
            requestWaveformPreviewLoad(workbenchState.activeTab == authoring::WorkbenchTab::waveform);
        refreshFromSession();
    }
}

void AuthoringPanel::refreshStructureBrowser()
{
    const auto& project = authoringSession.getProject();
    const auto selectedZone = authoringSession.getSelectedZone();
    const auto selectedGroup = authoringSession.getSelectedGroup();
    const auto selectedLayer = authoringSession.getSelectedLayer();

    std::vector<std::string> validSelectionIds;
    if (structureSelection.getKind() == authoring::StructureSelectionKind::layer)
        for (const auto& layer : project.authoring.layers) validSelectionIds.push_back(layer.id);
    else if (structureSelection.getKind() == authoring::StructureSelectionKind::group)
        for (const auto& group : project.authoring.groups) validSelectionIds.push_back(group.id);
    else if (structureSelection.getKind() == authoring::StructureSelectionKind::zone)
        for (const auto& zone : project.authoring.zones) validSelectionIds.push_back(zone.id);
    if (structureSelection.getKind() != authoring::StructureSelectionKind::none)
        structureSelection.reconcile(validSelectionIds);

    if (structureSelection.isEmpty())
    {
        if (!zoneMapSelectedZoneIds.empty() && selectedZone.has_value())
        {
            structureSelection.replace(authoring::StructureSelectionKind::zone,
                                       zoneMapSelectedZoneIds,
                                       selectedZone->id);
        }
        else if (selectedGroup.has_value())
        {
            structureSelection.replace(authoring::StructureSelectionKind::group,
                                       { selectedGroup->id },
                                       selectedGroup->id);
        }
        else if (selectedLayer.has_value())
        {
            structureSelection.replace(authoring::StructureSelectionKind::layer,
                                       { selectedLayer->id },
                                       selectedLayer->id);
        }
    }

    structureViewState.setScope(authoring::reconcileStructureScope(project, structureViewState.getScope()));
    authoring::InstrumentStructureBrowserOptions browserOptions;
    browserOptions.searchText = structureViewState.getSearchText();
    browserOptions.visibleOnly = structureViewState.getVisibleOnly();
    browserOptions.showOverlapsOnly = structureViewState.getShowOverlapsOnly();
    browserOptions.showPotentialCollisionsOnly = structureViewState.getShowPotentialCollisionsOnly();
    browserOptions.showExactStacksOnly = structureViewState.getShowExactStacksOnly();
    browserOptions.articulationFilter = structureViewState.getArticulationFilter();
    browserOptions.performanceEventFilter = structureViewState.getPerformanceEventFilter();
    structureBrowser.setRows(authoring::buildInstrumentStructureRows(
        project, structureSelection, structureViewState.getDisclosedIds(),
        structureViewState.getCollapsedIds(), browserOptions));
    structureBrowser.setSelection(structureSelection);
    if (scopedMapProjectionActive)
    {
        authoring::ScopedZoneProjectionOptions mapOptions;
        mapOptions.searchText = structureViewState.getSearchText();
        mapOptions.articulationFilter = structureViewState.getArticulationFilter();
        mapOptions.performanceEventFilter = structureViewState.getPerformanceEventFilter();
        mapOptions.includeHiddenContainers = false;
        const auto mapProjection = authoring::buildScopedZoneProjection(
            project, structureViewState.getScope(), structureSelection, mapOptions);
        zoneMap.setZoneSummaries(mapProjection.zones);
        zoneMap.setScopeSummary("Showing zones: "
                                + authoring::structureScopeName(project, structureViewState.getScope())
                                + " · " + std::to_string(mapProjection.zones.size()));
    }

    refreshStructureInspector();
    showMapButton.setToggleState(structureViewState.isMapPaneVisible(),
                                        juce::dontSendNotification);
}

void AuthoringPanel::refreshStructureInspector()
{
    structureInspector.setSnapshot(authoring::buildStructureInspectorSnapshot(
        authoringSession.getProject(), structureSelection));
}

void AuthoringPanel::refreshFromSession()
{
    const ScopedMessageThreadSpan timing(MessageThreadSpanKind::authoringRefresh);
    const juce::ScopedValueSetter<bool> refreshGuard(isRefreshing, true);

    rebuildZoneSelector();
    rebuildLayerList();
    rebuildGroupList();
    rebuildMacroList();
    rebuildMacroAssignmentList();
    rebuildDspScopeSelector();
    rebuildFxSelector();
    rebuildRoutingBusSelector();
    rebuildPerformanceBankSelector();
    rebuildTriggerSlotSelector();
    rebuildArticulationList();
    syncZoneMapSelectionState();
    refreshStructureBrowser();
    if (scopedMapProjectionActive)
    {
        authoring::ScopedZoneProjectionOptions options;
        options.searchText = structureViewState.getSearchText();
        options.articulationFilter = structureViewState.getArticulationFilter();
        options.performanceEventFilter = structureViewState.getPerformanceEventFilter();
        options.includeHiddenContainers = false;
        const auto projection = authoring::buildScopedZoneProjection(
            authoringSession.getProject(), structureViewState.getScope(), structureSelection, options);
        zoneMap.setZoneSummaries(projection.zones);
        zoneMap.setScopeSummary("Showing zones: "
                                + authoring::structureScopeName(authoringSession.getProject(), structureViewState.getScope())
                                + " · " + std::to_string(projection.zones.size()));
    }
    else
    {
        const auto summaries = buildVisibleZoneSummaries();
        zoneMap.setZoneSummaries(summaries);
        zoneMap.setScopeSummary("Showing zones: Instrument · " + std::to_string(summaries.size()));
    }
    zoneMap.setSelectionState({ zoneMapSelectedZoneIds,
                                authoringSession.getSelectedZone().has_value()
                                    ? authoringSession.getSelectedZone()->id
                                    : std::string {} });

    const auto& project = authoringSession.getProject();
    masterGainSlider.setValue(project.authoring.masterGainDb, juce::dontSendNotification);
    selectionSummaryViewModel = buildSelectionSummaryViewModel();
    zoneFieldValuesViewModel = buildZoneFieldValuesViewModel();

    summaryStrip.setViewModel(selectionSummaryViewModel);
    zoneMappingEditor.setViewModel(zoneFieldValuesViewModel);

    const auto articulations = authoringSession.getArticulations();
    if (selectedArticulationIndex >= 0
        && static_cast<std::size_t>(selectedArticulationIndex) < articulations.size())
    {
        const auto& articulation = articulations[static_cast<std::size_t>(selectedArticulationIndex)];
        articulationNameEditor.setText(juce::String::fromUTF8(articulation.displayName.c_str()),
                                       juce::dontSendNotification);
        articulationSwitchNoteSlider.setValue(articulation.activation.has_value()
                                                  ? articulation.activation->midiNote : 0,
                                              juce::dontSendNotification);
        articulationSwitchNoteValueLabel.setText(articulation.activation.has_value()
            ? formatMidiNoteName(articulation.activation->midiNote)
            : "No key switch assigned", juce::dontSendNotification);
        articulationDeleteReassignSelector.clear(juce::dontSendNotification);
        int firstReplacementId = 0;
        for (std::size_t index = 0; index < articulations.size(); ++index)
        {
            if (static_cast<int>(index) == selectedArticulationIndex)
                continue;
            const auto itemId = static_cast<int>(index) + 1;
            articulationDeleteReassignSelector.addItem(
                juce::String::fromUTF8(articulations[index].displayName.c_str()), itemId);
            if (firstReplacementId == 0)
                firstReplacementId = itemId;
        }
        articulationDeleteReassignSelector.setSelectedId(firstReplacementId, juce::dontSendNotification);
        articulationCreateButton.setEnabled(articulations.size() < 64);
        articulationDuplicateButton.setEnabled(articulations.size() < 64);
        articulationDefaultButton.setEnabled(!articulation.isDefault);
        articulationMoveUpButton.setEnabled(selectedArticulationIndex > 0);
        articulationMoveDownButton.setEnabled(selectedArticulationIndex + 1 < static_cast<int>(articulations.size()));
        articulationDeleteButton.setEnabled(articulations.size() > 1);
        articulationDeleteReassignSelector.setEnabled(articulations.size() > 1);
        articulationNameEditor.setEnabled(true);
        articulationSwitchNoteSlider.setEnabled(true);
        articulationClearSwitchButton.setEnabled(articulation.activation.has_value());
        articulationMidiLearnButton.setEnabled(true);
        articulationMidiLearnButton.setButtonText(keySwitchMidiLearnActive ? "Cancel MIDI Learn" : "MIDI Learn");
        articulationStatusLabel.setText(keySwitchMidiLearnActive
            ? "MIDI Learn active — play a MIDI key switch now (10 second timeout)."
            : (articulation.activation.has_value()
                  ? "Switch keys are consumed and do not trigger playable zones."
                  : "Choose a note, use the picker, or start MIDI Learn."), juce::dontSendNotification);
        for (std::size_t index = 0; index < articulationKeyButtons.size(); ++index)
        {
            const auto midiNote = static_cast<int>(index);
            const auto isSwitchKey = std::any_of(articulations.begin(), articulations.end(),
                                                 [&](const auto& candidate)
                                                 {
                                                     return candidate.activation.has_value()
                                                         && candidate.activation->midiNote == midiNote;
                                                 });
            const auto isPlayableKey = std::any_of(project.authoring.zones.begin(), project.authoring.zones.end(),
                                                    [&](const auto& zone)
                                                    {
                                                        return midiNote >= zone.keyLow && midiNote <= zone.keyHigh;
                                                    });
            auto& key = *articulationKeyButtons[index];
            key.setButtonText(formatMidiNoteName(midiNote));
            key.setColour(juce::TextButton::buttonColourId,
                          isSwitchKey ? authoringPanelAccent
                                      : (isPlayableKey ? authoringToggleTick : authoringControlSurface));
            key.setColour(juce::TextButton::textColourOffId,
                          (isSwitchKey || isPlayableKey) ? authoring::visual::textOnAccent
                                                        : authoring::visual::text);
        }
    }
    else
    {
        articulationNameEditor.setText({}, juce::dontSendNotification);
        articulationDeleteReassignSelector.clear(juce::dontSendNotification);
        articulationSwitchNoteValueLabel.setText("No articulation selected", juce::dontSendNotification);
        articulationStatusLabel.setText("Create an articulation to configure a key switch.", juce::dontSendNotification);
        articulationCreateButton.setEnabled(true);
        articulationDuplicateButton.setEnabled(false);
        articulationDefaultButton.setEnabled(false);
        articulationMoveUpButton.setEnabled(false);
        articulationMoveDownButton.setEnabled(false);
        articulationDeleteButton.setEnabled(false);
        articulationDeleteReassignSelector.setEnabled(false);
        articulationNameEditor.setEnabled(false);
        articulationSwitchNoteSlider.setEnabled(false);
        articulationClearSwitchButton.setEnabled(false);
        articulationMidiLearnButton.setEnabled(false);
    }

    if (const auto selectedGroup = authoringSession.getSelectedGroup(); selectedGroup.has_value())
    {
        groupNameEditor.setText(juce::String::fromUTF8(selectedGroup->displayName.c_str()),
                                juce::dontSendNotification);
        groupVisibilityToggle.setToggleState(selectedGroup->workspaceVisible, juce::dontSendNotification);
        groupGainSlider.setValue(selectedGroup->gainDb, juce::dontSendNotification);
        groupPanSlider.setValue(selectedGroup->pan, juce::dontSendNotification);
        groupVisibilityButton.setButtonText(selectedGroup->workspaceVisible ? "Hide Group" : "Show Group");
        groupVisibilityButton.setEnabled(true);
        groupPreviewAnchorButton.setEnabled(!selectedGroup->auditionAnchorZoneId.empty());
        groupMoveUpButton.setEnabled(selectedGroupIndex > 0);
        groupMoveDownButton.setEnabled(selectedGroupIndex + 1 < static_cast<int>(project.authoring.groups.size()));
        const auto selectedZoneIdsForGrouping = collectSelectedZoneIdsForGrouping();
        const auto assignableZoneCount = static_cast<int>(std::count_if(selectedZoneIdsForGrouping.begin(),
                                                                        selectedZoneIdsForGrouping.end(),
                                                                        [&](const std::string& zoneId)
                                                                        {
                                                                            const auto iterator =
                                                                                std::find_if(project.authoring.zones.begin(),
                                                                                             project.authoring.zones.end(),
                                                                                             [&](const auto& zone)
                                                                                             {
                                                                                                 return zone.id == zoneId
                                                                                                     && zone.groupId != selectedGroup->id;
                                                                                             });
                                                                            return iterator != project.authoring.zones.end();
                                                                        }));
        groupAssignZonesButton.setEnabled(assignableZoneCount > 0);

        groupRoutingBusIds.clear();
        groupRoutingSelector.clear(juce::dontSendNotification);
        groupRoutingBusIds.push_back({});
        groupRoutingSelector.addItem("(direct)", 1);
        auto selectedRoutingId = 1;
        for (std::size_t index = 0; index < project.authoring.routingBuses.size(); ++index)
        {
            const auto itemId = static_cast<int>(index) + 2;
            groupRoutingBusIds.push_back(project.authoring.routingBuses[index].id);
            groupRoutingSelector.addItem(juce::String::fromUTF8(project.authoring.routingBuses[index].displayName.c_str()),
                                         itemId);
            if (selectedGroup->routingBusId == project.authoring.routingBuses[index].id)
                selectedRoutingId = itemId;
        }
        groupRoutingSelector.setSelectedId(selectedRoutingId, juce::dontSendNotification);

        groupAnchorZoneIds.clear();
        groupAnchorSelector.clear(juce::dontSendNotification);
        auto selectedAnchorId = 0;
        int anchorItemId = 1;
        for (const auto& zone : project.authoring.zones)
        {
            if (zone.groupId != selectedGroup->id)
                continue;

            groupAnchorZoneIds.push_back(zone.id);
            groupAnchorSelector.addItem(juce::String::fromUTF8(zone.displayName.c_str()), anchorItemId);
            if (selectedGroup->auditionAnchorZoneId == zone.id)
                selectedAnchorId = anchorItemId;
            ++anchorItemId;
        }
        groupAnchorSelector.setSelectedId(selectedAnchorId > 0 ? selectedAnchorId : 1, juce::dontSendNotification);

        const auto memberCount = static_cast<int>(countZonesInGroup(project, selectedGroup->id));
        const auto hiddenGroupCount = static_cast<int>(std::count_if(project.authoring.groups.begin(),
                                                                     project.authoring.groups.end(),
                                                                     [](const auto& group)
                                                                     {
                                                                         return !group.workspaceVisible;
                                                                     }));
        groupSummaryLabel.setText(
            "Master " + juce::String(project.authoring.masterGainDb, 1) + " dB"
                + " | " + juce::String::fromUTF8(selectedGroup->displayName.c_str())
                + " | " + juce::String(memberCount) + " zones"
                + " | routing " + (selectedGroup->routingBusId.empty()
                                       ? juce::String("direct")
                                       : findRoutingBusDisplayName(project, selectedGroup->routingBusId))
                + " | " + buildMacroControllerSummaryForBus(project, selectedGroup->routingBusId),
            juce::dontSendNotification);
        groupVisibilityHintLabel.setText(
            hiddenGroupCount > 0
                ? juce::String(hiddenGroupCount) + " hidden group(s) are filtered from the zone map."
                : "All authored groups are currently visible on the zone map.",
            juce::dontSendNotification);
        groupDeleteButton.setEnabled(memberCount == 0);

        const auto roundRobinStatus = authoringSession.getSelectedGroupRoundRobinStatus();
        groupRoundRobinLabel.setText(
            roundRobinStatus.enabled
                ? "Round Robin | On | "
                    + juce::String(roundRobinStatus.mode == drs::engine::RoundRobinMode::random
                                       ? "Random"
                                       : "Cycle")
                : "Round Robin | Off",
            juce::dontSendNotification);
        groupRoundRobinHintLabel.setText(
            juce::String::fromUTF8(roundRobinStatus.state.c_str()),
            juce::dontSendNotification);
        groupSummaryLabel.setTooltip(groupSummaryLabel.getText());
        groupRoundRobinLabel.setTooltip(groupRoundRobinLabel.getText() + "\n" + groupRoundRobinHintLabel.getText());
        groupRoundRobinToggle.setToggleState(roundRobinStatus.enabled, juce::dontSendNotification);
        groupRoundRobinToggle.setEnabled(true);
        groupRoundRobinModeSelector.setSelectedId(
            roundRobinStatus.mode == drs::engine::RoundRobinMode::random ? 2 : 1,
            juce::dontSendNotification);
        groupRoundRobinModeSelector.setEnabled(roundRobinStatus.enabled);
    }
    else
    {
        groupNameEditor.setText({}, juce::dontSendNotification);
        groupVisibilityToggle.setToggleState(false, juce::dontSendNotification);
        groupGainSlider.setValue(0.0, juce::dontSendNotification);
        groupPanSlider.setValue(0.0, juce::dontSendNotification);
        groupRoutingSelector.clear(juce::dontSendNotification);
        groupAnchorSelector.clear(juce::dontSendNotification);
        groupSummaryLabel.setText("Master " + juce::String(project.authoring.masterGainDb, 1)
                                      + " dB | No group is selected.",
                                  juce::dontSendNotification);
        groupRoundRobinLabel.setText("Round Robin unavailable until a group is selected.", juce::dontSendNotification);
        groupRoundRobinHintLabel.setText("Create or select a group to inspect its visibility, routing, and RR entry points.",
                                         juce::dontSendNotification);
        groupSummaryLabel.setTooltip(groupSummaryLabel.getText());
        groupRoundRobinLabel.setTooltip(groupRoundRobinLabel.getText() + "\n" + groupRoundRobinHintLabel.getText());
        groupVisibilityHintLabel.setText("No group is selected.", juce::dontSendNotification);
        groupVisibilityButton.setButtonText("Hide Group");
        groupVisibilityButton.setEnabled(false);
        groupPreviewAnchorButton.setEnabled(false);
        groupMoveUpButton.setEnabled(false);
        groupMoveDownButton.setEnabled(false);
        groupDeleteButton.setEnabled(false);
        groupAssignZonesButton.setEnabled(false);
        groupRoundRobinToggle.setToggleState(false, juce::dontSendNotification);
        groupRoundRobinToggle.setEnabled(false);
        groupRoundRobinModeSelector.setSelectedId(1, juce::dontSendNotification);
        groupRoundRobinModeSelector.setEnabled(false);
    }

    if (!project.authoring.macros.empty())
    {
        if (const auto selectedMacro = authoringSession.getSelectedMacroIndex(); selectedMacro.has_value())
            selectedMacroIndex = static_cast<int>(*selectedMacro);

        const auto& macro = project.authoring.macros[static_cast<std::size_t>(selectedMacroIndex)];
        const auto* selectedTarget = selectedMacroTargetIndex >= 0
                && static_cast<std::size_t>(selectedMacroTargetIndex) < macro.targets.size()
            ? &macro.targets[static_cast<std::size_t>(selectedMacroTargetIndex)] : nullptr;
        macroNameEditor.setText(juce::String::fromUTF8(macro.name.c_str()), juce::dontSendNotification);
        macroExposeToggle.setToggleState(macro.exposedInPerformance, juce::dontSendNotification);
        macroAssignmentSelector.clear(juce::dontSendNotification);
        macroAssignmentSelector.addItem("(unassigned)", unassignedMacroAssignmentId);
        int selectedAssignmentId = unassignedMacroAssignmentId;
        for (std::size_t index = 0; index < curatedMacroAssignments.size(); ++index)
        {
            const auto itemId = curatedMacroAssignmentBase + static_cast<int>(index);
            macroAssignmentSelector.addItem(curatedMacroAssignments[index].label, itemId);
        }
        const auto preferredRoutingBusId = selectedDspScopeRoutingBusId();
        const auto dspAssignments = buildCuratedDspMacroAssignments(project,
                                                                    preferredRoutingBusId,
                                                                    selectedDspScopeInputSource());
        for (std::size_t index = 0; index < dspAssignments.size(); ++index)
        {
            const auto& assignment = dspAssignments[index];
            const auto itemId = curatedDspMacroAssignmentBase + static_cast<int>(index);
            macroAssignmentSelector.addItem(formatCuratedDspMacroAssignment(project,
                                                                            assignment,
                                                                            preferredRoutingBusId),
                                           itemId);
            if (selectedTarget != nullptr
                && selectedTarget->dspSlotId == assignment.slot->id
                && selectedTarget->dspParameterId == assignment.parameter->id)
            {
                selectedAssignmentId = itemId;
            }
        }

        if (selectedTarget != nullptr
            && selectedAssignmentId == unassignedMacroAssignmentId)
        {
            const auto assignmentIndex = findAssignmentIndex(selectedTarget->parameterId);
            if (assignmentIndex >= 0)
            {
                selectedAssignmentId = curatedMacroAssignmentBase + assignmentIndex;
            }
            else
            {
                const auto customItemId = curatedDspMacroAssignmentBase - 1;
                macroAssignmentSelector.addItem("Custom: "
                                                   + juce::String::fromUTF8(selectedTarget->parameterId.c_str()),
                                               customItemId);
                selectedAssignmentId = customItemId;
            }
        }
        macroAssignmentSelector.setSelectedId(selectedAssignmentId > 0 ? selectedAssignmentId : 1,
                                              juce::dontSendNotification);

        macroRoleSelector.clear(juce::dontSendNotification);
        int selectedRoleId = 0;
        const auto currentRole = selectedTarget != nullptr ? selectedTarget->role : std::string{};
        for (std::size_t index = 0; index < curatedMacroRoles.size(); ++index)
        {
            macroRoleSelector.addItem(curatedMacroRoles[index], static_cast<int>(index) + 1);
            if (currentRole == curatedMacroRoles[index])
                selectedRoleId = static_cast<int>(index) + 1;
        }
        if (selectedRoleId == 0 && !currentRole.empty())
        {
            const auto customRoleId = static_cast<int>(curatedMacroRoles.size()) + 1;
            macroRoleSelector.addItem("Custom: " + juce::String::fromUTF8(currentRole.c_str()), customRoleId);
            selectedRoleId = customRoleId;
        }
        macroRoleSelector.setSelectedId(selectedRoleId > 0 ? selectedRoleId : 1, juce::dontSendNotification);

        macroDefaultSlider.setRange(macro.minValue, macro.maxValue, 0.01);
        macroDefaultSlider.setValue(macro.defaultValue, juce::dontSendNotification);
        macroMinSlider.setValue(macro.minValue, juce::dontSendNotification);
        macroMaxSlider.setValue(macro.maxValue, juce::dontSendNotification);
        const auto rangeStatus = "Default " + juce::String(macro.defaultValue, 2)
            + " is within " + juce::String(macro.minValue, 2)
            + "-" + juce::String(macro.maxValue, 2)
            + ". Values are clamped and reordered by the existing transaction.";
        const auto assignmentDetail = selectedTarget != nullptr
            ? macroTargetDetail(*selectedTarget, selectedMacroTargetIndex,
                                static_cast<int>(macro.targets.size()))
            : juce::String("No target selected. Choose a supported target to create the first assignment.");
        macroSummaryLabel.setText(
            selectedTarget != nullptr
                ? macroTargetMappingSummary(*selectedTarget)
                    + " | " + juce::String::fromUTF8(selectedTarget->parameterPath.c_str())
                : juce::String("No assigned targets. Select a supported target or use Add Target."),
            juce::dontSendNotification);
        macroWorkbenchContent.setPresentationState(true, rangeStatus, assignmentDetail);
        macroCreateButton.setEnabled(true);
        macroDuplicateButton.setEnabled(true);
        macroDeleteButton.setEnabled(true);
        macroNameEditor.setEnabled(true);
        macroExposeToggle.setEnabled(true);
        macroAssignmentSelector.setEnabled(true);
        macroRoleSelector.setEnabled(!macro.targets.empty());
        macroDefaultSlider.setEnabled(true);
        macroMinSlider.setEnabled(true);
        macroMaxSlider.setEnabled(true);
        macroMoveUpButton.setEnabled(selectedMacroIndex > 0);
        macroMoveDownButton.setEnabled(selectedMacroIndex + 1 < static_cast<int>(project.authoring.macros.size()));
        macroAssignmentAddButton.setEnabled(true);
        macroAssignmentRemoveButton.setEnabled(selectedTarget != nullptr);
    }
    else
    {
        macroNameEditor.setText({}, juce::dontSendNotification);
        macroExposeToggle.setToggleState(false, juce::dontSendNotification);
        macroAssignmentSelector.clear(juce::dontSendNotification);
        macroRoleSelector.clear(juce::dontSendNotification);
        macroSummaryLabel.setText("No macros are authored in this project yet. Use Create, then target a group bus gain lane such as close mic, room, layer blend, or pedal noise. Group pan stays out of the first release.",
                                  juce::dontSendNotification);
        macroWorkbenchContent.setPresentationState(
            false,
            "No range is available until a macro is selected.",
            "Create or select a macro before assigning a target.");
        macroCreateButton.setEnabled(true);
        macroDuplicateButton.setEnabled(false);
        macroDeleteButton.setEnabled(false);
        macroNameEditor.setEnabled(false);
        macroExposeToggle.setEnabled(false);
        macroAssignmentSelector.setEnabled(false);
        macroRoleSelector.setEnabled(false);
        macroDefaultSlider.setEnabled(false);
        macroMinSlider.setEnabled(false);
        macroMaxSlider.setEnabled(false);
        macroMoveUpButton.setEnabled(false);
        macroMoveDownButton.setEnabled(false);
        macroAssignmentAddButton.setEnabled(false);
        macroAssignmentRemoveButton.setEnabled(false);
    }

    auto routingSelectedFxContext = juce::String("No selected insert.");
    auto routingMacroControlSummary = juce::String(
        "No selected parameter is available for Macro control assignment.");
    auto routingSignalPath = juce::String("No routing bus is selected.");
    auto routingWarningState = false;

    const auto hasScopedFx = selectedFxSlotIndex >= 0
        && static_cast<std::size_t>(selectedFxSlotIndex) < project.authoring.fxSlots.size();
    if (hasScopedFx)
    {
        const auto& fxSlot = project.authoring.fxSlots[static_cast<std::size_t>(selectedFxSlotIndex)];
        fxTypeSelector.clear(juce::dontSendNotification);
        int selectedFxTypeId = 0;
        for (std::size_t index = 0; index < curatedFxTypes.size(); ++index)
        {
            fxTypeSelector.addItem(curatedFxTypes[index], static_cast<int>(index) + 1);
            if (fxSlot.effectType == curatedFxTypes[index])
                selectedFxTypeId = static_cast<int>(index) + 1;
        }
        if (selectedFxTypeId == 0 && !fxSlot.effectType.empty())
        {
            const auto customTypeId = static_cast<int>(curatedFxTypes.size()) + 1;
            fxTypeSelector.addItem("Custom: " + juce::String::fromUTF8(fxSlot.effectType.c_str()), customTypeId);
            selectedFxTypeId = customTypeId;
        }
        fxTypeSelector.setSelectedId(selectedFxTypeId > 0 ? selectedFxTypeId : 1, juce::dontSendNotification);
        fxBypassedToggle.setToggleState(fxSlot.bypassed, juce::dontSendNotification);
        fxNameEditor.setText(juce::String::fromUTF8(fxSlot.displayName.c_str()), juce::dontSendNotification);
        fxParameterIds.clear();
        fxParameterSelector.clear(juce::dontSendNotification);
        const auto* descriptor = drs::engine::findCuratedDspEffect(fxSlot.effectType, fxSlot.effectVersion);
        const auto unavailable = descriptor == nullptr || fxSlot.unavailable || fxSlot.legacyInert;
        routingWarningState = unavailable;
        if (descriptor != nullptr && !descriptor->parameters.empty())
        {
            for (std::size_t index = 0; index < descriptor->parameters.size(); ++index)
            {
                const auto& parameter = descriptor->parameters[index];
                fxParameterIds.push_back(std::string(parameter.id));
                fxParameterSelector.addItem(juce::String(parameter.id.data(), static_cast<int>(parameter.id.size())),
                                            static_cast<int>(index) + 1);
            }
            selectedFxParameterIndex = std::clamp(selectedFxParameterIndex, 0,
                                                   static_cast<int>(descriptor->parameters.size()) - 1);
            fxParameterSelector.setSelectedId(selectedFxParameterIndex + 1, juce::dontSendNotification);
            const auto& parameter = descriptor->parameters[static_cast<std::size_t>(selectedFxParameterIndex)];
            const auto authored = std::find_if(fxSlot.parameters.begin(), fxSlot.parameters.end(),
                                               [&](const auto& value) { return value.id == parameter.id; });
            fxParameterSlider.setRange(parameter.minimum, parameter.maximum,
                                       std::max(0.0001, (parameter.maximum - parameter.minimum) / 1000.0));
            const auto value = authored == fxSlot.parameters.end() ? parameter.defaultValue : authored->value;
            fxParameterSlider.setValue(value, juce::dontSendNotification);
            fxParameterValueLabel.setText(
                juce::String(parameter.id.data(), static_cast<int>(parameter.id.size())) + ": "
                    + juce::String(value, 3) + " " + formatCuratedDspUnit(parameter.unit)
                    + " | default " + juce::String(parameter.defaultValue, 3),
                juce::dontSendNotification);
        }
        else
            fxParameterValueLabel.setText("Descriptor parameters unavailable for this preserved effect version.",
                                          juce::dontSendNotification);
        fxTypeSelector.setEnabled(!fxSlot.legacyInert);
        fxBypassedToggle.setEnabled(true);
        fxNameEditor.setEnabled(true);
        fxParameterSelector.setEnabled(!unavailable && !fxParameterIds.empty());
        fxParameterSlider.setEnabled(!unavailable && !fxParameterIds.empty());
        fxParameterResetButton.setEnabled(!unavailable && !fxParameterIds.empty());
        fxAssignMacroButton.setEnabled(!unavailable && !fxParameterIds.empty());
        const auto hasMacroControl = selectedFxParameterIndex >= 0
            && static_cast<std::size_t>(selectedFxParameterIndex) < fxParameterIds.size()
            && findMacroIndexForDspTarget(project,
                                          fxSlot.id,
                                          fxParameterIds[static_cast<std::size_t>(selectedFxParameterIndex)]).has_value();
        routingMacroControlSummary = hasMacroControl
            ? "Assigned to a Macro control. Edit Control opens the existing assignment."
            : "Not assigned to a Macro control. Create Control uses the current parameter.";
        fxAssignMacroButton.setButtonText(hasMacroControl ? "Edit Control" : "Create Control");
        fxAssignMacroButton.setTitle(fxAssignMacroButton.getButtonText());
        fxDuplicateButton.setEnabled(true);
        fxDeleteButton.setEnabled(true);
        fxMoveUpButton.setEnabled(!scopedFxSlotIds.empty() && scopedFxSlotIds.front() != fxSlot.id);
        fxMoveDownButton.setEnabled(!scopedFxSlotIds.empty() && scopedFxSlotIds.back() != fxSlot.id);
        fxMoveOwnerButton.setEnabled(fxOwnerSelector.getNumItems() > 1);
        if (const auto* ownerBus = findOwnerBusForFxSlot(project, fxSlot.id))
        {
            const auto position = std::find(ownerBus->fxSlotIds.begin(), ownerBus->fxSlotIds.end(),
                                            fxSlot.id);
            routingSelectedFxContext = "Owner "
                + juce::String::fromUTF8(ownerBus->displayName.c_str())
                + " | Insert "
                + juce::String(position == ownerBus->fxSlotIds.end()
                                   ? 0 : static_cast<int>(std::distance(ownerBus->fxSlotIds.begin(), position)) + 1)
                + " of " + juce::String(static_cast<int>(ownerBus->fxSlotIds.size()))
                + " | " + (fxSlot.bypassed ? "Bypassed" : "Active");
        }
        else
        {
            routingSelectedFxContext = "No owner bus | position unavailable | "
                + juce::String(fxSlot.bypassed ? "Bypassed" : "Active");
            routingWarningState = true;
        }
        fxSummaryLabel.setText(
            unavailable ? "Legacy effect — review to enable | " + juce::String::fromUTF8(fxSlot.id.c_str())
                : juce::String::fromUTF8(fxSlot.id.c_str()) + " | "
                    + (fxSlot.bypassed ? "bypassed" : "active") + " | cost "
                    + juce::String(descriptor != nullptr ? descriptor->cost.costUnits : 0) + " units",
            juce::dontSendNotification);
        std::uint32_t totalCost = 0;
        std::uint32_t scopedCost = 0;
        for (const auto& candidate : project.authoring.fxSlots)
        {
            const auto* candidateDescriptor = drs::engine::findCuratedDspEffect(candidate.effectType,
                                                                                  candidate.effectVersion);
            if (candidateDescriptor == nullptr || candidate.bypassed || candidate.unavailable || candidate.legacyInert)
                continue;
            totalCost += candidateDescriptor->cost.costUnits;
            if (std::find(scopedFxSlotIds.begin(), scopedFxSlotIds.end(), candidate.id) != scopedFxSlotIds.end())
                scopedCost += candidateDescriptor->cost.costUnits;
        }
        const auto preview = authoringPreviewStatusProvider ? authoringPreviewStatusProvider()
                                                            : AuthoringPreviewStatusSnapshot {};
        const auto tailCapable = descriptor != nullptr
            && (descriptor->stateClass == drs::engine::CuratedDspStateClass::delay
                || descriptor->stateClass == drs::engine::CuratedDspStateClass::reverb);
        fxDiagnosticsLabel.setText(
            "Preview: " + juce::String::fromUTF8(preview.available ? preview.stateLabel.c_str() : "unavailable")
                + " | chain " + juce::String(scopedCost) + " units | graph " + juce::String(totalCost)
                + "/128" + (totalCost > 128 ? " OVER BUDGET" : " within budget")
                + " | " + (tailCapable ? "tail-capable" : "no tail"),
            juce::dontSendNotification);
        routingWarningState = routingWarningState || totalCost > 128;
    }
    else
    {
        fxNameEditor.setText({}, juce::dontSendNotification);
        fxTypeSelector.clear(juce::dontSendNotification);
        fxParameterSelector.clear(juce::dontSendNotification);
        fxParameterIds.clear();
        fxTypeSelector.setEnabled(false);
        fxBypassedToggle.setEnabled(false);
        fxNameEditor.setEnabled(false);
        fxParameterSelector.setEnabled(false);
        fxParameterSlider.setEnabled(false);
        fxParameterResetButton.setEnabled(false);
        fxAssignMacroButton.setEnabled(false);
        fxAssignMacroButton.setButtonText("Create Control");
        fxAssignMacroButton.setTitle(fxAssignMacroButton.getButtonText());
        fxParameterValueLabel.setText("Select a catalog effect to inspect its descriptor value and default.",
                                      juce::dontSendNotification);
        fxDuplicateButton.setEnabled(false);
        fxMoveUpButton.setEnabled(false);
        fxMoveDownButton.setEnabled(false);
        fxDeleteButton.setEnabled(false);
        fxMoveOwnerButton.setEnabled(false);
        fxSummaryLabel.setText(selectedDspScopeRoutingBusId().empty()
                                   ? "No insert chain at this scope. Add Insert creates one explicitly."
                                   : "This insert chain is empty. Add Insert creates its first slot.",
                               juce::dontSendNotification);
        fxDiagnosticsLabel.setText("Preview: no scoped effect selected | graph diagnostics use immutable authoring data.",
                                   juce::dontSendNotification);
    }
    fxAddButton.setEnabled(true);

    if (!project.authoring.routingBuses.empty())
    {
        const auto& routingBus = project.authoring.routingBuses[static_cast<std::size_t>(selectedRoutingBusIndex)];
        routingInputSourceIds.clear();
        routingInputSourceIds.push_back("master");
        for (const auto& group : project.authoring.groups)
        {
            if (!group.id.empty())
                routingInputSourceIds.push_back("groups/" + group.id);
        }
        for (const auto& zone : project.authoring.zones)
            routingInputSourceIds.push_back(zone.id);

        routingInputSelector.clear(juce::dontSendNotification);
        int selectedInputId = 0;
        for (std::size_t index = 0; index < routingInputSourceIds.size(); ++index)
        {
            routingInputSelector.addItem(formatRoutingInputSourceLabel(project, routingInputSourceIds[index]),
                                         static_cast<int>(index) + 1);
            if (routingBus.inputSourceId == routingInputSourceIds[index])
                selectedInputId = static_cast<int>(index) + 1;
        }
        routingInputSelector.setSelectedId(selectedInputId > 0 ? selectedInputId : 1, juce::dontSendNotification);

        auto refreshInsertSelector = [&](juce::ComboBox& combo, const std::string& selectedFxId)
        {
            combo.clear(juce::dontSendNotification);
            combo.addItem("(none)", 1);

            int selectedId = 1;
            for (std::size_t index = 0; index < project.authoring.fxSlots.size(); ++index)
            {
                const auto itemId = static_cast<int>(index) + 2;
                combo.addItem(juce::String::fromUTF8(project.authoring.fxSlots[index].id.c_str()), itemId);
                if (project.authoring.fxSlots[index].id == selectedFxId)
                    selectedId = itemId;
            }

            combo.setSelectedId(selectedId, juce::dontSendNotification);
        };

        refreshInsertSelector(routingInsertOneSelector,
                              routingBus.fxSlotIds.empty() ? std::string{} : routingBus.fxSlotIds.front());
        refreshInsertSelector(routingInsertTwoSelector,
                              routingBus.fxSlotIds.size() < 2 ? std::string{} : routingBus.fxSlotIds[1]);

        routingSummaryLabel.setText(
            "Bus " + juce::String::fromUTF8(routingBus.id.c_str())
                + " | source " + formatRoutingInputSourceLabel(project, routingBus.inputSourceId)
                + " | chain " + joinIdList(routingBus.fxSlotIds)
                + " | " + buildMacroControllerSummaryForBus(project, routingBus.id),
            juce::dontSendNotification);
        routingSignalPath = "Input "
            + formatRoutingInputSourceLabel(project, routingBus.inputSourceId);
        for (const auto& fxSlotId : routingBus.fxSlotIds)
        {
            const auto fxSlot = std::find_if(project.authoring.fxSlots.begin(),
                                             project.authoring.fxSlots.end(),
                                             [&](const auto& candidate)
                                             {
                                                 return candidate.id == fxSlotId;
                                             });
            routingSignalPath << " > "
                              << (fxSlot != project.authoring.fxSlots.end()
                                      ? juce::String::fromUTF8(fxSlot->displayName.c_str())
                                      : juce::String::fromUTF8(fxSlotId.c_str()));
        }
        routingSignalPath << " > Output";
    }
    else
    {
        routingSummaryLabel.setText("No routing buses are authored in this project yet.", juce::dontSendNotification);
    }

    routingWorkbenchContent.setPresentationState(
        !project.authoring.routingBuses.empty(), hasScopedFx, routingWarningState,
        routingSignalPath, routingSelectedFxContext, routingMacroControlSummary);

    if (const auto selectedPerformanceBank = authoringSession.getSelectedPerformanceBank(); selectedPerformanceBank.has_value())
    {
        triggerEventSelector.clear(juce::dontSendNotification);
        int selectedTriggerEventId = 0;
        for (std::size_t index = 0; index < curatedTriggerEvents.size(); ++index)
        {
            triggerEventSelector.addItem(curatedTriggerEvents[index], static_cast<int>(index) + 1);
        }

        targetArticulationSelector.clear(juce::dontSendNotification);
        const auto articulationIds = buildArticulationIds(project);
        int selectedArticulationId = 0;
        for (std::size_t index = 0; index < articulationIds.size(); ++index)
        {
            targetArticulationSelector.addItem(juce::String::fromUTF8(articulationIds[index].c_str()),
                                               static_cast<int>(index) + 1);
        }

        phraseAssetSelector.clear(juce::dontSendNotification);
        phraseAssetSelector.addItem("(none)", 1);
        int selectedPhraseAssetId = 1;
        for (std::size_t index = 0; index < selectedPerformanceBank->phraseAssets.size(); ++index)
        {
            const auto itemId = static_cast<int>(index) + 2;
            phraseAssetSelector.addItem(juce::String::fromUTF8(selectedPerformanceBank->phraseAssets[index].displayName.c_str()),
                                        itemId);
        }

        chordModeSelector.clear(juce::dontSendNotification);
        int selectedChordModeId = 0;
        for (std::size_t index = 0; index < curatedChordModes.size(); ++index)
        {
            chordModeSelector.addItem(curatedChordModes[index], static_cast<int>(index) + 1);
        }

        if (selectedTriggerSlotIndex >= 0
            && static_cast<std::size_t>(selectedTriggerSlotIndex) < selectedPerformanceBank->triggerSlots.size())
        {
            const auto& triggerSlot = selectedPerformanceBank->triggerSlots[static_cast<std::size_t>(selectedTriggerSlotIndex)];
            for (std::size_t index = 0; index < curatedTriggerEvents.size(); ++index)
            {
                if (triggerSlot.triggerEvent == curatedTriggerEvents[index])
                    selectedTriggerEventId = static_cast<int>(index) + 1;
            }

            for (std::size_t index = 0; index < articulationIds.size(); ++index)
            {
                if (triggerSlot.targetArticulationId == articulationIds[index])
                    selectedArticulationId = static_cast<int>(index) + 1;
            }

            for (std::size_t index = 0; index < selectedPerformanceBank->phraseAssets.size(); ++index)
            {
                if (triggerSlot.phraseAssetId == selectedPerformanceBank->phraseAssets[index].id)
                    selectedPhraseAssetId = static_cast<int>(index) + 2;
            }

            for (std::size_t index = 0; index < curatedChordModes.size(); ++index)
            {
                if (triggerSlot.chordMode == curatedChordModes[index])
                    selectedChordModeId = static_cast<int>(index) + 1;
            }

            performanceSummaryLabel.setText(
                "Trigger " + juce::String::fromUTF8(triggerSlot.displayName.c_str())
                    + " | event " + juce::String::fromUTF8(triggerSlot.triggerEvent.c_str())
                    + " | articulation " + juce::String::fromUTF8(triggerSlot.targetArticulationId.c_str()),
                juce::dontSendNotification);
        }
        else
        {
            performanceSummaryLabel.setText("No trigger slot is selected in the active performance bank.",
                                            juce::dontSendNotification);
        }

        triggerEventSelector.setSelectedId(selectedTriggerEventId > 0 ? selectedTriggerEventId : 1,
                                           juce::dontSendNotification);
        targetArticulationSelector.setSelectedId(selectedArticulationId > 0 ? selectedArticulationId : 1,
                                                 juce::dontSendNotification);
        phraseAssetSelector.setSelectedId(selectedPhraseAssetId, juce::dontSendNotification);
        chordModeSelector.setSelectedId(selectedChordModeId > 0 ? selectedChordModeId : 1,
                                        juce::dontSendNotification);

        if (!selectedPerformanceBank->phraseAssets.empty())
        {
            const auto phraseAssetIndex = std::max(0, phraseAssetSelector.getSelectedId() - 2);
            if (phraseAssetSelector.getSelectedId() > 1
                && static_cast<std::size_t>(phraseAssetIndex) < selectedPerformanceBank->phraseAssets.size())
            {
                const auto& phraseAsset = selectedPerformanceBank->phraseAssets[static_cast<std::size_t>(phraseAssetIndex)];
                phraseSummaryLabel.setText(
                    juce::String::fromUTF8(phraseAsset.displayName.c_str())
                        + " | notes=" + juce::String(static_cast<int>(phraseAsset.notes.size()))
                        + " | beats=" + juce::String(phraseAsset.lengthBeats, 2)
                        + " | chord=" + juce::String::fromUTF8(phraseAsset.chordHint.c_str()),
                    juce::dontSendNotification);
            }
            else
            {
                phraseSummaryLabel.setText("Phrase library ready. Select a phrase asset to inspect it.",
                                           juce::dontSendNotification);
            }
        }
        else
        {
            phraseSummaryLabel.setText("No MIDI phrases have been imported for the active performance bank yet.",
                                       juce::dontSendNotification);
        }
    }
    else
    {
        performanceSummaryLabel.setText("No performance bank is selected.", juce::dontSendNotification);
        phraseSummaryLabel.setText("Performance phrases unavailable.", juce::dontSendNotification);
    }

    updateDynamicAccessibleText(performanceSummaryLabel,
                                performanceSummaryLabel.getText(),
                                "Performance summary: ");
    updateDynamicAccessibleText(phraseSummaryLabel,
                                phraseSummaryLabel.getText(),
                                "Phrase summary: ");
    refreshContextualAccessibility();

    refreshWaveformWorkbenchContent();
    refreshInspectorVisibility();
    observedDocumentRevision = authoringSession.getDocumentState().revision;
    observedWorkspaceSelectionRevision = authoringSession.getWorkspaceSelectionRevision();
    hasObservedSessionRevisions = true;
}

void AuthoringPanel::refreshSelectionFromSession()
{
    const juce::ScopedValueSetter<bool> refreshGuard(isRefreshing, true);
    const auto& project = authoringSession.getProject();
    const auto selectedZone = authoringSession.getSelectedZone();
    const auto previousSelectedGroupIndex = selectedGroupIndex;
    const auto previousSelectedLayerIndex = selectedLayerIndex;
    auto selectedZoneIndex = -1;
    if (selectedZone.has_value())
    {
        for (std::size_t index = 0; index < project.authoring.zones.size(); ++index)
        {
            if (project.authoring.zones[index].id == selectedZone->id)
            {
                selectedZoneIndex = static_cast<int>(index);
                break;
            }
        }
    }
    if (zoneSelector.getSelectedId() != selectedZoneIndex + 1)
        zoneSelector.setSelectedId(selectedZoneIndex + 1, juce::dontSendNotification);

    const auto selectedGroup = authoringSession.getSelectedGroup();
    selectedGroupIndex = -1;
    if (selectedGroup.has_value())
    {
        for (std::size_t index = 0; index < project.authoring.groups.size(); ++index)
        {
            if (project.authoring.groups[index].id == selectedGroup->id)
            {
                selectedGroupIndex = static_cast<int>(index);
                break;
            }
        }
    }
    const auto groupSelectionChanged = previousSelectedGroupIndex != selectedGroupIndex;
    if (groupSelectionChanged && selectedGroupIndex >= 0)
        groupList.setSelectedIndex(selectedGroupIndex);

    const auto selectedLayer = authoringSession.getSelectedLayer();
    selectedLayerIndex = -1;
    if (selectedLayer.has_value())
    {
        for (std::size_t index = 0; index < project.authoring.layers.size(); ++index)
        {
            if (project.authoring.layers[index].id == selectedLayer->id)
            {
                selectedLayerIndex = static_cast<int>(index);
                break;
            }
        }
    }
    const auto layerSelectionChanged = previousSelectedLayerIndex != selectedLayerIndex;
    if (layerSelectionChanged && selectedLayerIndex >= 0)
        layerList.setSelectedIndex(selectedLayerIndex);
    if (selectedLayer.has_value() && layerSelectionChanged)
    {
        layerNameEditor.setText(juce::String::fromUTF8(selectedLayer->displayName.c_str()),
                                juce::dontSendNotification);
        layerVisibilityToggle.setToggleState(selectedLayer->workspaceVisible, juce::dontSendNotification);
        layerGainSlider.setValue(selectedLayer->gainDb, juce::dontSendNotification);
        layerPanSlider.setValue(selectedLayer->pan, juce::dontSendNotification);
        layerCrossfadeSourceSelector.setSelectedId(
            selectedLayer->crossfade.source == drs::engine::LayerCrossfadeSource::velocity ? 2
                : selectedLayer->crossfade.source == drs::engine::LayerCrossfadeSource::controller ? 3 : 1,
            juce::dontSendNotification);
        layerCrossfadeControllerSlider.setValue(
            selectedLayer->crossfade.controllerNumber.value_or(1), juce::dontSendNotification);
        layerCrossfadeLowSlider.setValue(selectedLayer->crossfade.low, juce::dontSendNotification);
        layerCrossfadeHighSlider.setValue(selectedLayer->crossfade.high, juce::dontSendNotification);
        layerCrossfadeDirectionSelector.setSelectedId(
            selectedLayer->crossfade.direction == drs::engine::LayerCrossfadeDirection::fadeOut ? 2 : 1,
            juce::dontSendNotification);

        layerRoutingBusIds.clear();
        layerRoutingSelector.clear(juce::dontSendNotification);
        layerRoutingSelector.addItem("(none)", 1);
        layerRoutingBusIds.push_back({});
        int routingItem = 2;
        for (const auto& bus : project.authoring.routingBuses)
        {
            if (bus.inputSourceId != "layers/" + selectedLayer->id)
                continue;
            layerRoutingSelector.addItem(juce::String::fromUTF8(bus.displayName.c_str()), routingItem++);
            layerRoutingBusIds.push_back(bus.id);
        }
        const auto routingIndex = std::find(layerRoutingBusIds.begin(), layerRoutingBusIds.end(),
                                            selectedLayer->routingBusId);
        layerRoutingSelector.setSelectedId(routingIndex == layerRoutingBusIds.end()
                                               ? 1
                                               : static_cast<int>(std::distance(layerRoutingBusIds.begin(), routingIndex)) + 1,
                                           juce::dontSendNotification);

        layerAnchorGroupIds.clear();
        layerAnchorSelector.clear(juce::dontSendNotification);
        layerAnchorSelector.addItem("(none)", 1);
        layerAnchorGroupIds.push_back({});
        int anchorItem = 2;
        for (const auto& group : project.authoring.groups)
        {
            if (group.layerId != selectedLayer->id)
                continue;
            layerAnchorSelector.addItem(juce::String::fromUTF8(group.displayName.c_str()), anchorItem++);
            layerAnchorGroupIds.push_back(group.id);
        }
        const auto anchorIndex = std::find(layerAnchorGroupIds.begin(), layerAnchorGroupIds.end(),
                                           selectedLayer->auditionAnchorGroupId);
        layerAnchorSelector.setSelectedId(anchorIndex == layerAnchorGroupIds.end()
                                              ? 1
                                              : static_cast<int>(std::distance(layerAnchorGroupIds.begin(), anchorIndex)) + 1,
                                          juce::dontSendNotification);
    }

    syncZoneMapSelectionState();
    if (const auto selectedZone = authoringSession.getSelectedZone(); selectedZone.has_value()
        && (structureViewState.isMapPaneVisible()
            || structureSelection.getKind() == authoring::StructureSelectionKind::zone))
    {
        structureSelection.replace(authoring::StructureSelectionKind::zone,
                                   zoneMapSelectedZoneIds,
                                   selectedZone->id);
    }
    zoneMap.setSelectionState({ zoneMapSelectedZoneIds,
                                selectedZone.has_value() ? selectedZone->id : std::string {} });

    selectionSummaryViewModel = buildSelectionSummaryViewModel();
    zoneFieldValuesViewModel = buildZoneFieldValuesViewModel();
    summaryStrip.setViewModel(selectionSummaryViewModel);
    zoneMappingEditor.setViewModel(zoneFieldValuesViewModel);

    if (selectedGroup.has_value() && groupSelectionChanged)
    {
        groupNameEditor.setText(juce::String::fromUTF8(selectedGroup->displayName.c_str()),
                                juce::dontSendNotification);
        groupVisibilityToggle.setToggleState(selectedGroup->workspaceVisible, juce::dontSendNotification);
        groupGainSlider.setValue(selectedGroup->gainDb, juce::dontSendNotification);
        groupPanSlider.setValue(selectedGroup->pan, juce::dontSendNotification);
        groupVisibilityButton.setButtonText(selectedGroup->workspaceVisible ? "Hide Group" : "Show Group");
        groupVisibilityButton.setEnabled(true);
        groupPreviewAnchorButton.setEnabled(!selectedGroup->auditionAnchorZoneId.empty());
        groupMoveUpButton.setEnabled(selectedGroupIndex > 0);
        groupMoveDownButton.setEnabled(selectedGroupIndex + 1 < static_cast<int>(project.authoring.groups.size()));
        const auto memberCount = static_cast<int>(countZonesInGroup(project, selectedGroup->id));
        groupDeleteButton.setEnabled(memberCount == 0);
        groupSummaryLabel.setText(
            "Master " + juce::String(project.authoring.masterGainDb, 1) + " dB"
                + " | " + juce::String::fromUTF8(selectedGroup->displayName.c_str())
                + " | " + juce::String(memberCount) + " zones",
            juce::dontSendNotification);
    }

    refreshWaveformWorkbenchContent();
    refreshWorkbenchContextLabels();
    observedDocumentRevision = authoringSession.getDocumentState().revision;
    observedWorkspaceSelectionRevision = authoringSession.getWorkspaceSelectionRevision();
    hasObservedSessionRevisions = true;
}

void AuthoringPanel::applySelectedZoneEdit(const authoring::ZoneFieldValuesViewModel& values,
                                           const juce::String& label)
{
    const auto currentZone = authoringSession.getSelectedZone();
    if (!currentZone.has_value())
        return;

    if (zoneMapSelectedZoneIds.size() > 1 && label == "Update zone velocity range")
    {
        const auto lowVelocityChanged = values.velocityLow != currentZone->velocityLow;
        const auto highVelocityChanged = values.velocityHigh != currentZone->velocityHigh;
        auto zoneSummaries = authoringSession.getZoneSummaries();
        std::vector<drs::engine::AuthoringZoneSummary> editedZones;
        editedZones.reserve(zoneMapSelectedZoneIds.size());
        for (const auto& zoneId : zoneMapSelectedZoneIds)
        {
            const auto zoneIterator = std::find_if(zoneSummaries.begin(), zoneSummaries.end(),
                                                   [&](const auto& zone)
                                                   {
                                                       return zone.id == zoneId;
                                                   });
            if (zoneIterator == zoneSummaries.end())
                continue;

            auto editedZone = *zoneIterator;
            if (lowVelocityChanged && highVelocityChanged)
            {
                editedZone.velocityLow = values.velocityLow;
                editedZone.velocityHigh = values.velocityHigh;
            }
            else if (lowVelocityChanged)
                editedZone.velocityLow = std::min(values.velocityLow, editedZone.velocityHigh);
            else if (highVelocityChanged)
                editedZone.velocityHigh = std::max(values.velocityHigh, editedZone.velocityLow);
            editedZones.push_back(std::move(editedZone));
        }

        const auto result = authoringSession.updateZoneRanges(editedZones, label.toStdString());
        if (result.applied)
            refreshFromSession();
        return;
    }

    if (zoneMapSelectedZoneIds.size() > 1 && label == "Update zone release")
    {
        const auto result = authoringSession.updateZoneReleaseSeconds(
            zoneMapSelectedZoneIds, values.releaseSeconds, label.toStdString());
        if (result.applied)
            refreshFromSession();
        return;
    }

    auto editedZone = *currentZone;
    editedZone.rootKey = values.rootKey;
    editedZone.keyLow = values.keyLow;
    editedZone.keyHigh = values.keyHigh;
    editedZone.velocityLow = values.velocityLow;
    editedZone.velocityHigh = values.velocityHigh;
    editedZone.gainDb = values.gainDb;
    editedZone.pan = values.pan;
    editedZone.loopEnabled = values.loopEnabled;
    editedZone.loopMode = values.loopMode;
    editedZone.sampleEndFrame = values.sampleEndFrame;
    editedZone.releaseSeconds = values.releaseSeconds;
    editedZone.releaseShape = values.releaseShape;
    editedZone.triggerMode = values.triggerMode;
    editedZone.performance.event = values.performanceEvent;
    editedZone.performance.sustain = values.performanceSustain;
    editedZone.performance.pitchSource = values.performancePitchSource;
    editedZone.exclusiveGroupId = values.exclusiveGroupId;
    editedZone.exclusiveTargetGroupIds = values.exclusiveTargetGroupId.empty()
        ? std::vector<std::string> {} : std::vector<std::string> { values.exclusiveTargetGroupId };
    editedZone.chokeReleaseSeconds = values.chokeReleaseSeconds > 0.0
        ? std::optional<double> { values.chokeReleaseSeconds } : std::nullopt;

    authoringSession.updateSelectedZone(editedZone, label.toStdString());
    refreshFromSession();
}

void AuthoringPanel::applySelectedGroupNameEdit()
{
    if (isRefreshing)
        return;

    const auto selectedGroup = authoringSession.getSelectedGroup();
    if (!selectedGroup.has_value())
        return;

    auto editedGroup = *selectedGroup;
    const auto editedName = groupNameEditor.getText().trim();
    if (editedName.isEmpty() || editedName == juce::String::fromUTF8(selectedGroup->displayName.c_str()))
        return;

    editedGroup.displayName = editedName.toStdString();
    authoringSession.updateGroup(static_cast<std::size_t>(selectedGroupIndex),
                                 editedGroup,
                                 "Rename group");
    refreshFromSession();
}

void AuthoringPanel::applyProjectMasterGainEdit(const juce::String& label)
{
    if (isRefreshing)
        return;

    authoringSession.updateMasterGain(masterGainSlider.getValue(), label.toStdString());
    refreshFromSession();
}

void AuthoringPanel::applySelectedGroupMixEdit(const juce::String& label)
{
    const auto selectedGroup = authoringSession.getSelectedGroup();
    if (!selectedGroup.has_value())
        return;

    auto editedGroup = *selectedGroup;
    editedGroup.workspaceVisible = groupVisibilityToggle.getToggleState();
    editedGroup.gainDb = groupGainSlider.getValue();
    editedGroup.pan = groupPanSlider.getValue();

    const auto selectedRoutingIndex = std::max(0, groupRoutingSelector.getSelectedId() - 1);
    editedGroup.routingBusId = selectedRoutingIndex >= 0
            && static_cast<std::size_t>(selectedRoutingIndex) < groupRoutingBusIds.size()
        ? groupRoutingBusIds[static_cast<std::size_t>(selectedRoutingIndex)]
        : std::string {};

    const auto selectedAnchorIndex = std::max(0, groupAnchorSelector.getSelectedId() - 1);
    editedGroup.auditionAnchorZoneId = selectedAnchorIndex >= 0
            && static_cast<std::size_t>(selectedAnchorIndex) < groupAnchorZoneIds.size()
        ? groupAnchorZoneIds[static_cast<std::size_t>(selectedAnchorIndex)]
        : std::string {};

    authoringSession.updateGroup(static_cast<std::size_t>(selectedGroupIndex),
                                 editedGroup,
                                 label.toStdString());
    refreshFromSession();
}

void AuthoringPanel::createGroup()
{
    const auto& groups = authoringSession.getProject().authoring.groups;
    auto nextIndex = groups.size() + 1;
    std::string nextId;
    do
    {
        nextId = "group-" + std::to_string(nextIndex++);
    }
    while (std::any_of(groups.begin(),
                       groups.end(),
                       [&](const auto& group)
                       {
                           return group.id == nextId;
                       }));

    drs::engine::RuntimeProjectGroupDefinition group;
    group.id = nextId;
    group.displayName = "New Group " + std::to_string(groups.size() + 1);
    group.displayOrder = static_cast<int>(groups.size());
    group.workspaceVisible = true;

    const auto result = authoringSession.createGroup(group, "Create group");
    if (result.applied)
    {
        setActiveWorkbenchTab(authoring::WorkbenchTab::groups);
        refreshFromSession();
    }
    else
    {
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                               "Create Group Failed",
                                               buildIssueSummary(result.issues));
    }
}

void AuthoringPanel::createLayer()
{
    const auto& layers = authoringSession.getProject().authoring.layers;
    auto nextIndex = layers.size() + 1;
    std::string nextId;
    do
    {
        nextId = "layer-" + std::to_string(nextIndex++);
    }
    while (std::any_of(layers.begin(), layers.end(),
                       [&](const auto& layer) { return layer.id == nextId; }));

    drs::engine::RuntimeProjectLayerDefinition layer;
    layer.id = nextId;
    layer.displayName = "New Layer " + std::to_string(layers.size() + 1);
    layer.displayOrder = static_cast<int>(layers.size());
    layer.workspaceVisible = true;
    const auto result = authoringSession.createLayer(layer, "Create layer");
    if (result.applied)
        refreshFromSession();
    else
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                               "Create Layer Failed",
                                               buildIssueSummary(result.issues));
}

void AuthoringPanel::applySelectedLayerEdit(const juce::String& label)
{
    if (isRefreshing || selectedLayerIndex < 0)
        return;
    const auto selectedLayer = authoringSession.getSelectedLayer();
    if (!selectedLayer.has_value())
        return;

    auto editedLayer = *selectedLayer;
    const auto editedName = layerNameEditor.getText().trim();
    if (editedName.isEmpty())
        return;
    editedLayer.displayName = editedName.toStdString();
    editedLayer.workspaceVisible = layerVisibilityToggle.getToggleState();
    editedLayer.gainDb = layerGainSlider.getValue();
    editedLayer.pan = layerPanSlider.getValue();
    const auto routingIndex = layerRoutingSelector.getSelectedId() - 1;
    editedLayer.routingBusId = routingIndex >= 0
            && static_cast<std::size_t>(routingIndex) < layerRoutingBusIds.size()
        ? layerRoutingBusIds[static_cast<std::size_t>(routingIndex)] : std::string {};
    const auto anchorIndex = layerAnchorSelector.getSelectedId() - 1;
    editedLayer.auditionAnchorGroupId = anchorIndex >= 0
            && static_cast<std::size_t>(anchorIndex) < layerAnchorGroupIds.size()
        ? layerAnchorGroupIds[static_cast<std::size_t>(anchorIndex)] : std::string {};
    const auto sourceId = layerCrossfadeSourceSelector.getSelectedId();
    editedLayer.crossfade.source = sourceId == 2 ? drs::engine::LayerCrossfadeSource::velocity
        : sourceId == 3 ? drs::engine::LayerCrossfadeSource::controller
                        : drs::engine::LayerCrossfadeSource::none;
    editedLayer.crossfade.low = static_cast<int>(layerCrossfadeLowSlider.getValue());
    editedLayer.crossfade.high = static_cast<int>(layerCrossfadeHighSlider.getValue());
    if (editedLayer.crossfade.low >= editedLayer.crossfade.high)
        editedLayer.crossfade.high = std::min(127, editedLayer.crossfade.low + 1);
    editedLayer.crossfade.direction = layerCrossfadeDirectionSelector.getSelectedId() == 2
        ? drs::engine::LayerCrossfadeDirection::fadeOut
        : drs::engine::LayerCrossfadeDirection::fadeIn;
    if (editedLayer.crossfade.source == drs::engine::LayerCrossfadeSource::controller)
    {
        editedLayer.crossfade.controllerNumber = static_cast<int>(layerCrossfadeControllerSlider.getValue());
    }
    else
    {
        editedLayer.crossfade.controllerNumber.reset();
    }

    if (authoringSession.updateLayer(static_cast<std::size_t>(selectedLayerIndex),
                                     editedLayer,
                                     label.toStdString()).applied)
        refreshFromSession();
}

void AuthoringPanel::moveSelectedLayer(const int direction)
{
    if (selectedLayerIndex < 0)
        return;
    if (authoringSession.moveLayer(static_cast<std::size_t>(selectedLayerIndex),
                                   direction,
                                   direction < 0 ? "Move layer earlier" : "Move layer later").applied)
    {
        selectedLayerIndex = std::max(0, selectedLayerIndex + direction);
        refreshFromSession();
    }
}

void AuthoringPanel::assignSelectedGroupsToSelectedLayer()
{
    const auto selectedLayer = authoringSession.getSelectedLayer();
    const auto selectedGroup = authoringSession.getSelectedGroup();
    if (!selectedLayer.has_value() || !selectedGroup.has_value())
        return;

    const auto result = authoringSession.reassignGroupsToLayer(
        { selectedGroup->id }, selectedLayer->id, "Assign group to layer");
    if (result.applied)
        refreshFromSession();
    else
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                               "Assign Group Failed",
                                               buildIssueSummary(result.issues));
}

void AuthoringPanel::assignSelectedZonesToSelectedGroup()
{
    const auto selectedGroup = authoringSession.getSelectedGroup();
    if (!selectedGroup.has_value())
        return;

    const auto selectedZoneIds = collectSelectedZoneIdsForGrouping();
    if (selectedZoneIds.empty())
        return;

    const auto result = authoringSession.reassignZonesToGroup(
        selectedZoneIds,
        selectedGroup->id,
        selectedZoneIds.size() > 1 ? "Add selected zones to group" : "Add selected zone to group");
    if (result.applied)
    {
        refreshFromSession();
        return;
    }

    juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                           "Assign Zones To Group Failed",
                                           buildIssueSummary(result.issues));
}

void AuthoringPanel::deleteSelectedGroup()
{
    const auto selectedGroup = authoringSession.getSelectedGroup();
    if (!selectedGroup.has_value())
        return;

    if (authoringSession.deleteGroup(selectedGroup->id, "Delete group").applied)
        refreshFromSession();
}

void AuthoringPanel::moveSelectedGroup(int direction)
{
    if (selectedGroupIndex < 0)
        return;

    if (authoringSession.moveGroup(static_cast<std::size_t>(selectedGroupIndex),
                                   direction,
                                   direction < 0 ? "Move group earlier" : "Move group later").applied)
    {
        selectedGroupIndex = std::max(0, selectedGroupIndex + direction);
        refreshFromSession();
    }
}

void AuthoringPanel::toggleSelectedGroupVisibility()
{
    const auto selectedGroup = authoringSession.getSelectedGroup();
    if (!selectedGroup.has_value())
        return;

    auto editedGroup = *selectedGroup;
    editedGroup.workspaceVisible = !editedGroup.workspaceVisible;
    if (authoringSession.updateGroup(static_cast<std::size_t>(selectedGroupIndex),
                                     editedGroup,
                                     editedGroup.workspaceVisible ? "Show group on map" : "Hide group on map").applied)
    {
        refreshFromSession();
    }
}

void AuthoringPanel::previewSelectedGroupAnchor()
{
    if (!previewEnabledToggle.getToggleState())
        return;

    const auto previewRequest = authoringSession.buildSelectedGroupPreviewRequest();
    if (!previewRequest.available || !previewCommandCallback)
        return;

    constexpr auto source = drs::engine::AuthoringPreviewAuditionSource::inspector;
    const auto sourceIndex = std::min<std::size_t>(static_cast<std::size_t>(source),
                                                   timedPreviewNotes.size() - 1);
    releaseTimedPreview(sourceIndex);

    drs::engine::AuthoringPreviewCommand command;
    command.type = drs::engine::AuthoringPreviewCommandType::auditionSelectedGroup;
    command.source = source;
    command.midiNote = previewRequest.midiNote;
    command.velocity = static_cast<float>(previewRequest.velocity) / 127.0f;
    command.selectedZoneId = previewRequest.anchorZoneId;
    command.selectedGroupId = previewRequest.groupId;
    previewCommandCallback(command);

    timedPreviewNotes[sourceIndex] = { true, previewRequest.midiNote,
                                       juce::Time::getMillisecondCounterHiRes() + 180.0 };
    startTimer(previewReleaseTimerId, 10);
}

void AuthoringPanel::previewSelectedZone(
    drs::engine::AuthoringPreviewAuditionSource source,
    int explicitMidiNote,
    int explicitVelocity,
    std::string explicitZoneId)
{
    if (!previewEnabledToggle.getToggleState())
        return;

    const auto request = authoringSession.buildSelectedZonePreviewRequest();
    if (!request.available)
        return;

    const auto midiNote = explicitMidiNote >= 0 ? explicitMidiNote : request.midiNote;
    const auto velocity = explicitVelocity > 0 ? explicitVelocity : request.velocity;
    const auto sourceIndex = std::min<std::size_t>(static_cast<std::size_t>(source),
                                                   timedPreviewNotes.size() - 1);
    releaseTimedPreview(sourceIndex);

    if (!previewCommandCallback)
        return;

    drs::engine::AuthoringPreviewCommand command;
    command.type = drs::engine::AuthoringPreviewCommandType::auditionSelectedZone;
    command.source = source;
    command.midiNote = midiNote;
    command.velocity = static_cast<float>(velocity) / 127.0f;
    command.selectedZoneId = explicitZoneId.empty()
        ? authoringSession.getSelectedZone()->id
        : std::move(explicitZoneId);
    previewCommandCallback(command);

    timedPreviewNotes[sourceIndex] = { true, midiNote,
                                       juce::Time::getMillisecondCounterHiRes() + 180.0 };
    startTimer(previewReleaseTimerId, 10);
}

void AuthoringPanel::auditionVelocityCrossfade(const std::vector<int>& velocities)
{
    if (!previewEnabledToggle.getToggleState() || !previewCommandCallback || velocities.empty())
        return;
    const auto request = authoringSession.buildSelectedZonePreviewRequest();
    if (!request.available)
        return;

    crossfadeAuditionSequence.active = true;
    crossfadeAuditionSequence.midiNote = request.midiNote;
    crossfadeAuditionSequence.velocities = velocities;
    crossfadeAuditionSequence.nextIndex = 0;
    crossfadeAuditionSequence.nextAtMillis = juce::Time::getMillisecondCounterHiRes();
    dispatchNextCrossfadeAuditionStep();
}

void AuthoringPanel::dispatchNextCrossfadeAuditionStep()
{
    if (!crossfadeAuditionSequence.active || !previewCommandCallback)
    {
        crossfadeAuditionSequence.active = false;
        return;
    }
    if (crossfadeAuditionSequence.nextIndex >= crossfadeAuditionSequence.velocities.size())
    {
        crossfadeAuditionSequence.active = false;
        return;
    }

    constexpr auto source = drs::engine::AuthoringPreviewAuditionSource::inspector;
    const auto sourceIndex = static_cast<std::size_t>(source);
    releaseTimedPreview(sourceIndex);
    const auto velocity = std::clamp(crossfadeAuditionSequence.velocities[crossfadeAuditionSequence.nextIndex], 1, 127);
    drs::engine::AuthoringPreviewCommand command;
    command.type = drs::engine::AuthoringPreviewCommandType::auditionCurrentDraft;
    command.source = source;
    command.midiNote = crossfadeAuditionSequence.midiNote;
    command.velocity = static_cast<float>(velocity) / 127.0f;
    previewCommandCallback(command);

    timedPreviewNotes[sourceIndex] = { true, crossfadeAuditionSequence.midiNote,
                                       juce::Time::getMillisecondCounterHiRes() + 140.0 };
    ++crossfadeAuditionSequence.nextIndex;
    crossfadeAuditionSequence.nextAtMillis = juce::Time::getMillisecondCounterHiRes() + 240.0;
    startTimer(previewReleaseTimerId, 10);
}

void AuthoringPanel::releaseTimedPreview(std::size_t sourceIndex)
{
    if (sourceIndex >= timedPreviewNotes.size() || !timedPreviewNotes[sourceIndex].active)
        return;

    const auto note = timedPreviewNotes[sourceIndex].midiNote;
    timedPreviewNotes[sourceIndex] = {};
    const auto inspectorSourceIndex = static_cast<std::size_t>(
        drs::engine::AuthoringPreviewAuditionSource::inspector);
    if (sourceIndex == inspectorSourceIndex && waveformAuditionCueActive
        && waveformAuditionRegion.loopActive && waveformAuditionNoteOffMillis <= 0.0)
    {
        waveformAuditionNoteOffMillis = juce::Time::getMillisecondCounterHiRes();
        waveformLoopAuditionButton.setButtonText("Play Loop");
    }
    if (!previewCommandCallback)
        return;

    drs::engine::AuthoringPreviewCommand command;
    command.type = drs::engine::AuthoringPreviewCommandType::noteOff;
    command.source = static_cast<drs::engine::AuthoringPreviewAuditionSource>(sourceIndex);
    command.midiNote = note;
    previewCommandCallback(command);
}

void AuthoringPanel::prepareDraftPlaybackPreview()
{
    if (onPrepareDraftPlaybackRequested)
        onPrepareDraftPlaybackRequested();

    refreshNow();
}

void AuthoringPanel::publishDraftPlayback()
{
    if (onPublishDraftPlaybackRequested)
        onPublishDraftPlaybackRequested();

    refreshNow();
}

void AuthoringPanel::undoLastEdit()
{
    authoringSession.undo();
    refreshFromSession();
}

void AuthoringPanel::redoLastEdit()
{
    authoringSession.redo();
    refreshFromSession();
}

std::vector<drs::engine::AuthoringZoneSummary> AuthoringPanel::buildVisibleZoneSummaries() const
{
    const auto zoneSummaries = authoringSession.getZoneSummaries();
    const auto& project = authoringSession.getProject();
    const auto selectedZone = authoringSession.getSelectedZone();
    const auto selectedZoneId = selectedZone.has_value() ? selectedZone->id : std::string {};

    std::unordered_map<std::string, std::string> groupIdByZoneId;
    groupIdByZoneId.reserve(project.authoring.zones.size());
    for (const auto& zone : project.authoring.zones)
        groupIdByZoneId.emplace(zone.id, zone.groupId);

    std::unordered_map<std::string, bool> groupVisibilityById;
    groupVisibilityById.reserve(project.authoring.groups.size());
    for (const auto& group : project.authoring.groups)
        groupVisibilityById.emplace(group.id, group.workspaceVisible);

    std::unordered_set<std::string> multiSelectionIds;
    multiSelectionIds.reserve(zoneMapSelectedZoneIds.size());
    for (const auto& zoneId : zoneMapSelectedZoneIds)
        multiSelectionIds.insert(zoneId);

    std::vector<drs::engine::AuthoringZoneSummary> visibleZones;
    visibleZones.reserve(zoneSummaries.size());

    for (const auto& zone : zoneSummaries)
    {
        auto visibleZone = zone;
        visibleZone.selected = selectedZoneId == zone.id;
        visibleZone.additionallySelected = multiSelectionIds.count(zone.id) > 0 && !visibleZone.selected;

        auto groupVisible = true;
        if (const auto projectZoneIterator = groupIdByZoneId.find(visibleZone.id);
            projectZoneIterator != groupIdByZoneId.end())
        {
            if (const auto groupIterator = groupVisibilityById.find(projectZoneIterator->second);
                groupIterator != groupVisibilityById.end())
            {
                groupVisible = groupIterator->second;
            }
        }

        if (groupVisible
            || visibleZone.selected
            || visibleZone.additionallySelected)
        {
            visibleZones.push_back(std::move(visibleZone));
        }
    }

    return visibleZones;
}

void AuthoringPanel::syncZoneMapSelectionState()
{
    const auto selectedZone = authoringSession.getSelectedZone();
    if (!selectedZone.has_value())
    {
        zoneMapSelectedZoneIds.clear();
        return;
    }

    if (std::find(zoneMapSelectedZoneIds.begin(),
                  zoneMapSelectedZoneIds.end(),
                  selectedZone->id) == zoneMapSelectedZoneIds.end())
    {
        zoneMapSelectedZoneIds = { selectedZone->id };
        return;
    }

    std::unordered_set<std::string> selectedZoneIds;
    selectedZoneIds.reserve(zoneMapSelectedZoneIds.size());
    for (const auto& zoneId : zoneMapSelectedZoneIds)
        selectedZoneIds.insert(zoneId);

    std::vector<std::string> normalizedSelectionIds { selectedZone->id };
    normalizedSelectionIds.reserve(zoneMapSelectedZoneIds.size());

    for (const auto& zone : authoringSession.getProject().authoring.zones)
    {
        if (zone.id == selectedZone->id)
            continue;

        if (selectedZoneIds.count(zone.id) > 0)
        {
            normalizedSelectionIds.push_back(zone.id);
        }
    }

    zoneMapSelectedZoneIds = std::move(normalizedSelectionIds);
}

bool AuthoringPanel::applyZoneMapSelectionState(const authoring::ZoneMapCanvas::SelectionState& selectionState)
{
    const ScopedMessageThreadSpan timing(MessageThreadSpanKind::zoneSelection);
    const auto& zones = authoringSession.getProject().authoring.zones;
    std::unordered_set<std::string> validZoneIds;
    validZoneIds.reserve(zones.size());
    for (const auto& zone : zones)
        validZoneIds.insert(zone.id);

    std::vector<std::string> normalizedSelectionIds;
    normalizedSelectionIds.reserve(selectionState.zoneIds.size());
    std::unordered_set<std::string> seenZoneIds;
    seenZoneIds.reserve(selectionState.zoneIds.size());

    for (const auto& requestedZoneId : selectionState.zoneIds)
    {
        if (validZoneIds.count(requestedZoneId) == 0)
            continue;

        if (seenZoneIds.insert(requestedZoneId).second)
        {
            normalizedSelectionIds.push_back(requestedZoneId);
        }
    }

    if (normalizedSelectionIds.empty())
        return false;

    auto primaryZoneId = selectionState.primaryZoneId;
    if (std::find(normalizedSelectionIds.begin(),
                  normalizedSelectionIds.end(),
                  primaryZoneId) == normalizedSelectionIds.end())
    {
        primaryZoneId = normalizedSelectionIds.front();
    }

    normalizedSelectionIds.erase(std::remove(normalizedSelectionIds.begin(),
                                             normalizedSelectionIds.end(),
                                             primaryZoneId),
                                 normalizedSelectionIds.end());
    normalizedSelectionIds.insert(normalizedSelectionIds.begin(), primaryZoneId);

    const auto currentSelectedZone = authoringSession.getSelectedZone();
    const auto currentPrimaryZoneId = currentSelectedZone.has_value() ? currentSelectedZone->id : std::string {};
    const auto selectionChanged = zoneMapSelectedZoneIds != normalizedSelectionIds;
    const auto primaryChanged = currentPrimaryZoneId != primaryZoneId;

    zoneMapSelectedZoneIds = normalizedSelectionIds;

    if (primaryChanged)
    {
        const auto result = authoringSession.selectZone(primaryZoneId);
        if (!result.applied)
            return selectionChanged;
    }

    structureSelection.replace(authoring::StructureSelectionKind::zone,
                               normalizedSelectionIds,
                               primaryZoneId);

    return selectionChanged || primaryChanged;
}

std::size_t AuthoringPanel::getZoneMapSelectionCount() const
{
    return zoneMapSelectedZoneIds.size();
}

std::vector<std::string> AuthoringPanel::collectSelectedZoneIdsForGrouping() const
{
    std::vector<std::string> zoneIds = zoneMapSelectedZoneIds;
    if (!zoneIds.empty())
        return zoneIds;

    if (const auto selectedZone = authoringSession.getSelectedZone(); selectedZone.has_value())
        zoneIds.push_back(selectedZone->id);

    return zoneIds;
}

void AuthoringPanel::applySelectedMacroEdit(const juce::String& label,
                                            const MacroEditField field)
{
    const auto& macros = authoringSession.getProject().authoring.macros;
    if (selectedMacroIndex < 0 || static_cast<std::size_t>(selectedMacroIndex) >= macros.size())
        return;

    auto editedMacro = macros[static_cast<std::size_t>(selectedMacroIndex)];
    if (field == MacroEditField::name)
        editedMacro.name = macroNameEditor.getText().trim().toStdString();
    else if (field == MacroEditField::exposure)
        editedMacro.exposedInPerformance = macroExposeToggle.getToggleState();
    else if (field == MacroEditField::defaultValue)
        editedMacro.defaultValue = std::clamp(macroDefaultSlider.getValue(),
                                              editedMacro.minValue,
                                              editedMacro.maxValue);
    else if (field == MacroEditField::range)
    {
        auto minValue = macroMinSlider.getValue();
        auto maxValue = macroMaxSlider.getValue();
        if (minValue > maxValue)
            std::swap(minValue, maxValue);
        editedMacro.minValue = minValue;
        editedMacro.maxValue = maxValue;
        editedMacro.defaultValue = std::clamp(macroDefaultSlider.getValue(), minValue, maxValue);
        for (auto& target : editedMacro.targets)
        {
            target.sourceMinimum = minValue;
            target.sourceMaximum = maxValue;
        }
    }

    const auto assignmentId = macroAssignmentSelector.getSelectedId();
    auto editableTargetIndex = selectedMacroTargetIndex;
    if (field == MacroEditField::assignment && assignmentId == unassignedMacroAssignmentId)
    {
        if (editableTargetIndex >= 0
            && static_cast<std::size_t>(editableTargetIndex) < editedMacro.targets.size())
        {
            editedMacro.targets.erase(editedMacro.targets.begin() + editableTargetIndex);
            selectedMacroTargetIndex = editedMacro.targets.empty()
                ? -1 : std::min(editableTargetIndex,
                                static_cast<int>(editedMacro.targets.size()) - 1);
        }
    }
    else if (field == MacroEditField::assignment
             && assignmentId >= curatedMacroAssignmentBase
             && assignmentId < curatedMacroAssignmentBase + static_cast<int>(curatedMacroAssignments.size()))
    {
        if (editableTargetIndex < 0
            || static_cast<std::size_t>(editableTargetIndex) >= editedMacro.targets.size())
        {
            editedMacro.targets.push_back({});
            editableTargetIndex = static_cast<int>(editedMacro.targets.size()) - 1;
            selectedMacroTargetIndex = editableTargetIndex;
        }
        const auto& assignment = curatedMacroAssignments[static_cast<std::size_t>(assignmentId - curatedMacroAssignmentBase)];
        auto& target = editedMacro.targets[static_cast<std::size_t>(editableTargetIndex)];
        target.parameterId = assignment.parameterId;
        target.parameterPath = assignment.parameterPath;
        target.dspSlotId.clear();
        target.dspParameterId.clear();
        target.sourceMinimum = editedMacro.minValue;
        target.sourceMaximum = editedMacro.maxValue;
        target.destinationMinimum = 0.0;
        target.destinationMaximum = 1.0;
        target.curve = "linear";
        target.controlLaw = {};
        if (target.role.empty())
            target.role = assignment.defaultRole;
    }
    else if (field == MacroEditField::assignment
             && assignmentId >= curatedDspMacroAssignmentBase)
    {
        const auto dspAssignments = buildCuratedDspMacroAssignments(authoringSession.getProject(),
                                                                    selectedDspScopeRoutingBusId(),
                                                                    selectedDspScopeInputSource());
        const auto assignmentIndex = static_cast<std::size_t>(assignmentId - curatedDspMacroAssignmentBase);
        if (assignmentIndex < dspAssignments.size())
        {
            if (editableTargetIndex < 0
                || static_cast<std::size_t>(editableTargetIndex) >= editedMacro.targets.size())
            {
                editedMacro.targets.push_back({});
                editableTargetIndex = static_cast<int>(editedMacro.targets.size()) - 1;
                selectedMacroTargetIndex = editableTargetIndex;
            }
            const auto& assignment = dspAssignments[assignmentIndex];
            auto& target = editedMacro.targets[static_cast<std::size_t>(editableTargetIndex)];
            target.parameterId = "dsp." + assignment.slot->id + "." + std::string(assignment.parameter->id);
            target.parameterPath = "curatedDsp." + assignment.slot->id + "."
                + std::string(assignment.parameter->id);
            target.dspSlotId = assignment.slot->id;
            target.dspParameterId = std::string(assignment.parameter->id);
            target.sourceMinimum = editedMacro.minValue;
            target.sourceMaximum = editedMacro.maxValue;
            target.destinationMinimum = assignment.parameter->minimum;
            target.destinationMaximum = assignment.parameter->maximum;
            target.curve = "linear";
            if (target.role.empty())
                target.role = "mix";
        }
    }

    if (field == MacroEditField::role
        && editableTargetIndex >= 0
        && static_cast<std::size_t>(editableTargetIndex) < editedMacro.targets.size())
    {
        auto selectedRoleText = macroRoleSelector.getText().toStdString();
        if (selectedRoleText.rfind("Custom: ", 0) == 0)
            selectedRoleText = selectedRoleText.substr(8);
        editedMacro.targets[static_cast<std::size_t>(editableTargetIndex)].role = selectedRoleText;
    }

    const auto result = authoringSession.updateMacro(static_cast<std::size_t>(selectedMacroIndex),
                                                     editedMacro,
                                                     label.toStdString());
    if (!result.applied)
    {
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                               "Update Macro Failed",
                                               buildIssueSummary(result.issues));
    }
    refreshFromSession();
}

void AuthoringPanel::createMacro()
{
    drs::engine::RuntimeProjectMacroDefinition macro;
    macro.defaultValue = 0.5;
    macro.minValue = 0.0;
    macro.maxValue = 1.0;

    const auto result = authoringSession.createMacro(macro, "Create macro");
    if (result.applied)
    {
        setActiveWorkbenchTab(authoring::WorkbenchTab::macros);
        refreshFromSession();
        return;
    }

    juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                           "Create Macro Failed",
                                           buildIssueSummary(result.issues));
}

void AuthoringPanel::duplicateSelectedMacro()
{
    const auto selectedMacro = authoringSession.getSelectedMacro();
    if (!selectedMacro.has_value())
        return;

    const auto result = authoringSession.duplicateMacro(selectedMacro->id, "Duplicate macro");
    if (result.applied)
    {
        refreshFromSession();
        return;
    }

    juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                           "Duplicate Macro Failed",
                                           buildIssueSummary(result.issues));
}

void AuthoringPanel::deleteSelectedMacro()
{
    const auto selectedMacro = authoringSession.getSelectedMacro();
    if (!selectedMacro.has_value())
        return;

    const auto result = authoringSession.deleteMacro(selectedMacro->id, "Delete macro");
    if (result.applied)
    {
        refreshFromSession();
        return;
    }

    juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                           "Delete Macro Failed",
                                           buildIssueSummary(result.issues));
}

void AuthoringPanel::moveSelectedMacro(int direction)
{
    if (selectedMacroIndex < 0)
        return;

    const auto result = authoringSession.moveMacro(static_cast<std::size_t>(selectedMacroIndex),
                                                   direction,
                                                   direction < 0 ? "Move macro earlier" : "Move macro later");
    if (result.applied)
        selectedMacroIndex = std::max(0, selectedMacroIndex + direction);
    else
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                               "Move Macro Failed",
                                               buildIssueSummary(result.issues));

    refreshFromSession();
}

void AuthoringPanel::addMacroAssignment()
{
    const auto& macros = authoringSession.getProject().authoring.macros;
    if (selectedMacroIndex < 0
        || static_cast<std::size_t>(selectedMacroIndex) >= macros.size())
        return;

    auto editedMacro = macros[static_cast<std::size_t>(selectedMacroIndex)];
    auto nextTarget = drs::engine::RuntimeProjectMacroTargetDefinition {};
    auto foundTarget = false;

    for (const auto& assignment : curatedMacroAssignments)
    {
        const auto alreadyAssigned = std::any_of(
            editedMacro.targets.begin(), editedMacro.targets.end(),
            [&](const auto& target) { return target.parameterId == assignment.parameterId; });
        if (alreadyAssigned)
            continue;

        nextTarget.parameterId = assignment.parameterId;
        nextTarget.parameterPath = assignment.parameterPath;
        nextTarget.role = assignment.defaultRole;
        nextTarget.sourceMinimum = editedMacro.minValue;
        nextTarget.sourceMaximum = editedMacro.maxValue;
        foundTarget = true;
        break;
    }

    if (!foundTarget)
    {
        const auto dspAssignments = buildCuratedDspMacroAssignments(
            authoringSession.getProject(), selectedDspScopeRoutingBusId(),
            selectedDspScopeInputSource());
        for (const auto& assignment : dspAssignments)
        {
            const auto alreadyAssigned = std::any_of(
                editedMacro.targets.begin(), editedMacro.targets.end(),
                [&](const auto& target)
                {
                    return target.dspSlotId == assignment.slot->id
                        && target.dspParameterId == assignment.parameter->id;
                });
            if (alreadyAssigned)
                continue;

            nextTarget.parameterId = "dsp." + assignment.slot->id + "."
                + std::string(assignment.parameter->id);
            nextTarget.parameterPath = "curatedDsp." + assignment.slot->id + "."
                + std::string(assignment.parameter->id);
            nextTarget.role = "mix";
            nextTarget.dspSlotId = assignment.slot->id;
            nextTarget.dspParameterId = assignment.parameter->id;
            nextTarget.sourceMinimum = editedMacro.minValue;
            nextTarget.sourceMaximum = editedMacro.maxValue;
            nextTarget.destinationMinimum = assignment.parameter->minimum;
            nextTarget.destinationMaximum = assignment.parameter->maximum;
            foundTarget = true;
            break;
        }
    }

    if (!foundTarget)
    {
        juce::AlertWindow::showMessageBoxAsync(
            juce::AlertWindow::InfoIcon, "No Available Macro Target",
            "Every supported target in the current project context is already assigned to this macro.");
        return;
    }

    editedMacro.targets.push_back(std::move(nextTarget));
    const auto result = authoringSession.updateMacro(
        static_cast<std::size_t>(selectedMacroIndex), editedMacro,
        "Add macro assignment");
    if (result.applied)
        selectedMacroTargetIndex = static_cast<int>(editedMacro.targets.size()) - 1;
    else
        juce::AlertWindow::showMessageBoxAsync(
            juce::AlertWindow::WarningIcon, "Add Macro Assignment Failed",
            buildIssueSummary(result.issues));
    refreshFromSession();
}

void AuthoringPanel::removeSelectedMacroAssignment()
{
    const auto& macros = authoringSession.getProject().authoring.macros;
    if (selectedMacroIndex < 0
        || static_cast<std::size_t>(selectedMacroIndex) >= macros.size())
        return;

    auto editedMacro = macros[static_cast<std::size_t>(selectedMacroIndex)];
    if (selectedMacroTargetIndex < 0
        || static_cast<std::size_t>(selectedMacroTargetIndex) >= editedMacro.targets.size())
        return;

    editedMacro.targets.erase(editedMacro.targets.begin() + selectedMacroTargetIndex);
    const auto result = authoringSession.updateMacro(
        static_cast<std::size_t>(selectedMacroIndex), editedMacro,
        "Remove macro assignment");
    if (result.applied)
    {
        selectedMacroTargetIndex = editedMacro.targets.empty()
            ? -1 : std::min(selectedMacroTargetIndex,
                            static_cast<int>(editedMacro.targets.size()) - 1);
    }
    else
    {
        juce::AlertWindow::showMessageBoxAsync(
            juce::AlertWindow::WarningIcon, "Remove Macro Assignment Failed",
            buildIssueSummary(result.issues));
    }
    refreshFromSession();
}

void AuthoringPanel::applySelectedFxSlotEdit(const juce::String& label)
{
    const auto& fxSlots = authoringSession.getProject().authoring.fxSlots;
    if (selectedFxSlotIndex < 0 || static_cast<std::size_t>(selectedFxSlotIndex) >= fxSlots.size())
        return;

    auto editedFxSlot = fxSlots[static_cast<std::size_t>(selectedFxSlotIndex)];
    auto effectType = fxTypeSelector.getText().toStdString();
    if (effectType.rfind("Custom: ", 0) == 0)
        effectType = effectType.substr(8);
    editedFxSlot.effectType = effectType;
    editedFxSlot.displayName = fxNameEditor.getText().trim().toStdString();
    if (const auto* descriptor = drs::engine::findCuratedDspEffect(editedFxSlot.effectType, 1); descriptor != nullptr)
    {
        editedFxSlot.effectVersion = 1;
        editedFxSlot.unavailable = false;
        editedFxSlot.legacyInert = false;
        std::vector<drs::engine::RuntimeProjectFxSlotDefinition::ParameterValue> normalizedParameters;
        normalizedParameters.reserve(descriptor->parameters.size());
        for (const auto& parameter : descriptor->parameters)
        {
            const auto authored = std::find_if(editedFxSlot.parameters.begin(),
                                               editedFxSlot.parameters.end(),
                                               [&](const auto& value) { return value.id == parameter.id; });
            const auto authoredValueIsUsable = authored != editedFxSlot.parameters.end()
                && std::isfinite(authored->value)
                && authored->value >= parameter.minimum
                && authored->value <= parameter.maximum;
            normalizedParameters.push_back({
                std::string(parameter.id),
                authoredValueIsUsable ? authored->value : parameter.defaultValue
            });
        }
        editedFxSlot.parameters = std::move(normalizedParameters);
    }
    editedFxSlot.bypassed = fxBypassedToggle.getToggleState();

    authoringSession.updateFxSlot(static_cast<std::size_t>(selectedFxSlotIndex),
                                  editedFxSlot,
                                  label.toStdString());
    refreshFromSession();
}

std::string AuthoringPanel::ensureSelectedDspScopeRoutingBus()
{
    if (const auto existing = selectedDspScopeRoutingBusId(); !existing.empty())
        return existing;
    auto input = selectedDspScopeInputSource();
    auto id = "dsp-chain-" + input;
    std::replace(id.begin(), id.end(), '/', '-');
    std::size_t suffix = 2;
    const auto baseId = id;
    const auto idInUse = [&](const std::string& candidate)
    {
        return std::any_of(authoringSession.getProject().authoring.routingBuses.begin(),
                           authoringSession.getProject().authoring.routingBuses.end(),
                           [&](const auto& bus) { return bus.id == candidate; });
    };
    while (idInUse(id)) id = baseId + "-" + std::to_string(suffix++);
    drs::engine::RuntimeProjectRoutingBusDefinition bus;
    bus.id = id;
    bus.displayName = selectedDspScopeIndex == 0 ? "Zone Inserts"
        : selectedDspScopeIndex == 1 ? "Group Inserts" : "Instrument Inserts";
    bus.inputSourceId = input;
    return authoringSession.createRoutingBus(bus, "Create scoped insert chain").applied ? id : std::string {};
}

void AuthoringPanel::createScopedFxSlot()
{
    const auto owner = ensureSelectedDspScopeRoutingBus();
    if (owner.empty()) { refreshFromSession(); return; }
    auto id = std::string("fx-gain");
    std::size_t suffix = 2;
    const auto exists = [&](const std::string& candidate)
    {
        return std::any_of(authoringSession.getProject().authoring.fxSlots.begin(),
                           authoringSession.getProject().authoring.fxSlots.end(),
                           [&](const auto& slot) { return slot.id == candidate; });
    };
    while (exists(id)) id = "fx-gain-" + std::to_string(suffix++);
    drs::engine::RuntimeProjectFxSlotDefinition slot;
    slot.id = id;
    slot.displayName = "Gain";
    slot.effectType = "drs.gain";
    slot.effectVersion = 1;
    if (const auto* descriptor = drs::engine::findCuratedDspEffect(slot.effectType, slot.effectVersion))
        for (const auto& parameter : descriptor->parameters)
            slot.parameters.push_back({ std::string(parameter.id), parameter.defaultValue });
    const auto result = authoringSession.createFxSlot(slot, owner, "Add curated insert");
    if (result.applied)
    {
        selectedFxSlotIndex = static_cast<int>(authoringSession.getProject().authoring.fxSlots.size()) - 1;
        if (const auto* ownerBus = findRoutingBusById(authoringSession.getProject(), owner))
        {
            const auto& routingBuses = authoringSession.getProject().authoring.routingBuses;
            selectedRoutingBusIndex = static_cast<int>(std::distance(routingBuses.begin(),
                                                                     std::find_if(routingBuses.begin(),
                                                                                  routingBuses.end(),
                                                                                  [&](const auto& routingBus)
                                                                                  {
                                                                                      return routingBus.id == ownerBus->id;
                                                                                  })));
        }
    }
    refreshFromSession();
}

void AuthoringPanel::duplicateSelectedFxSlot()
{
    if (selectedFxSlotIndex < 0 || static_cast<std::size_t>(selectedFxSlotIndex) >= authoringSession.getProject().authoring.fxSlots.size())
        return;
    const auto& slot = authoringSession.getProject().authoring.fxSlots[static_cast<std::size_t>(selectedFxSlotIndex)];
    auto duplicateId = slot.id + "-copy";
    std::size_t suffix = 2;
    const auto exists = [&](const std::string& candidate)
    {
        return std::any_of(authoringSession.getProject().authoring.fxSlots.begin(),
                           authoringSession.getProject().authoring.fxSlots.end(),
                           [&](const auto& candidateSlot) { return candidateSlot.id == candidate; });
    };
    while (exists(duplicateId)) duplicateId = slot.id + "-copy-" + std::to_string(suffix++);
    authoringSession.duplicateFxSlot(slot.id, duplicateId, "Duplicate FX slot");
    refreshFromSession();
}

void AuthoringPanel::deleteSelectedFxSlot()
{
    if (selectedFxSlotIndex >= 0 && static_cast<std::size_t>(selectedFxSlotIndex) < authoringSession.getProject().authoring.fxSlots.size())
        authoringSession.deleteFxSlot(authoringSession.getProject().authoring.fxSlots[static_cast<std::size_t>(selectedFxSlotIndex)].id,
                                      "Delete FX slot");
    refreshFromSession();
}

void AuthoringPanel::moveSelectedFxSlot(const int direction)
{
    if (selectedFxSlotIndex >= 0 && static_cast<std::size_t>(selectedFxSlotIndex) < authoringSession.getProject().authoring.fxSlots.size())
        authoringSession.moveFxSlot(authoringSession.getProject().authoring.fxSlots[static_cast<std::size_t>(selectedFxSlotIndex)].id,
                                    direction, direction < 0 ? "Move FX earlier" : "Move FX later");
    refreshFromSession();
}

void AuthoringPanel::moveSelectedFxSlotToSelectedOwner()
{
    const auto ownerIndex = fxOwnerSelector.getSelectedId() - 1;
    if (ownerIndex >= 0 && static_cast<std::size_t>(ownerIndex) < fxOwnerBusIds.size()
        && selectedFxSlotIndex >= 0 && static_cast<std::size_t>(selectedFxSlotIndex) < authoringSession.getProject().authoring.fxSlots.size())
    {
        authoringSession.moveFxSlotToBus(authoringSession.getProject().authoring.fxSlots[static_cast<std::size_t>(selectedFxSlotIndex)].id,
                                         fxOwnerBusIds[static_cast<std::size_t>(ownerIndex)], "Move FX to owner");
    }
    refreshFromSession();
}

void AuthoringPanel::applySelectedFxParameterEdit(const juce::String& label)
{
    if (selectedFxSlotIndex >= 0 && static_cast<std::size_t>(selectedFxSlotIndex) < authoringSession.getProject().authoring.fxSlots.size()
        && selectedFxParameterIndex >= 0 && static_cast<std::size_t>(selectedFxParameterIndex) < fxParameterIds.size())
    {
        authoringSession.setFxSlotParameter(authoringSession.getProject().authoring.fxSlots[static_cast<std::size_t>(selectedFxSlotIndex)].id,
                                            fxParameterIds[static_cast<std::size_t>(selectedFxParameterIndex)],
                                            fxParameterSlider.getValue(), label.toStdString());
    }
    refreshFromSession();
}

void AuthoringPanel::assignSelectedFxParameterToMacro()
{
    const auto& project = authoringSession.getProject();
    if (selectedFxSlotIndex < 0
        || static_cast<std::size_t>(selectedFxSlotIndex) >= project.authoring.fxSlots.size()
        || selectedFxParameterIndex < 0
        || static_cast<std::size_t>(selectedFxParameterIndex) >= fxParameterIds.size())
    {
        return;
    }

    const auto& fxSlot = project.authoring.fxSlots[static_cast<std::size_t>(selectedFxSlotIndex)];
    const auto parameterId = fxParameterIds[static_cast<std::size_t>(selectedFxParameterIndex)];
    if (const auto existingMacroIndex = findMacroIndexForDspTarget(project, fxSlot.id, parameterId);
        existingMacroIndex.has_value())
    {
        authoringSession.selectMacro(project.authoring.macros[*existingMacroIndex].id);
        setActiveWorkbenchTab(authoring::WorkbenchTab::macros);
        refreshFromSession();
        return;
    }

    const auto* descriptor = drs::engine::findCuratedDspEffect(fxSlot.effectType, fxSlot.effectVersion);
    if (descriptor == nullptr)
    {
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                               "Create Control Failed",
                                               "The selected insert does not have a curated DSP descriptor.");
        return;
    }

    const auto parameterIterator = std::find_if(descriptor->parameters.begin(),
                                                descriptor->parameters.end(),
                                                [&](const auto& parameter)
                                                {
                                                    return parameter.id == parameterId;
                                                });
    if (parameterIterator == descriptor->parameters.end())
    {
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                               "Create Control Failed",
                                               "The selected parameter could not be resolved from the DSP catalog.");
        return;
    }

    juce::String scopeName = "Instrument";
    if (selectedDspScopeIndex == 0)
    {
        if (const auto selectedZone = authoringSession.getSelectedZone(); selectedZone.has_value())
            scopeName = juce::String::fromUTF8(selectedZone->displayName.c_str());
    }
    else if (selectedDspScopeIndex == 1)
    {
        if (const auto selectedGroup = authoringSession.getSelectedGroup(); selectedGroup.has_value())
            scopeName = juce::String::fromUTF8(selectedGroup->displayName.c_str());
    }

    auto suggestedName = scopeName.trim();
    if (!(fxSlot.effectType == "drs.gain" && parameterIterator->id == "gainDb"))
    {
        const auto parameterName = formatDspParameterName(parameterIterator->id);
        suggestedName = (suggestedName + " " + parameterName).trim();
    }
    if (suggestedName.isEmpty())
        suggestedName = "Control";

    const auto* authoredParameter = findAuthoredFxParameterValue(fxSlot, parameterId);
    const auto parameterValue = authoredParameter == nullptr ? parameterIterator->defaultValue : authoredParameter->value;
    const auto normalizedDefault = parameterIterator->maximum > parameterIterator->minimum
        ? juce::jlimit(0.0,
                       1.0,
                       (parameterValue - parameterIterator->minimum)
                           / (parameterIterator->maximum - parameterIterator->minimum))
        : 0.0;

    drs::engine::RuntimeProjectMacroDefinition macro;
    macro.name = suggestedName.toStdString();
    macro.exposedInPerformance = true;
    macro.minValue = 0.0;
    macro.maxValue = 1.0;
    macro.defaultValue = normalizedDefault;

    drs::engine::RuntimeProjectMacroTargetDefinition target;
    target.parameterId = "dsp." + fxSlot.id + "." + parameterId;
    target.parameterPath = "curatedDsp." + fxSlot.id + "." + parameterId;
    target.role = "mix";
    target.dspSlotId = fxSlot.id;
    target.dspParameterId = parameterId;
    target.sourceMinimum = macro.minValue;
    target.sourceMaximum = macro.maxValue;
    target.destinationMinimum = parameterIterator->minimum;
    target.destinationMaximum = parameterIterator->maximum;
    target.curve = "linear";
    const auto resolution = drs::engine::resolveCuratedDspControlLaw({ &*parameterIterator, target.role, {} });
    if (resolution.resolved)
    {
        target.controlLaw.id = std::string(resolution.controlLawId);
        target.controlLaw.version = 1;
        target.destinationMinimum = resolution.minimum;
        target.destinationMaximum = resolution.maximum;
        drs::engine::CompiledControlLaw compiledLaw;
        double resolvedDefault = macro.defaultValue;
        if (drs::engine::compileControlLaw(target.controlLaw.id, target.destinationMinimum,
                                           target.destinationMaximum, compiledLaw)
            && drs::engine::physicalToNormalized(compiledLaw, parameterValue, resolvedDefault))
            macro.defaultValue = resolvedDefault;
    }
    macro.targets.push_back(std::move(target));

    const auto result = authoringSession.createMacro(macro, "Create control from FX parameter");
    if (!result.applied)
    {
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                               "Create Control Failed",
                                               buildIssueSummary(result.issues));
        return;
    }

    setActiveWorkbenchTab(authoring::WorkbenchTab::macros);
    refreshFromSession();
}

void AuthoringPanel::resetSelectedFxParameter()
{
    if (selectedFxSlotIndex >= 0 && static_cast<std::size_t>(selectedFxSlotIndex) < authoringSession.getProject().authoring.fxSlots.size()
        && selectedFxParameterIndex >= 0 && static_cast<std::size_t>(selectedFxParameterIndex) < fxParameterIds.size())
    {
        authoringSession.resetFxSlotParameterToDefault(
            authoringSession.getProject().authoring.fxSlots[static_cast<std::size_t>(selectedFxSlotIndex)].id,
            fxParameterIds[static_cast<std::size_t>(selectedFxParameterIndex)], "Reset FX parameter");
    }
    refreshFromSession();
}

void AuthoringPanel::applySelectedRoutingBusEdit(const juce::String& label)
{
    const auto& routingBuses = authoringSession.getProject().authoring.routingBuses;
    if (selectedRoutingBusIndex < 0
        || static_cast<std::size_t>(selectedRoutingBusIndex) >= routingBuses.size())
    {
        return;
    }

    auto editedRoutingBus = routingBuses[static_cast<std::size_t>(selectedRoutingBusIndex)];
    const auto selectedInputIndex = std::max(0, routingInputSelector.getSelectedId() - 1);
    editedRoutingBus.inputSourceId = selectedInputIndex >= 0
            && static_cast<std::size_t>(selectedInputIndex) < routingInputSourceIds.size()
        ? routingInputSourceIds[static_cast<std::size_t>(selectedInputIndex)]
        : std::string("master");
    editedRoutingBus.fxSlotIds.clear();

    auto appendFxId = [&](const juce::ComboBox& selector)
    {
        const auto fxId = selector.getText().toStdString();
        if (fxId.empty() || fxId == "(none)")
            return;

        if (std::find(editedRoutingBus.fxSlotIds.begin(), editedRoutingBus.fxSlotIds.end(), fxId)
            == editedRoutingBus.fxSlotIds.end())
        {
            editedRoutingBus.fxSlotIds.push_back(fxId);
        }
    };

    appendFxId(routingInsertOneSelector);
    appendFxId(routingInsertTwoSelector);

    authoringSession.updateRoutingBus(static_cast<std::size_t>(selectedRoutingBusIndex),
                                      editedRoutingBus,
                                      label.toStdString());
    refreshFromSession();
}

void AuthoringPanel::applySelectedTriggerSlotEdit(const juce::String& label)
{
    const auto selectedPerformanceBank = authoringSession.getSelectedPerformanceBank();
    if (!selectedPerformanceBank.has_value()
        || selectedTriggerSlotIndex < 0
        || static_cast<std::size_t>(selectedTriggerSlotIndex) >= selectedPerformanceBank->triggerSlots.size())
    {
        return;
    }

    auto editedPerformanceBank = *selectedPerformanceBank;
    auto& triggerSlot = editedPerformanceBank.triggerSlots[static_cast<std::size_t>(selectedTriggerSlotIndex)];
    triggerSlot.triggerEvent = triggerEventSelector.getText().toStdString();
    triggerSlot.targetArticulationId = targetArticulationSelector.getText().toStdString();
    triggerSlot.phraseAssetId = phraseAssetSelector.getSelectedId() > 1 ? phraseAssetSelector.getText().toStdString() : std::string{};
    triggerSlot.chordMode = chordModeSelector.getText().toStdString();

    authoringSession.updatePerformanceBank(static_cast<std::size_t>(selectedPerformanceBankIndex),
                                           editedPerformanceBank,
                                           label.toStdString());
    refreshFromSession();
}

void AuthoringPanel::addRoundRobinResetRule()
{
    auto rules = authoringSession.getProject().authoring.roundRobinResetRules;
    if (rules.size() >= 128)
        return;
    drs::engine::RuntimeProjectRoundRobinResetRuleDefinition rule;
    const auto eventId = roundRobinResetEventSelector.getSelectedId();
    rule.event = eventId == 1 ? drs::engine::RoundRobinResetEvent::programActivation
        : eventId == 3 ? drs::engine::RoundRobinResetEvent::allNotesOff
        : eventId == 4 ? drs::engine::RoundRobinResetEvent::pedalDown
        : eventId == 5 ? drs::engine::RoundRobinResetEvent::pedalUp
                       : drs::engine::RoundRobinResetEvent::articulationChange;
    const auto targetId = roundRobinResetTargetSelector.getSelectedId();
    rule.targetAll = targetId <= 1;
    if (!rule.targetAll)
        rule.targetPoolId = roundRobinResetTargetSelector.getText().toStdString();
    rules.push_back(std::move(rule));
    const auto result = authoringSession.updateRoundRobinResetRules(std::move(rules), "Add Round Robin reset rule");
    if (result.applied)
        selectedRoundRobinResetIndex = static_cast<int>(authoringSession.getProject().authoring.roundRobinResetRules.size());
    refreshFromSession();
}

void AuthoringPanel::deleteRoundRobinResetRule()
{
    auto rules = authoringSession.getProject().authoring.roundRobinResetRules;
    if (selectedRoundRobinResetIndex < 0
        || static_cast<std::size_t>(selectedRoundRobinResetIndex) >= rules.size())
        return;
    rules.erase(rules.begin() + selectedRoundRobinResetIndex);
    const auto result = authoringSession.updateRoundRobinResetRules(std::move(rules), "Remove Round Robin reset rule");
    if (result.applied)
        selectedRoundRobinResetIndex = std::max(0, selectedRoundRobinResetIndex - 1);
    refreshFromSession();
}

void AuthoringPanel::updateSelectedRoundRobinResetRule()
{
    auto rules = authoringSession.getProject().authoring.roundRobinResetRules;
    if (selectedRoundRobinResetIndex < 0
        || static_cast<std::size_t>(selectedRoundRobinResetIndex) >= rules.size())
        return;
    auto& rule = rules[static_cast<std::size_t>(selectedRoundRobinResetIndex)];
    const auto eventId = roundRobinResetEventSelector.getSelectedId();
    rule.event = eventId == 1 ? drs::engine::RoundRobinResetEvent::programActivation
        : eventId == 3 ? drs::engine::RoundRobinResetEvent::allNotesOff
        : eventId == 4 ? drs::engine::RoundRobinResetEvent::pedalDown
        : eventId == 5 ? drs::engine::RoundRobinResetEvent::pedalUp
                       : drs::engine::RoundRobinResetEvent::articulationChange;
    const auto targetId = roundRobinResetTargetSelector.getSelectedId();
    rule.targetAll = targetId <= 1;
    rule.targetPoolId = rule.targetAll ? std::string {} : roundRobinResetTargetSelector.getText().toStdString();
    authoringSession.updateRoundRobinResetRules(std::move(rules), "Update Round Robin reset rule");
    refreshFromSession();
}

void AuthoringPanel::createArticulation()
{
    const auto articulations = authoringSession.getArticulations();
    auto baseId = std::string("articulation");
    auto suffix = static_cast<int>(articulations.size()) + 1;
    auto id = baseId + "-" + std::to_string(suffix);
    const auto hasId = [&](const std::string& candidate)
    {
        return std::any_of(articulations.begin(), articulations.end(),
                           [&](const auto& articulation) { return articulation.id == candidate; });
    };
    while (hasId(id))
        id = baseId + "-" + std::to_string(++suffix);

    drs::engine::RuntimeProjectArticulationDefinition articulation;
    articulation.id = id;
    articulation.displayName = "Articulation " + std::to_string(suffix);
    const auto result = authoringSession.createArticulation(articulation, "Create articulation");
    if (result.applied)
        selectedArticulationIndex = static_cast<int>(articulations.size());
    refreshFromSession();
}

void AuthoringPanel::duplicateSelectedArticulation()
{
    const auto articulations = authoringSession.getArticulations();
    if (selectedArticulationIndex < 0 || static_cast<std::size_t>(selectedArticulationIndex) >= articulations.size())
        return;
    auto duplicate = articulations[static_cast<std::size_t>(selectedArticulationIndex)];
    const auto baseId = duplicate.id + "-copy";
    auto id = baseId;
    int suffix = 2;
    const auto hasId = [&](const std::string& candidate)
    {
        return std::any_of(articulations.begin(), articulations.end(),
                           [&](const auto& articulation) { return articulation.id == candidate; });
    };
    while (hasId(id))
        id = baseId + "-" + std::to_string(suffix++);
    duplicate.id = id;
    duplicate.displayName += " Copy";
    duplicate.isDefault = false;
    // A duplicate must not silently create a collision with its source key switch.
    duplicate.activation.reset();
    const auto result = authoringSession.createArticulation(duplicate, "Duplicate articulation");
    if (result.applied)
        selectedArticulationIndex = static_cast<int>(articulations.size());
    refreshFromSession();
}

void AuthoringPanel::applySelectedArticulationEdit(const juce::String& label)
{
    const auto articulations = authoringSession.getArticulations();
    if (selectedArticulationIndex < 0 || static_cast<std::size_t>(selectedArticulationIndex) >= articulations.size())
        return;
    auto articulation = articulations[static_cast<std::size_t>(selectedArticulationIndex)];
    const auto name = articulationNameEditor.getText().trim();
    if (name.isEmpty())
    {
        refreshFromSession();
        return;
    }
    articulation.displayName = name.toStdString();
    if (label != "Rename articulation")
    {
        drs::engine::RuntimeProjectArticulationActivationDefinition activation;
        activation.midiNote = static_cast<int>(articulationSwitchNoteSlider.getValue());
        articulation.activation = activation;
    }
    const auto result = authoringSession.updateArticulation(static_cast<std::size_t>(selectedArticulationIndex),
                                                            articulation, label.toStdString());
    if (!result.applied)
    {
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                                "Key Switch Unavailable", buildIssueSummary(result.issues));
    }
    refreshFromSession();
}

void AuthoringPanel::moveSelectedArticulation(const int direction)
{
    const auto result = authoringSession.moveArticulation(static_cast<std::size_t>(selectedArticulationIndex), direction,
                                                           direction < 0 ? "Move articulation up" : "Move articulation down");
    if (result.applied)
        selectedArticulationIndex += direction;
    refreshFromSession();
}

void AuthoringPanel::deleteSelectedArticulation()
{
    const auto articulations = authoringSession.getArticulations();
    if (selectedArticulationIndex < 0 || static_cast<std::size_t>(selectedArticulationIndex) >= articulations.size())
        return;
    const auto replacementIndex = articulationDeleteReassignSelector.getSelectedId() - 1;
    if (replacementIndex < 0 || static_cast<std::size_t>(replacementIndex) >= articulations.size()
        || replacementIndex == selectedArticulationIndex)
        return;
    const auto result = authoringSession.deleteArticulation(
        articulations[static_cast<std::size_t>(selectedArticulationIndex)].id,
        articulations[static_cast<std::size_t>(replacementIndex)].id,
        "Delete articulation and reassign zones");
    if (result.applied)
        selectedArticulationIndex = std::min(selectedArticulationIndex,
                                              static_cast<int>(articulations.size()) - 2);
    refreshFromSession();
}

void AuthoringPanel::setSelectedArticulationDefault()
{
    const auto articulations = authoringSession.getArticulations();
    if (selectedArticulationIndex < 0 || static_cast<std::size_t>(selectedArticulationIndex) >= articulations.size())
        return;
    authoringSession.setDefaultArticulation(articulations[static_cast<std::size_t>(selectedArticulationIndex)].id,
                                            "Set default articulation");
    refreshFromSession();
}

void AuthoringPanel::clearSelectedArticulationKeySwitch()
{
    const auto articulations = authoringSession.getArticulations();
    if (selectedArticulationIndex < 0 || static_cast<std::size_t>(selectedArticulationIndex) >= articulations.size())
        return;
    auto articulation = articulations[static_cast<std::size_t>(selectedArticulationIndex)];
    articulation.activation.reset();
    authoringSession.updateArticulation(static_cast<std::size_t>(selectedArticulationIndex), articulation,
                                        "Clear key switch");
    refreshFromSession();
}

void AuthoringPanel::toggleKeySwitchMidiLearn()
{
    keySwitchMidiLearnActive = !keySwitchMidiLearnActive;
    keySwitchMidiLearnDeadlineMillis = keySwitchMidiLearnActive
        ? juce::Time::getMillisecondCounterHiRes() + 10000.0 : 0.0;
    if (keySwitchMidiLearnActive)
        startTimer(keySwitchMidiLearnTimerId, 100);
    else
        stopTimer(keySwitchMidiLearnTimerId);
    refreshFromSession();
}

bool AuthoringPanel::applyLearnedKeySwitchMidiNote(const int midiNote)
{
    if (!keySwitchMidiLearnActive || midiNote < 0 || midiNote > 127)
        return false;
    keySwitchMidiLearnActive = false;
    keySwitchMidiLearnDeadlineMillis = 0.0;
    stopTimer(keySwitchMidiLearnTimerId);
    articulationSwitchNoteSlider.setValue(midiNote, juce::dontSendNotification);
    applySelectedArticulationEdit("Learn key switch");
    return true;
}

void AuthoringPanel::importPhraseForSelectedBank()
{
    const auto selectedPerformanceBank = authoringSession.getSelectedPerformanceBank();
    if (!selectedPerformanceBank.has_value())
        return;

    const auto midiPath = phraseImportPathEditor.getText().trim().toStdString();
    if (midiPath.empty())
    {
        phraseSummaryLabel.setText("Choose a MIDI file path before importing a phrase.",
                                   juce::dontSendNotification);
        updateDynamicAccessibleText(phraseSummaryLabel,
                                    phraseSummaryLabel.getText(),
                                    "Phrase summary: ");
        return;
    }

    const juce::File midiFile(midiPath);
    const auto phraseId = midiFile.getFileNameWithoutExtension().replaceCharacters(" ", "-").toLowerCase().toStdString();
    const auto importResult = importMidiPhraseAsset(midiPath,
                                                    phraseId,
                                                    midiFile.getFileNameWithoutExtension().toStdString());
    if (!importResult.imported)
    {
        phraseSummaryLabel.setText("Import failed: "
                                       + juce::String::fromUTF8(importResult.issues.empty()
                                                                    ? importResult.state.c_str()
                                                                    : importResult.issues.front().c_str()),
                                   juce::dontSendNotification);
        updateDynamicAccessibleText(phraseSummaryLabel,
                                    phraseSummaryLabel.getText(),
                                    "Phrase summary: ");
        return;
    }

    auto editedPerformanceBank = *selectedPerformanceBank;
    const auto existingPhraseIterator = std::find_if(
        editedPerformanceBank.phraseAssets.begin(),
        editedPerformanceBank.phraseAssets.end(),
        [&](const auto& phraseAsset)
        {
            return phraseAsset.id == importResult.phraseAsset.id;
        });

    if (existingPhraseIterator == editedPerformanceBank.phraseAssets.end())
        editedPerformanceBank.phraseAssets.push_back(importResult.phraseAsset);
    else
        *existingPhraseIterator = importResult.phraseAsset;

    if (selectedTriggerSlotIndex >= 0
        && static_cast<std::size_t>(selectedTriggerSlotIndex) < editedPerformanceBank.triggerSlots.size())
    {
        auto& triggerSlot = editedPerformanceBank.triggerSlots[static_cast<std::size_t>(selectedTriggerSlotIndex)];
        if (triggerSlot.triggerEvent == "phrase-trigger")
        {
            triggerSlot.phraseAssetId = importResult.phraseAsset.id;
            if (triggerSlot.chordMode.empty())
                triggerSlot.chordMode = "follow-root";
        }
    }

    authoringSession.updatePerformanceBank(static_cast<std::size_t>(selectedPerformanceBankIndex),
                                           editedPerformanceBank,
                                           "Import performance phrase");
    phraseSummaryLabel.setText(
        juce::String::fromUTF8(importResult.phraseAsset.displayName.c_str())
            + " imported | notes=" + juce::String(static_cast<int>(importResult.phraseAsset.notes.size()))
            + " | chord=" + juce::String::fromUTF8(importResult.phraseAsset.chordHint.c_str()),
        juce::dontSendNotification);
    updateDynamicAccessibleText(phraseSummaryLabel,
                                phraseSummaryLabel.getText(),
                                "Phrase summary: ");
    refreshFromSession();
}
} // namespace drs::app
