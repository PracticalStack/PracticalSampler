#include "shared/authoring/InstrumentControlViewModels.h"

#include <algorithm>
#include <iomanip>
#include <sstream>

namespace drs::app::authoring
{
namespace
{
std::string categoryName(const drs::engine::RuntimeInstrumentControlCategory category)
{
    return drs::engine::runtimeInstrumentControlCategoryName(category);
}

std::string unitSuffix(const drs::engine::RuntimeInstrumentControlUnit unit)
{
    switch (unit)
    {
        case drs::engine::RuntimeInstrumentControlUnit::decibels: return " dB";
        case drs::engine::RuntimeInstrumentControlUnit::pan: return " pan";
        case drs::engine::RuntimeInstrumentControlUnit::cents: return " cents";
        case drs::engine::RuntimeInstrumentControlUnit::seconds: return " s";
        case drs::engine::RuntimeInstrumentControlUnit::percent: return "%";
        case drs::engine::RuntimeInstrumentControlUnit::generic:
        case drs::engine::RuntimeInstrumentControlUnit::integer:
        case drs::engine::RuntimeInstrumentControlUnit::boolean: break;
    }
    return {};
}

std::string formatValue(const drs::engine::RuntimeProjectInstrumentControlDefinition& control,
                        const double normalized)
{
    const auto physical = control.displayMinimum
        + (control.displayMaximum - control.displayMinimum)
            * std::clamp(normalized, 0.0, 1.0);
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(std::clamp(control.displayPrecision, 0, 6))
           << physical << unitSuffix(control.unit);
    return stream.str();
}

const drs::engine::RuntimeProjectMidiControlBindingDefinition* findBinding(
    const std::string& controlId,
    const std::vector<drs::engine::RuntimeProjectMidiControlBindingDefinition>& bindings)
{
    const auto found = std::find_if(bindings.begin(), bindings.end(), [&](const auto& binding)
    {
        return binding.enabled && binding.controlId == controlId;
    });
    return found == bindings.end() ? nullptr : &(*found);
}

bool scopesOverlap(const drs::engine::RuntimeMidiChannelScope& left,
                   const drs::engine::RuntimeMidiChannelScope& right)
{
    return left.kind == drs::engine::RuntimeMidiChannelScopeKind::any
        || right.kind == drs::engine::RuntimeMidiChannelScopeKind::any
        || left.channel == right.channel;
}

bool hasConflict(const drs::engine::RuntimeProjectMidiControlBindingDefinition& binding,
                 const std::vector<drs::engine::RuntimeProjectMidiControlBindingDefinition>& bindings)
{
    return std::any_of(bindings.begin(), bindings.end(), [&](const auto& other)
    {
        return other.enabled && other.id != binding.id
            && other.controllerNumber == binding.controllerNumber
            && scopesOverlap(other.channelScope, binding.channelScope);
    });
}
} // namespace

std::vector<InstrumentControlViewRow> buildInstrumentControlViewRows(
    const std::vector<drs::engine::RuntimeProjectInstrumentControlDefinition>& controls,
    const std::vector<drs::engine::RuntimeProjectMidiControlBindingDefinition>& bindings,
    const std::vector<std::pair<std::string, double>>& currentValues)
{
    std::vector<InstrumentControlViewRow> rows;
    rows.reserve(controls.size());
    for (const auto& control : controls)
    {
        if (!control.visible)
            continue;
        const auto value = std::find_if(currentValues.begin(), currentValues.end(),
                                        [&](const auto& candidate) { return candidate.first == control.id; });
        const auto normalized = value == currentValues.end() ? control.normalizedDefault : value->second;
        const auto* binding = findBinding(control.id, bindings);
        const auto source = binding == nullptr
            ? std::string("Unassigned")
            : "CC " + std::to_string(binding->controllerNumber)
            + (binding->channelScope.kind == drs::engine::RuntimeMidiChannelScopeKind::any
                       ? " / Any channel"
                       : " / Channel " + std::to_string(binding->channelScope.channel));
        const auto conflict = binding != nullptr && hasConflict(*binding, bindings);
        InstrumentControlViewRow row;
        row.id = control.id;
        row.title = control.displayName;
        row.category = categoryName(control.category);
        row.valueText = formatValue(control, normalized);
        row.defaultText = formatValue(control, control.normalizedDefault);
        row.sourceText = source;
        row.provenanceText = drs::engine::runtimeInstrumentControlProvenanceName(control.provenance);
        row.mixerSurface = control.category == drs::engine::RuntimeInstrumentControlCategory::mixer;
        row.conflict = conflict;
        row.accessibleText = row.title + ", " + row.category + ", value " + row.valueText
            + ", default " + row.defaultText + ", source " + row.sourceText;
        if (conflict)
            row.accessibleText += ", assignment conflict requires replacement or cancellation";
        rows.push_back(std::move(row));
    }
    std::stable_sort(rows.begin(), rows.end(), [&](const auto& left, const auto& right)
    {
        const auto leftControl = std::find_if(controls.begin(), controls.end(),
                                              [&](const auto& control) { return control.id == left.id; });
        const auto rightControl = std::find_if(controls.begin(), controls.end(),
                                               [&](const auto& control) { return control.id == right.id; });
        return leftControl->displayOrder < rightControl->displayOrder;
    });
    return rows;
}

std::vector<InstrumentControlAssignmentViewRow> buildInstrumentControlAssignmentViewRows(
    const std::vector<drs::engine::RuntimeProjectInstrumentControlDefinition>& controls,
    const std::vector<drs::engine::RuntimeProjectMidiControlBindingDefinition>& bindings)
{
    std::vector<InstrumentControlAssignmentViewRow> rows;
    for (const auto& control : controls)
    {
        const auto* binding = findBinding(control.id, bindings);
        InstrumentControlAssignmentViewRow row;
        row.id = control.id;
        row.destinationText = control.displayName;
        row.imported = binding != nullptr && binding->imported;
        row.sourceText = binding == nullptr ? "Unassigned" : "CC " + std::to_string(binding->controllerNumber);
        row.conflict = binding != nullptr && hasConflict(*binding, bindings);
        row.statusText = row.conflict ? "Conflict — Replace or Cancel"
                                      : (row.imported ? "Imported" : "User assigned");
        row.accessibleText = row.destinationText + ", " + row.sourceText + ", " + row.statusText;
        rows.push_back(std::move(row));
    }
    return rows;
}
} // namespace drs::app::authoring
