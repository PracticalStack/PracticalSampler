#include "drs/engine/RuntimeLoader.h"
#include "plugin/PluginProcessor.h"

#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors_headless/juce_audio_processors_headless.h>

#include <algorithm>
#include <iostream>
#include <filesystem>
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

bool containsFinding(const std::vector<drs::engine::PlaybackSnapshotFinding>& findings,
                     drs::engine::PlaybackSnapshotFindingSeverity severity,
                     const std::string& code,
                     const std::string& pathFragment)
{
    for (const auto& finding : findings)
    {
        if (finding.severity == severity
            && finding.code == code
            && finding.path.find(pathFragment) != std::string::npos)
        {
            return true;
        }
    }

    return false;
}

fs::path getScratchDirectory()
{
    auto path = fs::temp_directory_path() / "drs-phase1-realtime-safety-tests";
    fs::create_directories(path);
    return path;
}

juce::AudioBuffer<float> buildMultiChannelBuffer(int channelCount)
{
    constexpr int frameCount = 480;
    juce::AudioBuffer<float> buffer(channelCount, frameCount);

    for (int channelIndex = 0; channelIndex < channelCount; ++channelIndex)
    {
        for (int sampleIndex = 0; sampleIndex < frameCount; ++sampleIndex)
        {
            const auto phase = static_cast<float>(sampleIndex) / static_cast<float>(frameCount);
            buffer.setSample(channelIndex,
                             sampleIndex,
                             std::sin((phase * juce::MathConstants<float>::twoPi)
                                      + (0.2f * static_cast<float>(channelIndex))) * 0.25f);
        }
    }

    return buffer;
}

void writeAudioFile(const fs::path& filePath,
                    juce::AudioFormat& format,
                    const juce::AudioBuffer<float>& buffer,
                    double sampleRate = 48000.0)
{
    auto fileOutput = std::make_unique<juce::FileOutputStream>(juce::File(filePath.generic_string()));
    require(fileOutput->openedOk(), "Could not open output file for writing: " + filePath.generic_string());
    std::unique_ptr<juce::OutputStream> output = std::move(fileOutput);

    juce::AudioFormatWriterOptions options;
    options = options.withSampleRate(sampleRate)
        .withNumChannels(buffer.getNumChannels())
        .withBitsPerSample(24);

    auto writerOwner = format.createWriterFor(output, options);
    require(writerOwner != nullptr, "Could not create audio writer for: " + filePath.generic_string());
    require(writerOwner->writeFromAudioSampleBuffer(buffer, 0, buffer.getNumSamples()),
            "Could not write audio samples to: " + filePath.generic_string());
}

struct ReadyAuthoringPreviewContext
{
    std::size_t revision = 0;
    drs::engine::RuntimeProjectZoneDefinition zone;
};
} // namespace

int main()
{
    try
    {
        drs::plugin::Processor processor;
        processor.prepareToPlay(44100.0, 512);

        const auto primedSnapshot = processor.getRealtimeSafetySnapshot();
        require(primedSnapshot.available, "Realtime safety snapshot must be available.");
        require(primedSnapshot.preparedBlockSize == 512, "Realtime safety snapshot should remember the prepared block size.");
        require(primedSnapshot.referenceSampleCountLoaded >= 1,
                "Performance playback samples should be preloaded before the callback runs.");
        require(primedSnapshot.referenceWarmupCount >= 1,
                "Realtime safety snapshot should record an off-audio-thread warmup pass.");
        require(primedSnapshot.referenceSampleLoadsOnAudioThread == 0,
                "Preparing the processor should keep reference sample loading off the audio thread.");
        require(primedSnapshot.activePublishedRevision == 0,
                "Realtime safety snapshot should surface the bootstrap published revision as the active activation.");
        require(primedSnapshot.pendingPublishedRevision == 0,
                "Realtime safety snapshot should begin without a queued published activation.");
        require(primedSnapshot.activeVoiceCapacity >= primedSnapshot.activeVoiceCapacityLimit,
                "Active-voice storage should be reserved before realtime playback begins.");
        require(primedSnapshot.getAudioThreadViolationCount() == 0,
                "Primed processor should not report realtime-thread safety violations.");

        juce::AudioBuffer<float> buffer(2, 512);
        buffer.clear();
        juce::MidiBuffer hostMidi;
        hostMidi.addEvent(juce::MidiMessage::noteOn(1, 57, static_cast<juce::uint8>(100)), 0);
        processor.processBlock(buffer, hostMidi);

        require(buffer.getMagnitude(0, buffer.getNumSamples()) > 0.0001f,
                "Host MIDI note-on should render audible output through the performance path.");

        auto playbackSnapshot = processor.getRealtimeSafetySnapshot();
        require(playbackSnapshot.processBlockCount >= 1, "Realtime safety snapshot should count processed callbacks.");
        require(playbackSnapshot.callbackBudgetMicros > 0,
                "Realtime safety snapshot should expose a non-zero callback budget.");
        require(playbackSnapshot.referenceSampleLoadsOnAudioThread == 0,
                "First rendered note should not trigger reference sample I/O on the audio thread.");
        require(playbackSnapshot.activeVoiceCapacityGrowthCount == 0,
                "Voice allocation should not force the active-voice vector to grow in the callback.");
        require(playbackSnapshot.getAudioThreadViolationCount() == 0,
                "Performance callback should remain free of tracked realtime safety violations.");

        for (int index = 0; index < 32; ++index)
            processor.queuePerformanceSurfaceNoteOn(57 + (index % 3), 0.75f);

        juce::AudioBuffer<float> queuedBuffer(2, 512);
        queuedBuffer.clear();
        juce::MidiBuffer emptyMidi;
        processor.processBlock(queuedBuffer, emptyMidi);

        require(queuedBuffer.getMagnitude(0, queuedBuffer.getNumSamples()) > 0.0001f,
                "Queued performance-surface notes should render audible output.");

        playbackSnapshot = processor.getRealtimeSafetySnapshot();
        require(playbackSnapshot.processBlockCount >= 2,
                "Realtime safety snapshot should continue counting later callbacks.");
        require(playbackSnapshot.referenceSampleLoadsOnAudioThread == 0,
                "Queued performance playback should keep reference sample I/O off the callback thread.");
        require(playbackSnapshot.activeVoiceCapacityGrowthCount == 0,
                "Burst note starts should respect the pre-reserved active-voice capacity.");
        require(playbackSnapshot.getAudioThreadViolationCount() == 0,
                "Tracked realtime safety violations should remain at zero after burst playback.");

        const auto projectLoad = drs::engine::loadPhase2ReferenceProjectManifest();
        require(projectLoad.loaded, "Authoring preview isolation test should load the Phase 2 reference project.");
        const auto scratchDirectory = getScratchDirectory();
        const auto unsupportedPath = scratchDirectory / "unsupported-preview-source.txt";
        const auto surroundPath = scratchDirectory / "surround-preview-source.wav";
        const auto surroundBuffer = buildMultiChannelBuffer(4);
        juce::WavAudioFormat wavFormat;
        writeAudioFile(surroundPath, wavFormat, surroundBuffer);
        {
            juce::FileOutputStream unsupportedOutput(juce::File(unsupportedPath.generic_string()));
            require(unsupportedOutput.openedOk(), "Could not create unsupported realtime preview fixture.");
            unsupportedOutput.writeText("not audio", false, false, nullptr);
        }

        const auto stageReadyPreview = [&](drs::plugin::Processor& previewProcessor,
                                           const std::string& failureMessagePrefix) -> ReadyAuthoringPreviewContext
        {
            previewProcessor.prepareToPlay(44100.0, 512);
            previewProcessor.replaceAuthoringProject(projectLoad.project);

            const auto selection = previewProcessor.getAuthoringSession().selectZone("pad-a3-high");
            require(selection.applied, failureMessagePrefix + " should select the looping pad zone.");
            const auto readyRevision = previewProcessor.getAuthoringSession().getDocumentState().revision;
            require(previewProcessor.serviceMessageThreadWork(),
                    failureMessagePrefix + " should stage the selected-zone preview revision.");

            juce::AudioBuffer<float> previewBuffer(2, 512);
            previewBuffer.clear();
            juce::MidiBuffer emptyMidi;
            previewProcessor.processBlock(previewBuffer, emptyMidi);

            const auto snapshot = previewProcessor.getRealtimeSafetySnapshot();
            require(snapshot.activeAuthoringPreviewRevision == readyRevision,
                    failureMessagePrefix + " should activate the ready preview revision before the invalid edit lands.");
            require(snapshot.authoringPreviewRevisionState == "Ready",
                    failureMessagePrefix + " should report a ready authoring preview before the invalid edit lands.");

            const auto selectedZone = previewProcessor.getAuthoringSession().getSelectedZone();
            require(selectedZone.has_value(), failureMessagePrefix + " should keep the selected zone available.");
            return { readyRevision, *selectedZone };
        };

        const auto replaceSelectedZonePath = [&](const ReadyAuthoringPreviewContext& context,
                                                 const fs::path& samplePath)
        {
            auto editedProject = projectLoad.project;
            auto sampleSourceIterator = std::find_if(
                editedProject.sampleSources.begin(),
                editedProject.sampleSources.end(),
                [&](const drs::engine::RuntimeProjectSampleSource& sampleSource)
                {
                    return sampleSource.id == context.zone.sampleSourceId;
                });
            require(sampleSourceIterator != editedProject.sampleSources.end(),
                    "Preview failure coverage should locate the selected-zone sample source before mutating it.");
            sampleSourceIterator->path = samplePath.generic_string();
            editedProject.authoring.selectedZoneId = context.zone.id;
            return editedProject;
        };

        const auto requireFailedPreviewHint = [&](const fs::path& samplePath,
                                                  const std::string& expectedPrerequisite,
                                                  const std::string& guidanceNeedle,
                                                  const std::string& failureNeedle,
                                                  const std::string& caseLabel)
        {
            drs::plugin::Processor previewProcessor;
            const auto context = stageReadyPreview(previewProcessor, caseLabel);
            previewProcessor.replaceAuthoringProject(replaceSelectedZonePath(context, samplePath));

            const auto snapshot = previewProcessor.getRealtimeSafetySnapshot();
            require(snapshot.activeAuthoringPreviewRevision == context.revision,
                    caseLabel + " should preserve the last known-good preview revision.");
            require(snapshot.pendingAuthoringPreviewRevision == 0,
                    caseLabel + " should not leave a pending authoring preview activation behind.");
            require(snapshot.authoringPreviewRevisionState == "Failed",
                    caseLabel + " should surface a failed authoring preview state.");
            require(snapshot.authoringPreviewFailureState.find(failureNeedle) != std::string::npos,
                    caseLabel + " should expose the detailed preview failure cause.");

            const auto previewStatus = previewProcessor.getAuthoringPreviewStatusSnapshot();
            require(previewStatus.blockingPrerequisite == expectedPrerequisite,
                    caseLabel + " should surface the expected next prerequisite.");
            require(previewStatus.blockingGuidance.find(guidanceNeedle) != std::string::npos,
                    caseLabel + " should explain the next repair step.");
        };

        processor.replaceAuthoringProject(projectLoad.project);

        const auto authoringSelection = processor.getAuthoringSession().selectZone("pad-a3-high");
        require(authoringSelection.applied,
                "Authoring preview isolation test should select the looping pad zone.");
        const auto selectedPreviewRevision = processor.getAuthoringSession().getDocumentState().revision;
        require(processor.serviceMessageThreadWork(),
                "Message-thread servicing should stage the selected authoring preview revision.");

        const auto selectedZone = processor.getAuthoringSession().getSelectedZone();
        require(selectedZone.has_value(),
                "Authoring preview isolation test should keep the selected zone available.");

        auto previewActivationSnapshot = processor.getRealtimeSafetySnapshot();
        require(previewActivationSnapshot.activeAuthoringPreviewRevision == 0,
                "Authoring preview should keep the previous selected-zone activation active until the next block boundary.");
        require(previewActivationSnapshot.pendingAuthoringPreviewRevision == selectedPreviewRevision,
                "Message-thread servicing should queue the selected authoring revision as a pending preview activation.");
        require(previewActivationSnapshot.authoringPreviewRevisionState == "Preparing",
                "Queued authoring preview revisions should surface a preparing state before the callback handoff.");

        processor.queuePerformanceSurfaceNoteOn(57, 0.8f);

        juce::AudioBuffer<float> isolationBuffer(2, 512);
        isolationBuffer.clear();
        processor.processBlock(isolationBuffer, emptyMidi);

        auto isolationSnapshot = processor.getRealtimeSafetySnapshot();
        require(isolationSnapshot.performanceActiveVoiceCount >= 1,
                "Performance voice should remain active before authoring preview playback joins the render pass.");
        require(isolationSnapshot.authoringPreviewActiveVoiceCount == 0,
                "Authoring preview pool should begin empty before preview notes are queued.");
        require(isolationSnapshot.activeAuthoringPreviewRevision == selectedPreviewRevision,
                "Audio callback should activate the selected authoring revision at the block boundary.");
        require(isolationSnapshot.pendingAuthoringPreviewRevision == 0,
                "Pending authoring preview activation should clear once the block-boundary handoff runs.");
        require(isolationSnapshot.authoringPreviewRevisionState == "Ready",
                "Active authoring preview revisions should surface a ready state after the block-boundary handoff.");

        for (int index = 0; index < 32; ++index)
            processor.queueAuthoringPreviewNoteOn(selectedZone->rootKey + (index % 3), 0.7f);

        isolationBuffer.clear();
        processor.processBlock(isolationBuffer, emptyMidi);

        isolationSnapshot = processor.getRealtimeSafetySnapshot();
        require(isolationSnapshot.performanceActiveVoiceCount >= 1,
                "Authoring preview note pressure should not steal the live performance voice.");
        require(isolationSnapshot.authoringPreviewActiveVoiceCount >= 1,
                "Authoring preview notes should populate their own realtime voice pool.");
        require(isolationSnapshot.authoringSampleLoadsOnAudioThread == 0,
                "Selected authoring sample should be warmed before preview playback reaches the callback thread.");
        require(isolationSnapshot.activeVoiceCapacityGrowthCount == 0,
                "Preview voice bursts should stay within the pre-reserved realtime voice capacity.");
        require(isolationSnapshot.getAudioThreadViolationCount() == 0,
                "Voice-pool isolation should keep the callback free of tracked realtime violations.");

        drs::plugin::Processor failedPreviewProcessor;
        failedPreviewProcessor.prepareToPlay(44100.0, 512);
        failedPreviewProcessor.replaceAuthoringProject(projectLoad.project);

        const auto failedPreviewSelection = failedPreviewProcessor.getAuthoringSession().selectZone("pad-a3-high");
        require(failedPreviewSelection.applied,
                "Failed-preview regression test should select the looping pad zone.");
        const auto readyPreviewRevision = failedPreviewProcessor.getAuthoringSession().getDocumentState().revision;
        require(failedPreviewProcessor.serviceMessageThreadWork(),
                "Failed-preview regression test should stage the selected-zone preview revision.");

        juce::AudioBuffer<float> failedPreviewBuffer(2, 512);
        failedPreviewBuffer.clear();
        failedPreviewProcessor.processBlock(failedPreviewBuffer, emptyMidi);

        auto failedPreviewSnapshot = failedPreviewProcessor.getRealtimeSafetySnapshot();
        require(failedPreviewSnapshot.activeAuthoringPreviewRevision == readyPreviewRevision,
                "Failed-preview regression test should activate the last known-good preview revision first.");
        require(failedPreviewSnapshot.authoringPreviewRevisionState == "Ready",
                "Last known-good preview should report a ready state before the invalid edit lands.");

        auto invalidProject = failedPreviewProcessor.getAuthoringSession().getProject();
        const auto failedSelectedZone = failedPreviewProcessor.getAuthoringSession().getSelectedZone();
        require(failedSelectedZone.has_value(),
                "Failed-preview regression test should keep the selected zone available before the invalid edit lands.");
        const auto selectedZoneId = failedSelectedZone->id;
        auto sampleSourceIterator = std::find_if(invalidProject.sampleSources.begin(),
                                                 invalidProject.sampleSources.end(),
                                                 [&](const drs::engine::RuntimeProjectSampleSource& sampleSource)
                                                 {
                                                     return sampleSource.id == failedSelectedZone->sampleSourceId;
                                                 });
        require(sampleSourceIterator != invalidProject.sampleSources.end(),
                "Failed-preview regression test should locate the selected-zone sample source before breaking it.");
        sampleSourceIterator->path = invalidProject.contentRootPath + "/missing-preview-sample.wav";
        invalidProject.authoring.selectedZoneId = selectedZoneId;
        failedPreviewProcessor.replaceAuthoringProject(invalidProject);

        failedPreviewSnapshot = failedPreviewProcessor.getRealtimeSafetySnapshot();
        require(failedPreviewSnapshot.activeAuthoringPreviewRevision == readyPreviewRevision,
                "Invalid selected-zone edits should preserve the last known-good preview revision.");
        require(failedPreviewSnapshot.pendingAuthoringPreviewRevision == 0,
                "Failed preview preparation should not leave a pending activation behind.");
        require(failedPreviewSnapshot.authoringPreviewRevisionState == "Failed",
                "Invalid selected-zone edits should surface a failed preview state.");
        require(!failedPreviewSnapshot.authoringPreviewFailureState.empty(),
                "Invalid selected-zone edits should expose a blocking preview failure message.");

        failedPreviewProcessor.queueAuthoringPreviewNoteOn(failedSelectedZone->rootKey, 0.7f);
        auto failedPreviewMagnitude = 0.0f;
        for (int blockIndex = 0; blockIndex < 2; ++blockIndex)
        {
            failedPreviewBuffer.clear();
            failedPreviewProcessor.processBlock(failedPreviewBuffer, emptyMidi);
            failedPreviewMagnitude = std::max(
                failedPreviewMagnitude,
                failedPreviewBuffer.getMagnitude(0, failedPreviewBuffer.getNumSamples()));
        }

        failedPreviewSnapshot = failedPreviewProcessor.getRealtimeSafetySnapshot();
        require(failedPreviewMagnitude > 0.0001f,
                "Invalid preview edits should keep the last known-good preview audible.");
        require(failedPreviewSnapshot.activeAuthoringPreviewRevision == readyPreviewRevision,
                "Auditioning after a failed preview edit should continue using the last known-good preview revision.");
        require(failedPreviewSnapshot.authoringPreviewRevisionState == "Failed",
                "Auditioning after a failed preview edit should keep the failed status visible until the draft changes.");
        const auto failedPreviewStatus = failedPreviewProcessor.getAuthoringPreviewStatusSnapshot();
        require(failedPreviewStatus.blockingPrerequisite == "Relink or re-import the selected sample file.",
                "Failed preview status should surface the next prerequisite for a missing sample file.");
        require(failedPreviewStatus.blockingGuidance.find("missing-preview-sample.wav") != std::string::npos,
                "Failed preview guidance should identify the missing sample file that must be repaired.");
        require(failedPreviewSnapshot.authoringSampleLoadsOnAudioThread == 0,
                "Failed preview fallback should not reload authoring samples on the audio thread.");
        require(failedPreviewSnapshot.getAudioThreadViolationCount() == 0,
                "Failed preview fallback should remain free of tracked realtime violations.");

        requireFailedPreviewHint(unsupportedPath,
                                 "Convert the selected sample to a supported format.",
                                 "supported WAV, AIFF, or FLAC file",
                                 "supported audio format",
                                 "Unsupported-format preview failure");
        requireFailedPreviewHint(surroundPath,
                                 "Use a mono or stereo sample for this zone.",
                                 "mono or stereo file",
                                 "mono and stereo",
                                 "Channel-policy preview failure");

        const auto phase1Project = drs::engine::loadPhase1ReferenceProjectManifest();
        require(phase1Project.loaded,
                "Processor integration coverage should load the Phase 1 reference project before migration.");
        const auto migratedPhase1 = drs::engine::migrateRuntimeProjectToPhase2Authoring(phase1Project.project);
        require(migratedPhase1.valid,
                "Processor integration coverage should migrate the Phase 1 reference project before preview/publish.");

        drs::plugin::Processor migratedProcessor;
        migratedProcessor.prepareToPlay(44100.0, 512);
        migratedProcessor.replaceAuthoringProject(migratedPhase1.project);

        auto migratedPerformanceSnapshot = migratedProcessor.getEngineFacade().getPerformanceSnapshot();
        require(migratedPerformanceSnapshot.draftRevision == 0,
                "Replacing the processor authoring project should reset the facade draft revision to 0.");
        require(migratedPerformanceSnapshot.previewRevision == 0
                    && migratedPerformanceSnapshot.previewRevisionState == "Idle",
                "Replacing the processor authoring project should reset facade preview state to idle.");
        require(migratedPerformanceSnapshot.publishedRevision == 0
                    && migratedPerformanceSnapshot.publishedRevisionState == "Idle",
                "Replacing the processor authoring project should reset facade published state to idle.");

        require(!migratedProcessor.getEngineFacade().refreshPreviewToCurrentDraft(),
                "Migrated processor project without imported zones should fail preview preparation.");
        migratedPerformanceSnapshot = migratedProcessor.getEngineFacade().getPerformanceSnapshot();
        require(migratedPerformanceSnapshot.previewRevisionState
                    == "Prepared playback build rejected because the immutable snapshot is unavailable",
                "Migrated processor preview failure should surface the rejected prepared-playback state.");
        require(containsFinding(migratedPerformanceSnapshot.previewFindings,
                                drs::engine::PlaybackSnapshotFindingSeverity::error,
                                "no-playable-zones",
                                "authoring.zones"),
                "Migrated processor preview failure should surface the structured no-playable-zones finding.");

        require(!migratedProcessor.getEngineFacade().publishCurrentDraft(),
                "Migrated processor project without imported zones should fail publish preparation.");
        migratedPerformanceSnapshot = migratedProcessor.getEngineFacade().getPerformanceSnapshot();
        require(migratedPerformanceSnapshot.publishedRevisionState
                    == "Prepared playback build rejected because the immutable snapshot is unavailable",
                "Migrated processor publish failure should surface the rejected prepared-playback state.");
        require(containsFinding(migratedPerformanceSnapshot.publishedFindings,
                                drs::engine::PlaybackSnapshotFindingSeverity::error,
                                "no-playable-zones",
                                "authoring.zones"),
                "Migrated processor publish failure should surface the structured no-playable-zones finding.");

        drs::engine::RuntimeProjectSampleSource importedSampleSource;
        importedSampleSource.id = "processor-migrated-sine-a3";
        importedSampleSource.path = phase1Project.project.sampleSources[0].path;
        importedSampleSource.role = "imported-sustain";

        drs::engine::RuntimeProjectZoneDefinition importedZone;
        importedZone.id = "processor-migrated-zone-a3";
        importedZone.sampleSourceId = importedSampleSource.id;
        importedZone.displayName = "Processor Migrated Zone A3";
        importedZone.groupId = "main";
        importedZone.articulationId = "sustain";
        importedZone.rootKey = 57;
        importedZone.keyLow = 57;
        importedZone.keyHigh = 57;
        importedZone.velocityLow = 1;
        importedZone.velocityHigh = 127;

        const auto migratedImport = migratedProcessor.getAuthoringSession().appendImportedContent({ importedSampleSource },
                                                                                                  { importedZone },
                                                                                                  "Import migrated processor zone");
        require(migratedImport.applied,
                "Processor integration coverage should accept imported authoring content.");
        require(migratedProcessor.serviceMessageThreadWork(),
                "Message-thread servicing should sync imported authoring content into the draft-playback facade.");
        migratedPerformanceSnapshot = migratedProcessor.getEngineFacade().getPerformanceSnapshot();
        require(migratedPerformanceSnapshot.draftRevision == migratedImport.documentState.revision,
                "Imported processor authoring content should advance the facade draft revision.");

        require(migratedProcessor.getEngineFacade().refreshPreviewToCurrentDraft(),
                "Imported processor authoring content should prepare preview successfully.");
        require(migratedProcessor.getEngineFacade().waitForPreparedPlaybackIdle(),
                "Imported processor preview should settle through the prepared-playback worker.");
        require(migratedProcessor.serviceMessageThreadWork(),
                "Message-thread servicing should apply the imported processor preview build.");
        migratedPerformanceSnapshot = migratedProcessor.getEngineFacade().getPerformanceSnapshot();
        require(migratedPerformanceSnapshot.previewRevision == migratedImport.documentState.revision
                    && migratedPerformanceSnapshot.previewRevisionState == "Ready",
                "Imported processor authoring content should expose a ready facade preview revision.");
        require(migratedPerformanceSnapshot.previewPreparedSampleCount
                    == migratedProcessor.getAuthoringSession().getProject().sampleSources.size(),
                "Imported processor preview should materialize every migrated sample identity.");

        require(migratedProcessor.getEngineFacade().publishCurrentDraft(),
                "Imported processor authoring content should publish successfully.");
        require(migratedProcessor.getEngineFacade().waitForPreparedPlaybackIdle(),
                "Imported processor publish should settle through the prepared-playback worker.");
        require(migratedProcessor.serviceMessageThreadWork(),
                "Message-thread servicing should apply the imported processor publish build.");
        migratedPerformanceSnapshot = migratedProcessor.getEngineFacade().getPerformanceSnapshot();
        require(migratedPerformanceSnapshot.loaded,
                "Imported processor publish should expose a loaded performance context.");
        require(migratedPerformanceSnapshot.publishedRevision == migratedImport.documentState.revision
                    && migratedPerformanceSnapshot.publishedRevisionState == "Active",
                "Imported processor authoring content should expose an active facade published revision.");

        auto editedMigratedZone = *migratedProcessor.getAuthoringSession().getSelectedZone();
        editedMigratedZone.gainDb = 2.5;
        editedMigratedZone.pan = -0.2;
        const auto migratedEdit = migratedProcessor.getAuthoringSession().updateSelectedZone(
            editedMigratedZone,
            "Shape migrated processor zone");
        require(migratedEdit.applied,
                "Processor integration coverage should commit edited authoring content.");
        require(migratedProcessor.serviceMessageThreadWork(),
                "Message-thread servicing should sync edited authoring content into the draft-playback facade.");
        migratedPerformanceSnapshot = migratedProcessor.getEngineFacade().getPerformanceSnapshot();
        require(migratedPerformanceSnapshot.draftRevision == migratedEdit.documentState.revision,
                "Edited processor authoring content should advance the facade draft revision.");
        require(migratedPerformanceSnapshot.previewRevision == migratedImport.documentState.revision
                    && migratedPerformanceSnapshot.previewRevisionState == "Stale",
                "Edited processor authoring content should leave facade preview stale on the last prepared revision.");
        require(migratedPerformanceSnapshot.publishedRevision == migratedImport.documentState.revision
                    && migratedPerformanceSnapshot.publishedRevisionState == "Active",
                "Edited processor authoring content should preserve the last active published revision.");

        require(migratedProcessor.getEngineFacade().refreshPreviewToCurrentDraft(),
                "Edited processor authoring content should prepare preview successfully.");
        require(migratedProcessor.getEngineFacade().waitForPreparedPlaybackIdle(),
                "Edited processor preview should settle through the prepared-playback worker.");
        require(migratedProcessor.serviceMessageThreadWork(),
                "Message-thread servicing should apply the edited processor preview build.");
        migratedPerformanceSnapshot = migratedProcessor.getEngineFacade().getPerformanceSnapshot();
        require(migratedPerformanceSnapshot.previewRevision == migratedEdit.documentState.revision
                    && migratedPerformanceSnapshot.previewRevisionState == "Ready",
                "Edited processor authoring content should advance facade preview to the current revision.");

        require(migratedProcessor.getEngineFacade().publishCurrentDraft(),
                "Edited processor authoring content should publish successfully.");
        require(migratedProcessor.getEngineFacade().waitForPreparedPlaybackIdle(),
                "Edited processor publish should settle through the prepared-playback worker.");
        require(migratedProcessor.serviceMessageThreadWork(),
                "Message-thread servicing should apply the edited processor publish build.");
        migratedPerformanceSnapshot = migratedProcessor.getEngineFacade().getPerformanceSnapshot();
        require(migratedPerformanceSnapshot.publishedRevision == migratedEdit.documentState.revision
                    && migratedPerformanceSnapshot.publishedRevisionState == "Active",
                "Edited processor authoring content should advance the active facade published revision.");
        require(migratedPerformanceSnapshot.previewContentDigest == migratedPerformanceSnapshot.publishedContentDigest,
                "Edited processor publish should realign preview and publish snapshot digests.");
        require(migratedPerformanceSnapshot.previewPreparedContentDigest
                    == migratedPerformanceSnapshot.publishedPreparedContentDigest,
                "Edited processor publish should realign preview and publish prepared-playback digests.");

        auto invalidMigratedProject = migratedProcessor.getAuthoringSession().getProject();
        invalidMigratedProject.authoring.zones[0].sampleSourceId = "missing-source";
        const auto stableProcessorRevision = migratedProcessor.getAuthoringSession().getDocumentState().revision;
        const auto stableProcessorSnapshot = migratedProcessor.getEngineFacade().getPerformanceSnapshot();
        migratedProcessor.replaceAuthoringProject(invalidMigratedProject);
        migratedPerformanceSnapshot = migratedProcessor.getEngineFacade().getPerformanceSnapshot();
        require(migratedProcessor.getAuthoringSession().getDocumentState().revision == stableProcessorRevision,
                "Rejected processor authoring-project replacements must preserve the current authoring document revision.");
        require(migratedPerformanceSnapshot.draftRevision == stableProcessorSnapshot.draftRevision,
                "Rejected processor authoring-project replacements must preserve the facade draft revision.");
        require(migratedPerformanceSnapshot.previewRevision == stableProcessorSnapshot.previewRevision
                    && migratedPerformanceSnapshot.previewRevisionState == stableProcessorSnapshot.previewRevisionState,
                "Rejected processor authoring-project replacements must preserve preview state.");
        require(migratedPerformanceSnapshot.publishedRevision == stableProcessorSnapshot.publishedRevision
                    && migratedPerformanceSnapshot.publishedRevisionState == stableProcessorSnapshot.publishedRevisionState,
                "Rejected processor authoring-project replacements must preserve published state.");

        require(processor.getEngineFacade().stageDraftRevision(1),
                "Activation handoff test should stage a new draft revision.");
        require(processor.getEngineFacade().refreshPreviewToCurrentDraft(),
                "Activation handoff test should prepare preview before publish.");
        require(processor.getEngineFacade().waitForPreparedPlaybackIdle(),
                "Activation handoff test should let preview preparation settle.");
        require(processor.getEngineFacade().publishCurrentDraft(),
                "Activation handoff test should publish the prepared draft.");
        require(processor.getEngineFacade().waitForPreparedPlaybackIdle(),
                "Activation handoff test should let publish preparation settle.");
        require(processor.serviceMessageThreadWork(),
                "Message-thread servicing should stage the published activation into the processor handoff seam.");

        auto activationSnapshot = processor.getRealtimeSafetySnapshot();
        require(activationSnapshot.activePublishedRevision == 0,
                "Published activation should remain on the previous revision until the callback reaches a block boundary.");
        require(activationSnapshot.pendingPublishedRevision == 1,
                "Message-thread servicing should queue the next published revision as a pending activation.");
        require(activationSnapshot.pendingPreparedBuildId != 0,
                "Pending activation should carry the prepared build identity for the published revision.");

        juce::AudioBuffer<float> activationBuffer(2, 128);
        activationBuffer.clear();
        processor.processBlock(activationBuffer, emptyMidi);

        activationSnapshot = processor.getRealtimeSafetySnapshot();
        require(activationSnapshot.activePublishedRevision == 1,
                "Audio callback should activate the new published revision at the block boundary.");
        require(activationSnapshot.pendingPublishedRevision == 0,
                "Pending published activation should clear once the block-boundary handoff runs.");
        require(activationSnapshot.performanceActivationCount >= 2,
                "Realtime safety snapshot should count both bootstrap and block-boundary published activations.");
        require(activationSnapshot.retiredActivationBacklog >= 1,
                "Superseded published activations should be retired away from the audio thread.");

        require(processor.serviceMessageThreadWork(),
                "Message-thread servicing should drain retired published activations after the callback handoff.");
        activationSnapshot = processor.getRealtimeSafetySnapshot();
        require(activationSnapshot.retiredActivationBacklog == 0,
                "Retired published activations should drain after message-thread servicing.");
        require(activationSnapshot.retiredActivationCount >= 1,
                "Realtime safety snapshot should count retired published activations.");

        std::cout << "Phase 1 realtime safety tests passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Phase 1 realtime safety tests failed: " << exception.what() << std::endl;
        return 1;
    }
}
