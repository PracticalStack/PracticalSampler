#include "drs/engine/PublishedMacroBinding.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <string_view>
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

const PlaybackSnapshotMacroTarget* findDspTarget(const PlaybackSnapshotMacroDefault& macro)
{
    const auto target = std::find_if(macro.targets.begin(), macro.targets.end(), [](const auto& candidate)
    {
        return !candidate.dspSlotId.empty() || !candidate.dspParameterId.empty();
    });
    return target == macro.targets.end() ? nullptr : &*target;
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

std::string humanizeIdentifier(const std::string& identifier)
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

PublishedMacroPresentation fallbackPresentation(const PlaybackSnapshotMacroDefault& macro,
                                                const std::size_t authoredOrder)
{
    PublishedMacroPresentation presentation;
    presentation.authoredLabel = macro.name.empty() ? humanizeIdentifier(macro.id) : macro.name;
    presentation.sectionLabel = "Instrument";
    presentation.parameterLabel = "Control";
    presentation.authoredOrder = authoredOrder;
    presentation.accessibilityDescription = presentation.authoredLabel + ", "
        + presentation.sectionLabel + ", " + presentation.parameterLabel;
    return presentation;
}

void finalizePresentation(PublishedMacroPresentation& presentation,
                          const PlaybackSnapshotMacroDefault& macro,
                          const std::size_t authoredOrder)
{
    const auto fallback = fallbackPresentation(macro, authoredOrder);
    if (presentation.authoredLabel.empty()) presentation.authoredLabel = fallback.authoredLabel;
    if (presentation.sectionLabel.empty()) presentation.sectionLabel = fallback.sectionLabel;
    if (presentation.parameterLabel.empty()) presentation.parameterLabel = fallback.parameterLabel;
    presentation.authoredOrder = authoredOrder;
    if (presentation.accessibilityDescription.empty())
    {
        presentation.accessibilityDescription = presentation.authoredLabel + ", "
            + presentation.sectionLabel + ", " + presentation.parameterLabel;
        if (!presentation.valueUnit.empty())
            presentation.accessibilityDescription += ", " + presentation.valueUnit;
    }
}

bool compilePublishedControlLaw(const PlaybackSnapshotMacroTarget* target,
                                const std::string_view fallbackLawId,
                                const double destinationMinimum,
                                const double destinationMaximum,
                                CompiledControlLaw& result,
                                std::string& code,
                                std::string& message)
{
    // Legacy projects persisted a curve string. Keep that input readable, but
    // immediately compile it into the same immutable payload as authored laws.
    std::string_view lawId = fallbackLawId;
    if (target != nullptr)
    {
        if (!target->controlLaw.id.empty())
        {
            if (target->controlLaw.version != 1)
            {
                code = "published-macro-control-law-version-unsupported";
                message = "The authored control-law version is not supported by this runtime.";
                return false;
            }
            lawId = target->controlLaw.id;
        }
        else if (target->controlLaw.version != 0)
        {
            code = "published-macro-control-law-incomplete";
            message = "The authored control-law version requires a non-empty law id.";
            return false;
        }
        else if (target->curve == "logarithmic")
        {
            lawId = controlLawLogPositiveV1;
        }
        else if (target->curve != "linear")
        {
            code = "published-macro-control-law-legacy-curve-invalid";
            message = "The legacy macro curve cannot be compiled into a control law.";
            return false;
        }
    }

    if (!compileControlLaw(lawId, destinationMinimum, destinationMaximum, result))
    {
        code = "published-macro-control-law-incompatible";
        message = "The authored control law is unknown or incompatible with its destination range.";
        return false;
    }
    return true;
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
    std::unordered_map<std::string, std::size_t> authoredOrders;
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
        authoredOrders.emplace(macro.id, index);
    }

    if (std::any_of(result.findings.begin(), result.findings.end(), [](const auto& finding)
        { return finding.severity == PublishedMacroBindingFindingSeverity::error; }))
        return result;

    std::unordered_map<std::string, double> currentValues;
    for (const auto& value : request.currentValues)
        currentValues.emplace(value.stableAuthoredId, value.value);

    std::unordered_map<std::string, PublishedMacroPresentation> presentationHints;
    for (const auto& hint : request.presentationHints)
        presentationHints.emplace(hint.stableAuthoredId, hint.presentation);

    auto table = std::make_shared<ImmutablePublishedMacroBindingTable>();
    table->revision = request.revision;
    table->macroSchemaDigest = request.macroSchemaDigest;
    table->dspGraphDigest = request.dspGraphDigest;
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
            const auto authoredOrder = authoredOrders.at(macro.id);
            const auto hint = presentationHints.find(macro.id);
            binding.presentation = hint != presentationHints.end()
                ? hint->second : fallbackPresentation(macro, authoredOrder);
            finalizePresentation(binding.presentation, macro, authoredOrder);
            binding.publishedName = binding.presentation.authoredLabel;
            binding.minValue = macro.minValue;
            binding.maxValue = macro.maxValue;
            binding.defaultValue = macro.defaultValue;
            binding.exposedInPerformance = macro.exposedInPerformance;
            binding.renderTarget = classifyRenderTarget(macro);
            if (const auto* dspTarget = findDspTarget(macro))
            {
                if (request.dspControlLayout == nullptr)
                {
                    addFinding(result, PublishedMacroBindingFindingSeverity::error,
                               "published-macro-dsp-layout-missing", "authoredMacros." + macro.id,
                               "A structured DSP macro target requires a compiled DSP control layout.");
                    continue;
                }
                const auto control = std::find_if(request.dspControlLayout->controls.begin(),
                                                  request.dspControlLayout->controls.end(),
                                                  [&](const auto& descriptor)
                                                  {
                                                      return descriptor.slotId == dspTarget->dspSlotId
                                                          && descriptor.parameterId == dspTarget->dspParameterId;
                                                  });
                if (control == request.dspControlLayout->controls.end())
                {
                    addFinding(result, PublishedMacroBindingFindingSeverity::error,
                               "published-macro-dsp-target-missing", "authoredMacros." + macro.id,
                               "The structured DSP macro target is absent from the published graph.");
                    continue;
                }
                binding.renderTarget = PublishedMacroRenderTarget::dspControl;
                binding.dspControlIndex = control->controlIndex;
                binding.dspSlotId = dspTarget->dspSlotId;
                binding.dspParameterId = dspTarget->dspParameterId;
                binding.sourceMinimum = dspTarget->sourceMinimum;
                binding.sourceMaximum = dspTarget->sourceMaximum;
                binding.destinationMinimum = dspTarget->destinationMinimum;
                binding.destinationMaximum = dspTarget->destinationMaximum;
                std::string lawFindingCode;
                std::string lawFindingMessage;
                if (!compilePublishedControlLaw(dspTarget,
                                               controlLawLinearDbV1,
                                               binding.destinationMinimum,
                                               binding.destinationMaximum,
                                               binding.controlLaw,
                                               lawFindingCode,
                                               lawFindingMessage))
                {
                    addFinding(result, PublishedMacroBindingFindingSeverity::error,
                               std::move(lawFindingCode), "authoredMacros." + macro.id + ".controlLaw",
                               std::move(lawFindingMessage));
                    continue;
                }
            }
            else
            {
                std::string lawFindingCode;
                std::string lawFindingMessage;
                const auto fallbackLaw = binding.presentation.controlKind
                        == PublishedMacroControlKind::toggle
                    ? controlLawToggleV1 : controlLawLinearDbV1;
                if (!compilePublishedControlLaw(nullptr, fallbackLaw,
                                               binding.minValue, binding.maxValue,
                                               binding.controlLaw, lawFindingCode, lawFindingMessage))
                {
                    addFinding(result, PublishedMacroBindingFindingSeverity::error,
                               std::move(lawFindingCode), "authoredMacros." + macro.id,
                               std::move(lawFindingMessage));
                    continue;
                }
            }

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
            if (macro.exposedInPerformance)
                ++table->assignedExposedCount;
            else
                ++table->assignedHiddenCount;

            auto& callbackSlot = table->callbackView.slots[slot.slotIndex];
            callbackSlot.assigned = true;
            callbackSlot.renderTarget = binding.renderTarget;
            callbackSlot.minValue = binding.minValue;
            callbackSlot.maxValue = binding.maxValue;
            callbackSlot.publishedValue = binding.publishedValue;
            callbackSlot.dspControlIndex = binding.dspControlIndex;
            callbackSlot.sourceMinimum = binding.sourceMinimum;
            callbackSlot.sourceMaximum = binding.sourceMaximum;
            callbackSlot.destinationMinimum = binding.destinationMinimum;
            callbackSlot.destinationMaximum = binding.destinationMaximum;
            callbackSlot.controlLaw = binding.controlLaw;
        }
        table->bindings.push_back(std::move(binding));
    }

    for (const auto& macro : request.authoredMacros)
    {
        if (!assignedIds.count(macro.id))
        {
            table->unassignedStableAuthoredIds.push_back(macro.id);
            if (macro.exposedInPerformance)
            {
                ++table->unassignedExposedCount;
                addFinding(result, PublishedMacroBindingFindingSeverity::error,
                           "published-macro-exposed-slot-missing", "authoredMacros." + macro.id,
                           "The exposed authored macro has no compatible published host slot.");
            }
            else
            {
                ++table->unassignedHiddenCount;
                addFinding(result, PublishedMacroBindingFindingSeverity::warning,
                           "published-macro-unassigned", "authoredMacros." + macro.id,
                           "The authored macro has no compatible fixed host slot and remains unassigned.");
            }
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

    if (std::any_of(result.findings.begin(), result.findings.end(), [](const auto& finding)
        { return finding.severity == PublishedMacroBindingFindingSeverity::error; }))
        return result;

    result.built = true;
    result.table = std::move(table);
    return result;
}
} // namespace drs::engine
