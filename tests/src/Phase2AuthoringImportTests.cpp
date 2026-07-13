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
} // namespace

int main()
{
    try
    {
        const auto scratchDirectory = getScratchDirectory();
        const auto buffer = buildReferenceBuffer();
        juce::WavAudioFormat wavFormat;

        const auto cleanPath = scratchDirectory / "Pad_Sustain_C4_vel064_rr2.wav";
        const auto ambiguousPath = scratchDirectory / "MysteryTexture.wav";
        const auto conflictPath = scratchDirectory / "Lead_A4.wav";
        const auto canceledPath = scratchDirectory / "Shimmer_Bad Name!.wav";
        const auto unsupportedPath = scratchDirectory / "Unsupported.txt";

        juce::StringPairArray matchingMetadata;
        matchingMetadata.set("MidiUnityNote", "60");
        matchingMetadata.set("NumSampleLoops", "1");
        matchingMetadata.set("Loop0Start", "64");
        matchingMetadata.set("Loop0End", "192");

        juce::StringPairArray conflictingMetadata;
        conflictingMetadata.set("MidiUnityNote", "60");

        writeAudioFile(cleanPath, wavFormat, buffer, matchingMetadata);
        writeAudioFile(ambiguousPath, wavFormat, buffer, {});
        writeAudioFile(conflictPath, wavFormat, buffer, conflictingMetadata);
        writeAudioFile(canceledPath, wavFormat, buffer, {});

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
        require(directHeuristics.suggestedZone.roundRobinIndex == 2,
                "Direct filename heuristics round-robin parsing changed unexpectedly.");

        auto queue = drs::engine::createAuthoringImportQueue(
            {
                cleanPath.generic_string(),
                ambiguousPath.generic_string(),
                conflictPath.generic_string(),
                canceledPath.generic_string(),
                unsupportedPath.generic_string()
            },
            scratchDirectory.generic_string());

        require(queue.items.size() == 5, "Authoring import queue item count changed unexpectedly.");
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

        require(processedCount == 4, "Exactly four queue items should process after one pending item is canceled.");

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

        const auto& canceledItem = findItem(queue, canceledItemId);
        require(canceledItem.state == drs::engine::AuthoringImportItemState::canceled,
                "Canceled queue item state changed unexpectedly.");
        require(canceledItem.importResult.state.empty(),
                "Canceled queue item should not have been imported.");

        const auto& failedItem = queue.items[4];
        require(failedItem.state == drs::engine::AuthoringImportItemState::failed,
                "Unsupported text input should fail the import queue item.");
        require(!failedItem.importResult.issues.empty(),
                "Failed import queue item should preserve actionable import issues.");
        require(!drs::engine::acceptAuthoringImportQueueItem(queue, failedItem.id),
                "Failed queue item must not become accepted.");

        std::cout << "Phase 2 authoring import tests passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Phase 2 authoring import tests failed: " << exception.what() << std::endl;
        return 1;
    }
}
