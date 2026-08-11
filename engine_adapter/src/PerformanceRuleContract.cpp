#include "drs/engine/PerformanceRuleContract.h"

#include <algorithm>
#include <cmath>
#include <unordered_set>

namespace drs::engine
{
namespace
{
void addFinding(PerformanceRuleValidationResult& result,
                std::string code,
                std::string path,
                std::string message,
                std::string repair)
{
    result.findings.push_back({ std::move(code), std::move(path), std::move(message), std::move(repair) });
}

bool isPedalTransition(const PerformanceEventKind event) noexcept
{
    return event == PerformanceEventKind::pedalDown || event == PerformanceEventKind::pedalUp;
}
} // namespace

std::string_view performanceEventKindId(const PerformanceEventKind value) noexcept
{
    switch (value)
    {
        case PerformanceEventKind::noteOn: return "note-on";
        case PerformanceEventKind::noteOff: return "note-off";
        case PerformanceEventKind::release: return "release";
        case PerformanceEventKind::pedalDown: return "pedal-down";
        case PerformanceEventKind::pedalUp: return "pedal-up";
        case PerformanceEventKind::controllerChange: return "controller-change";
    }
    return {};
}

std::string_view performanceSustainConditionId(const PerformanceSustainCondition value) noexcept
{
    switch (value)
    {
        case PerformanceSustainCondition::any: return "any";
        case PerformanceSustainCondition::pedalUp: return "pedal-up";
        case PerformanceSustainCondition::pedalDown: return "pedal-down";
    }
    return {};
}

std::string_view performancePitchSourceId(const PerformancePitchSource value) noexcept
{
    switch (value)
    {
        case PerformancePitchSource::eventNote: return "event-note";
        case PerformancePitchSource::fixedRoot: return "fixed-root";
        case PerformancePitchSource::eventKeyFixedPitch: return "event-key-fixed-pitch";
    }
    return {};
}

std::string_view articulationActivationModeId(const ArticulationActivationMode value) noexcept
{
    return value == ArticulationActivationMode::latch ? "latch" : std::string_view {};
}

std::string_view roundRobinResetEventId(const RoundRobinResetEvent value) noexcept
{
    switch (value)
    {
        case RoundRobinResetEvent::programActivation: return "program-activation";
        case RoundRobinResetEvent::articulationChange: return "articulation-change";
        case RoundRobinResetEvent::allNotesOff: return "all-notes-off";
        case RoundRobinResetEvent::pedalDown: return "pedal-down";
        case RoundRobinResetEvent::pedalUp: return "pedal-up";
    }
    return {};
}

bool parsePerformanceEventKind(const std::string_view value, PerformanceEventKind& result) noexcept
{
    for (const auto candidate : { PerformanceEventKind::noteOn, PerformanceEventKind::noteOff,
                                  PerformanceEventKind::release, PerformanceEventKind::pedalDown,
                                  PerformanceEventKind::pedalUp, PerformanceEventKind::controllerChange })
        if (value == performanceEventKindId(candidate)) { result = candidate; return true; }
    return false;
}

bool parsePerformanceSustainCondition(const std::string_view value, PerformanceSustainCondition& result) noexcept
{
    for (const auto candidate : { PerformanceSustainCondition::any, PerformanceSustainCondition::pedalUp,
                                  PerformanceSustainCondition::pedalDown })
        if (value == performanceSustainConditionId(candidate)) { result = candidate; return true; }
    return false;
}

bool parsePerformancePitchSource(const std::string_view value, PerformancePitchSource& result) noexcept
{
    for (const auto candidate : { PerformancePitchSource::eventNote,
                                  PerformancePitchSource::fixedRoot,
                                  PerformancePitchSource::eventKeyFixedPitch })
        if (value == performancePitchSourceId(candidate)) { result = candidate; return true; }
    return false;
}

bool parseArticulationActivationMode(const std::string_view value, ArticulationActivationMode& result) noexcept
{
    if (value != "latch") return false;
    result = ArticulationActivationMode::latch;
    return true;
}

bool parseRoundRobinResetEvent(const std::string_view value, RoundRobinResetEvent& result) noexcept
{
    if (value == "program-activation") { result = RoundRobinResetEvent::programActivation; return true; }
    if (value == "articulation-change") { result = RoundRobinResetEvent::articulationChange; return true; }
    if (value == "all-notes-off") { result = RoundRobinResetEvent::allNotesOff; return true; }
    if (value == "pedal-down") { result = RoundRobinResetEvent::pedalDown; return true; }
    if (value == "pedal-up") { result = RoundRobinResetEvent::pedalUp; return true; }
    return false;
}

PerformanceRuleValidationResult validatePerformanceRuleDeclarations(
    const RuntimeProjectAuthoringState& authoring)
{
    PerformanceRuleValidationResult result;
    std::unordered_set<int> activationNotes;
    std::size_t activationCount = 0;
    for (std::size_t index = 0; index < authoring.articulations.size(); ++index)
    {
        const auto& articulation = authoring.articulations[index];
        if (!articulation.activation.has_value()) continue;
        ++activationCount;
        const auto& activation = *articulation.activation;
        const auto path = "authoring.articulations[" + std::to_string(index) + "].activation";
        if (activation.event != PerformanceEventKind::noteOn
            || activation.mode != ArticulationActivationMode::latch || !activation.consume)
            addFinding(result, "performance.activation.latch_required", path,
                       "Articulation activation must be a consuming note-on latch.",
                       "Use event note-on, mode latch, and consume true.");
        if (activation.midiNote < 0 || activation.midiNote > 127)
            addFinding(result, "performance.activation.note_range", path + ".midiNote",
                       "Activation MIDI note must be in the range 0-127.", "Choose a valid MIDI note.");
        else if (!activationNotes.insert(activation.midiNote).second)
            addFinding(result, "performance.activation.duplicate_note", path + ".midiNote",
                       "Each latch activation MIDI note must be unique.", "Assign a different switch key.");
        for (std::size_t zoneIndex = 0; zoneIndex < authoring.zones.size(); ++zoneIndex)
        {
            const auto& zone = authoring.zones[zoneIndex];
            if (zone.performance.event == PerformanceEventKind::noteOn
                && activation.midiNote >= zone.keyLow && activation.midiNote <= zone.keyHigh)
                addFinding(result, "performance.activation.playable_collision",
                           path + ".midiNote",
                           "Activation MIDI note overlaps a playable note-on zone.",
                           "Move the key switch outside all playable note-on ranges.");
        }
    }
    if (activationCount > 128)
        addFinding(result, "performance.activation.capacity", "authoring.articulations",
                   "Activation rule count exceeds the 128-rule limit.", "Remove or consolidate activation rules.");

    std::unordered_set<int> controllerDefaultNumbers;
    for (std::size_t index = 0; index < authoring.controllerDefaults.size(); ++index)
    {
        const auto& value = authoring.controllerDefaults[index];
        if (value.controllerNumber < 0 || value.controllerNumber > 127
            || value.value < 0 || value.value > 127)
            addFinding(result, "performance.controller_default.invalid",
                       "authoring.controllerDefaults[" + std::to_string(index) + "]",
                       "Controller defaults must use a valid CC number and a 0-127 value.",
                       "Choose a valid controller number and default value.");
        else if (!controllerDefaultNumbers.insert(value.controllerNumber).second)
            addFinding(result, "performance.controller_default.duplicate",
                       "authoring.controllerDefaults[" + std::to_string(index) + "]",
                       "Each controller may have only one authored default.",
                       "Merge duplicate defaults for the same controller.");
    }

    std::unordered_set<std::string> exclusiveGroups;
    for (std::size_t index = 0; index < authoring.zones.size(); ++index)
    {
        const auto& zone = authoring.zones[index];
        const auto path = "authoring.zones[" + std::to_string(index) + "]";
        if (performanceEventKindId(zone.performance.event).empty()
            || performanceSustainConditionId(zone.performance.sustain).empty()
            || performancePitchSourceId(zone.performance.pitchSource).empty())
            addFinding(result, "performance.zone.invalid_enum", path + ".performance",
                       "Zone performance declaration contains an unsupported enum value.",
                       "Use one of the v1 event, sustain, and pitch-source values.");
        if (zone.performance.event == PerformanceEventKind::release
            && (zone.performance.sustain != PerformanceSustainCondition::pedalUp
                || zone.triggerMode != ZoneTriggerMode::oneShot))
            addFinding(result, "performance.zone.release_recursion", path + ".performance",
                       "Release routes must be pedal-up one-shots so they cannot recursively release themselves.",
                       "Use sustain pedal-up and triggerMode one-shot.");
        if (isPedalTransition(zone.performance.event)
            && ((zone.performance.event == PerformanceEventKind::pedalDown
                    && zone.performance.sustain == PerformanceSustainCondition::pedalUp)
                || (zone.performance.event == PerformanceEventKind::pedalUp
                    && zone.performance.sustain == PerformanceSustainCondition::pedalDown)
                || zone.triggerMode != ZoneTriggerMode::oneShot))
            addFinding(result, "performance.zone.unreachable_condition", path + ".performance",
                       "Pedal routes must use a reachable sustain condition and one-shot playback.",
                       "Match pedal-down with pedal-down/any or pedal-up with pedal-up/any.");
        if (zone.performance.event == PerformanceEventKind::release
            && zone.performance.sustain == PerformanceSustainCondition::pedalDown)
            addFinding(result, "performance.zone.unreachable_condition", path + ".performance.sustain",
                       "Effective release is emitted only after the pedal is up.", "Use sustain pedal-up.");
        if (zone.performance.pitchSource == PerformancePitchSource::fixedRoot
            && !isPedalTransition(zone.performance.event)
            && zone.performance.event != PerformanceEventKind::controllerChange)
            addFinding(result, "performance.zone.fixed_pitch_invalid", path + ".performance.pitchSource",
                       "Fixed-root pitch is reserved for pedal transition and controller-change routes in v1.",
                           "Use event-note for note-on, note-off, and release routes.");
        if (!std::isfinite(zone.fineTuneCents) || zone.fineTuneCents < -1200.0 || zone.fineTuneCents > 1200.0)
            addFinding(result, "performance.zone.fine_tune_invalid", path + ".fineTuneCents",
                       "Fine tuning must be finite and between -1200 and 1200 cents.",
                       "Choose a bounded fine-tuning value.");
        if (!std::isfinite(zone.amplitudeVelocityTracking)
            || zone.amplitudeVelocityTracking < 0.0 || zone.amplitudeVelocityTracking > 100.0)
            addFinding(result, "performance.zone.velocity_tracking_invalid", path + ".amplitudeVelocityTracking",
                       "Amplitude velocity tracking must be finite and between 0 and 100 percent.",
                       "Choose a value supported by the native power law.");
        for (std::size_t conditionIndex = 0;
             conditionIndex < zone.controllerConditions.size(); ++conditionIndex)
        {
            const auto& condition = zone.controllerConditions[conditionIndex];
            if (condition.controllerNumber < 0 || condition.controllerNumber > 127
                || condition.minimumValue < 0 || condition.minimumValue > 127
                || condition.maximumValue < condition.minimumValue
                || condition.maximumValue > 127)
            {
                addFinding(result, "performance.zone.controller_condition_invalid",
                           path + ".controllerConditions[" + std::to_string(conditionIndex) + "]",
                           "Controller conditions must use a valid CC number and ordered 0-127 range.",
                           "Choose a valid controller number and inclusive range.");
            }
        }
        if (zone.performance.event == PerformanceEventKind::controllerChange)
        {
            const auto triggerController = zone.performance.triggerControllerNumber;
            const auto matchingCondition = triggerController.has_value()
                ? std::find_if(zone.controllerConditions.begin(), zone.controllerConditions.end(),
                               [&](const RuntimeControllerCondition& condition)
                               { return condition.controllerNumber == *triggerController; })
                : zone.controllerConditions.end();
            if (!triggerController.has_value() || *triggerController < 0 || *triggerController > 127
                || matchingCondition == zone.controllerConditions.end())
                addFinding(result, "performance.zone.controller_trigger_invalid",
                           path + ".performance.triggerControllerNumber",
                           "Controller-change routes must identify a valid trigger CC with a matching condition.",
                           "Set triggerControllerNumber to the CC used by on_loccN/on_hiccN.");
        }
        if (zone.rootKey < 0 || zone.rootKey > 127)
            addFinding(result, "performance.zone.root_key_range", path + ".rootKey",
                       "Zone root key must be in the range 0-127.", "Choose a valid MIDI root key.");
        if (!zone.exclusiveGroupId.empty()) exclusiveGroups.insert(zone.exclusiveGroupId);
        if (zone.exclusiveTargetGroupIds.size() > 8)
            addFinding(result, "performance.choke.target_capacity", path + ".exclusiveTargetGroupIds",
                       "A choke route may target at most eight exclusive groups.", "Remove or consolidate choke targets.");
        if (!zone.exclusiveTargetGroupIds.empty() && zone.exclusiveGroupId.empty())
            addFinding(result, "performance.choke.source_missing", path + ".exclusiveGroupId",
                       "Choke targets require the zone to belong to an exclusive group.", "Assign an exclusiveGroupId.");
        std::unordered_set<std::string> targets;
        for (const auto& target : zone.exclusiveTargetGroupIds)
        {
            if (target.empty() || !targets.insert(target).second || target == zone.exclusiveGroupId)
                addFinding(result, "performance.choke.target_invalid", path + ".exclusiveTargetGroupIds",
                           "Choke targets must be unique non-empty groups other than the source group.",
                           "Remove duplicate, empty, or self-target entries.");
        }
        if (zone.chokeReleaseSeconds.has_value()
            && (!std::isfinite(*zone.chokeReleaseSeconds) || *zone.chokeReleaseSeconds < 0.0
                || *zone.chokeReleaseSeconds > 5.0))
            addFinding(result, "performance.choke.release_time_invalid", path + ".chokeReleaseSeconds",
                       "Choke release time must be finite and between 0 and 5 seconds.",
                       "Choose a bounded choke release time.");
    }
    if (authoring.zones.size() > 4096)
        addFinding(result, "performance.route.capacity", "authoring.zones",
                   "Compiled trigger routes exceed the 4096-route limit.", "Split or simplify the instrument.");
    if (exclusiveGroups.size() > 64)
        addFinding(result, "performance.choke.group_capacity", "authoring.zones",
                   "Exclusive choke groups exceed the 64-group limit.", "Remove or consolidate choke groups.");
    for (std::size_t index = 0; index < authoring.zones.size(); ++index)
        for (const auto& target : authoring.zones[index].exclusiveTargetGroupIds)
            if (!target.empty() && !exclusiveGroups.count(target))
                addFinding(result, "performance.choke.target_unknown",
                           "authoring.zones[" + std::to_string(index) + "].exclusiveTargetGroupIds",
                           "Choke target does not name an existing exclusive group.",
                           "Create the target group or choose an existing group.");

    std::unordered_set<std::string> rrPoolIds;
    for (const auto& zone : authoring.zones)
        if (zone.roundRobin.has_value()) rrPoolIds.insert(zone.roundRobin->poolId);
    if (authoring.roundRobinResetRules.size() > 128)
        addFinding(result, "performance.rr_reset.capacity", "authoring.roundRobinResetRules",
                   "Round Robin reset rules exceed the 128-rule limit.", "Remove or consolidate reset rules.");
    for (std::size_t index = 0; index < authoring.roundRobinResetRules.size(); ++index)
    {
        const auto& rule = authoring.roundRobinResetRules[index];
        const auto path = "authoring.roundRobinResetRules[" + std::to_string(index) + "]";
        if (roundRobinResetEventId(rule.event).empty())
            addFinding(result, "performance.rr_reset.invalid_event", path + ".event",
                       "Round Robin reset event is unsupported.", "Use articulation-change or pedal-up.");
        if (rule.targetAll != rule.targetPoolId.empty())
            addFinding(result, "performance.rr_reset.target_invalid", path,
                       "A reset rule must target all pools or exactly one named pool.",
                       "Set target all or provide one targetPoolId.");
        if (!rule.targetAll && !rrPoolIds.count(rule.targetPoolId))
            addFinding(result, "performance.rr_reset.target_unknown", path + ".targetPoolId",
                       "Reset rule references an unknown Round Robin pool.", "Choose an existing pool or target all.");
    }
    result.valid = result.findings.empty();
    return result;
}
} // namespace drs::engine
