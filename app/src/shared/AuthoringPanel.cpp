#include "shared/AuthoringPanel.h"

#include "shared/authoring/AuthoringWorkspaceLayout.h"
#include "drs/engine/ControlLaw.h"
#include "drs/engine/CuratedDspCatalog.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <tuple>
#include <utility>

namespace drs::app
{
namespace
{
constexpr int statusTimerId = 1;
constexpr int previewReleaseTimerId = 2;
const auto authoringPanelBackground = juce::Colour::fromRGB(18, 24, 29);
const auto authoringPanelCard = juce::Colour::fromRGB(250, 247, 240);
const auto authoringPanelAccent = juce::Colour::fromRGB(181, 96, 21);
const auto authoringPanelMuted = juce::Colour::fromRGB(82, 86, 94);
const auto authoringControlSurface = juce::Colour::fromRGB(251, 248, 242);
const auto authoringControlSurfaceHover = juce::Colour::fromRGB(244, 239, 231);
const auto authoringControlOutline = juce::Colour::fromRGB(176, 160, 141);
const auto authoringFocusRing = juce::Colour::fromRGB(24, 29, 33);
const auto authoringFocusHalo = juce::Colour::fromRGBA(255, 255, 255, 232);
const auto authoringButtonFill = juce::Colour::fromRGB(122, 64, 18);
const auto authoringButtonFillPressed = juce::Colour::fromRGB(102, 52, 14);
const auto authoringButtonText = juce::Colours::white;
const auto authoringToggleTick = juce::Colour::fromRGB(28, 108, 88);

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
    label.setColour(juce::Label::textColourId, juce::Colour::fromRGB(24, 29, 33));
    label.setFont(juce::FontOptions(16.0f, juce::Font::bold));
}

void configureFieldLabel(juce::Label& label, const char* text)
{
    label.setText(text, juce::dontSendNotification);
    label.setColour(juce::Label::textColourId, juce::Colour::fromRGB(24, 29, 33));
    label.setFont(juce::FontOptions(14.0f, juce::Font::bold));
}

void configureMetadataLabel(juce::Label& label)
{
    label.setColour(juce::Label::textColourId, authoringPanelMuted);
    label.setFont(juce::FontOptions(13.0f));
    label.setJustificationType(juce::Justification::centredLeft);
}

void drawAuthoringFocusRing(juce::Graphics& g,
                            juce::Rectangle<float> bounds,
                            float cornerSize,
                            const juce::Colour& outlineColour)
{
    g.setColour(authoringFocusHalo);
    g.drawRoundedRectangle(bounds.expanded(1.0f), cornerSize + 1.0f, 3.0f);
    g.setColour(outlineColour);
    g.drawRoundedRectangle(bounds, cornerSize, 1.8f);
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

    return "Zone: " + findZoneDisplayName(project, inputSourceId);
}

juce::String buildGroupListStatusText(const drs::engine::RuntimeProjectModel& project,
                                      const drs::engine::RuntimeProjectGroupDefinition& group)
{
    juce::String status = group.workspaceVisible ? "Shown" : "Hidden";
    status << " | " << static_cast<int>(countZonesInGroup(project, group.id)) << " zones";

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
        guidance.statusText = "playback busy: Wait for the current playback build to finish applying.";
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
        refreshWaveformDrawerContent();
}

AuthoringPanel::AuthoringControlLookAndFeel::AuthoringControlLookAndFeel()
{
    setColour(juce::TextButton::buttonColourId, authoringButtonFill);
    setColour(juce::TextButton::buttonOnColourId, authoringButtonFillPressed);
    setColour(juce::TextButton::textColourOffId, authoringButtonText);
    setColour(juce::TextButton::textColourOnId, authoringButtonText);

    setColour(juce::ToggleButton::textColourId, authoringFocusRing);
    setColour(juce::ToggleButton::tickColourId, authoringToggleTick);
    setColour(juce::ToggleButton::tickDisabledColourId, authoringControlOutline);

    setColour(juce::ComboBox::backgroundColourId, authoringControlSurface);
    setColour(juce::ComboBox::textColourId, authoringFocusRing);
    setColour(juce::ComboBox::arrowColourId, authoringFocusRing.withAlpha(0.82f));
    setColour(juce::ComboBox::outlineColourId, authoringControlOutline);
    setColour(juce::ComboBox::focusedOutlineColourId, authoringFocusRing);

    setColour(juce::TextEditor::backgroundColourId, authoringControlSurface);
    setColour(juce::TextEditor::textColourId, authoringFocusRing);
    setColour(juce::TextEditor::outlineColourId, authoringControlOutline);
    setColour(juce::TextEditor::focusedOutlineColourId, authoringFocusRing);
    setColour(juce::TextEditor::highlightColourId, authoringToggleTick.withAlpha(0.18f));
    setColour(juce::TextEditor::highlightedTextColourId, authoringFocusRing);

    setColour(juce::Label::textColourId, authoringFocusRing);
    setColour(juce::Slider::thumbColourId, authoringButtonFill);
    setColour(juce::Slider::trackColourId, authoringToggleTick.withAlpha(0.76f));
    setColour(juce::Slider::backgroundColourId, authoringControlSurfaceHover);
    setColour(juce::Slider::textBoxTextColourId, authoringFocusRing);
    setColour(juce::Slider::textBoxBackgroundColourId, authoringControlSurface);
    setColour(juce::Slider::textBoxOutlineColourId, authoringControlOutline);

    setColour(juce::ListBox::backgroundColourId, authoringControlSurfaceHover);
    setColour(juce::ListBox::outlineColourId, authoringControlOutline);
}

void AuthoringPanel::AuthoringControlLookAndFeel::drawButtonBackground(juce::Graphics& g,
                                                                       juce::Button& button,
                                                                       const juce::Colour& backgroundColour,
                                                                       bool shouldDrawButtonAsHighlighted,
                                                                       bool shouldDrawButtonAsDown)
{
    auto bounds = button.getLocalBounds().toFloat().reduced(0.5f);
    const auto cornerSize = 7.0f;
    const auto hasFocus = button.hasKeyboardFocus(true);

    if (hasFocus)
    {
        drawAuthoringFocusRing(g, bounds.reduced(1.0f), cornerSize, findColour(juce::TextEditor::focusedOutlineColourId));
        bounds = bounds.reduced(3.0f);
    }

    auto fillColour = backgroundColour.withMultipliedAlpha(button.isEnabled() ? 1.0f : 0.42f);
    if (shouldDrawButtonAsDown)
        fillColour = fillColour.interpolatedWith(authoringButtonFillPressed, 0.45f);
    else if (shouldDrawButtonAsHighlighted)
        fillColour = fillColour.interpolatedWith(authoringControlSurface, 0.12f);

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
                               6.0f,
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
    const auto cornerSize = 4.0f;
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
                               6.0f,
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
                               DraftPlaybackActionCallback nextCancelSourceValidation)
    : authoringSession(session),
      waveformPreviewProvider(std::move(previewProvider)),
      waveformPreviewRequestCallback(std::move(nextWaveformPreviewRequestCallback)),
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
      groupList("authoringGroupList",
                "authoringGroupListBox",
                "authoringGroupListEmptyState"),
      macroList("authoringMacroList",
                "authoringMacroListBox",
                "authoringMacroListEmptyState")
{
    setLookAndFeel(&authoringLookAndFeel);
    setComponentID("authoringWorkspace");
    drawerState.open = isExpandedLayout(layoutMode);
    drawerState.activeTab = authoring::DrawerTab::waveform;

    playbackBanner.setComponentID("authoringPlaybackBanner");
    playbackBannerLabel.setComponentID("authoringPlaybackBannerLabel");
    playbackBannerPrepareButton.setComponentID("authoringPlaybackBannerPrepareButton");
    playbackBannerPublishButton.setComponentID("authoringPlaybackBannerPublishButton");
    configureMetadataLabel(waveformScopeLabel);
    configureMetadataLabel(drawerBreadcrumbLabel);
    configureMetadataLabel(waveformStatusLabel);
    configureMetadataLabel(waveformInfoLabel);
    configureMetadataLabel(loopInfoLabel);
    configureMetadataLabel(importMetricsLabel);
    configureMetadataLabel(sourceValidationLabel);
    playbackBannerLabel.setColour(juce::Label::textColourId, juce::Colour::fromRGB(24, 29, 33));
    playbackBannerLabel.setFont(juce::FontOptions(13.0f, juce::Font::bold));
    playbackBannerLabel.setJustificationType(juce::Justification::centredLeft);
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

    configureSectionLabel(waveformLabel, "Waveform Detail");
    configureSectionLabel(zoneLabel, "Selected Zone");
    configureSectionLabel(groupSectionLabel, "Zone Groups");
    configureSectionLabel(fxSectionLabel, "Selected FX");
    configureSectionLabel(routingSectionLabel, "Selected Bus");

    configureFieldLabel(groupNameLabel, "Group Name");
    configureFieldLabel(macroNameLabel, "Macro Name");
    configureFieldLabel(macroExposeLabel, "Perform");
    configureFieldLabel(macroAssignmentLabel, "Parameter");
    configureFieldLabel(macroRoleLabel, "Role");
    configureFieldLabel(macroDefaultLabel, "Default");
    configureFieldLabel(macroMinLabel, "Min");
    configureFieldLabel(macroMaxLabel, "Max");
    configureFieldLabel(fxTypeLabel, "Type");
    configureFieldLabel(fxScopeLabel, "Scope");
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
    drawerRegion.setComponentID("authoringDrawer");
    drawerTabStrip.setComponentID("authoringDrawerTabStrip");
    drawerContentHost.setComponentID("authoringDrawerContentHost");
    macroDrawerContent.setComponentID("authoringMacroContent");
    macroDrawerViewport.setComponentID("authoringMacroViewport");
    routingDrawerContent.setComponentID("authoringRoutingContent");
    routingDrawerViewport.setComponentID("authoringRoutingViewport");
    drawerToggleButton.setComponentID("authoringDrawerToggleButton");
    drawerWaveformTabButton.setComponentID("authoringDrawerWaveformTab");
    drawerGroupsTabButton.setComponentID("authoringDrawerGroupsTab");
    drawerMacrosTabButton.setComponentID("authoringDrawerMacrosTab");
    drawerRoutingTabButton.setComponentID("authoringDrawerRoutingTab");
    drawerPerformanceTabButton.setComponentID("authoringDrawerPerformanceTab");
    waveformLabel.setComponentID("authoringDrawerTitleLabel");
    waveformScopeLabel.setComponentID("authoringDrawerScopeLabel");
    drawerBreadcrumbLabel.setComponentID("authoringDrawerBreadcrumbLabel");
    waveformStatusLabel.setComponentID("authoringWaveformStatusLabel");
    waveformInfoLabel.setComponentID("authoringWaveformInfoLabel");
    loopInfoLabel.setComponentID("authoringWaveformLoopLabel");
    importMetricsLabel.setComponentID("authoringWaveformImportLabel");
    sourceValidationLabel.setComponentID("authoringWaveformValidationLabel");
    sourceValidationButton.setComponentID("authoringWaveformValidationButton");
    sourceValidationButton.setButtonText("Validate Sources");
    waveformPreview.setComponentID("authoringWaveformPreview");
    macroAssignmentSelector.setComponentID("authoringMacroAssignmentSelector");
    macroRoleSelector.setComponentID("authoringMacroRoleSelector");
    macroDefaultSlider.setComponentID("authoringMacroDefaultSlider");
    macroMinSlider.setComponentID("authoringMacroMinSlider");
    macroMaxSlider.setComponentID("authoringMacroMaxSlider");
    macroCreateButton.setComponentID("authoringMacroCreateButton");
    macroDuplicateButton.setComponentID("authoringMacroDuplicateButton");
    macroDeleteButton.setComponentID("authoringMacroDeleteButton");
    macroNameLabel.setComponentID("authoringMacroNameLabel");
    macroNameEditor.setComponentID("authoringMacroNameEditor");
    macroExposeLabel.setComponentID("authoringMacroExposeLabel");
    macroExposeToggle.setComponentID("authoringMacroExposeToggle");
    macroMoveUpButton.setComponentID("authoringMacroMoveUpButton");
    macroMoveDownButton.setComponentID("authoringMacroMoveDownButton");
    fxSectionLabel.setComponentID("authoringFxSectionLabel");
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
    phraseImportPathEditor.setComponentID("authoringPhraseImportPath");

    drawerToggleButton.onClick = [this]
    {
        setDrawerOpen(!drawerState.open);
    };
    drawerWaveformTabButton.setButtonText("Waveform");
    drawerGroupsTabButton.setButtonText("Groups");
    drawerMacrosTabButton.setButtonText("Macros");
    drawerRoutingTabButton.setButtonText("Routing");
    drawerPerformanceTabButton.setButtonText("Performance");
    drawerWaveformTabButton.onClick = [this] { setActiveDrawerTab(authoring::DrawerTab::waveform); };
    drawerGroupsTabButton.onClick = [this] { setActiveDrawerTab(authoring::DrawerTab::groups); };
    drawerMacrosTabButton.onClick = [this] { setActiveDrawerTab(authoring::DrawerTab::macros); };
    drawerRoutingTabButton.onClick = [this] { setActiveDrawerTab(authoring::DrawerTab::routing); };
    drawerPerformanceTabButton.onClick = [this] { setActiveDrawerTab(authoring::DrawerTab::performance); };
    playbackBannerPrepareButton.setButtonText("Prepare Draft");
    playbackBannerPrepareButton.onClick = [this] { prepareDraftPlaybackPreview(); };
    playbackBannerPublishButton.setButtonText("Publish Draft");
    playbackBannerPublishButton.onClick = [this] { publishDraftPlayback(); };
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
    summaryCallbacks.onMarkSavedRequested = [this] { markSavedCheckpoint(); };
    summaryStrip.setCallbacks(std::move(summaryCallbacks));

    authoring::ZoneFieldCallbacks zoneCallbacks;
    zoneCallbacks.onCommitRequested = [this](const authoring::ZoneFieldValuesViewModel& values,
                                             const std::string& label)
    {
        applySelectedZoneEdit(values, juce::String::fromUTF8(label.c_str()));
    };
    zoneCallbacks.onRestoreRootKeyRequested = [this]
    {
        if (onRestoreRootKeyRequested)
            onRestoreRootKeyRequested();
    };
    zoneCallbacks.onPreviewRequested = [this]
    {
        previewSelectedZone(drs::engine::AuthoringPreviewAuditionSource::inspector);
    };
    zoneMappingEditor.setCallbacks(std::move(zoneCallbacks));
    zoneMap.setOnZoneSelectionStateRequested([this](const authoring::ZoneMapCanvas::SelectionState& selectionState)
    {
        if (isRefreshing)
            return;

        if (applyZoneMapSelectionState(selectionState))
        {
            requestWaveformPreviewLoad(drawerState.activeTab == authoring::DrawerTab::waveform);
            refreshFromSession();
        }
    });
    zoneMap.setOnZoneAuditionRequested([this](const std::string& zoneId,
                                               int midiNote,
                                               int velocity)
    {
        previewSelectedZone(drs::engine::AuthoringPreviewAuditionSource::zoneMap,
                            midiNote, velocity, zoneId);
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
    zoneMap.setOnZoneRangeCommitRequested([this](const drs::engine::AuthoringZoneSummary& zone,
                                                 const std::string& label)
    {
        authoring::ZoneFieldValuesViewModel values;
        values.hasSelection = true;
        values.rootKey = zone.rootKey;
        values.keyLow = zone.keyLow;
        values.keyHigh = zone.keyHigh;
        values.velocityLow = zone.velocityLow;
        values.velocityHigh = zone.velocityHigh;
        values.gainDb = zone.gainDb;
        values.pan = zone.pan;
        values.loopEnabled = zone.loopEnabled;
        values.triggerMode = zone.triggerMode;
        applySelectedZoneEdit(values, juce::String::fromUTF8(label.c_str()));
    });
    groupList.setOnSelectionChanged([this](int nextIndex)
    {
        if (isRefreshing)
            return;

        const auto& groups = authoringSession.getProject().authoring.groups;
        if (nextIndex < 0 || static_cast<std::size_t>(nextIndex) >= groups.size())
            return;

        selectedGroupIndex = nextIndex;
        const auto result = authoringSession.selectGroup(groups[static_cast<std::size_t>(nextIndex)].id);
        if (!result.applied)
            return;

        setActiveDrawerTab(authoring::DrawerTab::groups);
        refreshFromSession();
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

    groupNameEditor.setMultiLine(false);
    groupNameEditor.setReturnKeyStartsNewLine(false);
    groupNameEditor.onReturnKey = [this] { applySelectedGroupNameEdit(); };
    groupNameEditor.onFocusLost = [this] { applySelectedGroupNameEdit(); };

    configureEditorSlider(groupGainSlider, -24.0, 24.0, 0.1);
    configureEditorSlider(groupPanSlider, -1.0, 1.0, 0.01);
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
        if (isRefreshing)
            return;

        const auto zoneIndex = zoneSelector.getSelectedId() - 1;
        const auto zones = authoringSession.getZoneSummaries();
        if (zoneIndex < 0 || static_cast<std::size_t>(zoneIndex) >= zones.size())
            return;

        authoringSession.selectZone(zones[static_cast<std::size_t>(zoneIndex)].id);
        requestWaveformPreviewLoad(drawerState.activeTab == authoring::DrawerTab::waveform);
        refreshFromSession();
    };

    previewEnabledToggle.onClick = [this]
    {
        if (!previewEnabledToggle.getToggleState() && previewCommandCallback)
        {
            drs::engine::AuthoringPreviewCommand command;
            command.type = drs::engine::AuthoringPreviewCommandType::stopAll;
            command.source = drs::engine::AuthoringPreviewAuditionSource::summaryPreview;
            previewCommandCallback(command);
        }
        refreshWaveformDrawerContent();
    };
    previewStopButton.onClick = [this]
    {
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
        refreshWaveformDrawerContent();
    };
    sourceValidationButton.onClick = [this]
    {
        updateSourceValidationAction();
        refreshWaveformDrawerContent();
    };

    macroList.setOnSelectionChanged([this](int nextIndex)
    {
        if (isRefreshing)
            return;

        const auto& macros = authoringSession.getProject().authoring.macros;
        if (nextIndex < 0 || static_cast<std::size_t>(nextIndex) >= macros.size())
            return;

        selectedMacroIndex = std::max(0, nextIndex);
        authoringSession.selectMacro(macros[static_cast<std::size_t>(nextIndex)].id);
        refreshFromSession();
    });

    macroCreateButton.setButtonText("Create");
    macroCreateButton.onClick = [this] { createMacro(); };
    macroDuplicateButton.setButtonText("Duplicate");
    macroDuplicateButton.onClick = [this] { duplicateSelectedMacro(); };
    macroDeleteButton.setButtonText("Delete");
    macroDeleteButton.onClick = [this] { deleteSelectedMacro(); };
    macroNameEditor.setMultiLine(false);
    macroNameEditor.setReturnKeyStartsNewLine(false);
    macroNameEditor.onReturnKey = [this] { applySelectedMacroEdit("Rename macro"); };
    macroNameEditor.onFocusLost = [this] { applySelectedMacroEdit("Rename macro"); };
    macroExposeToggle.setButtonText("Expose In Perform");
    macroExposeToggle.onClick = [this]
    {
        if (isRefreshing)
            return;

        applySelectedMacroEdit("Toggle macro exposure");
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

    bindCommitOnDragEnd(macroDefaultSlider, "Update macro default", [this](const juce::String& label) { applySelectedMacroEdit(label); });
    bindCommitOnDragEnd(macroMinSlider, "Update macro range", [this](const juce::String& label) { applySelectedMacroEdit(label); });
    bindCommitOnDragEnd(macroMaxSlider, "Update macro range", [this](const juce::String& label) { applySelectedMacroEdit(label); });

    macroAssignmentSelector.onChange = [this]
    {
        if (isRefreshing)
            return;

        applySelectedMacroEdit("Update macro assignment");
    };

    macroRoleSelector.onChange = [this]
    {
        if (isRefreshing)
            return;

        applySelectedMacroEdit("Update macro role");
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

    for (auto* component : {
             static_cast<juce::Component*>(&summaryStrip),
             static_cast<juce::Component*>(&playbackBanner),
             static_cast<juce::Component*>(&playbackBannerLabel),
             static_cast<juce::Component*>(&playbackBannerPrepareButton),
             static_cast<juce::Component*>(&playbackBannerPublishButton),
             static_cast<juce::Component*>(&drawerRegion),
             static_cast<juce::Component*>(&drawerTabStrip),
             static_cast<juce::Component*>(&drawerContentHost),
             static_cast<juce::Component*>(&waveformLabel),
             static_cast<juce::Component*>(&waveformScopeLabel),
             static_cast<juce::Component*>(&drawerBreadcrumbLabel),
             static_cast<juce::Component*>(&waveformStatusLabel),
             static_cast<juce::Component*>(&waveformInfoLabel),
             static_cast<juce::Component*>(&loopInfoLabel),
             static_cast<juce::Component*>(&importMetricsLabel),
             static_cast<juce::Component*>(&sourceValidationLabel),
             static_cast<juce::Component*>(&sourceValidationButton),
             static_cast<juce::Component*>(&drawerToggleButton),
             static_cast<juce::Component*>(&drawerWaveformTabButton),
             static_cast<juce::Component*>(&drawerMacrosTabButton),
             static_cast<juce::Component*>(&drawerRoutingTabButton),
             static_cast<juce::Component*>(&drawerPerformanceTabButton),
             static_cast<juce::Component*>(&zoneLabel),
             static_cast<juce::Component*>(&zoneSelector),
             static_cast<juce::Component*>(&previewEnabledToggle),
             static_cast<juce::Component*>(&previewStopButton),
             static_cast<juce::Component*>(&zoneMap),
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
             static_cast<juce::Component*>(&drawerGroupsTabButton),
             static_cast<juce::Component*>(&macroDrawerViewport),
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
             static_cast<juce::Component*>(&routingDrawerViewport),
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
             static_cast<juce::Component*>(&phraseSummaryLabel)
         })
    {
        addAndMakeVisible(component);
    }

    macroDrawerViewport.setViewedComponent(&macroDrawerContent, false);
    macroDrawerViewport.setScrollBarsShown(true, false);
    macroDrawerViewport.setScrollBarThickness(12);
    macroDrawerViewport.setWantsKeyboardFocus(false);
    macroDrawerContent.setSize(1, 1);
    for (auto* component : {
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
             static_cast<juce::Component*>(&macroMoveDownButton)
         })
    {
        macroDrawerContent.addAndMakeVisible(component);
    }

    routingDrawerViewport.setViewedComponent(&routingDrawerContent, false);
    routingDrawerViewport.setScrollBarsShown(true, false);
    routingDrawerViewport.setScrollBarThickness(12);
    routingDrawerViewport.setWantsKeyboardFocus(false);
    routingDrawerContent.setSize(1, 1);
    for (auto* component : {
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
             static_cast<juce::Component*>(&routingSummaryLabel)
         })
    {
        routingDrawerContent.addAndMakeVisible(component);
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
    for (std::size_t source = 0; source < timedPreviewNotes.size(); ++source)
        releaseTimedPreview(source);
    stopTimer(statusTimerId);
    stopTimer(previewReleaseTimerId);
    setLookAndFeel(nullptr);
}

void AuthoringPanel::configureAccessibilityAndFocus()
{
    configureAccessibleMetadata(*this,
                                "Authoring workspace",
                                "Phase 2 authoring workspace for zone mapping, compact drawers, routing, and performance editing.");
    configureAccessibleMetadata(playbackBanner,
                                "Draft playback action banner",
                                "Surfaces the next draft playback action close to the mapping workspace.");
    configureAccessibleMetadata(playbackBannerLabel,
                                "Draft playback action",
                                "Displays the next recommended draft playback action for the current workspace state.");
    configureAccessibleMetadata(playbackBannerPrepareButton,
                                "Prepare draft playback banner action",
                                "Builds the latest draft for playback preview from the workspace banner.",
                                "Press to prepare the latest draft for playback preview.");
    configureAccessibleMetadata(playbackBannerPublishButton,
                                "Publish draft playback banner action",
                                "Publishes the latest prepared draft to the performance path from the workspace banner.",
                                "Press to publish the latest prepared draft to the performance path.");
    playbackBannerPrepareButton.setExplicitFocusOrder(22);
    playbackBannerPublishButton.setExplicitFocusOrder(23);
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
                                "Hold Control while scrolling to zoom around the pointer. When zoomed, drag empty map space to pan. Use arrow keys to move the primary selection, hold Control to toggle extra zones, drag a box to multi-select, drag handles to edit ranges, or right-click to delete the selected sample.");
    zoneMap.setExplicitFocusOrder(30);

    configureAccessibleMetadata(drawerRegion,
                                "Authoring drawer",
                                "Hosts the waveform, macros, routing, and performance drawer surfaces.");
    configureAccessibleMetadata(drawerTabStrip,
                                "Drawer tab strip",
                                "Contains the drawer visibility control and drawer tab buttons.");
    configureAccessibleMetadata(drawerContentHost,
                                "Drawer content",
                                "Displays the active drawer body when the drawer is open.");
    configureAccessibleMetadata(macroDrawerViewport,
                                "Macro editor",
                                "Provides access to project macro creation, assignment, range, and ordering controls.",
                                "All macro controls are visible at standard workspace heights. Scroll vertically in unusually short host windows.");
    configureAccessibleMetadata(routingDrawerViewport,
                                "Scrollable routing inspector",
                                "Provides access to project routing, FX chain, ownership, and parameter controls.",
                                "Scroll vertically to reach the advanced FX controls.");

    configureAccessibleMetadata(drawerToggleButton,
                                "Drawer visibility",
                                "Shows or hides the active drawer content.",
                                "Press to collapse or expand the drawer.");
    drawerToggleButton.setExplicitFocusOrder(60);

    configureAccessibleMetadata(drawerWaveformTabButton,
                                "Waveform drawer tab",
                                "Shows zone-scoped waveform detail.",
                                "Press to switch the drawer to waveform detail.");
    configureAccessibleMetadata(drawerGroupsTabButton,
                                "Groups drawer tab",
                                "Shows group-scoped mixing, routing, and visibility detail.",
                                "Press to switch the drawer to group detail.");
    configureAccessibleMetadata(drawerMacrosTabButton,
                                "Macros drawer tab",
                                "Shows project-scoped macro assignments.",
                                "Press to switch the drawer to macro editing.");
    configureAccessibleMetadata(drawerRoutingTabButton,
                                "Routing drawer tab",
                                "Shows project-scoped FX and bus routing detail.",
                                "Press to switch the drawer to routing detail.");
    configureAccessibleMetadata(drawerPerformanceTabButton,
                                "Performance drawer tab",
                                "Shows bank-scoped performance and trigger detail.",
                                "Press to switch the drawer to performance detail.");
    drawerWaveformTabButton.setExplicitFocusOrder(61);
    drawerGroupsTabButton.setExplicitFocusOrder(62);
    drawerMacrosTabButton.setExplicitFocusOrder(63);
    drawerRoutingTabButton.setExplicitFocusOrder(64);
    drawerPerformanceTabButton.setExplicitFocusOrder(65);

    configureAccessibleMetadata(waveformLabel,
                                "Drawer title",
                                "Names the active drawer surface.");
    configureAccessibleMetadata(waveformScopeLabel,
                                "Drawer scope",
                                "Shows whether the active drawer is zone-, project-, bank-, or trigger-scoped.");
    configureAccessibleMetadata(drawerBreadcrumbLabel,
                                "Drawer breadcrumb",
                                "Shows the selection path for the active drawer.");
    configureAccessibleMetadata(waveformPreview,
                                "Waveform preview",
                                "Displays the selected zone waveform and loop region.");
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
    sourceValidationButton.setExplicitFocusOrder(66);

    configureAccessibleMetadata(macroList,
                                "Macro list",
                                "Lists project macros in compact rows.");
    macroList.getListBox().setExplicitFocusOrder(70);
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
    macroCreateButton.setExplicitFocusOrder(71);
    macroDuplicateButton.setExplicitFocusOrder(72);
    macroDeleteButton.setExplicitFocusOrder(73);
    macroNameEditor.setExplicitFocusOrder(74);
    macroExposeToggle.setExplicitFocusOrder(75);
    macroAssignmentSelector.setExplicitFocusOrder(76);
    macroRoleSelector.setExplicitFocusOrder(77);
    macroDefaultSlider.setExplicitFocusOrder(78);
    macroMinSlider.setExplicitFocusOrder(79);
    macroMaxSlider.setExplicitFocusOrder(80);
    macroMoveUpButton.setExplicitFocusOrder(81);
    macroMoveDownButton.setExplicitFocusOrder(82);

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
    fxSelector.setExplicitFocusOrder(80);
    fxScopeSelector.setExplicitFocusOrder(81);
    fxNameEditor.setExplicitFocusOrder(82);
    fxTypeSelector.setExplicitFocusOrder(83);
    fxBypassedToggle.setExplicitFocusOrder(84);
    fxAddButton.setExplicitFocusOrder(85);
    fxDuplicateButton.setExplicitFocusOrder(86);
    fxMoveUpButton.setExplicitFocusOrder(87);
    fxMoveDownButton.setExplicitFocusOrder(88);
    fxDeleteButton.setExplicitFocusOrder(89);
    fxOwnerSelector.setExplicitFocusOrder(90);
    fxMoveOwnerButton.setExplicitFocusOrder(91);
    fxParameterSelector.setExplicitFocusOrder(92);
    fxParameterSlider.setExplicitFocusOrder(93);
    fxParameterResetButton.setExplicitFocusOrder(94);
    fxAssignMacroButton.setExplicitFocusOrder(95);
    routingBusSelector.setExplicitFocusOrder(96);
    routingInputSelector.setExplicitFocusOrder(97);
    routingInsertOneSelector.setExplicitFocusOrder(98);
    routingInsertTwoSelector.setExplicitFocusOrder(99);
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
    groupVisibilityToggle.setExplicitFocusOrder(71);
    groupGainSlider.setExplicitFocusOrder(72);
    groupPanSlider.setExplicitFocusOrder(73);
    groupRoutingSelector.setExplicitFocusOrder(74);
    groupAnchorSelector.setExplicitFocusOrder(75);
    groupDeleteButton.setExplicitFocusOrder(76);
    groupAssignZonesButton.setExplicitFocusOrder(77);
    groupRoundRobinToggle.setExplicitFocusOrder(78);
    groupRoundRobinModeSelector.setExplicitFocusOrder(79);

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
}

void AuthoringPanel::paint(juce::Graphics& g)
{
    g.fillAll(authoringPanelBackground);

    auto bounds = getLocalBounds().toFloat().reduced(14.0f);
    g.setColour(authoringPanelAccent.withAlpha(0.22f));
    g.fillRoundedRectangle(bounds, 20.0f);

    g.setColour(authoringPanelCard);
    g.fillRoundedRectangle(bounds.reduced(4.0f), 18.0f);

    if (playbackBanner.isVisible() && !playbackBanner.getBounds().isEmpty())
    {
        auto bannerBounds = playbackBanner.getBounds().toFloat().expanded(2.0f, 1.0f);
        auto text = playbackBannerLabel.getText();
        auto bannerColour = juce::Colour::fromRGB(234, 223, 206);

        if (text.startsWithIgnoreCase("playback blocked"))
            bannerColour = juce::Colour::fromRGB(246, 223, 212);
        else if (text.startsWithIgnoreCase("playback action"))
            bannerColour = juce::Colour::fromRGB(238, 227, 208);
        else if (text.startsWithIgnoreCase("playback busy") || text.startsWithIgnoreCase("playback paused"))
            bannerColour = juce::Colour::fromRGB(231, 231, 214);

        g.setColour(bannerColour);
        g.fillRoundedRectangle(bannerBounds, 10.0f);
        g.setColour(authoringControlOutline);
        g.drawRoundedRectangle(bannerBounds, 10.0f, 1.0f);
    }
}

void AuthoringPanel::resized()
{
    auto area = getLocalBounds().reduced(28);
    const auto shortHeightLayout = getHeight() < authoring::shortHeightBreakpoint;

    summaryStrip.setBounds(area.removeFromTop(authoring::heroHeight));

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

    const auto showPlaybackBannerInLayout = playbackBanner.isVisible() && !shortHeightLayout;
    if (showPlaybackBannerInLayout)
    {
        area.removeFromTop(shortHeightLayout ? 6 : 8);
        auto bannerRow = area.removeFromTop(32);
        playbackBanner.setBounds(bannerRow);
        auto bannerContent = bannerRow.reduced(12, 4);
        auto actionWidth = 96;
        auto labelArea = bannerContent;
        if (playbackBannerPublishButton.isVisible())
            labelArea.removeFromRight(actionWidth + 8);
        if (playbackBannerPrepareButton.isVisible())
            labelArea.removeFromRight(actionWidth + 8);
        playbackBannerLabel.setBounds(labelArea);

        auto actionArea = bannerContent.removeFromRight(bannerContent.getRight() - labelArea.getRight()).withTrimmedLeft(8);
        if (playbackBannerPrepareButton.isVisible() && playbackBannerPublishButton.isVisible())
        {
            playbackBannerPrepareButton.setBounds(actionArea.removeFromLeft(actionWidth));
            actionArea.removeFromLeft(8);
            playbackBannerPublishButton.setBounds(actionArea.removeFromLeft(actionWidth));
        }
        else if (playbackBannerPrepareButton.isVisible())
        {
            playbackBannerPrepareButton.setBounds(actionArea.removeFromLeft(actionWidth));
            playbackBannerPublishButton.setBounds({});
        }
        else if (playbackBannerPublishButton.isVisible())
        {
            playbackBannerPublishButton.setBounds(actionArea.removeFromLeft(actionWidth));
            playbackBannerPrepareButton.setBounds({});
        }
        else
        {
            playbackBannerPrepareButton.setBounds({});
            playbackBannerPublishButton.setBounds({});
        }
    }
    else
    {
        playbackBanner.setBounds({});
        playbackBannerLabel.setBounds({});
        playbackBannerPrepareButton.setBounds({});
        playbackBannerPublishButton.setBounds({});
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
    const auto groupDrawerInShortLayout = shortHeightLayout
        && drawerState.activeTab == authoring::DrawerTab::groups;
    const auto macroDrawerInShortLayout = shortHeightLayout
        && drawerState.activeTab == authoring::DrawerTab::macros;
    const auto routingDrawerInShortLayout = shortHeightLayout
        && drawerState.activeTab == authoring::DrawerTab::routing;
    const auto inspectorDrawerInShortLayout = groupDrawerInShortLayout || routingDrawerInShortLayout;
    const auto drawerOpenHeight = inspectorDrawerInShortLayout
        ? authoring::shortInspectorDrawerOpenHeight
        : (macroDrawerInShortLayout
               ? std::max(authoring::compactDrawerOpenHeight, 252)
               : (drawerState.activeTab == authoring::DrawerTab::macros
                      ? authoring::macroDrawerOpenHeight
                      : (expanded ? authoring::expandedDrawerOpenHeight
                                  : authoring::compactDrawerOpenHeight)));
    const auto drawerHeight = authoring::drawerTabStripHeight + (drawerState.open ? drawerOpenHeight : 0);
    auto drawerArea = area.removeFromBottom(std::min(drawerHeight, area.getHeight()));
    drawerRegion.setBounds(drawerArea);
    drawerTabStrip.setBounds(drawerArea.removeFromTop(authoring::drawerTabStripHeight));
    drawerContentHost.setBounds(drawerArea);

    auto toggleArea = drawerTabStrip.getBounds().reduced(0, 4);
    drawerToggleButton.setBounds(toggleArea.removeFromRight(110));

    auto tabArea = drawerTabStrip.getBounds().reduced(0, 4);
    constexpr auto tabGap = 8;
    const auto tabCount = 5;
    const auto desiredTabWidth = expanded ? 104 : 96;
    const auto tabWidth = juce::jlimit(72,
                                       desiredTabWidth,
                                       (tabArea.getWidth() - (tabGap * (tabCount - 1)) - 114) / tabCount);
    drawerWaveformTabButton.setBounds(tabArea.removeFromLeft(tabWidth));
    tabArea.removeFromLeft(tabGap);
    drawerGroupsTabButton.setBounds(tabArea.removeFromLeft(tabWidth));
    tabArea.removeFromLeft(tabGap);
    drawerMacrosTabButton.setBounds(tabArea.removeFromLeft(tabWidth));
    tabArea.removeFromLeft(tabGap);
    drawerRoutingTabButton.setBounds(tabArea.removeFromLeft(tabWidth));
    tabArea.removeFromLeft(tabGap);
    drawerPerformanceTabButton.setBounds(tabArea.removeFromLeft(tabWidth + (expanded ? 8 : 2)));

    auto drawerEditorArea = drawerContentHost.getBounds().reduced(12, inspectorDrawerInShortLayout ? 6 : 10);
    if (drawerState.activeTab == authoring::DrawerTab::groups)
    {
        auto headingRow = drawerEditorArea.removeFromTop(groupDrawerInShortLayout ? 20 : 22);
        waveformLabel.setBounds(headingRow.removeFromLeft(std::min(160, headingRow.getWidth())));
        waveformScopeLabel.setBounds(headingRow);
        drawerEditorArea.removeFromTop(groupDrawerInShortLayout ? 0 : 1);
        drawerBreadcrumbLabel.setBounds(drawerEditorArea.removeFromTop(14));
        drawerEditorArea.removeFromTop(groupDrawerInShortLayout ? 2 : 3);
    }
    else
    {
        waveformLabel.setBounds(drawerEditorArea.removeFromTop(22));
        drawerEditorArea.removeFromTop(2);
        waveformScopeLabel.setBounds(drawerEditorArea.removeFromTop(14));
        drawerEditorArea.removeFromTop(1);
        drawerBreadcrumbLabel.setBounds(drawerEditorArea.removeFromTop(14));
        drawerEditorArea.removeFromTop(3);
    }

    if (drawerState.activeTab == authoring::DrawerTab::waveform)
    {
        const auto waveformMetadataHeight = 4 + 18 + 2 + 18 + 2 + 18 + 2 + 24 + 2 + 24 + 2 + 18;
        const auto waveformPreviewHeight = juce::jmax(
            0,
            juce::jmin(authoring::waveformPreviewHeight,
                       drawerEditorArea.getHeight() - waveformMetadataHeight));
        waveformPreview.setBounds(drawerEditorArea.removeFromTop(waveformPreviewHeight));
        drawerEditorArea.removeFromTop(4);
        waveformStatusLabel.setBounds(drawerEditorArea.removeFromTop(18));
        drawerEditorArea.removeFromTop(2);
        waveformInfoLabel.setBounds(drawerEditorArea.removeFromTop(18));
        drawerEditorArea.removeFromTop(2);
        loopInfoLabel.setBounds(drawerEditorArea.removeFromTop(18));
        drawerEditorArea.removeFromTop(2);
        importMetricsLabel.setBounds(drawerEditorArea.removeFromTop(24));
        drawerEditorArea.removeFromTop(2);
        sourceValidationButton.setBounds(drawerEditorArea.removeFromTop(24));
        drawerEditorArea.removeFromTop(2);
        sourceValidationLabel.setBounds(drawerEditorArea.removeFromTop(18));
    }

    if (drawerState.activeTab == authoring::DrawerTab::groups)
    {
        const auto fieldRowHeight = groupDrawerInShortLayout ? 21 : (expanded ? 26 : 24);
        const auto summaryRowHeight = groupDrawerInShortLayout ? 16 : (expanded ? 20 : 18);
        const auto actionRowHeight = groupDrawerInShortLayout ? 24 : (expanded ? 30 : 28);

        auto row = drawerEditorArea.removeFromTop(fieldRowHeight);
        layoutLabelAndField(row, groupNameLabel, groupNameEditor, 92);
        drawerEditorArea.removeFromTop(2);

        row = drawerEditorArea.removeFromTop(fieldRowHeight);
        layoutLabelAndField(row, groupVisibilityLabel, groupVisibilityToggle, 74);
        drawerEditorArea.removeFromTop(2);

        row = drawerEditorArea.removeFromTop(fieldRowHeight);
        layoutDualLabelAndFieldRow(row,
                                   groupGainLabel,
                                   groupGainSlider,
                                   42,
                                   groupPanLabel,
                                   groupPanSlider,
                                   36);
        drawerEditorArea.removeFromTop(2);

        row = drawerEditorArea.removeFromTop(fieldRowHeight);
        layoutDualLabelAndFieldRow(row,
                                   groupRoutingLabel,
                                   groupRoutingSelector,
                                   84,
                                   groupAnchorLabel,
                                   groupAnchorSelector,
                                   96);
        drawerEditorArea.removeFromTop(2);
        auto summaryRow = drawerEditorArea.removeFromTop(summaryRowHeight);
        auto groupSummaryArea = summaryRow.removeFromLeft((summaryRow.getWidth() - 12) / 2);
        summaryRow.removeFromLeft(12);
        groupSummaryLabel.setBounds(groupSummaryArea);
        groupRoundRobinLabel.setBounds(summaryRow);
        groupRoundRobinHintLabel.setBounds({});
        drawerEditorArea.removeFromTop(groupDrawerInShortLayout ? 3 : (expanded ? 6 : 4));

        auto actionRow = drawerEditorArea.removeFromTop(actionRowHeight);
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
    else if (drawerState.activeTab == authoring::DrawerTab::macros)
    {
        macroDrawerViewport.setBounds(drawerEditorArea);
        const auto macroContentWidth = std::max(420,
                                                macroDrawerViewport.getWidth()
                                                    - macroDrawerViewport.getScrollBarThickness());
        const auto macroContentHeight = expanded ? 196 : 176;
        macroDrawerContent.setSize(macroContentWidth, macroContentHeight);

        auto macroEditorArea = macroDrawerContent.getLocalBounds();
        auto actionRow = macroEditorArea.removeFromTop(28);
        constexpr auto macroActionGap = 6;
        const auto buttonWidth = std::max(52, (actionRow.getWidth() - (macroActionGap * 4)) / 5);
        macroCreateButton.setBounds(actionRow.removeFromLeft(buttonWidth));
        actionRow.removeFromLeft(std::min(macroActionGap, actionRow.getWidth()));
        macroDuplicateButton.setBounds(actionRow.removeFromLeft(buttonWidth));
        actionRow.removeFromLeft(std::min(macroActionGap, actionRow.getWidth()));
        macroDeleteButton.setBounds(actionRow.removeFromLeft(buttonWidth));
        actionRow.removeFromLeft(std::min(macroActionGap, actionRow.getWidth()));
        macroMoveUpButton.setBounds(actionRow.removeFromLeft(buttonWidth));
        actionRow.removeFromLeft(std::min(macroActionGap, actionRow.getWidth()));
        macroMoveDownButton.setBounds(actionRow);
        macroEditorArea.removeFromTop(4);
        macroList.setBounds(macroEditorArea.removeFromTop(44));

        macroEditorArea.removeFromTop(4);
        auto row = macroEditorArea.removeFromTop(28);
        layoutDualLabelAndFieldRow(row,
                                   macroNameLabel,
                                   macroNameEditor,
                                   76,
                                   macroExposeLabel,
                                   macroExposeToggle,
                                   54);
        macroEditorArea.removeFromTop(4);

        row = macroEditorArea.removeFromTop(28);
        layoutDualLabelAndFieldRow(row,
                                   macroAssignmentLabel,
                                   macroAssignmentSelector,
                                   76,
                                   macroRoleLabel,
                                   macroRoleSelector,
                                   56);
        macroEditorArea.removeFromTop(4);

        row = macroEditorArea.removeFromTop(32);
        constexpr auto valueColumnGap = 12;
        const auto valueColumnWidth = (row.getWidth() - (valueColumnGap * 2)) / 3;
        auto defaultColumn = row.removeFromLeft(valueColumnWidth);
        row.removeFromLeft(std::min(valueColumnGap, row.getWidth()));
        auto minimumColumn = row.removeFromLeft(valueColumnWidth);
        row.removeFromLeft(std::min(valueColumnGap, row.getWidth()));
        layoutLabelAndField(defaultColumn, macroDefaultLabel, macroDefaultSlider, 56);
        layoutLabelAndField(minimumColumn, macroMinLabel, macroMinSlider, 40);
        layoutLabelAndField(row, macroMaxLabel, macroMaxSlider, 40);
        if (expanded)
        {
            macroEditorArea.removeFromTop(4);
            macroSummaryLabel.setBounds(macroEditorArea.removeFromTop(16));
        }
    }
    else if (drawerState.activeTab == authoring::DrawerTab::routing)
    {
        routingDrawerViewport.setBounds(drawerEditorArea);
        const auto routingContentWidth = std::max(320,
                                                  routingDrawerViewport.getWidth()
                                                      - routingDrawerViewport.getScrollBarThickness());
        const auto routingContentHeight = expanded ? 364 : 160;
        routingDrawerContent.setSize(routingContentWidth, routingContentHeight);

        auto routingEditorArea = routingDrawerContent.getLocalBounds().reduced(0, 0);
        constexpr auto routingColumnGap = 12;
        auto splitRoutingRow = [=](juce::Rectangle<int> row)
        {
            auto left = row.removeFromLeft((row.getWidth() - routingColumnGap) / 2);
            row.removeFromLeft(std::min(routingColumnGap, row.getWidth()));
            return std::pair { left, row };
        };
        auto takeRoutingRow = [&](int height = 28, int gapAfter = 4)
        {
            auto row = routingEditorArea.removeFromTop(height);
            routingEditorArea.removeFromTop(std::min(gapAfter, routingEditorArea.getHeight()));
            return row;
        };

        auto headerRow = takeRoutingRow(24);
        auto [leftHeader, rightHeader] = splitRoutingRow(headerRow);
        fxSectionLabel.setBounds(leftHeader);
        routingSectionLabel.setBounds(rightHeader);

        if (expanded)
        {
            auto [left, right] = splitRoutingRow(takeRoutingRow());
            layoutLabelAndField(left, fxScopeLabel, fxScopeSelector, 44);
            fxScopeBreadcrumbLabel.setBounds(right);
        }

        auto [left, right] = splitRoutingRow(takeRoutingRow());
        fxSelector.setBounds(left);
        routingBusSelector.setBounds(right);

        std::tie(left, right) = splitRoutingRow(takeRoutingRow());
        layoutLabelAndField(left, fxTypeLabel, fxTypeSelector, 44);
        layoutLabelAndField(right, routingInputLabel, routingInputSelector, 44);

        std::tie(left, right) = splitRoutingRow(takeRoutingRow());
        fxBypassedToggle.setBounds(left);
        layoutLabelAndField(right, routingInsertOneLabel, routingInsertOneSelector, 44);

        layoutLabelAndField(takeRoutingRow(28, expanded ? 6 : 0),
                            routingInsertTwoLabel,
                            routingInsertTwoSelector,
                            56);

        if (expanded)
        {
            std::tie(left, right) = splitRoutingRow(takeRoutingRow());
            fxNameEditor.setBounds(left);
            fxOwnerSelector.setBounds(right);

            auto row = takeRoutingRow();
            const auto buttonWidth = std::max(64, (row.getWidth() - 20) / 5);
            fxAddButton.setBounds(row.removeFromLeft(buttonWidth)); row.removeFromLeft(5);
            fxDuplicateButton.setBounds(row.removeFromLeft(buttonWidth)); row.removeFromLeft(5);
            fxMoveUpButton.setBounds(row.removeFromLeft(buttonWidth)); row.removeFromLeft(5);
            fxMoveDownButton.setBounds(row.removeFromLeft(buttonWidth)); row.removeFromLeft(5);
            fxDeleteButton.setBounds(row);

            std::tie(left, right) = splitRoutingRow(takeRoutingRow());
            fxMoveOwnerButton.setBounds(left);
            fxParameterSelector.setBounds(right);

            std::tie(left, right) = splitRoutingRow(takeRoutingRow());
            fxParameterSlider.setBounds(left);
            fxParameterResetButton.setBounds(right.removeFromLeft((right.getWidth() - 5) / 2));
            right.removeFromLeft(5);
            fxAssignMacroButton.setBounds(right);

            std::tie(left, right) = splitRoutingRow(takeRoutingRow(18, 2));
            fxParameterValueLabel.setBounds(left);
            fxSummaryLabel.setBounds(right);

            std::tie(left, right) = splitRoutingRow(takeRoutingRow(18, 0));
            fxDiagnosticsLabel.setBounds(left);
            routingSummaryLabel.setBounds(right);
        }
    }
    else if (drawerState.activeTab == authoring::DrawerTab::performance)
    {
        if (drawerEditorArea.getWidth() < 420)
        {
            performanceBankSelector.setBounds(drawerEditorArea.removeFromTop(28));
            drawerEditorArea.removeFromTop(4);
            triggerSlotSelector.setBounds(drawerEditorArea.removeFromTop(28));
        }
        else
        {
            auto selectorRow = drawerEditorArea.removeFromTop(28);
            performanceBankSelector.setBounds(selectorRow.removeFromLeft(280));
            selectorRow.removeFromLeft(10);
            triggerSlotSelector.setBounds(selectorRow.removeFromLeft(280));
        }

        drawerEditorArea.removeFromTop(4);
        auto row = drawerEditorArea.removeFromTop(28);
        layoutDualLabelAndFieldRow(row,
                                   triggerEventLabel,
                                   triggerEventSelector,
                                   52,
                                   targetArticulationLabel,
                                   targetArticulationSelector,
                                   72);
        drawerEditorArea.removeFromTop(4);

        row = drawerEditorArea.removeFromTop(28);
        layoutDualLabelAndFieldRow(row,
                                   phraseAssetLabel,
                                   phraseAssetSelector,
                                   48,
                                   chordModeLabel,
                                   chordModeSelector,
                                   72);
        drawerEditorArea.removeFromTop(4);

        row = drawerEditorArea.removeFromTop(28);
        auto buttonArea = row.removeFromRight(180);
        row.removeFromRight(10);
        layoutLabelAndField(row, phraseImportPathLabel, phraseImportPathEditor, 56);
        phraseImportButton.setBounds(buttonArea);
        if (expanded)
        {
            drawerEditorArea.removeFromTop(6);
            performanceSummaryLabel.setBounds(drawerEditorArea.removeFromTop(20));
            drawerEditorArea.removeFromTop(4);
            phraseSummaryLabel.setBounds(drawerEditorArea.removeFromTop(24));
        }
    }

    area.removeFromTop(inspectorDrawerInShortLayout ? 4 : 8);
    auto shellArea = area;
    const auto desiredInspectorWidth = expanded ? authoring::expandedInspectorPreferredWidth
                                                : authoring::compactInspectorPreferredWidth;
    const auto minimumInspectorWidth = expanded ? authoring::expandedInspectorMinWidth
                                                : authoring::compactInspectorMinWidth;
    const auto maximumInspectorWidth = expanded ? authoring::expandedInspectorMaxWidth
                                                : authoring::compactInspectorMaxWidth;
    const auto inspectorWidth = juce::jlimit(minimumInspectorWidth,
                                             std::min(maximumInspectorWidth, std::max(minimumInspectorWidth, shellArea.getWidth() / 2)),
                                             desiredInspectorWidth);

    auto inspector = shellArea.removeFromRight(inspectorWidth);
    shellArea.removeFromRight(14);

    auto layoutGroupManager = [&](juce::Rectangle<int> groupManagerArea)
    {
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
    };

    const auto desiredGroupManagerHeight = expanded ? 112 : 124;
    const auto canStackGroupManager = shellArea.getHeight() >= authoring::minimumMapVisibleHeight + desiredGroupManagerHeight + 8;
    if (canStackGroupManager)
    {
        layoutGroupManager(shellArea.removeFromTop(desiredGroupManagerHeight));
    }
    else
    {
        const auto groupManagerWidth = shortHeightLayout
            ? std::min(280, std::max(240, shellArea.getWidth() - 200))
            : std::min(expanded ? 248 : 224,
                       std::max(188, shellArea.getWidth() / 3));
        layoutGroupManager(shellArea.removeFromLeft(groupManagerWidth));
        shellArea.removeFromLeft(std::min(10, shellArea.getWidth()));
    }

    zoneMap.setBounds(shellArea);
    zoneMappingEditor.setBounds(inspector);
}

void AuthoringPanel::reloadFromSession()
{
    refreshFromSession();
}

void AuthoringPanel::refreshNow()
{
    selectionSummaryViewModel = buildSelectionSummaryViewModel();
    summaryStrip.setViewModel(selectionSummaryViewModel);
    refreshDraftPlaybackBanner();
    refreshWaveformDrawerContent();
}

void AuthoringPanel::refreshDraftPlaybackBanner()
{
    const auto previousBannerVisible = playbackBanner.isVisible();
    const auto previousPrepareVisible = playbackBannerPrepareButton.isVisible();
    const auto previousPublishVisible = playbackBannerPublishButton.isVisible();
    const auto previousBannerText = playbackBannerLabel.getText();

    auto setButtonState = [](juce::TextButton& button,
                             bool shouldShow,
                             const juce::String& enabledDescription,
                             const juce::String& disabledDescription,
                             const juce::String& enabledHelpText,
                             const juce::String& disabledHelpText)
    {
        button.setEnabled(shouldShow);
        setVisibleAndAccessible(button, shouldShow);
        updateAccessibleDescriptionAndHelpText(button,
                                              shouldShow ? enabledDescription : disabledDescription,
                                              shouldShow ? enabledHelpText : disabledHelpText);
    };

    if (!draftPlaybackStatusProvider)
    {
        playbackBannerLabel.setText({}, juce::dontSendNotification);
        setVisibleAndAccessible(playbackBanner, false);
        setVisibleAndAccessible(playbackBannerLabel, false);
        setButtonState(playbackBannerPrepareButton,
                       false,
                       "Builds the latest draft for playback preview from the workspace banner.",
                       "Unavailable because draft playback status is not available in this shell.",
                       "Press to prepare the latest draft for playback preview.",
                       "Draft playback status is unavailable in this shell.");
        setButtonState(playbackBannerPublishButton,
                       false,
                       "Publishes the latest prepared draft to the performance path from the workspace banner.",
                       "Unavailable because draft playback status is not available in this shell.",
                       "Press to publish the latest prepared draft to the performance path.",
                       "Draft playback status is unavailable in this shell.");
    }
    else
    {
        const auto playbackStatus = draftPlaybackStatusProvider();
        const auto playbackGuidance = buildDraftPlaybackGuidance(authoringSession, playbackStatus);
        const auto bannerText = juce::String::fromUTF8(playbackGuidance.statusText.c_str());
        const auto shouldShowBanner = bannerText.isNotEmpty()
            && !bannerText.startsWithIgnoreCase("playback ready:");
        const auto shouldShowPrepare = shouldShowBanner && playbackGuidance.canPrepareDraftPlayback;
        const auto shouldShowPublish = shouldShowBanner && playbackGuidance.canPublishDraftPlayback;

        playbackBannerLabel.setText(bannerText, juce::dontSendNotification);
        setVisibleAndAccessible(playbackBanner, shouldShowBanner);
        setVisibleAndAccessible(playbackBannerLabel, shouldShowBanner);
        playbackBanner.setTitle("Draft playback action banner");
        playbackBanner.setDescription("Workspace draft playback guidance: " + bannerText);
        playbackBanner.setHelpText(shouldShowBanner
                                       ? "Follow the workspace draft playback guidance before moving back into performance playback."
                                       : "Draft playback is already current on the performance path.");
        updateDynamicAccessibleText(playbackBannerLabel, bannerText, "Draft playback action: ");

        setButtonState(playbackBannerPrepareButton,
                       shouldShowPrepare,
                       "Builds the latest draft for playback preview from the workspace banner.",
                       "Unavailable because the latest draft does not currently need preview preparation.",
                       "Press to prepare the latest draft for playback preview.",
                       "Wait until the banner asks you to prepare the latest draft.");
        setButtonState(playbackBannerPublishButton,
                       shouldShowPublish,
                       "Publishes the latest prepared draft to the performance path from the workspace banner.",
                       "Unavailable because the latest draft is not ready to publish yet.",
                       "Press to publish the latest prepared draft to the performance path.",
                       "Wait until the banner asks you to publish the ready draft.");
    }

    const auto bannerVisibilityChanged = previousBannerVisible != playbackBanner.isVisible()
        || previousPrepareVisible != playbackBannerPrepareButton.isVisible()
        || previousPublishVisible != playbackBannerPublishButton.isVisible();
    const auto bannerTextChanged = previousBannerText != playbackBannerLabel.getText();

    if (bannerVisibilityChanged)
        resized();

    if (bannerVisibilityChanged || bannerTextChanged)
        repaint();
}

authoring::SelectionSummaryViewModel AuthoringPanel::buildSelectionSummaryViewModel() const
{
    authoring::SelectionSummaryViewModel viewModel;
    const auto& documentState = authoringSession.getDocumentState();
    viewModel.title = "Phase 2 Authoring Workspace";
    viewModel.statusText = "Revision " + std::to_string(documentState.revision)
        + " | dirty=" + std::string(documentState.dirty ? "yes" : "no")
        + " | undo=" + std::to_string(documentState.undoDepth)
        + " | redo=" + std::to_string(documentState.redoDepth);
    viewModel.sourceText = "Sample source: none";
    viewModel.articulationText = "Articulation: none";
    viewModel.playbackText = "Draft playback: status unavailable";
    viewModel.canUndo = documentState.undoDepth > 0;
    viewModel.canRedo = documentState.redoDepth > 0;
    viewModel.dirty = documentState.dirty;

    if (draftPlaybackStatusProvider)
    {
        const auto playbackStatus = draftPlaybackStatusProvider();
        const auto playbackGuidance = buildDraftPlaybackGuidance(authoringSession, playbackStatus);
        viewModel.playbackText = "Draft playback: draft r" + std::to_string(playbackStatus.draftRevision)
            + " | preview r" + std::to_string(playbackStatus.preview.revision)
            + " (" + playbackStatus.preview.state + ")"
            + " | published r" + std::to_string(playbackStatus.performance.revision)
            + " (" + playbackStatus.performance.state + ")";
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

    const auto project = authoringSession.getProject();
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
        viewModel.triggerMode = zone->triggerMode;

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

void AuthoringPanel::setDrawerOpen(bool shouldOpen)
{
    if (drawerState.open == shouldOpen)
        return;

    drawerState.open = shouldOpen;
    refreshDrawerVisibility();
    resized();
}

void AuthoringPanel::setActiveDrawerTab(authoring::DrawerTab nextTab)
{
    drawerState.activeTab = nextTab;
    drawerState.open = true;
    refreshDrawerVisibility();
    resized();
    if (drawerState.activeTab == authoring::DrawerTab::waveform)
        requestWaveformPreviewLoad(true);
}

void AuthoringPanel::timerCallback(int timerId)
{
    if (timerId == statusTimerId)
    {
        refreshNow();
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
    if (!anyPending)
        stopTimer(previewReleaseTimerId);
}

void AuthoringPanel::refreshDrawerVisibility()
{
    const auto waveformTab = drawerState.activeTab == authoring::DrawerTab::waveform;
    const auto groupsTab = drawerState.activeTab == authoring::DrawerTab::groups;
    const auto macrosTab = drawerState.activeTab == authoring::DrawerTab::macros;
    const auto routingTab = drawerState.activeTab == authoring::DrawerTab::routing;
    const auto performanceTab = drawerState.activeTab == authoring::DrawerTab::performance;
    const auto drawerContentVisible = drawerState.open;
    const auto expanded = isExpandedLayout(layoutMode);
    const auto* focusedComponent = juce::Component::getCurrentlyFocusedComponent();
    const auto focusWithinWaveform = isComponentFocusedWithin(focusedComponent, waveformPreview)
        || isComponentFocusedWithin(focusedComponent, waveformStatusLabel)
        || isComponentFocusedWithin(focusedComponent, waveformInfoLabel)
        || isComponentFocusedWithin(focusedComponent, loopInfoLabel)
        || isComponentFocusedWithin(focusedComponent, importMetricsLabel)
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
        || isComponentFocusedWithin(focusedComponent, macroCreateButton)
        || isComponentFocusedWithin(focusedComponent, macroDuplicateButton)
        || isComponentFocusedWithin(focusedComponent, macroDeleteButton)
        || isComponentFocusedWithin(focusedComponent, macroNameEditor)
        || isComponentFocusedWithin(focusedComponent, macroExposeToggle)
        || isComponentFocusedWithin(focusedComponent, macroAssignmentSelector)
        || isComponentFocusedWithin(focusedComponent, macroRoleSelector)
        || isComponentFocusedWithin(focusedComponent, macroDefaultSlider)
        || isComponentFocusedWithin(focusedComponent, macroMinSlider)
        || isComponentFocusedWithin(focusedComponent, macroMaxSlider)
        || isComponentFocusedWithin(focusedComponent, macroMoveUpButton)
        || isComponentFocusedWithin(focusedComponent, macroMoveDownButton);
    const auto focusWithinRouting = isComponentFocusedWithin(focusedComponent, fxSelector)
        || isComponentFocusedWithin(focusedComponent, fxTypeSelector)
        || isComponentFocusedWithin(focusedComponent, fxBypassedToggle)
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

    refreshDrawerContextLabels();

    drawerToggleButton.setButtonText(drawerState.open ? "Hide Drawer" : "Show Drawer");
    drawerToggleButton.setTitle(drawerToggleButton.getButtonText());
    setVisibleAndAccessible(drawerContentHost, drawerContentVisible);
    setVisibleAndAccessible(waveformLabel, drawerContentVisible);
    setVisibleAndAccessible(waveformScopeLabel, drawerContentVisible);
    setVisibleAndAccessible(drawerBreadcrumbLabel, drawerContentVisible);
    setVisibleAndAccessible(waveformPreview, drawerContentVisible && waveformTab);
    setVisibleAndAccessible(waveformStatusLabel, drawerContentVisible && waveformTab);
    setVisibleAndAccessible(waveformInfoLabel, drawerContentVisible && waveformTab);
    setVisibleAndAccessible(loopInfoLabel, drawerContentVisible && waveformTab);
    setVisibleAndAccessible(importMetricsLabel, drawerContentVisible && waveformTab);
    setVisibleAndAccessible(sourceValidationLabel, drawerContentVisible && waveformTab);
    setVisibleAndAccessible(sourceValidationButton, drawerContentVisible && waveformTab);

    setVisibleAndAccessible(groupNameLabel, drawerContentVisible && groupsTab);
    setVisibleAndAccessible(groupNameEditor, drawerContentVisible && groupsTab);
    setVisibleAndAccessible(groupVisibilityLabel, drawerContentVisible && groupsTab);
    setVisibleAndAccessible(groupVisibilityToggle, drawerContentVisible && groupsTab);
    setVisibleAndAccessible(groupGainLabel, drawerContentVisible && groupsTab);
    setVisibleAndAccessible(groupGainSlider, drawerContentVisible && groupsTab);
    setVisibleAndAccessible(groupPanLabel, drawerContentVisible && groupsTab);
    setVisibleAndAccessible(groupPanSlider, drawerContentVisible && groupsTab);
    setVisibleAndAccessible(groupRoutingLabel, drawerContentVisible && groupsTab);
    setVisibleAndAccessible(groupRoutingSelector, drawerContentVisible && groupsTab);
    setVisibleAndAccessible(groupAnchorLabel, drawerContentVisible && groupsTab);
    setVisibleAndAccessible(groupAnchorSelector, drawerContentVisible && groupsTab);
    setVisibleAndAccessible(groupDeleteButton, drawerContentVisible && groupsTab);
    setVisibleAndAccessible(groupAssignZonesButton, drawerContentVisible && groupsTab);
    setVisibleAndAccessible(groupSummaryLabel, drawerContentVisible && groupsTab);
    setVisibleAndAccessible(groupRoundRobinLabel, drawerContentVisible && groupsTab);
    setVisibleAndAccessible(groupRoundRobinHintLabel, false);
    setVisibleAndAccessible(groupRoundRobinToggle, drawerContentVisible && groupsTab);
    setVisibleAndAccessible(groupRoundRobinModeSelector, drawerContentVisible && groupsTab);

    setVisibleAndAccessible(macroDrawerViewport, drawerContentVisible && macrosTab);
    setVisibleAndAccessible(macroList, drawerContentVisible && macrosTab);
    setVisibleAndAccessible(macroCreateButton, drawerContentVisible && macrosTab);
    setVisibleAndAccessible(macroDuplicateButton, drawerContentVisible && macrosTab);
    setVisibleAndAccessible(macroDeleteButton, drawerContentVisible && macrosTab);
    setVisibleAndAccessible(macroNameLabel, drawerContentVisible && macrosTab);
    setVisibleAndAccessible(macroNameEditor, drawerContentVisible && macrosTab);
    setVisibleAndAccessible(macroExposeLabel, drawerContentVisible && macrosTab);
    setVisibleAndAccessible(macroExposeToggle, drawerContentVisible && macrosTab);
    setVisibleAndAccessible(macroAssignmentLabel, drawerContentVisible && macrosTab);
    setVisibleAndAccessible(macroAssignmentSelector, drawerContentVisible && macrosTab);
    setVisibleAndAccessible(macroRoleLabel, drawerContentVisible && macrosTab);
    setVisibleAndAccessible(macroRoleSelector, drawerContentVisible && macrosTab);
    setVisibleAndAccessible(macroDefaultLabel, drawerContentVisible && macrosTab);
    setVisibleAndAccessible(macroDefaultSlider, drawerContentVisible && macrosTab);
    setVisibleAndAccessible(macroMinLabel, drawerContentVisible && macrosTab);
    setVisibleAndAccessible(macroMinSlider, drawerContentVisible && macrosTab);
    setVisibleAndAccessible(macroMaxLabel, drawerContentVisible && macrosTab);
    setVisibleAndAccessible(macroMaxSlider, drawerContentVisible && macrosTab);
    setVisibleAndAccessible(macroMoveUpButton, drawerContentVisible && macrosTab);
    setVisibleAndAccessible(macroMoveDownButton, drawerContentVisible && macrosTab);
    setVisibleAndAccessible(macroSummaryLabel, drawerContentVisible && macrosTab && expanded);

    setVisibleAndAccessible(routingDrawerViewport, drawerContentVisible && routingTab);
    setVisibleAndAccessible(fxSectionLabel, drawerContentVisible && routingTab);
    setVisibleAndAccessible(fxScopeLabel, drawerContentVisible && routingTab && expanded);
    setVisibleAndAccessible(fxScopeSelector, drawerContentVisible && routingTab && expanded);
    setVisibleAndAccessible(fxScopeBreadcrumbLabel, drawerContentVisible && routingTab && expanded);
    setVisibleAndAccessible(fxSelector, drawerContentVisible && routingTab);
    setVisibleAndAccessible(fxNameEditor, drawerContentVisible && routingTab && expanded);
    setVisibleAndAccessible(fxTypeLabel, drawerContentVisible && routingTab);
    setVisibleAndAccessible(fxTypeSelector, drawerContentVisible && routingTab);
    setVisibleAndAccessible(fxBypassedToggle, drawerContentVisible && routingTab);
    setVisibleAndAccessible(fxAddButton, drawerContentVisible && routingTab && expanded);
    setVisibleAndAccessible(fxDuplicateButton, drawerContentVisible && routingTab && expanded);
    setVisibleAndAccessible(fxMoveUpButton, drawerContentVisible && routingTab && expanded);
    setVisibleAndAccessible(fxMoveDownButton, drawerContentVisible && routingTab && expanded);
    setVisibleAndAccessible(fxDeleteButton, drawerContentVisible && routingTab && expanded);
    setVisibleAndAccessible(fxOwnerSelector, drawerContentVisible && routingTab && expanded);
    setVisibleAndAccessible(fxMoveOwnerButton, drawerContentVisible && routingTab && expanded);
    setVisibleAndAccessible(fxParameterSelector, drawerContentVisible && routingTab && expanded);
    setVisibleAndAccessible(fxParameterSlider, drawerContentVisible && routingTab && expanded);
    setVisibleAndAccessible(fxParameterResetButton, drawerContentVisible && routingTab && expanded);
    setVisibleAndAccessible(fxAssignMacroButton, drawerContentVisible && routingTab && expanded);
    setVisibleAndAccessible(fxParameterValueLabel, drawerContentVisible && routingTab && expanded);
    setVisibleAndAccessible(fxSummaryLabel, drawerContentVisible && routingTab && expanded);
    setVisibleAndAccessible(fxDiagnosticsLabel, drawerContentVisible && routingTab && expanded);
    setVisibleAndAccessible(routingSectionLabel, drawerContentVisible && routingTab);
    setVisibleAndAccessible(routingBusSelector, drawerContentVisible && routingTab);
    setVisibleAndAccessible(routingInputLabel, drawerContentVisible && routingTab);
    setVisibleAndAccessible(routingInputSelector, drawerContentVisible && routingTab);
    setVisibleAndAccessible(routingInsertOneLabel, drawerContentVisible && routingTab);
    setVisibleAndAccessible(routingInsertOneSelector, drawerContentVisible && routingTab);
    setVisibleAndAccessible(routingInsertTwoLabel, drawerContentVisible && routingTab);
    setVisibleAndAccessible(routingInsertTwoSelector, drawerContentVisible && routingTab);
    setVisibleAndAccessible(routingSummaryLabel, drawerContentVisible && routingTab && expanded);

    setVisibleAndAccessible(performanceBankSelector, drawerContentVisible && performanceTab);
    setVisibleAndAccessible(triggerSlotSelector, drawerContentVisible && performanceTab);
    setVisibleAndAccessible(triggerEventLabel, drawerContentVisible && performanceTab);
    setVisibleAndAccessible(triggerEventSelector, drawerContentVisible && performanceTab);
    setVisibleAndAccessible(targetArticulationLabel, drawerContentVisible && performanceTab);
    setVisibleAndAccessible(targetArticulationSelector, drawerContentVisible && performanceTab);
    setVisibleAndAccessible(phraseAssetLabel, drawerContentVisible && performanceTab);
    setVisibleAndAccessible(phraseAssetSelector, drawerContentVisible && performanceTab);
    setVisibleAndAccessible(chordModeLabel, drawerContentVisible && performanceTab);
    setVisibleAndAccessible(chordModeSelector, drawerContentVisible && performanceTab);
    setVisibleAndAccessible(phraseImportPathLabel, drawerContentVisible && performanceTab);
    setVisibleAndAccessible(phraseImportPathEditor, drawerContentVisible && performanceTab);
    setVisibleAndAccessible(phraseImportButton, drawerContentVisible && performanceTab);
    setVisibleAndAccessible(performanceSummaryLabel, drawerContentVisible && performanceTab && expanded);
    setVisibleAndAccessible(phraseSummaryLabel, drawerContentVisible && performanceTab && expanded);

    drawerWaveformTabButton.setToggleState(waveformTab, juce::dontSendNotification);
    drawerGroupsTabButton.setToggleState(groupsTab, juce::dontSendNotification);
    drawerMacrosTabButton.setToggleState(macrosTab, juce::dontSendNotification);
    drawerRoutingTabButton.setToggleState(routingTab, juce::dontSendNotification);
    drawerPerformanceTabButton.setToggleState(performanceTab, juce::dontSendNotification);

    const auto focusedDrawerContentBecameHidden = !drawerContentVisible
        ? (focusWithinWaveform || focusWithinGroups || focusWithinMacros || focusWithinRouting || focusWithinPerformance)
        : (waveformTab ? (focusWithinGroups || focusWithinMacros || focusWithinRouting || focusWithinPerformance)
                       : groupsTab ? (focusWithinWaveform || focusWithinMacros || focusWithinRouting || focusWithinPerformance)
                                   : macrosTab ? (focusWithinWaveform || focusWithinGroups || focusWithinRouting || focusWithinPerformance)
                                               : routingTab ? (focusWithinWaveform || focusWithinGroups || focusWithinMacros || focusWithinPerformance)
                                                            : (focusWithinWaveform || focusWithinGroups || focusWithinMacros || focusWithinRouting));

    if (focusedDrawerContentBecameHidden)
    {
        if (!drawerContentVisible)
            drawerToggleButton.grabKeyboardFocus();
        else if (waveformTab)
            drawerWaveformTabButton.grabKeyboardFocus();
        else if (groupsTab)
            drawerGroupsTabButton.grabKeyboardFocus();
        else if (macrosTab)
            drawerMacrosTabButton.grabKeyboardFocus();
        else if (routingTab)
            drawerRoutingTabButton.grabKeyboardFocus();
        else
            drawerPerformanceTabButton.grabKeyboardFocus();
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
                                           hasSelectedMacro
                                               ? "Chooses the parameter assigned to " + macroName + "."
                                               : "Unavailable because no macro is selected.",
                                           hasSelectedMacro
                                               ? "Open the list to choose a parameter target for " + macroName + "."
                                               : "Author a macro before editing its parameter assignment.");
    updateAccessibleDescriptionAndHelpText(macroRoleSelector,
                                           hasSelectedMacro
                                               ? "Chooses the semantic role for " + macroName + "."
                                               : "Unavailable because no macro is selected.",
                                           hasSelectedMacro
                                               ? "Open the list to choose a role for " + macroName + "."
                                               : "Author a macro before editing its role.");
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
    zoneMap.setVisible(true);
    zoneMappingEditor.setVisible(true);

    refreshDrawerVisibility();
}

void AuthoringPanel::refreshDrawerContextLabels()
{
    const auto& project = authoringSession.getProject();

    switch (drawerState.activeTab)
    {
        case authoring::DrawerTab::waveform:
        {
            waveformLabel.setText("Waveform Detail", juce::dontSendNotification);
            waveformScopeLabel.setText("Zone-scoped selection detail", juce::dontSendNotification);

            if (const auto selectedZone = authoringSession.getSelectedZone(); selectedZone.has_value())
            {
                drawerBreadcrumbLabel.setText("Project > Zones > "
                                                  + juce::String::fromUTF8(selectedZone->displayName.c_str()),
                                              juce::dontSendNotification);
            }
            else
            {
                drawerBreadcrumbLabel.setText("Project > Zones", juce::dontSendNotification);
            }
            break;
        }
        case authoring::DrawerTab::macros:
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
            drawerBreadcrumbLabel.setText(breadcrumb, juce::dontSendNotification);
            break;
        }
        case authoring::DrawerTab::groups:
        {
            waveformLabel.setText("Group Inspector", juce::dontSendNotification);
            waveformScopeLabel.setText("Group-scoped mix and visibility detail", juce::dontSendNotification);

            juce::String breadcrumb = "Project > Groups";
            if (const auto selectedGroup = authoringSession.getSelectedGroup(); selectedGroup.has_value())
                breadcrumb << " > " << juce::String::fromUTF8(selectedGroup->displayName.c_str());

            drawerBreadcrumbLabel.setText(breadcrumb, juce::dontSendNotification);
            break;
        }
        case authoring::DrawerTab::routing:
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

            drawerBreadcrumbLabel.setText("Project > Routing > FX: " + fxName + " | Bus: " + busName,
                                          juce::dontSendNotification);
            break;
        }
        case authoring::DrawerTab::performance:
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

                drawerBreadcrumbLabel.setText(breadcrumb, juce::dontSendNotification);
            }
            else
            {
                waveformScopeLabel.setText("Bank-scoped performance detail", juce::dontSendNotification);
                drawerBreadcrumbLabel.setText("Project > Performance", juce::dontSendNotification);
            }
            break;
        }
        default:
            break;
    }

    updateDynamicAccessibleText(waveformLabel, waveformLabel.getText(), "Active drawer title: ");
    updateDynamicAccessibleText(waveformScopeLabel, waveformScopeLabel.getText(), "Active drawer scope: ");
    updateDynamicAccessibleText(drawerBreadcrumbLabel, drawerBreadcrumbLabel.getText(), "Active drawer breadcrumb: ");
}

void AuthoringPanel::refreshWaveformDrawerContent()
{
    AuthoringWaveformPreview preview;
    if (waveformPreviewProvider)
        preview = waveformPreviewProvider();

    AuthoringPreviewStatusSnapshot previewStatus;
    if (authoringPreviewStatusProvider)
        previewStatus = authoringPreviewStatusProvider();

    waveformPreview.setPreview(preview);
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
            preview.loopEnabled
                ? "Loop " + juce::String(static_cast<int>(preview.loopStartFrame))
                    + " - " + juce::String(static_cast<int>(preview.loopEndFrame))
                : "Loop disabled for selected zone",
            juce::dontSendNotification);
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

void AuthoringPanel::refreshFromSession()
{
    const juce::ScopedValueSetter<bool> refreshGuard(isRefreshing, true);

    rebuildZoneSelector();
    rebuildGroupList();
    rebuildMacroList();
    rebuildDspScopeSelector();
    rebuildFxSelector();
    rebuildRoutingBusSelector();
    rebuildPerformanceBankSelector();
    rebuildTriggerSlotSelector();
    syncZoneMapSelectionState();
    zoneMap.setZoneSummaries(buildVisibleZoneSummaries());
    zoneMap.setSelectionState({ zoneMapSelectedZoneIds,
                                authoringSession.getSelectedZone().has_value()
                                    ? authoringSession.getSelectedZone()->id
                                    : std::string {} });

    const auto& project = authoringSession.getProject();
    selectionSummaryViewModel = buildSelectionSummaryViewModel();
    zoneFieldValuesViewModel = buildZoneFieldValuesViewModel();

    summaryStrip.setViewModel(selectionSummaryViewModel);
    refreshDraftPlaybackBanner();
    zoneMappingEditor.setViewModel(zoneFieldValuesViewModel);

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
            juce::String::fromUTF8(selectedGroup->displayName.c_str())
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
        groupSummaryLabel.setText("No group is selected.", juce::dontSendNotification);
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
            if (!macro.targets.empty()
                && macro.targets.front().dspSlotId == assignment.slot->id
                && macro.targets.front().dspParameterId == assignment.parameter->id)
            {
                selectedAssignmentId = itemId;
            }
        }

        if (!macro.targets.empty() && selectedAssignmentId == 0)
        {
            const auto assignmentIndex = findAssignmentIndex(macro.targets.front().parameterId);
            if (assignmentIndex >= 0)
            {
                selectedAssignmentId = curatedMacroAssignmentBase + assignmentIndex;
            }
            else
            {
                const auto customItemId = curatedDspMacroAssignmentBase - 1;
                macroAssignmentSelector.addItem("Custom: "
                                                   + juce::String::fromUTF8(macro.targets.front().parameterId.c_str()),
                                               customItemId);
                selectedAssignmentId = customItemId;
            }
        }
        macroAssignmentSelector.setSelectedId(selectedAssignmentId > 0 ? selectedAssignmentId : 1,
                                              juce::dontSendNotification);

        macroRoleSelector.clear(juce::dontSendNotification);
        int selectedRoleId = 0;
        const auto currentRole = !macro.targets.empty() ? macro.targets.front().role : std::string{};
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
        macroSummaryLabel.setText(
            juce::String(macro.exposedInPerformance ? "Perform | " : "Hidden | ")
                + (macro.targets.empty()
                       ? juce::String("No parameter assigned | Workflow: select a group bus gain parameter, expose it in Perform, then Publish.")
                       : "Target "
                             + juce::String::fromUTF8(macro.targets.front().parameterPath.c_str()))
                + " | range " + juce::String(macro.minValue, 2)
                + " to " + juce::String(macro.maxValue, 2)
                + (macro.targets.empty() ? juce::String {}
                   : macro.targets.front().controlLaw.id.empty()
                       ? " | legacy curve " + juce::String::fromUTF8(macro.targets.front().curve.c_str())
                       : " | law " + juce::String::fromUTF8(macro.targets.front().controlLaw.id.c_str())
                           + " (" + juce::String(macro.targets.front().destinationMinimum, 1)
                           + " to " + juce::String(macro.targets.front().destinationMaximum, 1) + ")")
                + " | Release scope: group gain lanes only (mic, layer, pedal/noise).",
            juce::dontSendNotification);
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
    }
    else
    {
        macroNameEditor.setText({}, juce::dontSendNotification);
        macroExposeToggle.setToggleState(false, juce::dontSendNotification);
        macroAssignmentSelector.clear(juce::dontSendNotification);
        macroRoleSelector.clear(juce::dontSendNotification);
        macroSummaryLabel.setText("No macros are authored in this project yet. Use Create, then target a group bus gain lane such as close mic, room, layer blend, or pedal noise. Group pan stays out of the first release.",
                                  juce::dontSendNotification);
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
    }

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
        fxAssignMacroButton.setButtonText(hasMacroControl ? "Edit Control" : "Create Control");
        fxAssignMacroButton.setTitle(fxAssignMacroButton.getButtonText());
        fxDuplicateButton.setEnabled(true);
        fxDeleteButton.setEnabled(true);
        fxMoveUpButton.setEnabled(!scopedFxSlotIds.empty() && scopedFxSlotIds.front() != fxSlot.id);
        fxMoveDownButton.setEnabled(!scopedFxSlotIds.empty() && scopedFxSlotIds.back() != fxSlot.id);
        fxMoveOwnerButton.setEnabled(fxOwnerSelector.getNumItems() > 1);
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
    }
    else
    {
        routingSummaryLabel.setText("No routing buses are authored in this project yet.", juce::dontSendNotification);
    }

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

    refreshWaveformDrawerContent();
    refreshInspectorVisibility();
}

void AuthoringPanel::applySelectedZoneEdit(const authoring::ZoneFieldValuesViewModel& values,
                                           const juce::String& label)
{
    const auto currentZone = authoringSession.getSelectedZone();
    if (!currentZone.has_value())
        return;

    auto editedZone = *currentZone;
    editedZone.rootKey = values.rootKey;
    editedZone.keyLow = values.keyLow;
    editedZone.keyHigh = values.keyHigh;
    editedZone.velocityLow = values.velocityLow;
    editedZone.velocityHigh = values.velocityHigh;
    editedZone.gainDb = values.gainDb;
    editedZone.pan = values.pan;
    editedZone.loopEnabled = values.loopEnabled;
    editedZone.triggerMode = values.triggerMode;

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
        setActiveDrawerTab(authoring::DrawerTab::groups);
        refreshFromSession();
    }
    else
    {
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                               "Create Group Failed",
                                               buildIssueSummary(result.issues));
    }
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

void AuthoringPanel::releaseTimedPreview(std::size_t sourceIndex)
{
    if (sourceIndex >= timedPreviewNotes.size() || !timedPreviewNotes[sourceIndex].active)
        return;

    const auto note = timedPreviewNotes[sourceIndex].midiNote;
    timedPreviewNotes[sourceIndex] = {};
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

void AuthoringPanel::markSavedCheckpoint()
{
    authoringSession.markSaved();
    refreshFromSession();
}

std::vector<drs::engine::AuthoringZoneSummary> AuthoringPanel::buildVisibleZoneSummaries() const
{
    const auto zoneSummaries = authoringSession.getZoneSummaries();
    const auto& project = authoringSession.getProject();
    const auto selectedZone = authoringSession.getSelectedZone();
    std::vector<drs::engine::AuthoringZoneSummary> visibleZones;
    visibleZones.reserve(zoneSummaries.size());

    for (const auto& zone : zoneSummaries)
    {
        auto visibleZone = zone;
        visibleZone.selected = selectedZone.has_value() && selectedZone->id == zone.id;
        visibleZone.additionallySelected
            = std::find(zoneMapSelectedZoneIds.begin(), zoneMapSelectedZoneIds.end(), zone.id)
            != zoneMapSelectedZoneIds.end()
            && !visibleZone.selected;

        const auto projectZoneIterator = std::find_if(project.authoring.zones.begin(),
                                                      project.authoring.zones.end(),
                                                      [&](const auto& projectZone)
                                                      {
                                                          return projectZone.id == visibleZone.id;
                                                      });
        const auto groupIterator = projectZoneIterator == project.authoring.zones.end()
            ? project.authoring.groups.end()
            : std::find_if(project.authoring.groups.begin(),
                           project.authoring.groups.end(),
                           [&](const auto& group)
                           {
                               return group.id == projectZoneIterator->groupId;
                           });
        if (groupIterator == project.authoring.groups.end()
            || groupIterator->workspaceVisible
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

    std::vector<std::string> normalizedSelectionIds { selectedZone->id };
    normalizedSelectionIds.reserve(zoneMapSelectedZoneIds.size());

    for (const auto& zone : authoringSession.getProject().authoring.zones)
    {
        if (zone.id == selectedZone->id)
            continue;

        if (std::find(zoneMapSelectedZoneIds.begin(), zoneMapSelectedZoneIds.end(), zone.id)
            != zoneMapSelectedZoneIds.end())
        {
            normalizedSelectionIds.push_back(zone.id);
        }
    }

    zoneMapSelectedZoneIds = std::move(normalizedSelectionIds);
}

bool AuthoringPanel::applyZoneMapSelectionState(const authoring::ZoneMapCanvas::SelectionState& selectionState)
{
    const auto& zones = authoringSession.getProject().authoring.zones;
    std::vector<std::string> normalizedSelectionIds;
    normalizedSelectionIds.reserve(selectionState.zoneIds.size());

    for (const auto& requestedZoneId : selectionState.zoneIds)
    {
        const auto exists = std::any_of(zones.begin(),
                                        zones.end(),
                                        [&](const auto& zone)
                                        {
                                            return zone.id == requestedZoneId;
                                        });
        if (!exists)
            continue;

        if (std::find(normalizedSelectionIds.begin(),
                      normalizedSelectionIds.end(),
                      requestedZoneId) == normalizedSelectionIds.end())
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

void AuthoringPanel::applySelectedMacroEdit(const juce::String& label)
{
    const auto& macros = authoringSession.getProject().authoring.macros;
    if (selectedMacroIndex < 0 || static_cast<std::size_t>(selectedMacroIndex) >= macros.size())
        return;

    auto editedMacro = macros[static_cast<std::size_t>(selectedMacroIndex)];
    editedMacro.name = macroNameEditor.getText().trim().toStdString();
    editedMacro.exposedInPerformance = macroExposeToggle.getToggleState();
    auto minValue = macroMinSlider.getValue();
    auto maxValue = macroMaxSlider.getValue();
    if (minValue > maxValue)
        std::swap(minValue, maxValue);

    editedMacro.minValue = minValue;
    editedMacro.maxValue = maxValue;
    editedMacro.defaultValue = std::clamp(macroDefaultSlider.getValue(), editedMacro.minValue, editedMacro.maxValue);

    const auto assignmentId = macroAssignmentSelector.getSelectedId();
    if (assignmentId == unassignedMacroAssignmentId)
    {
        editedMacro.targets.clear();
    }
    else if (assignmentId >= curatedMacroAssignmentBase
             && assignmentId < curatedMacroAssignmentBase + static_cast<int>(curatedMacroAssignments.size()))
    {
        if (editedMacro.targets.empty())
            editedMacro.targets.push_back({});
        const auto& assignment = curatedMacroAssignments[static_cast<std::size_t>(assignmentId - curatedMacroAssignmentBase)];
        editedMacro.targets.front().parameterId = assignment.parameterId;
        editedMacro.targets.front().parameterPath = assignment.parameterPath;
        editedMacro.targets.front().dspSlotId.clear();
        editedMacro.targets.front().dspParameterId.clear();
        editedMacro.targets.front().sourceMinimum = editedMacro.minValue;
        editedMacro.targets.front().sourceMaximum = editedMacro.maxValue;
        if (editedMacro.targets.front().role.empty())
            editedMacro.targets.front().role = assignment.defaultRole;
    }
    else if (assignmentId >= curatedDspMacroAssignmentBase)
    {
        const auto dspAssignments = buildCuratedDspMacroAssignments(authoringSession.getProject(),
                                                                    selectedDspScopeRoutingBusId(),
                                                                    selectedDspScopeInputSource());
        const auto assignmentIndex = static_cast<std::size_t>(assignmentId - curatedDspMacroAssignmentBase);
        if (assignmentIndex < dspAssignments.size())
        {
            if (editedMacro.targets.empty())
                editedMacro.targets.push_back({});
            const auto& assignment = dspAssignments[assignmentIndex];
            auto& target = editedMacro.targets.front();
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

    if (!editedMacro.targets.empty())
    {
        auto selectedRoleText = macroRoleSelector.getText().toStdString();
        if (selectedRoleText.rfind("Custom: ", 0) == 0)
            selectedRoleText = selectedRoleText.substr(8);
        editedMacro.targets.front().role = selectedRoleText;
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
        setActiveDrawerTab(authoring::DrawerTab::macros);
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
        setActiveDrawerTab(authoring::DrawerTab::macros);
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

    setActiveDrawerTab(authoring::DrawerTab::macros);
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
