#include "drs/engine/InstrumentControlContract.h"

#include <algorithm>
#include <cmath>
#include <initializer_list>
#include <unordered_set>

namespace drs::engine
{
namespace
{
template <typename T>
const char* parseName(const std::string& value,
                      const std::initializer_list<std::pair<const char*, T>>& names,
                      T& result) noexcept
{
    for (const auto& [name, enumValue] : names)
    {
        if (value == name)
        {
            result = enumValue;
            return name;
        }
    }
    return nullptr;
}

bool finiteRange(const double minimum, const double maximum) noexcept
{
    return std::isfinite(minimum) && std::isfinite(maximum) && minimum <= maximum;
}

bool sourcesOverlap(const RuntimeMidiChannelScope& left,
                    const RuntimeMidiChannelScope& right) noexcept
{
    return left.kind == RuntimeMidiChannelScopeKind::any
        || right.kind == RuntimeMidiChannelScopeKind::any
        || left.channel == right.channel;
}
} // namespace

const char* runtimeInstrumentControlCategoryName(const RuntimeInstrumentControlCategory value) noexcept
{
    switch (value)
    {
        case RuntimeInstrumentControlCategory::mixer: return "mixer";
        case RuntimeInstrumentControlCategory::tuning: return "tuning";
        case RuntimeInstrumentControlCategory::envelope: return "envelope";
        case RuntimeInstrumentControlCategory::dynamics: return "dynamics";
        case RuntimeInstrumentControlCategory::tone: return "tone";
        case RuntimeInstrumentControlCategory::hidden: return "hidden";
    }
    return "hidden";
}

const char* runtimeInstrumentControlKindName(const RuntimeInstrumentControlKind value) noexcept
{
    switch (value)
    {
        case RuntimeInstrumentControlKind::normalized: return "normalized";
        case RuntimeInstrumentControlKind::bipolar: return "bipolar";
        case RuntimeInstrumentControlKind::decibels: return "decibels";
        case RuntimeInstrumentControlKind::cents: return "cents";
        case RuntimeInstrumentControlKind::seconds: return "seconds";
        case RuntimeInstrumentControlKind::percent: return "percent";
        case RuntimeInstrumentControlKind::stepped: return "stepped";
        case RuntimeInstrumentControlKind::toggle: return "toggle";
    }
    return "normalized";
}

const char* runtimeInstrumentControlUnitName(const RuntimeInstrumentControlUnit value) noexcept
{
    switch (value)
    {
        case RuntimeInstrumentControlUnit::generic: return "generic";
        case RuntimeInstrumentControlUnit::decibels: return "decibels";
        case RuntimeInstrumentControlUnit::pan: return "pan";
        case RuntimeInstrumentControlUnit::cents: return "cents";
        case RuntimeInstrumentControlUnit::seconds: return "seconds";
        case RuntimeInstrumentControlUnit::percent: return "percent";
        case RuntimeInstrumentControlUnit::integer: return "integer";
        case RuntimeInstrumentControlUnit::boolean: return "boolean";
    }
    return "generic";
}

const char* runtimeInstrumentControlProvenanceName(const RuntimeInstrumentControlProvenance value) noexcept
{
    switch (value)
    {
        case RuntimeInstrumentControlProvenance::authored: return "authored";
        case RuntimeInstrumentControlProvenance::importedSfz: return "imported_sfz";
        case RuntimeInstrumentControlProvenance::migrated: return "migrated";
    }
    return "authored";
}

const char* runtimeInstrumentControlTargetKindName(const RuntimeInstrumentControlTargetKind value) noexcept
{
    switch (value)
    {
        case RuntimeInstrumentControlTargetKind::gain: return "gain";
        case RuntimeInstrumentControlTargetKind::pan: return "pan";
        case RuntimeInstrumentControlTargetKind::tune: return "tune";
        case RuntimeInstrumentControlTargetKind::envelopeHold: return "envelope_hold";
        case RuntimeInstrumentControlTargetKind::envelopeDecay: return "envelope_decay";
        case RuntimeInstrumentControlTargetKind::envelopeSustain: return "envelope_sustain";
    }
    return "gain";
}

const char* runtimeInstrumentControlContributionModeName(
    const RuntimeInstrumentControlContributionMode value) noexcept
{
    switch (value)
    {
        case RuntimeInstrumentControlContributionMode::multiply: return "multiply";
        case RuntimeInstrumentControlContributionMode::add: return "add";
        case RuntimeInstrumentControlContributionMode::replace: return "replace";
    }
    return "replace";
}

const char* runtimeMidiChannelScopeKindName(const RuntimeMidiChannelScopeKind value) noexcept
{
    return value == RuntimeMidiChannelScopeKind::exact ? "exact" : "any";
}

bool parseRuntimeInstrumentControlCategory(const std::string& value,
                                          RuntimeInstrumentControlCategory& result) noexcept
{
    return parseName(value, {{ "mixer", RuntimeInstrumentControlCategory::mixer },
                              { "tuning", RuntimeInstrumentControlCategory::tuning },
                              { "envelope", RuntimeInstrumentControlCategory::envelope },
                              { "dynamics", RuntimeInstrumentControlCategory::dynamics },
                              { "tone", RuntimeInstrumentControlCategory::tone },
                              { "hidden", RuntimeInstrumentControlCategory::hidden }},
                    result) != nullptr;
}

bool parseRuntimeInstrumentControlKind(const std::string& value,
                                       RuntimeInstrumentControlKind& result) noexcept
{
    return parseName(value, {{ "normalized", RuntimeInstrumentControlKind::normalized },
                              { "bipolar", RuntimeInstrumentControlKind::bipolar },
                              { "decibels", RuntimeInstrumentControlKind::decibels },
                              { "cents", RuntimeInstrumentControlKind::cents },
                              { "seconds", RuntimeInstrumentControlKind::seconds },
                              { "percent", RuntimeInstrumentControlKind::percent },
                              { "stepped", RuntimeInstrumentControlKind::stepped },
                              { "toggle", RuntimeInstrumentControlKind::toggle }},
                    result) != nullptr;
}

bool parseRuntimeInstrumentControlUnit(const std::string& value,
                                       RuntimeInstrumentControlUnit& result) noexcept
{
    return parseName(value, {{ "generic", RuntimeInstrumentControlUnit::generic },
                              { "decibels", RuntimeInstrumentControlUnit::decibels },
                              { "pan", RuntimeInstrumentControlUnit::pan },
                              { "cents", RuntimeInstrumentControlUnit::cents },
                              { "seconds", RuntimeInstrumentControlUnit::seconds },
                              { "percent", RuntimeInstrumentControlUnit::percent },
                              { "integer", RuntimeInstrumentControlUnit::integer },
                              { "boolean", RuntimeInstrumentControlUnit::boolean }},
                    result) != nullptr;
}

bool parseRuntimeInstrumentControlProvenance(const std::string& value,
                                             RuntimeInstrumentControlProvenance& result) noexcept
{
    return parseName(value, {{ "authored", RuntimeInstrumentControlProvenance::authored },
                              { "imported_sfz", RuntimeInstrumentControlProvenance::importedSfz },
                              { "migrated", RuntimeInstrumentControlProvenance::migrated }},
                    result) != nullptr;
}

bool parseRuntimeInstrumentControlTargetKind(const std::string& value,
                                             RuntimeInstrumentControlTargetKind& result) noexcept
{
    return parseName(value, {{ "gain", RuntimeInstrumentControlTargetKind::gain },
                              { "pan", RuntimeInstrumentControlTargetKind::pan },
                              { "tune", RuntimeInstrumentControlTargetKind::tune },
                              { "envelope_hold", RuntimeInstrumentControlTargetKind::envelopeHold },
                              { "envelope_decay", RuntimeInstrumentControlTargetKind::envelopeDecay },
                              { "envelope_sustain", RuntimeInstrumentControlTargetKind::envelopeSustain }},
                    result) != nullptr;
}

bool parseRuntimeInstrumentControlContributionMode(
    const std::string& value,
    RuntimeInstrumentControlContributionMode& result) noexcept
{
    return parseName(value, {{ "multiply", RuntimeInstrumentControlContributionMode::multiply },
                              { "add", RuntimeInstrumentControlContributionMode::add },
                              { "replace", RuntimeInstrumentControlContributionMode::replace }},
                    result) != nullptr;
}

bool parseRuntimeMidiChannelScopeKind(const std::string& value,
                                      RuntimeMidiChannelScopeKind& result) noexcept
{
    return parseName(value, {{ "any", RuntimeMidiChannelScopeKind::any },
                              { "exact", RuntimeMidiChannelScopeKind::exact }},
                    result) != nullptr;
}

RuntimeInstrumentControlValidationResult validateInstrumentControlCatalog(
    const std::vector<RuntimeProjectInstrumentControlDefinition>& controls,
    const std::vector<RuntimeProjectInstrumentControlTargetDefinition>& targets,
    const std::vector<RuntimeProjectMidiControlBindingDefinition>& bindings)
{
    RuntimeInstrumentControlValidationResult result;
    std::unordered_set<std::string> controlIds;
    std::unordered_set<std::string> targetIds;
    std::unordered_set<std::string> bindingIds;

    if (controls.size() > maximumInstrumentControls)
        result.issues.push_back("instrumentControls exceeds maximumInstrumentControls");

    for (const auto& control : controls)
    {
        if (control.id.empty())
            result.issues.push_back("instrument control id must not be empty");
        if (!controlIds.insert(control.id).second)
            result.issues.push_back("duplicate instrument control id: " + control.id);
        if (control.displayName.empty())
            result.issues.push_back("instrument control displayName must not be empty: " + control.id);
        if (!std::isfinite(control.normalizedDefault)
            || control.normalizedDefault < 0.0 || control.normalizedDefault > 1.0)
            result.issues.push_back("instrument control normalizedDefault must be in [0,1]: " + control.id);
        if (!finiteRange(control.displayMinimum, control.displayMaximum))
            result.issues.push_back("instrument control display range is invalid: " + control.id);
        if (control.displayPrecision < 0 || control.displayPrecision > 8)
            result.issues.push_back("instrument control displayPrecision is invalid: " + control.id);
        if (control.importedSourceController.has_value()
            && (*control.importedSourceController < 0 || *control.importedSourceController > 127))
            result.issues.push_back("instrument control imported source CC is invalid: " + control.id);
    }

    for (const auto& target : targets)
    {
        if (target.id.empty())
            result.issues.push_back("instrument control target id must not be empty");
        if (!targetIds.insert(target.id).second)
            result.issues.push_back("duplicate instrument control target id: " + target.id);
        if (controlIds.find(target.controlId) == controlIds.end())
            result.issues.push_back("instrument control target references unknown control: " + target.controlId);
        if (!finiteRange(target.sourceMinimum, target.sourceMaximum)
            || !finiteRange(target.destinationMinimum, target.destinationMaximum))
            result.issues.push_back("instrument control target range is invalid: " + target.id);
        if (target.curveIndex < -1 || target.curveIndex >= 128)
            result.issues.push_back("instrument control target curve index is invalid: " + target.id);
        for (const auto point : target.curvePoints)
        {
            if (!std::isfinite(point))
            {
                result.issues.push_back("instrument control target curve contains non-finite data: " + target.id);
                break;
            }
        }
    }

    for (const auto& binding : bindings)
    {
        if (binding.id.empty())
            result.issues.push_back("MIDI control binding id must not be empty");
        if (!bindingIds.insert(binding.id).second)
            result.issues.push_back("duplicate MIDI control binding id: " + binding.id);
        if (controlIds.find(binding.controlId) == controlIds.end())
            result.issues.push_back("MIDI control binding references unknown control: " + binding.controlId);
        if (binding.controllerNumber < 0 || binding.controllerNumber > 127)
            result.issues.push_back("MIDI control binding CC is invalid: " + binding.id);
        if (binding.channelScope.kind == RuntimeMidiChannelScopeKind::exact
            && (binding.channelScope.channel < 1 || binding.channelScope.channel > 16))
            result.issues.push_back("MIDI control binding exact channel is invalid: " + binding.id);

        if (!binding.enabled)
            continue;
        for (const auto& other : bindings)
        {
            if (&other == &binding || !other.enabled || other.id >= binding.id)
                continue;
            if (other.controllerNumber == binding.controllerNumber
                && sourcesOverlap(other.channelScope, binding.channelScope))
                result.issues.push_back("MIDI control binding source conflict: " + binding.id
                                        + " with " + other.id);
        }
    }

    result.valid = result.issues.empty();
    return result;
}

double resolveImportedSfzControlDefault(
    const RuntimeProjectInstrumentControlDefinition& control,
    const std::vector<RuntimeProjectInstrumentControlTargetDefinition>& targets,
    const bool hasExplicitControllerDefault) noexcept
{
    if (hasExplicitControllerDefault
        || control.provenance != RuntimeInstrumentControlProvenance::importedSfz
        || control.normalizedDefault != 0.0)
    {
        return control.normalizedDefault;
    }

    const auto controlsGain = std::any_of(targets.begin(), targets.end(),
        [&](const auto& target)
        {
            return target.controlId == control.id
                && target.targetKind == RuntimeInstrumentControlTargetKind::gain;
        });
    return controlsGain ? 1.0 : control.normalizedDefault;
}
} // namespace drs::engine
