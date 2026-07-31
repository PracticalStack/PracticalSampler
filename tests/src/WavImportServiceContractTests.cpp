#include "shared/WavImportService.h"

#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

namespace
{
void require(const bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}
} // namespace

int main()
{
    using namespace drs::app;

    try
    {
        using Stage = WavImportBatchStage;
        require(isWavImportBatchStageTransitionAllowed(Stage::idle, Stage::queued)
                    && isWavImportBatchStageTransitionAllowed(Stage::queued, Stage::staging)
                    && isWavImportBatchStageTransitionAllowed(Stage::staging, Stage::inspecting)
                    && isWavImportBatchStageTransitionAllowed(Stage::inspecting, Stage::completed)
                    && isWavImportBatchStageTransitionAllowed(Stage::completed, Stage::consumed),
                "The ordinary WAV import service lifecycle must remain executable.");
        require(isWavImportBatchStageTransitionAllowed(Stage::queued, Stage::canceled)
                    && isWavImportBatchStageTransitionAllowed(Stage::staging, Stage::superseded)
                    && isWavImportBatchStageTransitionAllowed(Stage::inspecting, Stage::failed),
                "Queued and in-flight WAV work must have explicit terminal exits.");
        require(!isWavImportBatchStageTransitionAllowed(Stage::idle, Stage::completed)
                    && !isWavImportBatchStageTransitionAllowed(Stage::queued, Stage::inspecting)
                    && !isWavImportBatchStageTransitionAllowed(Stage::canceled, Stage::completed),
                "Invalid WAV import lifecycle shortcuts must be rejected.");

        WavImportRequestIdentity identity;
        identity.ownerId = 41;
        identity.generation = 7;
        identity.projectId = "wav-project";
        identity.baseRevision = 12;
        identity.contentRootPath = "content/root";
        identity.selectedGroupId = "pad-group";

        WavImportItemProgress progress;
        progress.itemId = "wav-item-1";
        progress.sourcePath = "Samples/Input.wav";
        progress.stagedPath = "Samples/.staging/wav-item-1/Input.wav";
        progress.stage = WavImportItemStage::fingerprinting;
        progress.bytesProcessed = 4096;
        progress.totalBytes = 16384;
        progress.fingerprintBytesProcessed = 4096;
        progress.fingerprintTotalBytes = 16384;
        progress.status = "Fingerprinting source sample";

        WavImportCompletionItem completionItem;
        completionItem.itemId = progress.itemId;
        completionItem.sourcePath = progress.sourcePath;
        completionItem.stagedPath = progress.stagedPath;
        completionItem.finalPath = "Samples/Input.wav";
        completionItem.stage = WavImportItemStage::ready;
        completionItem.sourceBytes = 16384;
        completionItem.copiedBytes = 16384;
        completionItem.fingerprint.sourcePath = progress.sourcePath;
        completionItem.fingerprint.fingerprinted = true;
        completionItem.fingerprint.fingerprintHex = "abc123";
        completionItem.inspection.sourcePath = progress.sourcePath;
        completionItem.inspection.inspected = true;
        completionItem.inspection.accepted = true;
        completionItem.inspection.metadata.sourcePath = progress.sourcePath;
        completionItem.inspection.metadata.sourceChecksumHex = "abc123";

        auto completion = std::make_shared<WavImportCompletionPayload>();
        completion->identity = identity;
        completion->disposition = WavImportTerminalDisposition::completed;
        completion->status = "WAV batch completed";
        completion->totalItemCount = 1;
        completion->successfulItemCount = 1;
        completion->totalBytesProcessed = completionItem.copiedBytes;
        completion->totalBytesExpected = completionItem.sourceBytes;
        completion->items.push_back(completionItem);

        static_assert(std::is_same_v<decltype(std::declval<WavImportCompletionItem>().inspection),
                                     drs::engine::SampleInspectionResult>,
                      "WAV completion items must retain metadata-only inspection results.");
        static_assert(!std::is_same_v<decltype(std::declval<WavImportCompletionItem>().inspection),
                                      drs::engine::SampleImportResult>,
                      "WAV completion items must not retain decoded sample-import results.");
        static_assert(std::is_same_v<decltype(std::declval<WavImportBatchSnapshot>().completion),
                                     std::shared_ptr<const WavImportCompletionPayload>>,
                      "WAV batch snapshots must expose immutable completion payloads.");

        WavImportBatchSnapshot snapshot;
        snapshot.identity = identity;
        snapshot.stage = WavImportBatchStage::completed;
        snapshot.terminalDisposition = WavImportTerminalDisposition::completed;
        snapshot.status = "WAV import completion ready";
        snapshot.totalItemCount = 1;
        snapshot.completedItemCount = 1;
        snapshot.successfulItemCount = 1;
        snapshot.totalBytesProcessed = 16384;
        snapshot.totalBytesExpected = 16384;
        snapshot.items.push_back(progress);
        snapshot.completion = completion;

        require(snapshot.identity.ownerId == 41
                    && snapshot.identity.generation == 7
                    && snapshot.identity.projectId == "wav-project"
                    && snapshot.identity.baseRevision == 12,
                "WAV request identity must carry owner, generation, project, and base revision.");
        require(snapshot.completion != nullptr
                    && snapshot.completion->identity.selectedGroupId == "pad-group"
                    && snapshot.completion->items.front().inspection.accepted,
                "Immutable WAV completion payloads must retain group identity and metadata-only inspection facts.");
        require(snapshot.completion->items.front().inspection.metadata.sourceChecksumHex == "abc123",
                "Completion payloads must preserve the fingerprint-backed inspection metadata.");

        std::cout << "WAV import service contract tests passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "WAV import service contract tests failed: "
                  << exception.what() << std::endl;
        return 1;
    }
}
