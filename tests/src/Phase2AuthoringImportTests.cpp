#include "drs/engine/SampleImport.h"

#include <juce_audio_formats/juce_audio_formats.h>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

namespace
{
namespace fs = std::filesystem;

void require(bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

fs::path getScratchDirectory()
{
    auto path = fs::temp_directory_path() / "drs-phase2-authoring-import-tests";
    fs::create_directories(path);
    return path;
}

juce::AudioBuffer<float> buildReferenceBuffer()
{
    constexpr int frameCount = 480;
    juce::AudioBuffer<float> buffer(2, frameCount);

    for (int sampleIndex = 0; sampleIndex < frameCount; ++sampleIndex)
    {
        const auto phase = static_cast<float>(sampleIndex) / static_cast<float>(frameCount);
        buffer.setSample(0, sampleIndex, std::sin(phase * juce::MathConstants<float>::twoPi));
        buffer.setSample(1, sampleIndex, std::cos(phase * juce::MathConstants<float>::twoPi) * 0.5f);
    }

    return buffer;
}

void writeAudioFile(const fs::path& filePath,
                    juce::AudioFormat& format,
                    const juce::AudioBuffer<float>& buffer,
                    const juce::StringPairArray& metadata)
{
    auto fileOutput = std::make_unique<juce::FileOutputStream>(juce::File(filePath.generic_string()));
    require(fileOutput->openedOk(), "Could not open output file for writing: " + filePath.generic_string());
    std::unique_ptr<juce::OutputStream> output = std::move(fileOutput);

    juce::AudioFormatWriterOptions options;
    options = options.withSampleRate(48000.0)
        .withNumChannels(buffer.getNumChannels())
        .withBitsPerSample(24);

    for (const auto& key : metadata.getAllKeys())
        options = options.withMetadata(key, metadata[key]);

    auto writerOwner = format.createWriterFor(output, options);
    require(writerOwner != nullptr, "Could not create audio writer for: " + filePath.generic_string());
    require(writerOwner->writeFromAudioSampleBuffer(buffer, 0, buffer.getNumSamples()),
            "Could not write audio samples to: " + filePath.generic_string());
}

const drs::engine::AuthoringImportQueueItem& findItem(const drs::engine::AuthoringImportQueue& queue,
                                                      const std::string& itemId)
{
    const auto iterator = std::find_if(queue.items.begin(),
                                       queue.items.end(),
                                       [&](const drs::engine::AuthoringImportQueueItem& item)
                                       {
                                           return item.id == itemId;
                                       });
    if (iterator == queue.items.end())
        throw std::runtime_error("Queue item '" + itemId + "' was not found.");

    return *iterator;
}

bool hasFindingCode(const drs::engine::AuthoringImportQueueItem& item, const std::string& code)
{
    return std::any_of(item.findings.begin(),
                       item.findings.end(),
                       [&](const drs::engine::AuthoringImportFinding& finding)
                       {
                           return finding.code == code;
                       });
}

bool hasFindingCode(const std::vector<drs::engine::AuthoringImportFinding>& findings,
                    const std::string& code)
{
    return std::any_of(findings.begin(),
                       findings.end(),
                       [&](const drs::engine::AuthoringImportFinding& finding)
                       {
                           return finding.code == code;
                       });
}
} // namespace

int main()
{
    try
    {
        const auto scratchDirectory = getScratchDirectory();
        const auto buffer = buildReferenceBuffer();
        juce::WavAudioFormat wavFormat;

        const auto cleanSiblingOnePath = scratchDirectory / "Pad_Sustain_C4_vel064_rr1.wav";
        const auto cleanPath = scratchDirectory / "Pad_Sustain_C4_vel064_rr2.wav";
        const auto cleanSiblingThreePath = scratchDirectory / "Pad_Sustain_C4_vel064_rr3.wav";
        const auto ambiguousPath = scratchDirectory / "MysteryTexture.wav";
        const auto conflictPath = scratchDirectory / "Lead_A4.wav";
        const auto policyWarningPath = scratchDirectory / "Shimmer_Bad Name!.wav";
        const auto canceledPath = scratchDirectory / "Canceled_C3.wav";
        const auto unsupportedPath = scratchDirectory / "Unsupported.txt";
        const auto missingPath = scratchDirectory / "Missing_C4.wav";
        const auto sparseOnePath = scratchDirectory / "Brush_Sustain_D4_vel096_rr1.wav";
        const auto sparseThreePath = scratchDirectory / "Brush_Sustain_D4_vel096_rr3.wav";

        juce::StringPairArray matchingMetadata;
        matchingMetadata.set("MidiUnityNote", "60");
        matchingMetadata.set("NumSampleLoops", "1");
        matchingMetadata.set("Loop0Start", "64");
        matchingMetadata.set("Loop0End", "192");

        juce::StringPairArray conflictingMetadata;
        conflictingMetadata.set("MidiUnityNote", "60");

        writeAudioFile(cleanSiblingOnePath, wavFormat, buffer, matchingMetadata);
        writeAudioFile(cleanPath, wavFormat, buffer, matchingMetadata);
        writeAudioFile(cleanSiblingThreePath, wavFormat, buffer, matchingMetadata);
        writeAudioFile(ambiguousPath, wavFormat, buffer, {});
        writeAudioFile(conflictPath, wavFormat, buffer, conflictingMetadata);
        writeAudioFile(policyWarningPath, wavFormat, buffer, {});
        writeAudioFile(canceledPath, wavFormat, buffer, matchingMetadata);
        writeAudioFile(sparseOnePath, wavFormat, buffer, matchingMetadata);
        writeAudioFile(sparseThreePath, wavFormat, buffer, matchingMetadata);

        {
            juce::FileOutputStream unsupportedOutput(juce::File(unsupportedPath.generic_string()));
            require(unsupportedOutput.openedOk(), "Could not create unsupported text fixture.");
            unsupportedOutput.writeText("not audio", false, false, nullptr);
        }

        const auto directHeuristics = drs::engine::parseSampleFilenameHeuristics(cleanPath.generic_string());
        require(directHeuristics.suggestedZone.suggested, "Direct filename heuristics should produce a zone suggestion.");
        require(directHeuristics.suggestedZone.zone.rootKey == 60, "Direct filename heuristics root key changed unexpectedly.");
        require(directHeuristics.suggestedZone.zone.velocityLow == 33
                    && directHeuristics.suggestedZone.zone.velocityHigh == 64,
                "Direct filename heuristics velocity bucket changed unexpectedly.");
        require(directHeuristics.suggestedZone.zone.roundRobin.has_value(),
                "Direct filename heuristics should now infer an explicit round-robin descriptor.");
        require(directHeuristics.suggestedZone.zone.roundRobin->slotCount == 3
                    && directHeuristics.suggestedZone.zone.roundRobin->slotIndex == 2
                    && directHeuristics.suggestedZone.zone.roundRobin->mode
                        == drs::engine::RoundRobinMode::sequential,
                "Direct filename heuristics round-robin slot metadata changed unexpectedly.");
        require(directHeuristics.suggestedZone.zone.roundRobinLength == 3
                    && directHeuristics.suggestedZone.zone.roundRobinPosition == 2,
                "Direct filename heuristics should keep round-robin scalar metadata aligned.");

        const auto sparseHeuristics = drs::engine::parseSampleFilenameHeuristics(sparseThreePath.generic_string());
        require(!sparseHeuristics.suggestedZone.zone.roundRobin.has_value(),
                "Sparse round-robin sibling pools should remain ungrouped until reviewed.");
        require(hasFindingCode(sparseHeuristics.findings, "round_robin.sparse_slots"),
                "Sparse round-robin sibling pools should surface a typed review finding.");

        const auto cleanSiblingOneHeuristics =
            drs::engine::parseSampleFilenameHeuristics(cleanSiblingOnePath.generic_string());
        const auto cleanSiblingThreeHeuristics =
            drs::engine::parseSampleFilenameHeuristics(cleanSiblingThreePath.generic_string());
        require(cleanSiblingOneHeuristics.suggestedZone.zone.roundRobin.has_value()
                    && cleanSiblingThreeHeuristics.suggestedZone.zone.roundRobin.has_value(),
                "Complete round-robin sibling fixtures should infer descriptors before batch reconciliation.");

        std::vector<drs::engine::RuntimeProjectZoneDefinition> acceptedRoundRobinSubset {
            directHeuristics.suggestedZone.zone,
            cleanSiblingThreeHeuristics.suggestedZone.zone
        };
        drs::engine::reconcileBatchInferredRoundRobinDescriptors(acceptedRoundRobinSubset);
        require(acceptedRoundRobinSubset[0].roundRobin.has_value()
                    && acceptedRoundRobinSubset[1].roundRobin.has_value(),
                "Accepted multi-zone round-robin subsets should stay grouped after reconciliation.");
        require(acceptedRoundRobinSubset[0].roundRobin->slotCount == 2
                    && acceptedRoundRobinSubset[0].roundRobin->slotIndex == 1
                    && acceptedRoundRobinSubset[1].roundRobin->slotCount == 2
                    && acceptedRoundRobinSubset[1].roundRobin->slotIndex == 2,
                "Accepted multi-zone round-robin subsets should be renumbered into dense contiguous slots.");
        require(acceptedRoundRobinSubset[0].roundRobin->poolId == acceptedRoundRobinSubset[1].roundRobin->poolId,
                "Accepted multi-zone round-robin subsets should stay in the same reconciled pool.");
        require(acceptedRoundRobinSubset[0].roundRobinLength == 2
                    && acceptedRoundRobinSubset[0].roundRobinPosition == 1
                    && acceptedRoundRobinSubset[1].roundRobinLength == 2
                    && acceptedRoundRobinSubset[1].roundRobinPosition == 2,
                "Accepted multi-zone round-robin subsets should keep scalar metadata aligned after reconciliation.");

        std::vector<drs::engine::RuntimeProjectZoneDefinition> soloRoundRobinZone {
            directHeuristics.suggestedZone.zone
        };
        drs::engine::reconcileBatchInferredRoundRobinDescriptors(soloRoundRobinZone);
        require(!soloRoundRobinZone[0].roundRobin.has_value()
                    && soloRoundRobinZone[0].roundRobinLength == 0
                    && soloRoundRobinZone[0].roundRobinPosition == 0,
                "Single accepted round-robin zones should collapse back to a plain zone after reconciliation.");

        auto alternateRoundRobinOne = cleanSiblingOneHeuristics.suggestedZone.zone;
        auto alternateRoundRobinTwo = directHeuristics.suggestedZone.zone;
        alternateRoundRobinOne.rootKey = 62;
        alternateRoundRobinOne.keyLow = 62;
        alternateRoundRobinOne.keyHigh = 62;
        alternateRoundRobinTwo.rootKey = 62;
        alternateRoundRobinTwo.keyLow = 62;
        alternateRoundRobinTwo.keyHigh = 62;

        std::vector<drs::engine::RuntimeProjectZoneDefinition> splitRoundRobinPools {
            cleanSiblingOneHeuristics.suggestedZone.zone,
            directHeuristics.suggestedZone.zone,
            alternateRoundRobinOne,
            alternateRoundRobinTwo
        };
        drs::engine::reconcileBatchInferredRoundRobinDescriptors(splitRoundRobinPools);
        require(splitRoundRobinPools[0].roundRobin.has_value()
                    && splitRoundRobinPools[1].roundRobin.has_value()
                    && splitRoundRobinPools[2].roundRobin.has_value()
                    && splitRoundRobinPools[3].roundRobin.has_value(),
                "Split accepted round-robin pools should remain grouped within each compatible pairing key.");
        require(splitRoundRobinPools[0].roundRobin->poolId == splitRoundRobinPools[1].roundRobin->poolId
                    && splitRoundRobinPools[2].roundRobin->poolId == splitRoundRobinPools[3].roundRobin->poolId
                    && splitRoundRobinPools[0].roundRobin->poolId != splitRoundRobinPools[2].roundRobin->poolId,
                "Compatible round-robin subgroups should receive distinct pool ids when one inferred pool splits.");
        require(splitRoundRobinPools[0].roundRobin->slotCount == 2
                    && splitRoundRobinPools[1].roundRobin->slotCount == 2
                    && splitRoundRobinPools[2].roundRobin->slotCount == 2
                    && splitRoundRobinPools[3].roundRobin->slotCount == 2,
                "Split accepted round-robin pools should be renumbered independently inside each compatible subgroup.");

        const auto ambiguousRootInference = drs::engine::inferSampleRootKey(ambiguousPath.generic_string());
        require(!ambiguousRootInference.resolved, "Ambiguous root-key inference should require manual confirmation.");
        require(ambiguousRootInference.source == "manual",
                "Ambiguous root-key inference source changed unexpectedly.");

        const auto conflictImport = drs::engine::inspectSampleFile(conflictPath.generic_string());
        require(conflictImport.accepted, "Conflict fixture should still inspect for root-key inference.");
        const auto conflictRootInference = drs::engine::inferSampleRootKey(conflictPath.generic_string(),
                                                                           &conflictImport.metadata);
        require(conflictRootInference.resolved, "Conflicting root-key sources should still resolve a primary candidate.");
        require(conflictRootInference.rootKey == 69,
                "Filename root-key inference should remain primary when metadata conflicts.");
        require(conflictRootInference.source == "filename",
                "Conflicting root-key inference should continue to identify the winning source.");

        auto queue = drs::engine::createAuthoringImportQueue(
            {
                cleanPath.generic_string(),
                ambiguousPath.generic_string(),
                conflictPath.generic_string(),
                canceledPath.generic_string(),
                policyWarningPath.generic_string(),
                unsupportedPath.generic_string(),
                missingPath.generic_string()
            },
            scratchDirectory.generic_string());

        queue.items.front().knownFingerprintHex = "known-clean-fingerprint";

        require(queue.items.size() == 7, "Authoring import queue item count changed unexpectedly.");
        const auto canceledItemId = queue.items[3].id;
        require(drs::engine::cancelAuthoringImportQueueItem(queue, canceledItemId),
                "Pending queue item should be cancelable before processing.");

        std::size_t processedCount = 0;
        std::string cleanItemId;
        while (true)
        {
            const auto step = drs::engine::processNextAuthoringImportQueueItem(queue);
            if (!step.processed)
            {
                require(step.queueDrained, "Queue should report drained when no more pending items remain.");
                break;
            }

            ++processedCount;
            if (processedCount == 1)
                cleanItemId = step.itemId;
        }

        require(processedCount == 6, "Exactly six queue items should process after one pending item is canceled.");

        const auto& cleanItem = findItem(queue, cleanItemId);
        require(cleanItem.state == drs::engine::AuthoringImportItemState::inferred,
                "Portable filename with complete tokens should land in the inferred state.");
        require(cleanItem.suggestedZone.zone.rootKey == 60, "Clean import root key changed unexpectedly.");
        require(cleanItem.suggestedZone.zone.keyLow == 60 && cleanItem.suggestedZone.zone.keyHigh == 60,
                "Clean import key range should collapse to the inferred root key.");
        require(cleanItem.suggestedZone.zone.velocityLow == 33 && cleanItem.suggestedZone.zone.velocityHigh == 64,
                "Clean import velocity range changed unexpectedly.");
        require(cleanItem.suggestedZone.zone.articulationId == "sustain",
                "Clean import articulation inference changed unexpectedly.");
        require(cleanItem.suggestedZone.zone.roundRobin.has_value()
                    && cleanItem.suggestedZone.zone.roundRobin->slotCount == 3
                    && cleanItem.suggestedZone.zone.roundRobin->slotIndex == 2,
                "Clean import should preserve the inferred round-robin pool descriptor.");
        require(cleanItem.suggestedZone.zone.loopEnabled,
                "Clean import should preserve loop metadata into the suggested zone.");
        require(drs::engine::acceptAuthoringImportQueueItem(queue, cleanItem.id),
                "Inferred queue item should become accepted when the user confirms it.");
        require(findItem(queue, cleanItem.id).state == drs::engine::AuthoringImportItemState::accepted,
                "Accepted queue item state changed unexpectedly.");

        const auto& ambiguousItem = queue.items[1];
        require(ambiguousItem.state == drs::engine::AuthoringImportItemState::warning,
                "Ambiguous filename should require confirmation.");
        require(hasFindingCode(ambiguousItem, "root_key.ambiguous"),
                "Ambiguous filename should report a root-key confirmation finding.");
        require(ambiguousItem.suggestedZone.zone.keyLow == 0 && ambiguousItem.suggestedZone.zone.keyHigh == 127,
                "Ambiguous filename should keep a full-range draft zone.");

        const auto& conflictItem = queue.items[2];
        require(conflictItem.state == drs::engine::AuthoringImportItemState::warning,
                "Metadata conflict should surface a warning state.");
        require(hasFindingCode(conflictItem, "root_key.conflict"),
                "Metadata conflict should report a structured root-key conflict finding.");
        require(conflictItem.suggestedZone.zone.rootKey == 69,
                "Filename note should stay active when metadata conflicts with it.");

        const auto& policyWarningItem = queue.items[4];
        require(policyWarningItem.state == drs::engine::AuthoringImportItemState::warning,
                "Portable-name policy warnings should surface a warning state.");
        require(hasFindingCode(policyWarningItem, "import.policy_warning"),
                "Portable-name policy warnings should surface a structured policy finding.");
        require(policyWarningItem.suggestedZone.suggested,
                "Policy warnings should still preserve the inferred draft zone.");

        const auto& canceledItem = findItem(queue, canceledItemId);
        require(canceledItem.state == drs::engine::AuthoringImportItemState::canceled,
                "Canceled queue item state changed unexpectedly.");
        require(canceledItem.inspectionResult.state.empty(),
                "Canceled queue item should not have been imported.");

        const auto& unsupportedItem = queue.items[5];
        require(unsupportedItem.state == drs::engine::AuthoringImportItemState::failed,
                "Unsupported text input should fail the import queue item.");
        require(!unsupportedItem.inspectionResult.issues.empty(),
                "Failed import queue item should preserve actionable import issues.");
        require(!drs::engine::acceptAuthoringImportQueueItem(queue, unsupportedItem.id),
                "Failed queue item must not become accepted.");

        const auto& missingItem = queue.items[6];
        require(missingItem.state == drs::engine::AuthoringImportItemState::failed,
                "Missing batch inputs should fail the import queue item.");
        require(missingItem.inspectionResult.state == "Sample missing",
                "Missing batch inputs should preserve the missing-file disposition.");
        require(!missingItem.inspectionResult.issues.empty(),
                "Missing batch inputs should preserve an actionable missing-file issue.");

        require(cleanItem.inspectionResult.metadata.sourceChecksumHex == "known-clean-fingerprint",
                "Queue processing should reuse a supplied fingerprint instead of recomputing it.");

        auto fingerprintQueue = drs::engine::createAuthoringImportQueue({cleanPath.generic_string()},
                                                                        scratchDirectory.generic_string());
        fingerprintQueue.items.front().knownFingerprintHex = "precomputed-fingerprint";
        drs::engine::resetSampleImportIoCounters();
        const auto fingerprintStep = drs::engine::processNextAuthoringImportQueueItem(fingerprintQueue);
        require(fingerprintStep.processed, "Single-item fingerprint reuse queue should process normally.");
        require(fingerprintQueue.items.front().inspectionResult.metadata.sourceChecksumHex == "precomputed-fingerprint",
                "Single-item queue should retain the supplied fingerprint in inspection metadata.");
        const auto fingerprintReuseCounters = drs::engine::getSampleImportIoCounters();
        require(fingerprintReuseCounters.fingerprintOpenCount == 0,
                "Queue processing should not reopen the fingerprint stream when a fingerprint is supplied.");
        require(fingerprintReuseCounters.fullFrameReadCount == 0,
                "Queue processing should not perform full-frame reads after moving to metadata-only inspection.");

        require(queue.metrics.totalItemCount == 7,
                "Queue metrics total-item count changed unexpectedly.");
        require(queue.metrics.pendingCount == 0,
                "Queue metrics should report no pending items after the batch drains.");
        require(queue.metrics.processedCount == 6,
                "Queue metrics processed-count changed unexpectedly for a mixed batch.");
        require(queue.metrics.warningItemCount == 3,
                "Queue metrics warning-count changed unexpectedly for a mixed batch.");
        require(queue.metrics.failedItemCount == 2,
                "Queue metrics failed-count changed unexpectedly for a mixed batch.");
        require(queue.metrics.canceledItemCount == 1,
                "Queue metrics canceled-count changed unexpectedly for a mixed batch.");
        require(queue.metrics.acceptedItemCount == 1,
                "Queue metrics accepted-count changed unexpectedly after confirming one inferred item.");

        std::cout << "Phase 2 authoring import tests passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Phase 2 authoring import tests failed: " << exception.what() << std::endl;
        return 1;
    }
}
