#include "shared/WavImportWorkflow.h"

#include <juce_audio_basics/juce_audio_basics.h>

#include <algorithm>
#include <cctype>
#include <filesystem>

namespace drs::app
{
namespace
{
namespace fs = std::filesystem;

std::string slugifyText(const std::string& text)
{
    std::string slug;
    bool previousWasDash = false;

    for (const auto character : text)
    {
        if (std::isalnum(static_cast<unsigned char>(character)) != 0)
        {
            slug.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(character))));
            previousWasDash = false;
        }
        else if (!previousWasDash)
        {
            slug.push_back('-');
            previousWasDash = true;
        }
    }

    while (!slug.empty() && slug.back() == '-')
        slug.pop_back();

    return slug.empty() ? "imported-sample" : slug;
}

std::string makeUniqueId(const std::string& preferredId, std::unordered_set<std::string>& usedIds)
{
    auto candidate = slugifyText(preferredId);
    auto suffix = 2;

    while (!usedIds.insert(candidate).second)
        candidate = slugifyText(preferredId) + "-" + std::to_string(suffix++);

    return candidate;
}

void queuePendingFinalization(PreparedWavImportBatch& batch,
                              const PreparedWavImportItem& item,
                              const std::string& sampleSourceId)
{
    const auto iterator = std::find_if(batch.availableFinalizationItems.begin(),
                                       batch.availableFinalizationItems.end(),
                                       [&](const WavImportFinalizationItem& finalizationItem)
                                       {
                                           return finalizationItem.stagedPath == item.sourcePath
                                               || finalizationItem.finalPath == item.sourcePath;
                                       });
    if (iterator == batch.availableFinalizationItems.end())
        return;

    auto finalizationItem = *iterator;
    finalizationItem.sampleSourceId = sampleSourceId;
    batch.pendingFinalizationItems.push_back(std::move(finalizationItem));
    batch.availableFinalizationItems.erase(iterator);
}

void appendImportedItem(PreparedWavImportBatch& batch,
                        const PreparedWavImportItem& item,
                        drs::engine::RuntimeProjectZoneDefinition zone)
{
    auto sampleSourceId = makeUniqueId(item.suggestedZone.sourceSampleId.empty()
                                           ? fs::path(item.sourcePath).stem().generic_string()
                                           : item.suggestedZone.sourceSampleId,
                                       batch.usedSampleSourceIds);

    drs::engine::RuntimeProjectSampleSource sampleSource;
    sampleSource.id = sampleSourceId;
    sampleSource.path = item.sourcePath;
    sampleSource.role = item.suggestedZone.zone.articulationId.empty()
        ? "imported"
        : "imported-" + item.suggestedZone.zone.articulationId;

    zone.id = makeUniqueId(zone.id.empty() ? sampleSourceId : zone.id, batch.usedZoneIds);
    zone.sampleSourceId = sampleSourceId;
    if (!batch.selectedGroupId.empty())
        zone.groupId = batch.selectedGroupId;

    queuePendingFinalization(batch, item, sampleSourceId);
    batch.importedSampleSources.push_back(std::move(sampleSource));
    batch.importedZones.push_back(std::move(zone));
}

bool hasWarningState(const PreparedWavImportItem& item)
{
    if (item.suggestedZone.rootKeySource == "manual")
        return true;

    return std::any_of(item.findings.begin(),
                       item.findings.end(),
                       [](const auto& finding)
                       {
                           return finding.severity == drs::engine::AuthoringImportFindingSeverity::warning;
                       });
}

std::string describeSkippedItem(const PreparedWavImportItem& item)
{
    if (!item.issues.empty())
        return item.issues.front();

    if (!item.findings.empty())
    {
        const auto& finding = item.findings.front();
        if (!finding.detail.empty())
            return finding.summary + ": " + finding.detail;
        if (!finding.summary.empty())
            return finding.summary;
    }

    return "Skipped " + fs::path(item.sourcePath).filename().generic_string() + ".";
}
} // namespace

PreparedWavImportBatch prepareWavImportBatchFromCompletion(
    const WavImportCompletionPayload& completion,
    const drs::engine::RuntimeProjectModel& currentProject,
    const std::string& selectedGroupId)
{
    PreparedWavImportBatch batch;
    batch.currentProject = currentProject;
    batch.selectedGroupId = selectedGroupId;
    batch.queuedItemCount = completion.totalItemCount;

    for (const auto& sampleSource : batch.currentProject.sampleSources)
        batch.usedSampleSourceIds.insert(sampleSource.id);
    for (const auto& zone : batch.currentProject.authoring.zones)
        batch.usedZoneIds.insert(zone.id);

    batch.items.reserve(completion.items.size());
    for (const auto& item : completion.items)
    {
        PreparedWavImportItem preparedItem;
        preparedItem.sourcePath = item.stagedPath.empty() ? item.finalPath : item.stagedPath;
        preparedItem.findings = item.findings;
        preparedItem.issues = item.inspection.issues;
        preparedItem.suggestedZone = item.suggestedZone;
        preparedItem.accepted = item.stage == WavImportItemStage::ready;
        preparedItem.warning = preparedItem.accepted && hasWarningState(preparedItem);
        batch.items.push_back(std::move(preparedItem));
        if (item.stage == WavImportItemStage::ready
            && !item.stagedPath.empty()
            && !item.finalPath.empty()
            && item.stagedPath != item.finalPath)
        {
            batch.availableFinalizationItems.push_back(
                { item.stagedPath, item.finalPath, {} });
        }
    }

    continuePreparedWavImportBatch(batch);
    return batch;
}

void continuePreparedWavImportBatch(PreparedWavImportBatch& batch)
{
    if (batch.pendingManualRoot.has_value())
        return;

    while (batch.itemIndex < batch.items.size())
    {
        const auto& item = batch.items[batch.itemIndex++];

        if (!item.accepted)
        {
            ++batch.skippedCount;
            batch.details.push_back(describeSkippedItem(item));
            continue;
        }

        auto zone = item.suggestedZone.zone;
        if (item.suggestedZone.rootKeySource == "manual")
        {
            batch.pendingManualRoot = WavImportManualRootPrompt {
                item.sourcePath,
                fs::path(item.sourcePath).filename().generic_string(),
                zone.rootKey,
            };
            batch.pendingManualItem = item;
            batch.pendingManualZone = zone;
            return;
        }

        appendImportedItem(batch, item, zone);
        if (item.warning)
        {
            ++batch.warningCount;
            if (!item.findings.empty())
                batch.details.push_back(item.findings.front().summary + ": " + item.findings.front().detail);
        }
    }
}

void resolvePreparedWavImportManualRoot(PreparedWavImportBatch& batch,
                                        std::optional<int> selectedRootKey)
{
    if (!batch.pendingManualRoot.has_value() || !batch.pendingManualItem.has_value()
        || !batch.pendingManualZone.has_value())
    {
        return;
    }

    const auto sourceDisplayName = batch.pendingManualRoot->sourceDisplayName;
    const auto item = *batch.pendingManualItem;
    auto zone = *batch.pendingManualZone;

    batch.pendingManualRoot.reset();
    batch.pendingManualItem.reset();
    batch.pendingManualZone.reset();

    if (!selectedRootKey.has_value())
    {
        ++batch.skippedCount;
        batch.details.push_back("Skipped " + sourceDisplayName + " because its root key was not confirmed.");
        continuePreparedWavImportBatch(batch);
        return;
    }

    zone.rootKey = *selectedRootKey;
    appendImportedItem(batch, item, zone);
    ++batch.warningCount;
    batch.details.push_back("Selected root key "
                            + juce::MidiMessage::getMidiNoteName(zone.rootKey, true, true, 4).toStdString()
                            + " for " + sourceDisplayName + ".");
    continuePreparedWavImportBatch(batch);
}

bool hasPreparedWavImportCommit(const PreparedWavImportBatch& batch) noexcept
{
    return !batch.pendingManualRoot.has_value()
        && !batch.importedSampleSources.empty()
        && !batch.importedZones.empty();
}

PreparedWavImportCommit takePreparedWavImportCommit(PreparedWavImportBatch&& batch)
{
    PreparedWavImportCommit commit;
    commit.importedCount = batch.importedSampleSources.size();
    commit.warningCount = batch.warningCount;
    commit.skippedCount = batch.skippedCount;
    commit.details = std::move(batch.details);
    commit.sampleSources = std::move(batch.importedSampleSources);
    commit.zones = std::move(batch.importedZones);
    commit.finalizationItems = std::move(batch.pendingFinalizationItems);
    drs::engine::reconcileBatchInferredRoundRobinDescriptors(commit.zones);
    return commit;
}

bool finalizePreparedWavImportCommit(PreparedWavImportCommit& commit, std::vector<std::string>& issues)
{
    struct CompletedMove
    {
        WavImportFinalizationItem item;
        std::string previousSampleSourcePath;
    };

    std::vector<CompletedMove> completedMoves;
    for (const auto& item : commit.finalizationItems)
    {
        std::error_code error;
        fs::rename(item.stagedPath, item.finalPath, error);
        if (error)
        {
            issues.push_back("Could not finalize " + fs::path(item.finalPath).filename().generic_string()
                             + " into the project Samples folder.");
            for (auto iterator = completedMoves.rbegin(); iterator != completedMoves.rend(); ++iterator)
            {
                std::error_code rollbackError;
                fs::rename(iterator->item.finalPath, iterator->item.stagedPath, rollbackError);

                const auto sampleIterator = std::find_if(commit.sampleSources.begin(),
                                                         commit.sampleSources.end(),
                                                         [&](const auto& sampleSource)
                                                         {
                                                             return sampleSource.id
                                                                 == iterator->item.sampleSourceId;
                                                         });
                if (sampleIterator != commit.sampleSources.end())
                    sampleIterator->path = iterator->previousSampleSourcePath;
            }
            return false;
        }

        const auto sampleIterator = std::find_if(commit.sampleSources.begin(),
                                                 commit.sampleSources.end(),
                                                 [&](const auto& sampleSource)
                                                 {
                                                     return sampleSource.id == item.sampleSourceId;
                                                 });
        auto previousSampleSourcePath = std::string {};
        if (sampleIterator != commit.sampleSources.end())
        {
            previousSampleSourcePath = sampleIterator->path;
            sampleIterator->path = item.finalPath;
        }

        completedMoves.push_back({ item, previousSampleSourcePath });
    }

    return true;
}

void rollbackPreparedWavImportCommit(PreparedWavImportCommit& commit) noexcept
{
    for (auto iterator = commit.finalizationItems.rbegin(); iterator != commit.finalizationItems.rend(); ++iterator)
    {
        std::error_code error;
        if (fs::exists(iterator->finalPath, error))
            fs::rename(iterator->finalPath, iterator->stagedPath, error);

        const auto sampleIterator = std::find_if(commit.sampleSources.begin(),
                                                 commit.sampleSources.end(),
                                                 [&](const auto& sampleSource)
                                                 {
                                                     return sampleSource.id == iterator->sampleSourceId;
                                                 });
        if (sampleIterator != commit.sampleSources.end())
            sampleIterator->path = iterator->stagedPath;
    }
}

juce::String buildWavImportSummaryMessage(std::size_t importedCount,
                                          std::size_t warningCount,
                                          std::size_t skippedCount,
                                          const std::vector<std::string>& details)
{
    juce::String summary("Imported " + juce::String(static_cast<int>(importedCount)) + " file");
    if (importedCount != 1)
        summary += "s";

    summary += " into the current project.";
    summary += "\nWarnings: " + juce::String(static_cast<int>(warningCount));
    summary += "\nSkipped: " + juce::String(static_cast<int>(skippedCount));

    if (!details.empty())
    {
        summary += "\n\nDetails:";
        for (std::size_t index = 0; index < details.size() && index < 8; ++index)
            summary += "\n- " + juce::String::fromUTF8(details[index].c_str());
    }

    return summary;
}
} // namespace drs::app
