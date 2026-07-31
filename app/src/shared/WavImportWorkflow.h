#pragma once

#include "shared/WavImportService.h"

#include <juce_gui_extra/juce_gui_extra.h>

#include <cstdint>
#include <cstddef>
#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

namespace drs::app
{
struct PreparedWavImportItem
{
    std::string sourcePath;
    std::vector<drs::engine::AuthoringImportFinding> findings;
    std::vector<std::string> issues;
    drs::engine::AuthoringImportZoneSuggestion suggestedZone;
    bool accepted = false;
    bool warning = false;
};

struct WavImportFinalizationItem
{
    std::string stagedPath;
    std::string finalPath;
    std::string sampleSourceId;
};

struct WavImportManualRootPrompt
{
    std::string sourcePath;
    std::string sourceDisplayName;
    int initialRootKey = 60;
};

struct PreparedWavImportBatch
{
    drs::engine::RuntimeProjectModel currentProject;
    std::string selectedGroupId;
    std::vector<PreparedWavImportItem> items;
    std::unordered_set<std::string> usedSampleSourceIds;
    std::unordered_set<std::string> usedZoneIds;
    std::vector<WavImportFinalizationItem> availableFinalizationItems;
    std::vector<drs::engine::RuntimeProjectSampleSource> importedSampleSources;
    std::vector<drs::engine::RuntimeProjectZoneDefinition> importedZones;
    std::vector<WavImportFinalizationItem> pendingFinalizationItems;
    std::size_t queuedItemCount = 0;
    std::size_t warningCount = 0;
    std::size_t skippedCount = 0;
    std::size_t itemIndex = 0;
    std::vector<std::string> details;
    std::optional<WavImportManualRootPrompt> pendingManualRoot;
    std::optional<PreparedWavImportItem> pendingManualItem;
    std::optional<drs::engine::RuntimeProjectZoneDefinition> pendingManualZone;
};

struct PreparedWavImportCommit
{
    std::vector<drs::engine::RuntimeProjectSampleSource> sampleSources;
    std::vector<drs::engine::RuntimeProjectZoneDefinition> zones;
    std::vector<WavImportFinalizationItem> finalizationItems;
    std::size_t importedCount = 0;
    std::size_t warningCount = 0;
    std::size_t skippedCount = 0;
    std::vector<std::string> details;
};

PreparedWavImportBatch prepareWavImportBatchFromCompletion(
    const WavImportCompletionPayload& completion,
    const drs::engine::RuntimeProjectModel& currentProject,
    const std::string& selectedGroupId);
void continuePreparedWavImportBatch(PreparedWavImportBatch& batch);
void resolvePreparedWavImportManualRoot(PreparedWavImportBatch& batch,
                                        std::optional<int> selectedRootKey);
bool hasPreparedWavImportCommit(const PreparedWavImportBatch& batch) noexcept;
PreparedWavImportCommit takePreparedWavImportCommit(PreparedWavImportBatch&& batch);
bool finalizePreparedWavImportCommit(PreparedWavImportCommit& commit, std::vector<std::string>& issues);
void rollbackPreparedWavImportCommit(PreparedWavImportCommit& commit) noexcept;
juce::String buildWavImportSummaryMessage(std::size_t importedCount,
                                          std::size_t warningCount,
                                          std::size_t skippedCount,
                                          const std::vector<std::string>& details);
} // namespace drs::app
