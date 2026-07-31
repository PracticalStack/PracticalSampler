#include "shared/WavImportWorkflow.h"
#include "../support/WavImportTestSupport.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

namespace
{
namespace fs = std::filesystem;

void require(const bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

struct WorkflowFixture
{
    fs::path root;
    fs::path projectRoot;
    fs::path samplesDirectory;
    drs::tests::GeneratedWavImportBatchCorpus corpus;
    drs::engine::RuntimeProjectModel project;
};

WorkflowFixture makeFixture()
{
    const auto unique = std::to_string(
        static_cast<std::uint64_t>(std::chrono::steady_clock::now().time_since_epoch().count()));
    WorkflowFixture fixture;
    fixture.root = fs::temp_directory_path() / ("drs-wav-import-workflow-" + unique);
    const auto sourceDirectory = fixture.root / "source";
    fixture.projectRoot = fixture.root / "project";
    fixture.samplesDirectory = fixture.projectRoot / "Samples";
    fs::create_directories(fixture.samplesDirectory);
    fixture.corpus = drs::tests::createGeneratedWavImportBatchCorpus(sourceDirectory);
    fixture.project.projectId = "wav-workflow-project";
    fixture.project.displayName = "WAV Workflow Project";
    fixture.project.contentRootPath = fixture.projectRoot.generic_string();
    return fixture;
}

bool containsLineFragment(const std::vector<std::string>& lines, const std::string& fragment)
{
    return std::any_of(lines.begin(),
                       lines.end(),
                       [&](const auto& line)
                       {
                           return line.find(fragment) != std::string::npos;
                       });
}

const drs::engine::RuntimeProjectSampleSource& requireSampleSource(
    const std::vector<drs::engine::RuntimeProjectSampleSource>& sampleSources,
    const std::string& sampleSourceId)
{
    const auto iterator = std::find_if(sampleSources.begin(),
                                       sampleSources.end(),
                                       [&](const auto& sampleSource)
                                       {
                                           return sampleSource.id == sampleSourceId;
                                       });
    if (iterator == sampleSources.end())
        throw std::runtime_error("Sample source '" + sampleSourceId + "' was not found.");

    return *iterator;
}
} // namespace

int main()
{
    try
    {
        const auto completionFixture = makeFixture();
        const auto completionStageRoot = completionFixture.projectRoot / "Samples" / ".staging" / "wav-import-test";
        fs::create_directories(completionStageRoot);
        const auto cleanStagePath = completionStageRoot / completionFixture.corpus.cleanPath.filename();
        const auto ambiguousStagePath = completionStageRoot / completionFixture.corpus.ambiguousPath.filename();
        require(fs::copy_file(completionFixture.corpus.cleanPath,
                              cleanStagePath,
                              fs::copy_options::overwrite_existing),
                "Could not stage the clean completion fixture.");
        require(fs::copy_file(completionFixture.corpus.ambiguousPath,
                              ambiguousStagePath,
                              fs::copy_options::overwrite_existing),
                "Could not stage the ambiguous completion fixture.");

        drs::app::WavImportCompletionPayload completionPayload;
        completionPayload.identity.projectId = "wav-completion-workflow-project";
        completionPayload.identity.contentRootPath = completionFixture.projectRoot.generic_string();
        completionPayload.identity.selectedGroupId = "kit";
        completionPayload.totalItemCount = 3;

        drs::app::WavImportCompletionItem readyItem;
        readyItem.itemId = "ready-clean";
        readyItem.sourcePath = completionFixture.corpus.cleanPath.generic_string();
        readyItem.stagedPath = cleanStagePath.generic_string();
        readyItem.finalPath = (completionFixture.samplesDirectory / completionFixture.corpus.cleanPath.filename()).generic_string();
        readyItem.stage = drs::app::WavImportItemStage::ready;
        readyItem.findings = {};
        readyItem.suggestedZone = drs::engine::parseSampleFilenameHeuristics(readyItem.sourcePath).suggestedZone;

        drs::app::WavImportCompletionItem manualItem;
        manualItem.itemId = "manual-ambiguous";
        manualItem.sourcePath = completionFixture.corpus.ambiguousPath.generic_string();
        manualItem.stagedPath = ambiguousStagePath.generic_string();
        manualItem.finalPath = (completionFixture.samplesDirectory / completionFixture.corpus.ambiguousPath.filename()).generic_string();
        manualItem.stage = drs::app::WavImportItemStage::ready;
        {
            const auto heuristics = drs::engine::parseSampleFilenameHeuristics(manualItem.sourcePath);
            manualItem.findings = heuristics.findings;
            manualItem.suggestedZone = heuristics.suggestedZone;
        }

        drs::app::WavImportCompletionItem failedItem;
        failedItem.itemId = "failed-unsupported";
        failedItem.sourcePath = completionFixture.corpus.unsupportedPath.generic_string();
        failedItem.stagedPath = (completionStageRoot / completionFixture.corpus.unsupportedPath.filename()).generic_string();
        failedItem.finalPath = (completionFixture.samplesDirectory / completionFixture.corpus.unsupportedPath.filename()).generic_string();
        failedItem.stage = drs::app::WavImportItemStage::failed;
        failedItem.inspection.issues = { "Unsupported source sample format." };

        completionPayload.items = { readyItem, manualItem, failedItem };

        auto completionBatch = drs::app::prepareWavImportBatchFromCompletion(completionPayload,
                                                                             completionFixture.project,
                                                                             "kit");
        require(completionBatch.pendingManualRoot.has_value()
                    && completionBatch.pendingManualRoot->sourceDisplayName == "MysteryTexture.wav",
                "Completion-derived WAV workflow batches should preserve manual root-key prompts.");

        auto skippedManualBatch = drs::app::prepareWavImportBatchFromCompletion(completionPayload,
                                                                                completionFixture.project,
                                                                                "kit");
        require(skippedManualBatch.pendingManualRoot.has_value(),
                "Completion-derived WAV workflow batches should preserve manual prompts before a user skip.");
        drs::app::resolvePreparedWavImportManualRoot(skippedManualBatch, std::nullopt);
        require(drs::app::hasPreparedWavImportCommit(skippedManualBatch),
                "Completion-derived WAV workflow batches should still emit a commit when other files remain importable.");
        auto skippedManualCommit = drs::app::takePreparedWavImportCommit(std::move(skippedManualBatch));
        require(skippedManualCommit.importedCount == 1
                    && skippedManualCommit.finalizationItems.size() == 1,
                "Completion-derived WAV commits should finalize only the items that remain selected after a manual skip.");

        while (completionBatch.pendingManualRoot.has_value())
        {
            const auto prompt = *completionBatch.pendingManualRoot;
            drs::app::resolvePreparedWavImportManualRoot(
                completionBatch,
                prompt.sourceDisplayName == "MysteryTexture.wav"
                    ? std::optional<int>(62)
                    : std::optional<int>(prompt.initialRootKey));
        }
        require(drs::app::hasPreparedWavImportCommit(completionBatch),
                "Completion-derived WAV workflow batches should produce a commit payload after prompt resolution.");

        auto completionCommit = drs::app::takePreparedWavImportCommit(std::move(completionBatch));
        require(completionCommit.importedCount == 2
                    && completionCommit.finalizationItems.size() == 2,
                "Completion-derived WAV commits should preserve ready-item finalization mappings.");
        for (const auto& zone : completionCommit.zones)
        {
            require(zone.groupId == "kit",
                    "Completion-derived WAV commit preparation should stamp the selected group onto every imported zone.");
        }
        const auto completionSummary = drs::app::buildWavImportSummaryMessage(completionCommit.importedCount,
                                                                              completionCommit.warningCount,
                                                                              completionCommit.skippedCount,
                                                                              completionCommit.details);
        require(completionSummary.contains("Imported 2 files")
                    && completionSummary.contains("Warnings: "
                                                  + juce::String(static_cast<int>(completionCommit.warningCount)))
                    && completionSummary.contains("Skipped: "
                                                  + juce::String(static_cast<int>(completionCommit.skippedCount))),
                "Completion-derived WAV summary builder should preserve the shell-facing import counts.");
        std::vector<std::string> finalizationIssues;
        require(drs::app::finalizePreparedWavImportCommit(completionCommit, finalizationIssues),
                "Completion-derived WAV commits should finalize staged files into the project Samples folder.");
        const auto finalizedCompletionSources = std::count_if(
            completionCommit.sampleSources.begin(),
            completionCommit.sampleSources.end(),
            [&](const auto& sampleSource)
            {
                return sampleSource.path == readyItem.finalPath || sampleSource.path == manualItem.finalPath;
            });
        require(finalizationIssues.empty()
                    && finalizedCompletionSources == 2
                    && fs::exists(fs::path(readyItem.finalPath))
                    && fs::exists(fs::path(manualItem.finalPath))
                    && !fs::exists(cleanStagePath)
                    && !fs::exists(ambiguousStagePath),
                "Finalizing a completion-derived WAV commit should move ready staged files to their reserved final paths and update the commit sample paths.");
        require(requireSampleSource(completionCommit.sampleSources, readyItem.suggestedZone.sourceSampleId).path
                        == readyItem.finalPath
                    && requireSampleSource(completionCommit.sampleSources, manualItem.suggestedZone.sourceSampleId).path
                        == manualItem.finalPath,
                "Finalizing a completion-derived WAV commit should update the commit sample-source paths to the final files.");
        drs::app::rollbackPreparedWavImportCommit(completionCommit);
        require(fs::exists(cleanStagePath)
                    && fs::exists(ambiguousStagePath)
                    && !fs::exists(fs::path(readyItem.finalPath))
                    && !fs::exists(fs::path(manualItem.finalPath)),
                "Rolling back a finalized completion-derived WAV commit should restore staged files and clear final paths.");
        require(requireSampleSource(completionCommit.sampleSources, readyItem.suggestedZone.sourceSampleId).path
                        == cleanStagePath.generic_string()
                    && requireSampleSource(completionCommit.sampleSources, manualItem.suggestedZone.sourceSampleId).path
                        == ambiguousStagePath.generic_string(),
                "Rolling back a finalized completion-derived WAV commit should also restore the staged sample-source paths.");

        const auto finalizeFailureFixture = makeFixture();
        const auto finalizeFailureStageRoot
            = finalizeFailureFixture.projectRoot / "Samples" / ".staging" / "wav-import-finalize-failure";
        fs::create_directories(finalizeFailureStageRoot);
        const auto finalizeFailureStageOne
            = finalizeFailureStageRoot / finalizeFailureFixture.corpus.cleanPath.filename();
        const auto finalizeFailureStageTwo
            = finalizeFailureStageRoot / finalizeFailureFixture.corpus.cleanSiblingThreePath.filename();
        require(fs::copy_file(finalizeFailureFixture.corpus.cleanPath,
                              finalizeFailureStageOne,
                              fs::copy_options::overwrite_existing),
                "Could not stage the first finalize-failure fixture.");
        require(fs::copy_file(finalizeFailureFixture.corpus.cleanSiblingThreePath,
                              finalizeFailureStageTwo,
                              fs::copy_options::overwrite_existing),
                "Could not stage the second finalize-failure fixture.");

        drs::app::WavImportCompletionPayload finalizeFailurePayload;
        finalizeFailurePayload.identity.projectId = "wav-finalize-failure-project";
        finalizeFailurePayload.identity.contentRootPath = finalizeFailureFixture.projectRoot.generic_string();
        finalizeFailurePayload.identity.selectedGroupId = "kit";
        finalizeFailurePayload.totalItemCount = 2;

        drs::app::WavImportCompletionItem finalizeReadyOne;
        finalizeReadyOne.itemId = "finalize-ready-one";
        finalizeReadyOne.sourcePath = finalizeFailureFixture.corpus.cleanPath.generic_string();
        finalizeReadyOne.stagedPath = finalizeFailureStageOne.generic_string();
        finalizeReadyOne.finalPath = (finalizeFailureFixture.samplesDirectory
                                      / finalizeFailureFixture.corpus.cleanPath.filename()).generic_string();
        finalizeReadyOne.stage = drs::app::WavImportItemStage::ready;
        finalizeReadyOne.suggestedZone
            = drs::engine::parseSampleFilenameHeuristics(finalizeReadyOne.sourcePath).suggestedZone;

        drs::app::WavImportCompletionItem finalizeReadyTwo;
        finalizeReadyTwo.itemId = "finalize-ready-two";
        finalizeReadyTwo.sourcePath = finalizeFailureFixture.corpus.cleanSiblingThreePath.generic_string();
        finalizeReadyTwo.stagedPath = finalizeFailureStageTwo.generic_string();
        finalizeReadyTwo.finalPath = (finalizeFailureFixture.samplesDirectory
                                      / finalizeFailureFixture.corpus.cleanSiblingThreePath.filename()).generic_string();
        finalizeReadyTwo.stage = drs::app::WavImportItemStage::ready;
        finalizeReadyTwo.suggestedZone
            = drs::engine::parseSampleFilenameHeuristics(finalizeReadyTwo.sourcePath).suggestedZone;

        finalizeFailurePayload.items = { finalizeReadyOne, finalizeReadyTwo };
        auto finalizeFailureBatch = drs::app::prepareWavImportBatchFromCompletion(finalizeFailurePayload,
                                                                                  finalizeFailureFixture.project,
                                                                                  "kit");
        require(drs::app::hasPreparedWavImportCommit(finalizeFailureBatch),
                "Finalize-failure coverage should still prepare a commit from ready completion items.");
        auto finalizeFailureCommit = drs::app::takePreparedWavImportCommit(std::move(finalizeFailureBatch));
        require(finalizeFailureCommit.finalizationItems.size() == 2,
                "Finalize-failure coverage requires two ready staged files.");
        require(fs::create_directories(fs::path(finalizeReadyTwo.finalPath)),
                "Finalize-failure coverage could not create the blocking final-path directory.");

        std::vector<std::string> finalizeFailureIssues;
        require(!drs::app::finalizePreparedWavImportCommit(finalizeFailureCommit, finalizeFailureIssues),
                "A blocked second final-path rename should fail the completion finalization step.");
        require(!finalizeFailureIssues.empty(),
                "Finalize-failure coverage should publish an actionable finalization issue.");
        require(fs::exists(finalizeFailureStageOne)
                    && fs::exists(finalizeFailureStageTwo)
                    && !fs::exists(fs::path(finalizeReadyOne.finalPath))
                    && fs::is_directory(fs::path(finalizeReadyTwo.finalPath)),
                "Failed finalization should restore earlier moved files and leave only the injected blocker behind.");
        require(requireSampleSource(finalizeFailureCommit.sampleSources,
                                    finalizeReadyOne.suggestedZone.sourceSampleId).path
                        == finalizeFailureStageOne.generic_string()
                    && requireSampleSource(finalizeFailureCommit.sampleSources,
                                           finalizeReadyTwo.suggestedZone.sourceSampleId).path
                        == finalizeFailureStageTwo.generic_string(),
                "Failed finalization should restore the commit sample-source paths to the staged files.");

        const auto manualSequenceFixture = makeFixture();
        const auto manualSequenceStageRoot = manualSequenceFixture.projectRoot / "Samples" / ".staging" / "wav-import-sequence";
        fs::create_directories(manualSequenceStageRoot);
        const auto sequenceCleanStagePath = manualSequenceStageRoot / manualSequenceFixture.corpus.cleanPath.filename();
        const auto manualSequenceFirstStagePath = manualSequenceStageRoot / "MysteryTexture-skip.wav";
        const auto manualSequenceSecondStagePath = manualSequenceStageRoot / "MysteryTexture-keep.wav";
        require(fs::copy_file(manualSequenceFixture.corpus.cleanPath,
                              sequenceCleanStagePath,
                              fs::copy_options::overwrite_existing),
                "Could not stage the clean sequence fixture.");
        require(fs::copy_file(manualSequenceFixture.corpus.ambiguousPath,
                              manualSequenceFirstStagePath,
                              fs::copy_options::overwrite_existing),
                "Could not stage the first manual-sequence fixture.");
        require(fs::copy_file(manualSequenceFixture.corpus.ambiguousPath,
                              manualSequenceSecondStagePath,
                              fs::copy_options::overwrite_existing),
                "Could not stage the second manual-sequence fixture.");

        drs::app::WavImportCompletionPayload manualSequencePayload;
        manualSequencePayload.identity.projectId = "wav-manual-sequence-project";
        manualSequencePayload.identity.contentRootPath = manualSequenceFixture.projectRoot.generic_string();
        manualSequencePayload.identity.selectedGroupId = "kit";
        manualSequencePayload.totalItemCount = 3;

        drs::app::WavImportCompletionItem sequenceReadyItem;
        sequenceReadyItem.itemId = "sequence-clean";
        sequenceReadyItem.sourcePath = manualSequenceFixture.corpus.cleanPath.generic_string();
        sequenceReadyItem.stagedPath = sequenceCleanStagePath.generic_string();
        sequenceReadyItem.finalPath = (manualSequenceFixture.samplesDirectory
                                       / manualSequenceFixture.corpus.cleanPath.filename()).generic_string();
        sequenceReadyItem.stage = drs::app::WavImportItemStage::ready;
        sequenceReadyItem.suggestedZone = drs::engine::parseSampleFilenameHeuristics(sequenceReadyItem.sourcePath).suggestedZone;

        const auto ambiguousHeuristics = drs::engine::parseSampleFilenameHeuristics(
            manualSequenceFixture.corpus.ambiguousPath.generic_string());

        drs::app::WavImportCompletionItem skippedManualSequenceItem;
        skippedManualSequenceItem.itemId = "sequence-manual-skip";
        skippedManualSequenceItem.sourcePath = manualSequenceFixture.corpus.ambiguousPath.generic_string();
        skippedManualSequenceItem.stagedPath = manualSequenceFirstStagePath.generic_string();
        skippedManualSequenceItem.finalPath = (manualSequenceFixture.samplesDirectory / "MysteryTexture-skip.wav").generic_string();
        skippedManualSequenceItem.stage = drs::app::WavImportItemStage::ready;
        skippedManualSequenceItem.findings = ambiguousHeuristics.findings;
        skippedManualSequenceItem.suggestedZone = ambiguousHeuristics.suggestedZone;

        drs::app::WavImportCompletionItem acceptedManualSequenceItem;
        acceptedManualSequenceItem.itemId = "sequence-manual-keep";
        acceptedManualSequenceItem.sourcePath = manualSequenceFixture.corpus.ambiguousPath.generic_string();
        acceptedManualSequenceItem.stagedPath = manualSequenceSecondStagePath.generic_string();
        acceptedManualSequenceItem.finalPath = (manualSequenceFixture.samplesDirectory / "MysteryTexture-keep.wav").generic_string();
        acceptedManualSequenceItem.stage = drs::app::WavImportItemStage::ready;
        acceptedManualSequenceItem.findings = ambiguousHeuristics.findings;
        acceptedManualSequenceItem.suggestedZone = ambiguousHeuristics.suggestedZone;

        manualSequencePayload.items = { skippedManualSequenceItem, acceptedManualSequenceItem, sequenceReadyItem };

        auto manualSequenceBatch = drs::app::prepareWavImportBatchFromCompletion(manualSequencePayload,
                                                                                 manualSequenceFixture.project,
                                                                                 "kit");
        require(manualSequenceBatch.pendingManualRoot.has_value(),
                "Completion-derived WAV workflow batches should stop on the first manual decision in a sequence.");
        drs::app::resolvePreparedWavImportManualRoot(manualSequenceBatch, std::nullopt);
        require(manualSequenceBatch.pendingManualRoot.has_value(),
                "Skipping one completion-derived manual root decision should resume the remaining decision sequence.");
        drs::app::resolvePreparedWavImportManualRoot(manualSequenceBatch, 64);
        require(drs::app::hasPreparedWavImportCommit(manualSequenceBatch),
                "Completion-derived WAV workflow batches should still produce a commit after a manual skip and resume sequence.");

        auto manualSequenceCommit = drs::app::takePreparedWavImportCommit(std::move(manualSequenceBatch));
        require(manualSequenceCommit.importedCount == 2
                    && manualSequenceCommit.finalizationItems.size() == 2
                    && manualSequenceCommit.skippedCount >= 1
                    && containsLineFragment(manualSequenceCommit.details, "Skipped")
                    && containsLineFragment(manualSequenceCommit.details, "Selected root key"),
                "Completion-derived WAV manual root sequences should allow per-item skip/accept decisions and continue with the remaining imports.");

        std::cout << "WAV import workflow tests passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "WAV import workflow tests failed: "
                  << exception.what() << std::endl;
        return 1;
    }
}
