#pragma once

#include "drs/engine/SampleDataSource.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace drs::test
{
struct LargeInstrumentScaleFixture
{
    std::vector<engine::SampleDataSourceDescriptor> sources;
    std::vector<std::size_t> routeSourceIndices;
    std::vector<std::uint64_t> cachePressurePages;
    std::uint64_t sourceAudioBytes = 0;
    std::uint64_t decodedFloatBytes = 0;
    std::uint64_t sparsePackageOffset = 0;
};

struct WaveformRegionLargeSourceFixture
{
    engine::SampleDataSourceDescriptor singleLongSource;
    std::vector<engine::SampleDataSourceDescriptor> multiSourceInstrument;
    std::uint64_t multiSourceAudioBytes = 0;
    std::uint64_t playbackStartFrame = 0;
    std::uint64_t playbackEndFrame = 0;
    std::uint64_t loopStartFrame = 0;
    std::uint64_t loopEndFrame = 0;
};

inline WaveformRegionLargeSourceFixture makeWaveformRegionLargeSourceFixture(
    const std::size_t sourceCount = 64)
{
    constexpr std::uint64_t targetAudioBytes = 500ull * 1024ull * 1024ull;
    constexpr std::uint64_t bytesPerFrame = 6;
    WaveformRegionLargeSourceFixture fixture;
    const auto framesPerSource = (targetAudioBytes / sourceCount) / bytesPerFrame;
    fixture.multiSourceInstrument.reserve(sourceCount);
    for (std::size_t index = 0; index < sourceCount; ++index)
    {
        engine::SampleDataSourceDescriptor descriptor;
        descriptor.kind = engine::SampleDataSourceKind::deterministicFake;
        descriptor.sourceId = "waveform-500mb-source-" + std::to_string(index);
        descriptor.canonicalSourceIdentity = "fixture://" + descriptor.sourceId;
        descriptor.provenanceIdentity = "waveform-phase7-generation-1";
        descriptor.formatName = "pcm24";
        descriptor.channelLayout = "stereo";
        descriptor.checksumHex = "fixture";
        descriptor.sampleRate = 48000.0;
        descriptor.frameCount = framesPerSource;
        descriptor.channelCount = 2;
        descriptor.bytesPerFrame = bytesPerFrame;
        descriptor.dataOffsetBytes = 44;
        descriptor.dataSizeBytes = descriptor.frameCount * bytesPerFrame;
        fixture.multiSourceAudioBytes += descriptor.dataSizeBytes;
        fixture.multiSourceInstrument.push_back(std::move(descriptor));
    }

    fixture.singleLongSource = fixture.multiSourceInstrument.front();
    fixture.singleLongSource.sourceId = "waveform-single-500mb-source";
    fixture.singleLongSource.canonicalSourceIdentity
        = "fixture://waveform-single-500mb-source";
    fixture.singleLongSource.frameCount = targetAudioBytes / bytesPerFrame;
    fixture.singleLongSource.dataSizeBytes
        = fixture.singleLongSource.frameCount * bytesPerFrame;
    fixture.playbackStartFrame = fixture.singleLongSource.frameCount - 2'000'000ull;
    fixture.playbackEndFrame = fixture.singleLongSource.frameCount - 100'000ull;
    fixture.loopStartFrame = fixture.singleLongSource.frameCount - 1'500'000ull;
    fixture.loopEndFrame = fixture.singleLongSource.frameCount - 250'000ull;
    return fixture;
}

inline LargeInstrumentScaleFixture makeLargeInstrumentScaleFixture(
    const std::size_t sourceCount = 641,
    const std::size_t routeCount = 1704)
{
    LargeInstrumentScaleFixture fixture;
    fixture.sources.reserve(sourceCount);
    fixture.routeSourceIndices.reserve(routeCount);
    constexpr std::uint64_t ordinaryFrames = 509000;
    constexpr std::uint64_t longSampleFrames = 3600000;
    for (std::size_t index = 0; index < sourceCount; ++index)
    {
        engine::SampleDataSourceDescriptor descriptor;
        descriptor.kind = engine::SampleDataSourceKind::deterministicFake;
        descriptor.sourceId = "large-source-" + std::to_string(index);
        descriptor.canonicalSourceIdentity = "fixture://" + descriptor.sourceId;
        descriptor.provenanceIdentity = "large-scale-generation-1";
        descriptor.formatName = "pcm24";
        descriptor.channelLayout = "stereo";
        descriptor.checksumHex = "fixture";
        descriptor.sampleRate = 48000.0;
        descriptor.frameCount = index == 0 ? longSampleFrames : ordinaryFrames;
        descriptor.channelCount = 2;
        descriptor.bytesPerFrame = 6;
        descriptor.dataOffsetBytes = index == sourceCount - 1
            ? 5ull * 1024ull * 1024ull * 1024ull : 44;
        descriptor.dataSizeBytes = descriptor.frameCount * descriptor.bytesPerFrame;
        fixture.sourceAudioBytes += descriptor.dataSizeBytes;
        fixture.decodedFloatBytes += descriptor.frameCount * 2ull * sizeof(float);
        fixture.sources.push_back(std::move(descriptor));
    }
    for (std::size_t route = 0; route < routeCount; ++route)
        fixture.routeSourceIndices.push_back(route % sourceCount);
    for (std::uint64_t page = 0; page < 1024; ++page)
        fixture.cachePressurePages.push_back(page);
    for (std::uint64_t page = 1024; page-- > 0;)
        fixture.cachePressurePages.push_back(page);
    fixture.sparsePackageOffset = 5ull * 1024ull * 1024ull * 1024ull;
    return fixture;
}
} // namespace drs::test
