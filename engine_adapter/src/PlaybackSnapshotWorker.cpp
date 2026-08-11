#include "drs/engine/PlaybackSnapshotWorker.h"
#include "drs/engine/PreparedPlayback.h"
#include "drs/engine/PublishedMacroBinding.h"

#include <algorithm>
#include <cmath>
#include <unordered_set>

namespace drs::engine
{
namespace
{
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
        const auto overflowIndex = std::find_if(
            authoring.macros.begin(), authoring.macros.end(),
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
} // namespace

PlaybackSnapshotWorker::PlaybackSnapshotWorker()
    : worker([this] { run(); })
{
}

PlaybackSnapshotWorker::~PlaybackSnapshotWorker()
{
    {
        const std::lock_guard<std::mutex> lock(mutex);
        stopRequested = true;
        queued.clear();
    }
    condition.notify_all();
    if (worker.joinable())
        worker.join();
}

bool PlaybackSnapshotWorker::submit(PlaybackSnapshotWorkerRequest request)
{
    if (!request.snapshotRequest.accepted || request.contractRequestId == 0 || request.project == nullptr)
        return false;

    {
        const std::lock_guard<std::mutex> lock(mutex);
        if (stopRequested)
            return false;
        queued.erase(std::remove_if(queued.begin(), queued.end(), [&](const auto& pending)
        {
            return pending.lane == request.lane;
        }), queued.end());
        queued.push_back(std::move(request));
    }
    condition.notify_one();
    return true;
}

std::vector<PlaybackSnapshotWorkerCompletion> PlaybackSnapshotWorker::drainCompleted()
{
    const std::lock_guard<std::mutex> lock(mutex);
    auto result = std::move(completed);
    completed.clear();
    return result;
}

void PlaybackSnapshotWorker::cancelLane(const PlaybackSnapshotWorkLane lane)
{
    const std::lock_guard<std::mutex> lock(mutex);
    queued.erase(std::remove_if(queued.begin(), queued.end(), [&](const auto& pending)
    {
        return pending.lane == lane;
    }), queued.end());
    completed.erase(std::remove_if(completed.begin(), completed.end(), [&](const auto& result)
    {
        return result.lane == lane;
    }), completed.end());
}

void PlaybackSnapshotWorker::run()
{
    for (;;)
    {
        PlaybackSnapshotWorkerRequest request;
        {
            std::unique_lock<std::mutex> lock(mutex);
            condition.wait(lock, [&] { return stopRequested || !queued.empty(); });
            if (stopRequested)
                return;

            const auto performance = std::find_if(queued.begin(), queued.end(), [](const auto& candidate)
            {
                return candidate.lane == PlaybackSnapshotWorkLane::performance;
            });
            const auto selected = performance != queued.end() ? performance : queued.begin();
            request = std::move(*selected);
            queued.erase(selected);
        }

        PlaybackSnapshotBuilder builder;
        auto result = builder.buildSnapshot(request.snapshotRequest, *request.project);
        if (request.lane == PlaybackSnapshotWorkLane::preview)
            result = scopePlaybackSnapshotForPreparation(result, request.preparationScope);

        PlaybackSnapshotWorkerCompletion completion;
        completion.lane = request.lane;
        completion.contractRequestId = request.contractRequestId;
        completion.snapshotResult = std::move(result);
        if (request.lane == PlaybackSnapshotWorkLane::performance)
        {
            completion.macroSchemaDigest = computePlaybackSnapshotMacroSchemaDigest(
                completion.snapshotResult.snapshot);
            const auto preflight = preflightPublishedMacros(request.project->authoring);
            completion.exposedMacroCount = preflight.exposedCount;
            completion.hiddenMacroCount = preflight.hiddenCount;
            completion.publishPreflightFinding = preflight.finding;
        }

        {
            const std::lock_guard<std::mutex> lock(mutex);
            if (stopRequested)
                return;
            completed.push_back(std::move(completion));
        }
    }
}
} // namespace drs::engine
