#include "drs/engine/InstrumentControlBinding.h"

#include <algorithm>
#include <cmath>
#include <unordered_map>

namespace drs::engine
{
namespace
{
void addIssue(std::vector<RuntimeInstrumentControlBindingIssue>& issues,
              const char* code,
              std::string detail)
{
    issues.push_back({ code, std::move(detail) });
}
} // namespace

bool InstrumentControlBindingTable::compile(
    const std::vector<RuntimeProjectInstrumentControlDefinition>& controls,
    const std::vector<RuntimeProjectMidiControlBindingDefinition>& bindings,
    std::vector<RuntimeInstrumentControlBindingIssue>& issues)
{
    controlCountValue = 0;
    anyChannel.fill(invalidControlIndex);
    for (auto& channels : exactChannel)
        channels.fill(invalidControlIndex);
    defaults.fill(0.0);
    destinationControllers.fill(-1);
    for (auto& id : ids)
        id.clear();

    if (controls.size() > maximumInstrumentControls)
    {
        addIssue(issues, "control-capacity-exceeded",
                 "Instrument control count exceeds the bounded runtime table.");
        return false;
    }

    std::unordered_map<std::string, std::size_t> indexById;
    indexById.reserve(controls.size());
    for (const auto& control : controls)
    {
        if (control.id.empty() || indexById.find(control.id) != indexById.end())
        {
            addIssue(issues, "control-id-invalid",
                     "Instrument control IDs must be non-empty and unique.");
            continue;
        }
        if (!std::isfinite(control.normalizedDefault)
            || control.normalizedDefault < 0.0 || control.normalizedDefault > 1.0)
        {
            addIssue(issues, "control-default-invalid",
                     "Instrument control '" + control.id + "' has a non-normalized default.");
            continue;
        }
        const auto index = controlCountValue++;
        indexById.emplace(control.id, index);
        ids[index] = control.id;
        defaults[index] = control.normalizedDefault;
        destinationControllers[index] = control.importedSourceController.value_or(-1);
    }

    bool valid = issues.empty();
    for (const auto& binding : bindings)
    {
        if (!binding.enabled)
            continue;
        const auto control = indexById.find(binding.controlId);
        if (control == indexById.end())
        {
            addIssue(issues, "binding-control-missing",
                     "Binding '" + binding.id + "' references an unknown control.");
            valid = false;
            continue;
        }
        if (binding.controllerNumber < 0 || binding.controllerNumber > 127)
        {
            addIssue(issues, "binding-cc-invalid",
                     "Binding '" + binding.id + "' has an invalid CC number.");
            valid = false;
            continue;
        }
        if (binding.channelScope.kind == RuntimeMidiChannelScopeKind::any)
        {
            auto& destination = anyChannel[static_cast<std::size_t>(binding.controllerNumber)];
            if (destination != invalidControlIndex)
            {
                addIssue(issues, "binding-source-conflict",
                         "More than one Any Channel binding claims the same CC.");
                valid = false;
            }
            else
            {
                destination = control->second;
                // The binding is the authoritative runtime source. Imported
                // controls may carry the original source as provenance, but
                // authored/manual rebinding must immediately retarget the
                // audio contribution without requiring a second table.
                destinationControllers[control->second] = binding.controllerNumber;
            }
        }
        else if (binding.channelScope.channel < 1 || binding.channelScope.channel > 16)
        {
            addIssue(issues, "binding-channel-invalid",
                     "Binding '" + binding.id + "' has an invalid exact MIDI channel.");
            valid = false;
        }
        else
        {
            auto& destination = exactChannel[static_cast<std::size_t>(binding.controllerNumber)]
                                           [binding.channelScope.channel - 1u];
            if (destination != invalidControlIndex)
            {
                addIssue(issues, "binding-source-conflict",
                         "More than one exact-channel binding claims the same CC/channel.");
                valid = false;
            }
            else
            {
                destination = control->second;
                destinationControllers[control->second] = binding.controllerNumber;
            }
        }
    }

    return valid && issues.empty();
}

std::size_t InstrumentControlBindingTable::resolve(
    const std::uint8_t midiChannel1Based,
    const std::uint8_t controllerNumber) const noexcept
{
    if (midiChannel1Based >= 1 && midiChannel1Based <= 16)
    {
        const auto exact = exactChannel[controllerNumber][midiChannel1Based - 1u];
        if (exact != invalidControlIndex)
            return exact;
    }
    return anyChannel[controllerNumber];
}

double InstrumentControlBindingTable::defaultValue(const std::size_t controlIndex) const noexcept
{
    return controlIndex < controlCountValue ? defaults[controlIndex] : 0.0;
}

int InstrumentControlBindingTable::destinationController(const std::size_t controlIndex) const noexcept
{
    return controlIndex < controlCountValue ? destinationControllers[controlIndex] : -1;
}

const std::string& InstrumentControlBindingTable::controlId(const std::size_t controlIndex) const noexcept
{
    static const std::string empty;
    return controlIndex < controlCountValue ? ids[controlIndex] : empty;
}

InstrumentControlRuntimeState::InstrumentControlRuntimeState() noexcept
{
    for (auto& value : values)
        value.store(0.0, std::memory_order_relaxed);
}

void InstrumentControlRuntimeState::prepare(const InstrumentControlBindingTable& table) noexcept
{
    controlCountValue = std::min(table.controlCount(), maximumInstrumentControls);
    for (std::size_t index = 0; index < maximumInstrumentControls; ++index)
    {
        defaults[index] = index < controlCountValue ? table.defaultValue(index) : 0.0;
        values[index].store(defaults[index], std::memory_order_release);
    }
    valueGeneration.fetch_add(1, std::memory_order_acq_rel);
}

void InstrumentControlRuntimeState::resetAll() noexcept
{
    for (std::size_t index = 0; index < controlCountValue; ++index)
        values[index].store(defaults[index], std::memory_order_release);
    valueGeneration.fetch_add(1, std::memory_order_acq_rel);
}

bool InstrumentControlRuntimeState::resetControl(const std::size_t controlIndex) noexcept
{
    if (controlIndex >= controlCountValue)
        return false;
    values[controlIndex].store(defaults[controlIndex], std::memory_order_release);
    valueGeneration.fetch_add(1, std::memory_order_acq_rel);
    return true;
}

bool InstrumentControlRuntimeState::setControlNormalized(const std::size_t controlIndex,
                                                          const double normalized) noexcept
{
    if (controlIndex >= controlCountValue || !std::isfinite(normalized))
        return false;
    values[controlIndex].store(std::clamp(normalized, 0.0, 1.0), std::memory_order_release);
    valueGeneration.fetch_add(1, std::memory_order_acq_rel);
    return true;
}

bool InstrumentControlRuntimeState::applyMidi(
    const std::uint8_t midiChannel1Based,
    const std::uint8_t controllerNumber,
    const std::uint8_t value,
    const InstrumentControlBindingTable& table) noexcept
{
    const auto index = table.resolve(midiChannel1Based, controllerNumber);
    if (index == InstrumentControlBindingTable::invalidControlIndex)
        return false;
    return setControlNormalized(index, static_cast<double>(value) / 127.0);
}

double InstrumentControlRuntimeState::currentValue(const std::size_t controlIndex) const noexcept
{
    return controlIndex < controlCountValue
        ? values[controlIndex].load(std::memory_order_acquire) : 0.0;
}

double InstrumentControlRuntimeState::defaultValue(const std::size_t controlIndex) const noexcept
{
    return controlIndex < controlCountValue ? defaults[controlIndex] : 0.0;
}
} // namespace drs::engine
