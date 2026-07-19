#include "plugin/PluginProcessor.h"
#include "plugin/RealtimeGuard.h"
#include "drs/engine/RuntimeLoader.h"

#include <juce_audio_basics/juce_audio_basics.h>

#include <cstdlib>
#include <iostream>
#include <new>
#include <stdexcept>
#include <string>
#include <vector>

#if defined(_MSC_VER)
#include <malloc.h>
#endif

namespace
{
void notifyAllocation() noexcept
{
    drs::plugin::recordRealtimeGuardOperation(drs::plugin::RealtimeGuardOperation::allocation);
}

void notifyDeallocation(void* pointer) noexcept
{
    if (pointer != nullptr)
        drs::plugin::recordRealtimeGuardOperation(drs::plugin::RealtimeGuardOperation::deallocation);
}

void* allocateRaw(std::size_t size)
{
    notifyAllocation();
    if (auto* pointer = std::malloc(size == 0 ? 1 : size))
        return pointer;
    throw std::bad_alloc {};
}

void* allocateAlignedRaw(std::size_t size, std::size_t alignment)
{
    notifyAllocation();
#if defined(_MSC_VER)
    if (auto* pointer = _aligned_malloc(size == 0 ? 1 : size, alignment))
        return pointer;
#else
    void* pointer = nullptr;
    if (posix_memalign(&pointer, alignment, size == 0 ? 1 : size) == 0)
        return pointer;
#endif
    throw std::bad_alloc {};
}

void freeAlignedRaw(void* pointer) noexcept
{
    notifyDeallocation(pointer);
#if defined(_MSC_VER)
    _aligned_free(pointer);
#else
    std::free(pointer);
#endif
}

void require(bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

std::size_t operationCount(const drs::plugin::ProcessorRealtimeSafetySnapshot& snapshot,
                           drs::plugin::RealtimeGuardOperation operation)
{
    using Operation = drs::plugin::RealtimeGuardOperation;
    switch (operation)
    {
        case Operation::allocation: return snapshot.allocationsOnAudioThread;
        case Operation::deallocation: return snapshot.deallocationsOnAudioThread;
        case Operation::blockingLock: return snapshot.blockingLockAttemptsOnAudioThread;
        case Operation::wait: return snapshot.waitsOnAudioThread;
        case Operation::fileOpen: return snapshot.fileOpenEntriesOnAudioThread;
        case Operation::fileRead: return snapshot.fileReadEntriesOnAudioThread;
        case Operation::pathResolution: return snapshot.samplePathResolutionsOnAudioThread;
        case Operation::sampleDecode: return snapshot.sampleDecodeEntriesOnAudioThread;
        case Operation::streamDecode: return snapshot.streamDecodeEntriesOnAudioThread;
        case Operation::largeResourceDestruction: return snapshot.largeResourceDestructionsOnAudioThread;
        case Operation::finalSharedOwnershipRelease: return snapshot.finalSharedOwnershipReleasesOnAudioThread;
        case Operation::overBudget: return snapshot.overBudgetCallbackCount;
        case Operation::none:
        case Operation::count:
            return 0;
    }
    return 0;
}

void runNegativeCase(drs::plugin::RealtimeGuardOperation operation, const std::string& label)
{
    drs::plugin::Processor processor;
    processor.prepareToPlay(48000.0, 1024);
    processor.setRealtimeGuardTestInjection(operation);

    juce::AudioBuffer<float> buffer(2, 1024);
    juce::MidiBuffer midi;
    processor.processBlock(buffer, midi);

    const auto snapshot = processor.getRealtimeSafetySnapshot();
    require(operationCount(snapshot, operation) >= 1, label + " did not trip its dedicated counter.");
    require(snapshot.getRealtimeGuardFailureCount() >= 1,
            label + " did not fail the aggregate realtime guard.");
    if (operation == drs::plugin::RealtimeGuardOperation::overBudget)
        require(snapshot.getAudioThreadViolationCount() == 0,
                label + " contaminated a prohibited-operation counter.");
    else
        require(snapshot.getAudioThreadViolationCount() >= 1,
                label + " did not fail the prohibited-operation aggregate.");

    const std::vector<drs::plugin::RealtimeGuardOperation> operations {
        drs::plugin::RealtimeGuardOperation::allocation,
        drs::plugin::RealtimeGuardOperation::deallocation,
        drs::plugin::RealtimeGuardOperation::blockingLock,
        drs::plugin::RealtimeGuardOperation::wait,
        drs::plugin::RealtimeGuardOperation::fileOpen,
        drs::plugin::RealtimeGuardOperation::fileRead,
        drs::plugin::RealtimeGuardOperation::pathResolution,
        drs::plugin::RealtimeGuardOperation::sampleDecode,
        drs::plugin::RealtimeGuardOperation::streamDecode,
        drs::plugin::RealtimeGuardOperation::largeResourceDestruction,
        drs::plugin::RealtimeGuardOperation::finalSharedOwnershipRelease
    };
    for (const auto other : operations)
    {
        if (other != operation)
            require(operationCount(snapshot, other) == 0,
                    label + " contaminated another dedicated guard counter.");
    }
}

void runCleanMaximumLoadCase()
{
    using Profile = drs::plugin::RealtimeCallbackBudgetProfile;
    require(Profile::supports(44100.0, Profile::minimumBlockSize),
            "Budget profile should support its 44.1 kHz minimum block boundary.");
    require(Profile::supports(48000.0, Profile::maximumBlockSize),
            "Budget profile should support its 48 kHz maximum block boundary.");
    require(!Profile::supports(96000.0, Profile::maximumBlockSize),
            "Budget profile should reject undeclared sample rates.");
    require(Profile::deadlineMicros(48000.0, 1024) == 21333,
            "48 kHz / 1024 callback deadline should remain deterministic.");

    drs::plugin::Processor processor;
    processor.prepareToPlay(48000.0, static_cast<int>(Profile::maximumBlockSize));
    const auto authoringProject = drs::engine::loadPhase2ReferenceProjectManifest();
    require(authoringProject.loaded, "Maximum-load case should load the authoring preview fixture.");
    processor.replaceAuthoringProject(authoringProject.project);
    require(processor.getAuthoringSession().selectZone("pad-a3-high").applied,
            "Maximum-load case should select an authoring preview zone.");
    require(processor.serviceMessageThreadWork(),
            "Maximum-load case should install the selected preview activation off audio.");
    require(processor.getEngineFacade().refreshPreviewToCurrentDraft()
                && processor.getEngineFacade().waitForPreparedPlaybackIdle()
                && processor.serviceMessageThreadWork(),
            "Maximum-load case should prepare the normalized Preview payload off audio.");
    require(processor.getEngineFacade().publishCurrentDraft()
                && processor.getEngineFacade().waitForPreparedPlaybackIdle()
                && processor.serviceMessageThreadWork(),
            "Maximum-load case should install the normalized Performance payload off audio.");

    for (std::size_t index = 0; index < Profile::targetPolyphonyPerContext; ++index)
    {
        processor.queuePerformanceSurfaceNoteOn(48 + static_cast<int>(index % 24), 0.8f);
        processor.queueAuthoringPreviewNoteOn(48 + static_cast<int>(index % 24), 0.7f);
    }

    juce::MidiBuffer hostMidi;
    const auto queuedNoteEvents = Profile::targetPolyphonyPerContext * Profile::playbackContextCount;
    for (std::size_t index = queuedNoteEvents; index < Profile::maximumEventsPerBlock; ++index)
    {
        hostMidi.addEvent(juce::MidiMessage::controllerEvent(
                              1,
                              20 + static_cast<int>(index % 80),
                              static_cast<int>(index % 128)),
                          static_cast<int>(index % Profile::maximumBlockSize));
    }

    juce::AudioBuffer<float> buffer(2, static_cast<int>(Profile::maximumBlockSize));
    processor.processBlock(buffer, hostMidi);
    const auto snapshot = processor.getRealtimeSafetySnapshot();
    require(snapshot.getRealtimeGuardFailureCount() == 0,
            "Clean maximum-load playback reported a realtime guard failure: alloc="
                + std::to_string(snapshot.allocationsOnAudioThread)
                + ", free=" + std::to_string(snapshot.deallocationsOnAudioThread)
                + ", lock=" + std::to_string(snapshot.blockingLockAttemptsOnAudioThread)
                + ", wait=" + std::to_string(snapshot.waitsOnAudioThread)
                + ", file=" + std::to_string(snapshot.fileOpenEntriesOnAudioThread
                                                + snapshot.fileReadEntriesOnAudioThread)
                + ", path=" + std::to_string(snapshot.samplePathResolutionsOnAudioThread)
                + ", decode=" + std::to_string(snapshot.sampleDecodeEntriesOnAudioThread
                                                  + snapshot.streamDecodeEntriesOnAudioThread)
                + ", release=" + std::to_string(snapshot.largeResourceReleasesOnAudioThread)
                + ", growth=" + std::to_string(snapshot.activeVoiceCapacityGrowthCount)
                + ", budget=" + std::to_string(snapshot.overBudgetCallbackCount));
    require(snapshot.processBlockCount == 1, "Clean maximum-load case should execute exactly one callback.");
    require(snapshot.callbackBudgetMicros == Profile::deadlineMicros(48000.0, Profile::maximumBlockSize),
            "Clean maximum-load case should report the declared callback deadline.");
    require(snapshot.activeVoiceCapacityLimit
                == Profile::targetPolyphonyPerContext * Profile::playbackContextCount,
            "Realtime capacity should match the declared two-context target polyphony.");
    require(snapshot.performanceActiveVoiceCount == Profile::targetPolyphonyPerContext
                && snapshot.authoringPreviewActiveVoiceCount == Profile::targetPolyphonyPerContext,
            "Clean maximum-load case should exercise the declared polyphony in both playback contexts: performance="
                + std::to_string(snapshot.performanceActiveVoiceCount)
                + ", preview=" + std::to_string(snapshot.authoringPreviewActiveVoiceCount));
    require(buffer.getMagnitude(0, buffer.getNumSamples()) > 0.0001f,
            "Clean maximum-load case should render audible output.");
}
} // namespace

void* operator new(std::size_t size) { return allocateRaw(size); }
void* operator new[](std::size_t size) { return allocateRaw(size); }
void operator delete(void* pointer) noexcept { notifyDeallocation(pointer); std::free(pointer); }
void operator delete[](void* pointer) noexcept { notifyDeallocation(pointer); std::free(pointer); }
void operator delete(void* pointer, std::size_t) noexcept { notifyDeallocation(pointer); std::free(pointer); }
void operator delete[](void* pointer, std::size_t) noexcept { notifyDeallocation(pointer); std::free(pointer); }
void* operator new(std::size_t size, std::align_val_t alignment)
{
    return allocateAlignedRaw(size, static_cast<std::size_t>(alignment));
}
void* operator new[](std::size_t size, std::align_val_t alignment)
{
    return allocateAlignedRaw(size, static_cast<std::size_t>(alignment));
}
void operator delete(void* pointer, std::align_val_t) noexcept { freeAlignedRaw(pointer); }
void operator delete[](void* pointer, std::align_val_t) noexcept { freeAlignedRaw(pointer); }
void operator delete(void* pointer, std::size_t, std::align_val_t) noexcept { freeAlignedRaw(pointer); }
void operator delete[](void* pointer, std::size_t, std::align_val_t) noexcept { freeAlignedRaw(pointer); }

int main()
{
    try
    {
        const std::vector<std::pair<drs::plugin::RealtimeGuardOperation, std::string>> negativeCases {
            { drs::plugin::RealtimeGuardOperation::allocation, "allocation" },
            { drs::plugin::RealtimeGuardOperation::deallocation, "deallocation" },
            { drs::plugin::RealtimeGuardOperation::blockingLock, "blocking lock" },
            { drs::plugin::RealtimeGuardOperation::wait, "wait" },
            { drs::plugin::RealtimeGuardOperation::fileOpen, "file open" },
            { drs::plugin::RealtimeGuardOperation::fileRead, "file read" },
            { drs::plugin::RealtimeGuardOperation::pathResolution, "path resolution" },
            { drs::plugin::RealtimeGuardOperation::sampleDecode, "sample decode" },
            { drs::plugin::RealtimeGuardOperation::streamDecode, "stream decode" },
            { drs::plugin::RealtimeGuardOperation::largeResourceDestruction, "large-resource destruction" },
            { drs::plugin::RealtimeGuardOperation::finalSharedOwnershipRelease, "final shared-ownership release" },
            { drs::plugin::RealtimeGuardOperation::overBudget, "over-budget callback" }
        };

        for (const auto& [operation, label] : negativeCases)
            runNegativeCase(operation, label);

        runCleanMaximumLoadCase();
        std::cout << "Sprint 4 Entry Gate EG4 realtime guard matrix passed: 12 negative cases and clean maximum load."
                  << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Sprint 4 Entry Gate EG4 realtime guard matrix failed: " << exception.what() << std::endl;
        return 1;
    }
}
