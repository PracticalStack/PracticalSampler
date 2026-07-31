#include "shared/WavImportService.h"
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
using namespace std::chrono_literals;

void require(const bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

struct AnalysisFixture
{
    fs::path root;
    drs::tests::GeneratedWavImportBatchCorpus corpus;
    drs::app::WavImportRequest request;
};

AnalysisFixture makeFixture()
{
    const auto unique = std::to_string(
        static_cast<std::uint64_t>(std::chrono::steady_clock::now().time_since_epoch().count()));
    AnalysisFixture fixture;
    fixture.root = fs::temp_directory_path() / ("drs-wav-import-analysis-" + unique);
    const auto sourceDirectory = fixture.root / "source";
    const auto contentRoot = fixture.root / "project";
    fs::create_directories(contentRoot);
    fixture.corpus = drs::tests::createGeneratedWavImportBatchCorpus(sourceDirectory);
    fixture.request.sourcePaths = {
        fixture.corpus.cleanPath.generic_string(),
        fixture.corpus.policyWarningPath.generic_string(),
        fixture.corpus.unsupportedPath.generic_string(),
    };
    fixture.request.projectId = "wav-analysis-project";
    fixture.request.baseRevision = 19;
    fixture.request.contentRootPath = contentRoot.generic_string();
    fixture.request.selectedGroupId = "pads";
    return fixture;
}

bool hasFinding(const drs::app::WavImportCompletionItem& item, const std::string& code)
{
    return std::any_of(item.findings.begin(),
                       item.findings.end(),
                       [&](const auto& finding)
                       {
                           return finding.code == code;
                       });
}
} // namespace

int main()
{
    using namespace drs::app;

    try
    {
        const auto fixture = makeFixture();
        WavImportServiceOptions options;
        options.copyChunkBytes = 64;
        options.fingerprintChunkBytes = 1024;

        WavImportService service(std::move(options));
        auto client = service.openClient();
        const auto accepted = client.submit(fixture.request);
        require(accepted.disposition == WavImportSubmitDisposition::accepted,
                "The WAV analysis request must be accepted.");
        require(client.waitForTerminal(5s),
                "The WAV analysis request must reach a terminal state.");

        const auto snapshot = client.getSnapshot();
        require(snapshot != nullptr
                    && snapshot->stage == WavImportBatchStage::completed
                    && snapshot->terminalDisposition == WavImportTerminalDisposition::partiallyCompleted
                    && snapshot->completion != nullptr
                    && snapshot->copyDurationMicros > 0
                    && snapshot->fingerprintDurationMicros > 0
                    && snapshot->inspectionDurationMicros > 0
                    && snapshot->totalDurationMicros > 0,
                "Mixed WAV analysis input should complete with a partially completed immutable payload.");

        const auto& completion = *snapshot->completion;
        require(completion.totalItemCount == 3
                    && completion.successfulItemCount == 2
                    && completion.failedItemCount == 1
                    && completion.warningItemCount >= 1,
                "Mixed WAV analysis should preserve successful, failed, and warning item counts.");

        const auto cleanIterator = std::find_if(completion.items.begin(),
                                                completion.items.end(),
                                                [&](const auto& item)
                                                {
                                                    return item.sourcePath == fixture.corpus.cleanPath.generic_string();
                                                });
        require(cleanIterator != completion.items.end(),
                "The clean WAV source must appear in the immutable completion payload.");
        require(cleanIterator->fingerprint.fingerprinted
                    && cleanIterator->inspection.accepted
                    && cleanIterator->inspection.metadata.sourceChecksumHex
                        == cleanIterator->fingerprint.fingerprintHex
                    && !cleanIterator->filenameTokens.empty()
                    && cleanIterator->suggestedZone.suggested
                    && cleanIterator->suggestedZone.zone.sampleSourceId == "pad-sustain-c4-vel064-rr2"
                    && cleanIterator->copyDurationMicros > 0
                    && cleanIterator->fingerprintDurationMicros > 0
                    && cleanIterator->inspectionDurationMicros > 0
                    && cleanIterator->totalDurationMicros >= cleanIterator->copyDurationMicros
                    && hasFinding(*cleanIterator, "round_robin.detected"),
                "Successful WAV items must preserve real fingerprint, metadata inspection, and filename inference.");
        require(fs::exists(fs::path(cleanIterator->stagedPath)),
                "Successful staged WAV files must remain available until a later commit or consume step.");

        const auto warningIterator = std::find_if(completion.items.begin(),
                                                  completion.items.end(),
                                                  [&](const auto& item)
                                                  {
                                                      return item.sourcePath
                                                          == fixture.corpus.policyWarningPath.generic_string();
                                                  });
        require(warningIterator != completion.items.end()
                    && warningIterator->stage == WavImportItemStage::ready
                    && hasFinding(*warningIterator, "import.policy_warning"),
                "Portability-warning WAV items must surface warning findings without losing readiness.");

        const auto failedIterator = std::find_if(completion.items.begin(),
                                                 completion.items.end(),
                                                 [&](const auto& item)
                                                 {
                                                     return item.sourcePath
                                                         == fixture.corpus.unsupportedPath.generic_string();
                                                 });
        require(failedIterator != completion.items.end()
                    && failedIterator->stage == WavImportItemStage::failed
                    && !failedIterator->inspection.accepted
                    && !failedIterator->fingerprint.fingerprintHex.empty(),
                "Unsupported staged sources must fail inspection while preserving the staged fingerprint result.");
        require(!fs::exists(fs::path(failedIterator->stagedPath)),
                "Failed staged WAV artifacts must be cleaned up once the batch reaches a terminal state.");

        const auto warningProgress = std::find_if(snapshot->items.begin(),
                                                  snapshot->items.end(),
                                                  [&](const auto& item)
                                                  {
                                                      return item.sourcePath
                                                          == fixture.corpus.policyWarningPath.generic_string();
                                                  });
        require(warningProgress != snapshot->items.end()
                    && warningProgress->warningCount >= 1
                    && warningProgress->fingerprintBytesProcessed == warningProgress->fingerprintTotalBytes
                    && warningProgress->copyDurationMicros > 0
                    && warningProgress->fingerprintDurationMicros > 0
                    && warningProgress->inspectionDurationMicros > 0
                    && warningProgress->totalDurationMicros >= warningProgress->copyDurationMicros,
                "Published item progress must retain warning counts, byte totals, and real durations.");

        const auto metrics = service.getMetrics();
        require(metrics.completedCount == 1
                    && metrics.lastTerminalGeneration == accepted.identity.generation
                    && metrics.lastCopyDurationMicros > 0
                    && metrics.lastFingerprintDurationMicros > 0
                    && metrics.lastInspectionDurationMicros > 0
                    && metrics.lastBatchDurationMicros > 0
                    && metrics.averageBatchDurationMicros > 0
                    && metrics.maxBatchDurationMicros >= metrics.lastBatchDurationMicros,
                "Service metrics must retain real aggregate durations for the last terminal generation.");

        std::cout << "WAV import analysis tests passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "WAV import analysis tests failed: "
                  << exception.what() << std::endl;
        return 1;
    }
}
