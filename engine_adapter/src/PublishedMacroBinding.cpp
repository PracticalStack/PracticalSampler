#include "drs/engine/PublishedMacroBinding.h"

#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <unordered_set>

namespace drs::engine
{
namespace
{
void addFinding(PublishedMacroBindingBuildResult& result,
                PublishedMacroBindingFindingSeverity severity,
                std::string code,
                std::string path,
                std::string message)
{
    result.findings.push_back({ severity, std::move(code), std::move(path), std::move(message) });
}

PublishedMacroRenderTarget classifyRenderTarget(const PlaybackSnapshotMacroDefault& macro)
{
    if (macro.id == "tone")
        return PublishedMacroRenderTarget::toneVelocity;
    if (macro.id == "motion")
        return PublishedMacroRenderTarget::motionPitch;

    for (const auto& target : macro.targets)
    {
        if (target.parameterPath.find("triggerVelocity") != std::string::npos
            || target.role.find("velocity") != std::string::npos)
            return PublishedMacroRenderTarget::toneVelocity;
        if (target.parameterPath.find("noteTravel") != std::string::npos
            || target.role.find("pitch") != std::string::npos)
            return PublishedMacroRenderTarget::motionPitch;
    }
    return PublishedMacroRenderTarget::none;
}

const PublishedMacroBinding* findPreviousBinding(
    const ImmutablePublishedMacroBindingTablePtr& table,
    const std::string& stableAuthoredId)
{
    if (table == nullptr)
        return nullptr;
    const auto iterator = std::find_if(table->bindings.begin(), table->bindings.end(),
                                      [&](const auto& binding)
                                      {
                                          return binding.assigned
                                              && binding.stableAuthoredId == stableAuthoredId;
                                      });
    return iterator != table->bindings.end() ? &*iterator : nullptr;
}
} // namespace

PublishedMacroBindingBuildResult buildPublishedMacroBindingTable(
    const PublishedMacroBindingBuildRequest& request)
{
    PublishedMacroBindingBuildResult result;
    if (request.macroSchemaDigest.empty())
        addFinding(result, PublishedMacroBindingFindingSeverity::error,
                   "published-macro-schema-digest-missing", "macroSchemaDigest",
                   "A published macro binding table requires the captured schema digest.");
    if (request.hostSlots.size() > maximumPublishedMacroHostSlots)
        addFinding(result, PublishedMacroBindingFindingSeverity::error,
                   "published-macro-host-capacity-exceeded", "hostSlots",
                   "The fixed host macro topology exceeds its bounded slot capacity.");
    if (request.authoredMacros.size() > maximumPublishedMacroHostSlots)
        addFinding(result, PublishedMacroBindingFindingSeverity::error,
                   "published-macro-authored-capacity-exceeded", "authoredMacros",
                   "The authored macro schema exceeds the published maximum macro count.");

    std::unordered_set<std::size_t> slotIndices;
    std::unordered_set<std::string> hostParameterIds;
    std::unordered_set<std::string> hostStableIds;
    for (std::size_t index = 0; index < request.hostSlots.size(); ++index)
    {
        const auto& slot = request.hostSlots[index];
        const auto path = "hostSlots[" + std::to_string(index) + "]";
        if (slot.slotIndex >= maximumPublishedMacroHostSlots
            || slot.hostParameterId.empty() || slot.stableAuthoredId.empty()
            || !slotIndices.insert(slot.slotIndex).second
            || !hostParameterIds.insert(slot.hostParameterId).second
            || !hostStableIds.insert(slot.stableAuthoredId).second)
        {
            addFinding(result, PublishedMacroBindingFindingSeverity::error,
                       "published-macro-host-slot-invalid", path,
                       "Fixed host slots require unique bounded indices, parameter ids, and stable authored ids.");
        }
    }

    std::unordered_map<std::string, const PlaybackSnapshotMacroDefault*> authoredById;
    for (std::size_t index = 0; index < request.authoredMacros.size(); ++index)
    {
        const auto& macro = request.authoredMacros[index];
        const auto path = "authoredMacros[" + std::to_string(index) + "]";
        const auto finite = std::isfinite(macro.minValue) && std::isfinite(macro.maxValue)
            && std::isfinite(macro.defaultValue);
        if (macro.id.empty() || !authoredById.emplace(macro.id, &macro).second)
            addFinding(result, PublishedMacroBindingFindingSeverity::error,
                       "published-macro-authored-id-invalid", path + ".id",
                       "Published authored macro ids must be non-empty and unique.");
        if (!finite || macro.minValue > macro.maxValue
            || macro.defaultValue < macro.minValue || macro.defaultValue > macro.maxValue)
            addFinding(result, PublishedMacroBindingFindingSeverity::error,
                       "published-macro-authored-range-invalid", path,
                       "Published macro ranges and defaults must be finite and internally valid.");
    }

    if (std::any_of(result.findings.begin(), result.findings.end(), [](const auto& finding)
        { return finding.severity == PublishedMacroBindingFindingSeverity::error; }))
        return result;

    std::unordered_map<std::string, double> currentValues;
    for (const auto& value : request.currentValues)
        currentValues.emplace(value.stableAuthoredId, value.value);

    auto table = std::make_shared<ImmutablePublishedMacroBindingTable>();
    table->revision = request.revision;
    table->macroSchemaDigest = request.macroSchemaDigest;
    table->callbackView.revision = request.revision;
    table->callbackView.hostSlotCount = request.hostSlots.size();
    table->bindings.reserve(request.hostSlots.size());

    std::unordered_set<std::string> assignedIds;
    for (const auto& slot : request.hostSlots)
    {
        PublishedMacroBinding binding;
        binding.hostSlotIndex = slot.slotIndex;
        binding.hostParameterId = slot.hostParameterId;
        binding.stableAuthoredId = slot.stableAuthoredId;
        const auto authored = authoredById.find(slot.stableAuthoredId);
        if (authored != authoredById.end())
        {
            const auto& macro = *authored->second;
            binding.assigned = true;
            binding.publishedName = macro.name;
            binding.minValue = macro.minValue;
            binding.maxValue = macro.maxValue;
            binding.defaultValue = macro.defaultValue;
            binding.renderTarget = classifyRenderTarget(macro);

            const auto* previous = findPreviousBinding(request.previousActiveTable, macro.id);
            auto migratedValue = macro.defaultValue;
            if (previous != nullptr)
            {
                const auto current = currentValues.find(macro.id);
                migratedValue = current != currentValues.end() && std::isfinite(current->second)
                    ? current->second : previous->publishedValue;
            }
            else if (request.previousActiveTable == nullptr)
            {
                const auto initial = currentValues.find(macro.id);
                if (initial != currentValues.end() && std::isfinite(initial->second))
                    migratedValue = initial->second;
            }
            binding.publishedValue = std::clamp(migratedValue, macro.minValue, macro.maxValue);
            assignedIds.insert(macro.id);

            auto& callbackSlot = table->callbackView.slots[slot.slotIndex];
            callbackSlot.assigned = true;
            callbackSlot.renderTarget = binding.renderTarget;
            callbackSlot.minValue = binding.minValue;
            callbackSlot.maxValue = binding.maxValue;
            callbackSlot.publishedValue = binding.publishedValue;
        }
        table->bindings.push_back(std::move(binding));
    }

    for (const auto& macro : request.authoredMacros)
    {
        if (!assignedIds.count(macro.id))
        {
            table->unassignedStableAuthoredIds.push_back(macro.id);
            addFinding(result, PublishedMacroBindingFindingSeverity::warning,
                       "published-macro-unassigned", "authoredMacros." + macro.id,
                       "The authored macro has no compatible fixed host slot and remains unassigned.");
        }
    }

    if (request.previousActiveTable != nullptr)
    {
        for (const auto& previous : request.previousActiveTable->bindings)
        {
            if (previous.assigned && !assignedIds.count(previous.stableAuthoredId))
                table->retiredStableAuthoredIds.push_back(previous.stableAuthoredId);
        }
    }

    result.built = true;
    result.table = std::move(table);
    return result;
}
} // namespace drs::engine
