#include "drs/engine/RuntimeLoader.h"
#include "plugin/PluginProcessor.h"

#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors_headless/juce_audio_processors_headless.h>

#include <algorithm>
#include <cmath>
#include <condition_variable>
#include <filesystem>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace
{
namespace fs = std::filesystem;

void require(bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

std::string describeImportIoCounters(const drs::engine::SampleImportIoCounters& counters)
{
    return "fingerprintOpenCount=" + std::to_string(counters.fingerprintOpenCount)
        + ", readerOpenCount=" + std::to_string(counters.readerOpenCount)
        + ", bytesReadCount=" + std::to_string(counters.bytesReadCount)
        + ", fullFrameReadCount=" + std::to_string(counters.fullFrameReadCount)
        + ", copyCount=" + std::to_string(counters.copyCount)
        + ", peakChunkReadCount=" + std::to_string(counters.peakChunkReadCount);
}

void requireNoImportIo(const std::string& context)
{
    const auto counters = drs::engine::getSampleImportIoCounters();
    require(counters.fingerprintOpenCount == 0
                && counters.readerOpenCount == 0
                && counters.bytesReadCount == 0
                && counters.fullFrameReadCount == 0
                && counters.copyCount == 0
                && counters.peakChunkReadCount == 0,
            context + " unexpectedly performed sample import IO inline: "
                + describeImportIoCounters(counters));
}

bool hasVisibleWaveform(const drs::app::AuthoringWaveformPreview& preview)
{
    constexpr auto amplitudeEpsilon = 1.0e-4f;

    return std::any_of(preview.points.begin(),
                       preview.points.end(),
                       [&](const drs::app::AuthoringWaveformPreviewPoint& point)
                       {
                           return std::abs(point.minValue) > amplitudeEpsilon
                               || std::abs(point.maxValue) > amplitudeEpsilon
                               || std::abs(point.maxValue - point.minValue) > amplitudeEpsilon;
                       });
}

std::vector<float> renderSelectedZonePreview(const std::string& zoneId)
{
    drs::plugin::Processor processor;
    const auto projectLoad = drs::engine::loadPhase2ReferenceProjectManifest();
    require(projectLoad.loaded, "Phase 2 reference project must load for waveform preview playback validation.");
    processor.replaceAuthoringProject(projectLoad.project);
    processor.prepareToPlay(44100.0, 512);

    const auto selectionResult = processor.getAuthoringSession().selectZone(zoneId);
    require(selectionResult.applied, "Could not select zone '" + zoneId + "' for playback validation.");

    const auto zone = processor.getAuthoringSession().getSelectedZone();
    require(zone.has_value(), "Selected zone should remain available during playback validation.");
    const auto selectedRevision = processor.getAuthoringSession().getDocumentState().revision;
    processor.requestAuthoringPreview(drs::engine::AuthoringPreviewScope::selectedZone);
    juce::AudioBuffer<float> buffer(2, 512);
    juce::MidiBuffer midiBuffer;
    auto servicedPreviewRoute = false;
    auto previewRouteReady = false;
    const auto previewDeadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
    while (std::chrono::steady_clock::now() < previewDeadline)
    {
        servicedPreviewRoute = processor.serviceMessageThreadWork() || servicedPreviewRoute;
        buffer.clear();
        midiBuffer.clear();
        processor.processBlock(buffer, midiBuffer);
        servicedPreviewRoute = processor.serviceMessageThreadWork() || servicedPreviewRoute;
        const auto previewStatus = processor.getAuthoringPreviewStatusSnapshot();
        if (previewStatus.activationState == drs::engine::AuthoringPreviewActivationState::active
            && previewStatus.activeRevision == selectedRevision
            && previewStatus.selectedZoneId == zone->id)
        {
            previewRouteReady = true;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    require(servicedPreviewRoute && previewRouteReady,
            "Selected-zone playback should prepare and activate the on-demand Preview route off the audio thread.");

    processor.queueAuthoringPreviewNoteOn(std::clamp(zone->rootKey, zone->keyLow, zone->keyHigh), 0.8f);
    std::vector<float> rendered;
    constexpr int renderBlockCount = 8;
    rendered.reserve(static_cast<std::size_t>(buffer.getNumChannels()
                                              * buffer.getNumSamples()
                                              * renderBlockCount));
    for (int block = 0; block < renderBlockCount; ++block)
    {
        buffer.clear();
        midiBuffer.clear();
        processor.processBlock(buffer, midiBuffer);
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        {
            for (int sampleIndex = 0; sampleIndex < buffer.getNumSamples(); ++sampleIndex)
                rendered.push_back(buffer.getSample(channel, sampleIndex));
        }
    }

    return rendered;
}

bool buffersDiffer(const std::vector<float>& first, const std::vector<float>& second)
{
    if (first.size() != second.size())
        return true;

    constexpr auto differenceThreshold = 1.0e-3f;
    for (std::size_t index = 0; index < first.size(); ++index)
    {
        if (std::abs(first[index] - second[index]) > differenceThreshold)
            return true;
    }

    return false;
}

float bufferPeak(const std::vector<float>& buffer)
{
    auto peak = 0.0f;
    for (const auto sample : buffer)
        peak = std::max(peak, std::abs(sample));
    return peak;
}

fs::path makeScratchDirectory()
{
    const auto unique = std::to_string(
        static_cast<std::uint64_t>(std::chrono::steady_clock::now().time_since_epoch().count()));
    auto path = fs::temp_directory_path() / ("drs-phase2-waveform-preview-tests-" + unique);
    fs::create_directories(path);
    return path;
}

juce::AudioBuffer<float> buildPreviewFixtureBuffer(const int channelCount,
                                                   const int frameCount,
                                                   const float amplitude)
{
    juce::AudioBuffer<float> buffer(channelCount, frameCount);
    for (int channelIndex = 0; channelIndex < channelCount; ++channelIndex)
    {
        for (int sampleIndex = 0; sampleIndex < frameCount; ++sampleIndex)
        {
            const auto phase = static_cast<float>(sampleIndex) / static_cast<float>(frameCount);
            buffer.setSample(channelIndex,
                             sampleIndex,
                             std::sin((phase * juce::MathConstants<float>::twoPi)
                                          + (0.17f * static_cast<float>(channelIndex)))
                                 * amplitude);
        }
    }
    return buffer;
}

void writeAudioFile(const fs::path& filePath,
                    const juce::AudioBuffer<float>& buffer,
                    const juce::StringPairArray& metadata = {})
{
    juce::WavAudioFormat wavFormat;
    const auto targetFile = juce::File(filePath.generic_string());
    if (targetFile.existsAsFile())
        targetFile.deleteFile();
    auto fileOutput = std::make_unique<juce::FileOutputStream>(targetFile);
    require(fileOutput->openedOk(), "Could not open waveform preview scratch fixture for writing.");
    std::unique_ptr<juce::OutputStream> output = std::move(fileOutput);

    juce::AudioFormatWriterOptions options;
    options = options.withSampleRate(48000.0)
        .withNumChannels(buffer.getNumChannels())
        .withBitsPerSample(24);

    for (const auto& key : metadata.getAllKeys())
        options = options.withMetadata(key, metadata[key]);

    auto writer = wavFormat.createWriterFor(output, options);
    require(writer != nullptr, "Could not create waveform preview scratch writer.");
    require(writer->writeFromAudioSampleBuffer(buffer, 0, buffer.getNumSamples()),
            "Could not write waveform preview scratch fixture.");
}

drs::engine::RuntimeProjectModel makeScratchProject(const fs::path& root, const fs::path& samplePath)
{
    const auto projectLoad = drs::engine::loadPhase2ReferenceProjectManifest();
    require(projectLoad.loaded, "Scratch waveform preview project requires the Phase 2 reference manifest.");

    auto project = projectLoad.project;
    project.projectId = "phase2-waveform-preview-scratch";
    project.displayName = "Phase 2 Waveform Preview Scratch";
    project.contentRootPath = root.generic_string();
    project.defaultInstrumentManifestPath = (root / "phase2-waveform-preview-scratch.drinst").generic_string();

    const auto selectedZone = std::find_if(project.authoring.zones.begin(),
                                           project.authoring.zones.end(),
                                           [&](const auto& zone)
                                           {
                                               return zone.id == project.authoring.selectedZoneId;
                                           });
    require(selectedZone != project.authoring.zones.end(),
            "Scratch waveform preview project requires a selected reference zone.");

    auto sampleSource = std::find_if(project.sampleSources.begin(),
                                     project.sampleSources.end(),
                                     [&](const auto& item)
                                     {
                                         return item.id == selectedZone->sampleSourceId;
                                     });
    require(sampleSource != project.sampleSources.end(),
            "Scratch waveform preview project requires the selected reference sample source.");
    sampleSource->path = samplePath.generic_string();
    return project;
}

struct DecodedWaveformReference
{
    std::uint64_t frameCount = 0;
    std::uint32_t channelCount = 0;
    std::vector<std::vector<float>> channels;
};

DecodedWaveformReference decodeWaveformReference(const fs::path& filePath)
{
    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();
    auto reader = std::unique_ptr<juce::AudioFormatReader>(
        formatManager.createReaderFor(juce::File(filePath.generic_string())));
    require(reader != nullptr, "Waveform reference decode requires a readable audio fixture.");

    juce::AudioBuffer<float> buffer(static_cast<int>(reader->numChannels),
                                    static_cast<int>(reader->lengthInSamples));
    require(reader->read(&buffer,
                         0,
                         static_cast<int>(reader->lengthInSamples),
                         0,
                         true,
                         true),
            "Waveform reference decode requires a complete sample read.");

    DecodedWaveformReference decoded;
    decoded.frameCount = static_cast<std::uint64_t>(buffer.getNumSamples());
    decoded.channelCount = static_cast<std::uint32_t>(buffer.getNumChannels());
    decoded.channels.resize(static_cast<std::size_t>(buffer.getNumChannels()));
    for (int channelIndex = 0; channelIndex < buffer.getNumChannels(); ++channelIndex)
    {
        decoded.channels[static_cast<std::size_t>(channelIndex)].assign(
            buffer.getReadPointer(channelIndex),
            buffer.getReadPointer(channelIndex) + buffer.getNumSamples());
    }
    return decoded;
}

std::vector<drs::engine::WaveformPeakPoint> buildReferenceWaveformPeaks(
    const DecodedWaveformReference& decoded,
    const std::size_t displayPointCount,
    const drs::engine::WaveformPeakChannelReduction channelReduction)
{
    if (decoded.frameCount == 0)
        return {};

    const auto pointCount = std::max<std::size_t>(
        1,
        std::min<std::size_t>(displayPointCount, static_cast<std::size_t>(decoded.frameCount)));
    std::vector<drs::engine::WaveformPeakPoint> points(pointCount);
    std::vector<bool> initialized(pointCount, false);

    for (std::uint64_t frameIndex = 0; frameIndex < decoded.frameCount; ++frameIndex)
    {
        const auto pointIndex = std::min<std::size_t>(
            pointCount - 1,
            static_cast<std::size_t>((frameIndex * pointCount) / decoded.frameCount));

        auto frameMin = decoded.channels.front()[static_cast<std::size_t>(frameIndex)];
        auto frameMax = frameMin;

        switch (channelReduction)
        {
            case drs::engine::WaveformPeakChannelReduction::firstChannel:
                break;
            case drs::engine::WaveformPeakChannelReduction::averageChannels:
            {
                auto sampleSum = 0.0f;
                for (const auto& channel : decoded.channels)
                    sampleSum += channel[static_cast<std::size_t>(frameIndex)];
                frameMin = sampleSum / static_cast<float>(decoded.channelCount);
                frameMax = frameMin;
                break;
            }
            case drs::engine::WaveformPeakChannelReduction::channelExtrema:
            {
                for (std::size_t channelIndex = 1; channelIndex < decoded.channels.size(); ++channelIndex)
                {
                    const auto value = decoded.channels[channelIndex][static_cast<std::size_t>(frameIndex)];
                    frameMin = std::min(frameMin, value);
                    frameMax = std::max(frameMax, value);
                }
                break;
            }
        }

        auto& point = points[pointIndex];
        if (!initialized[pointIndex])
        {
            point.minValue = frameMin;
            point.maxValue = frameMax;
            initialized[pointIndex] = true;
            continue;
        }

        point.minValue = std::min(point.minValue, frameMin);
        point.maxValue = std::max(point.maxValue, frameMax);
    }

    return points;
}

void requireEquivalentWaveformPeaks(const drs::engine::WaveformPeakBuildResult& incremental,
                                    const std::vector<drs::engine::WaveformPeakPoint>& reference,
                                    const std::string& context)
{
    require(incremental.points.size() == reference.size(),
            context + " point count changed unexpectedly.");

    constexpr auto tolerance = 1.0e-4f;
    for (std::size_t index = 0; index < reference.size(); ++index)
    {
        require(std::abs(incremental.points[index].minValue - reference[index].minValue) <= tolerance
                    && std::abs(incremental.points[index].maxValue - reference[index].maxValue) <= tolerance,
                context + " peak envelope diverged from the full-buffer reference at point "
                    + std::to_string(index) + ".");
    }
}

bool waitForWaveformPreviewReady(drs::plugin::Processor& processor,
                                 drs::app::AuthoringWaveformPreview& previewOut)
{
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
    while (std::chrono::steady_clock::now() < deadline)
    {
        auto preview = processor.getAuthoringWaveformPreview();
        if (preview.available && !preview.points.empty() && preview.state == "Ready")
        {
            previewOut = std::move(preview);
            return true;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }

    return false;
}
} // namespace

int main()
{
    try
    {
        juce::ScopedJuceInitialiser_GUI gui;

        drs::plugin::Processor processor;
        const auto projectLoad = drs::engine::loadPhase2ReferenceProjectManifest();
        require(projectLoad.loaded, "Phase 2 reference project must load for waveform preview validation.");
        drs::engine::resetSampleImportIoCounters();
        processor.replaceAuthoringProject(projectLoad.project);

        const auto metrics = processor.getAuthoringImportResponsivenessSnapshot();
        require(metrics.available, "Authoring import responsiveness snapshot should be available.");
        require(metrics.totalItemCount == 2, "Phase 2 reference project sample-source count changed unexpectedly.");
        require(metrics.state == "not-run",
                "Phase 2 authoring import metrics should begin in the honest not-run state after project replacement.");
        require(metrics.pendingCount == 0, "Phase 2 authoring import metrics should not report pending items.");
        require(metrics.processedCount == 0,
                "Phase 2 authoring import metrics should not claim processed project samples before an explicit import batch runs.");
        require(metrics.failedItemCount == 0, "Phase 2 authoring import metrics should not report failed items.");
        require(metrics.acceptedItemCount == 0
                    && metrics.warningItemCount == 0
                    && metrics.lastProcessDurationMicros == 0
                    && metrics.averageProcessDurationMicros == 0
                    && metrics.maxProcessDurationMicros == 0
                    && metrics.lastProcessedItemId.empty(),
                "Phase 2 authoring import metrics should remain zeroed until an explicit WAV import batch runs.");

        {
            std::mutex waveformMutex;
            std::condition_variable waveformCondition;
            auto waveformPausedAtBuild = false;
            auto releaseWaveformBuild = false;

            drs::app::WaveformPreviewServiceOptions previewOptions;
            previewOptions.stageObserver = [&](const drs::app::WaveformPreviewServiceStage stage)
            {
                if (stage != drs::app::WaveformPreviewServiceStage::building)
                    return;

                std::unique_lock<std::mutex> lock(waveformMutex);
                waveformPausedAtBuild = true;
                waveformCondition.notify_all();
                waveformCondition.wait(lock, [&] { return releaseWaveformBuild; });
            };

            drs::plugin::Processor pausedPreviewProcessor(previewOptions);
            require(pausedPreviewProcessor.replaceAuthoringProject(projectLoad.project),
                    "Paused preview processor must accept the Phase 2 reference project.");
            drs::engine::resetSampleImportIoCounters();
            pausedPreviewProcessor.authorizeAuthoringWaveformPreviewLoad();
            {
                std::unique_lock<std::mutex> lock(waveformMutex);
                require(waveformCondition.wait_for(lock, std::chrono::seconds(5), [&] { return waveformPausedAtBuild; }),
                        "Paused preview coverage must reach the waveform-building checkpoint.");
            }

            const auto pausedPreview = pausedPreviewProcessor.getAuthoringWaveformPreview();
            require(pausedPreview.state == "Loading",
                    "Paused preview coverage should leave the shell in a loading state while the worker is blocked.");
            requireNoImportIo("Paused waveform preview authorization");

            {
                std::lock_guard<std::mutex> lock(waveformMutex);
                releaseWaveformBuild = true;
            }
            waveformCondition.notify_all();

            drs::app::AuthoringWaveformPreview resumedPreview;
            require(waitForWaveformPreviewReady(pausedPreviewProcessor, resumedPreview),
                    "Paused preview coverage should still reach a ready waveform preview after release.");
        }

        drs::engine::resetSampleImportIoCounters();
        processor.authorizeAuthoringWaveformPreviewLoad();
        drs::app::AuthoringWaveformPreview defaultPreview;
        require(waitForWaveformPreviewReady(processor, defaultPreview),
                "Default Phase 2 selected zone should publish a ready waveform preview.");
        require(defaultPreview.available, "Default Phase 2 selected zone should expose a waveform preview.");
        require(defaultPreview.formatName == "WAV file", "Phase 2 default waveform fixture format changed unexpectedly.");
        require(defaultPreview.sampleRate == 44100.0, "Phase 2 default waveform fixture sample rate changed unexpectedly.");
        require(defaultPreview.channelCount == 2, "Phase 2 default waveform fixture channel count changed unexpectedly.");
        require(defaultPreview.frameCount == 44100, "Phase 2 default waveform fixture frame count changed unexpectedly.");
        require(defaultPreview.playbackEndFrameExclusive == defaultPreview.frameCount,
                "An omitted playback end must resolve to the physical source end in preview metadata.");
        require(!defaultPreview.points.empty(), "Phase 2 default waveform fixture should include preview points.");
        require(hasVisibleWaveform(defaultPreview),
                "Phase 2 default waveform fixture should render visible waveform amplitude data.");
        require(!defaultPreview.loopEnabled, "Lead waveform preview should begin with looping disabled.");
        require(defaultPreview.loopStartFrame == 0 && defaultPreview.loopEndFrame == 0,
                "Lead waveform preview loop markers changed unexpectedly.");
        const auto defaultPreviewCounters = drs::engine::getSampleImportIoCounters();
        require(defaultPreviewCounters.fullFrameReadCount == 0,
                "Waveform preview should not decode the full sample just to build display peaks.");
        require(defaultPreviewCounters.peakChunkReadCount > 0,
                "Waveform preview should build bounded waveform peaks in chunks.");

        const auto selectionResult = processor.getAuthoringSession().selectZone("pad-a3-high");
        require(selectionResult.applied, "Selecting the looping pad zone should succeed before preview validation.");

        drs::engine::resetSampleImportIoCounters();
        processor.authorizeAuthoringWaveformPreviewLoad();
        drs::app::AuthoringWaveformPreview loopPreview;
        require(waitForWaveformPreviewReady(processor, loopPreview),
                "Looping Phase 2 selected zone should publish a ready waveform preview.");
        require(loopPreview.available, "Looping Phase 2 selected zone should expose a waveform preview.");
        require(loopPreview.sampleRate == 44100.0, "Looping Phase 2 waveform sample rate changed unexpectedly.");
        require(loopPreview.channelCount == 1, "Looping Phase 2 waveform channel count changed unexpectedly.");
        require(loopPreview.frameCount == 88200, "Looping Phase 2 waveform frame count changed unexpectedly.");
        require(loopPreview.playbackEndFrameExclusive == loopPreview.frameCount,
                "Legacy looping zones must preview through the physical source end.");
        require(!loopPreview.points.empty(), "Looping Phase 2 waveform preview should include preview points.");
        require(hasVisibleWaveform(loopPreview),
                "Looping Phase 2 waveform preview should render visible waveform amplitude data.");
        require(loopPreview.loopEnabled, "Looping Phase 2 zone should report loop-enabled preview metadata.");
        require(loopPreview.loopStartFrame == 512, "Looping Phase 2 zone loop start changed unexpectedly.");
        require(loopPreview.loopEndFrame == 22016, "Looping Phase 2 zone loop end changed unexpectedly.");
        const auto loopPreviewCounters = drs::engine::getSampleImportIoCounters();
        require(loopPreviewCounters.fullFrameReadCount == 0,
                "Looping waveform preview should also avoid full-frame decode reads.");
        require(loopPreviewCounters.peakChunkReadCount > 0,
                "Looping waveform preview should record bounded waveform peak chunk reads.");

        auto boundedPreviewZone = *processor.getAuthoringSession().getSelectedZone();
        boundedPreviewZone.sampleEndFrame = 30000;
        require(processor.getAuthoringSession().updateSelectedZone(
                    boundedPreviewZone, "Set preview playback end").applied,
                "Waveform preview coverage must accept a non-destructive playback end edit.");
        processor.authorizeAuthoringWaveformPreviewLoad();
        drs::app::AuthoringWaveformPreview boundedPreview;
        require(waitForWaveformPreviewReady(processor, boundedPreview)
                    && boundedPreview.playbackEndFrameExclusive == 30000,
                "Waveform preview metadata must expose the authored exclusive playback end.");

        const auto leadRender = renderSelectedZonePreview("lead-a4-sustain");
        const auto padRender = renderSelectedZonePreview("pad-a3-high");
        std::cout << "Waveform selected-zone playback peaks: lead=" << bufferPeak(leadRender)
                  << " pad=" << bufferPeak(padRender) << '\n';
        require(buffersDiffer(leadRender, padRender),
                "Selected-zone playback should render different audio for different authoring samples.");

        {
            const auto matrixRoot = makeScratchDirectory();
            const auto monoPath = matrixRoot / "mono.wav";
            const auto stereoPath = matrixRoot / "stereo.wav";
            const auto surroundPath = matrixRoot / "surround.wav";
            const auto silencePath = matrixRoot / "silence.wav";
            const auto shortPath = matrixRoot / "short.wav";
            const auto partialPath = matrixRoot / "partial.wav";
            const auto loopedPath = matrixRoot / "looped.wav";

            writeAudioFile(monoPath, buildPreviewFixtureBuffer(1, 511, 0.35f));
            writeAudioFile(stereoPath, buildPreviewFixtureBuffer(2, 2048, 0.55f));
            writeAudioFile(surroundPath, buildPreviewFixtureBuffer(4, 513, 0.42f));
            writeAudioFile(silencePath, juce::AudioBuffer<float>(2, 1024));
            writeAudioFile(shortPath, buildPreviewFixtureBuffer(2, 16, 0.7f));
            writeAudioFile(partialPath, buildPreviewFixtureBuffer(2, 997, 0.3f));

            juce::StringPairArray loopMetadata;
            loopMetadata.set("MidiUnityNote", "60");
            loopMetadata.set("NumSampleLoops", "1");
            loopMetadata.set("Loop0Start", "128");
            loopMetadata.set("Loop0End", "1536");
            writeAudioFile(loopedPath, buildPreviewFixtureBuffer(2, 2048, 0.48f), loopMetadata);

            struct ComparisonCase
            {
                const char* name;
                fs::path path;
                bool expectLoopRange = false;
                std::uint64_t expectedLoopStart = 0;
                std::uint64_t expectedLoopEnd = 0;
            };

            const std::array<ComparisonCase, 7> comparisonCases
            {
                ComparisonCase{ "mono", monoPath, false, 0, 0 },
                ComparisonCase{ "stereo", stereoPath, false, 0, 0 },
                ComparisonCase{ "surround", surroundPath, false, 0, 0 },
                ComparisonCase{ "silence", silencePath, false, 0, 0 },
                ComparisonCase{ "short", shortPath, false, 0, 0 },
                ComparisonCase{ "partial", partialPath, false, 0, 0 },
                ComparisonCase{ "looped", loopedPath, true, 128, 1536 }
            };

            for (const auto& comparisonCase : comparisonCases)
            {
                drs::engine::WaveformPeakBuildOptions options;
                options.displayPointCount = 192;
                options.chunkFrameCount = 64;
                options.channelReduction = drs::engine::WaveformPeakChannelReduction::channelExtrema;
                const auto incremental = drs::engine::buildWaveformPeaks(comparisonCase.path.generic_string(),
                                                                         {},
                                                                         options);
                require(incremental.built,
                        std::string("Incremental waveform peak build should succeed for the ")
                            + comparisonCase.name + " case.");

                const auto decoded = decodeWaveformReference(comparisonCase.path);
                const auto reference = buildReferenceWaveformPeaks(
                    decoded,
                    options.displayPointCount,
                    options.channelReduction);
                requireEquivalentWaveformPeaks(
                    incremental,
                    reference,
                    std::string("Waveform peak equivalence for the ") + comparisonCase.name + " case");

                if (comparisonCase.expectLoopRange)
                {
                    require(incremental.metadata.loopRangePresent
                                && incremental.metadata.loopStartFrame == comparisonCase.expectedLoopStart
                                && incremental.metadata.loopEndFrame == comparisonCase.expectedLoopEnd,
                            "Looped waveform peak equivalence case should preserve embedded loop metadata.");
                }
            }
        }

        {
            const auto scratch = makeScratchDirectory();
            fs::create_directories(scratch / "Samples");
            const auto scratchSamplePath = scratch / "Samples" / "scratch-preview.wav";
            writeAudioFile(scratchSamplePath, buildPreviewFixtureBuffer(2, 4096, 0.25f));

            drs::plugin::Processor scratchProcessor;
            require(scratchProcessor.replaceAuthoringProject(makeScratchProject(scratch, scratchSamplePath)),
                    "Waveform preview scratch processor must accept the temporary project.");

            drs::engine::resetSampleImportIoCounters();
            scratchProcessor.authorizeAuthoringWaveformPreviewLoad();
            drs::app::AuthoringWaveformPreview firstScratchPreview;
            require(waitForWaveformPreviewReady(scratchProcessor, firstScratchPreview),
                    "Waveform preview scratch project should publish a ready initial preview.");
            require(firstScratchPreview.frameCount == 4096,
                    "Waveform preview scratch project initial frame count changed unexpectedly.");
            const auto firstScratchCounters = drs::engine::getSampleImportIoCounters();
            require(firstScratchCounters.fullFrameReadCount == 0
                        && firstScratchCounters.peakChunkReadCount > 0,
                    "Scratch waveform preview should use chunked peak reads without full-frame decode.");

            drs::engine::resetSampleImportIoCounters();
            scratchProcessor.authorizeAuthoringWaveformPreviewLoad();
            const auto cachedScratchPreview = scratchProcessor.getAuthoringWaveformPreview();
            require(cachedScratchPreview.available
                        && cachedScratchPreview.state == "Ready"
                        && cachedScratchPreview.frameCount == firstScratchPreview.frameCount,
                    "Unchanged scratch waveform selections should reuse the cached preview.");
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            const auto cachedScratchCounters = drs::engine::getSampleImportIoCounters();
            require(cachedScratchCounters.readerOpenCount == 0
                        && cachedScratchCounters.fingerprintOpenCount == 0
                        && cachedScratchCounters.peakChunkReadCount == 0,
                    "Unchanged scratch waveform selections must not trigger another background peak build.");

            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            writeAudioFile(scratchSamplePath, buildPreviewFixtureBuffer(2, 8192, 0.55f));
            require(juce::File(scratchSamplePath.generic_string()).getSize() > 0,
                    "Changed scratch waveform source should remain present after rewrite.");
            const auto rebuiltDirectly = drs::engine::buildWaveformPeaks(scratchSamplePath.generic_string());
            require(rebuiltDirectly.built && rebuiltDirectly.metadata.frameCount == 8192,
                    "Changed scratch waveform source rewrite should be visible to direct waveform peak building.");

            drs::engine::resetSampleImportIoCounters();
            scratchProcessor.authorizeAuthoringWaveformPreviewLoad();
            drs::app::AuthoringWaveformPreview refreshedScratchPreview;
            require(waitForWaveformPreviewReady(scratchProcessor, refreshedScratchPreview),
                    "Changed scratch waveform sources should publish a refreshed ready preview.");
            require(refreshedScratchPreview.frameCount == 8192,
                    "Changed scratch waveform sources must invalidate and replace the cached preview. "
                        "Observed state=" + refreshedScratchPreview.state
                        + ", frameCount=" + std::to_string(refreshedScratchPreview.frameCount)
                        + ", initialFrameCount=" + std::to_string(firstScratchPreview.frameCount) + ".");
            const auto refreshedScratchCounters = drs::engine::getSampleImportIoCounters();
            require(refreshedScratchCounters.fullFrameReadCount == 0
                        && refreshedScratchCounters.peakChunkReadCount > 0,
                    "Changed scratch waveform sources should rebuild chunked peaks instead of reusing stale cache.");
        }

        std::cout << "Phase 2 waveform preview tests passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Phase 2 waveform preview tests failed: " << exception.what() << std::endl;
        return 1;
    }
}
