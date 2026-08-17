#pragma once

#include "drs/engine/SfzImport.h"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace drs::engine
{
// SFZ region values are converted once at the import boundary. Practical
// Sampler keeps half-open frame ranges internally even though SFZ end and
// loop_end identify inclusive frames.
enum class SfzRegionLoopMode : std::uint8_t
{
    noLoop = 0,
    oneShot,
    loopContinuous,
    loopSustain
};

enum class SfzRegionValueOrigin : std::uint8_t
{
    defaultValue = 0,
    sourceAudioMetadata,
    wavLoopMetadata,
    inheritedOpcode,
    localOpcode,
    nativeAuthoring
};

enum class SfzRegionMappingDisposition : std::uint8_t
{
    exact = 0,
    normalized,
    unsupported,
    invalid
};

struct SfzRegionValueProvenance
{
    SfzRegionValueOrigin origin = SfzRegionValueOrigin::defaultValue;
    std::string opcode;
    SfzImportSourceLocation location;
};

struct SfzRegionFrameValue
{
    bool present = false;
    std::uint64_t frame = 0;
    SfzRegionValueProvenance provenance;
};

struct SfzRegionLoopModeValue
{
    SfzRegionLoopMode mode = SfzRegionLoopMode::noLoop;
    SfzRegionValueProvenance provenance;
};

struct SfzWaveLoopMetadata
{
    // WAV smpl loop endpoints and SFZ loop_end are both represented here as
    // inclusive source-frame positions. Resolution converts the end to the
    // native exclusive boundary.
    std::uint64_t startFrame = 0;
    std::uint64_t endFrameInclusive = 0;
};

struct SfzRegionSourceMetadata
{
    std::optional<std::uint64_t> frameCount;
    std::optional<SfzWaveLoopMetadata> firstLoop;
};

struct SfzRegionContract
{
    // SFZ end=-1 is a defined silent-region sentinel. It is represented
    // explicitly rather than forced through an unsigned frame conversion.
    bool playbackSuppressed = false;
    SfzRegionFrameValue playbackStart;
    SfzRegionFrameValue playbackEndExclusive;
    SfzRegionLoopModeValue loopMode;
    SfzRegionFrameValue loopStart;
    SfzRegionFrameValue loopEndExclusive;

    bool loopEnabledCompatibility() const noexcept
    {
        return loopMode.mode == SfzRegionLoopMode::loopContinuous
            || loopMode.mode == SfzRegionLoopMode::loopSustain;
    }

    bool hasResolvedLoopRange() const noexcept
    {
        return loopStart.present && loopEndExclusive.present
            && loopStart.frame < loopEndExclusive.frame;
    }
};

struct SfzRegionResolutionResult
{
    bool valid = false;
    SfzRegionMappingDisposition disposition = SfzRegionMappingDisposition::exact;
    SfzRegionContract region;
    std::vector<SfzImportFinding> findings;
};

std::optional<SfzRegionLoopMode> parseSfzRegionLoopMode(const std::string& value) noexcept;
const char* sfzRegionLoopModeName(SfzRegionLoopMode mode) noexcept;

SfzRegionResolutionResult resolveSfzRegionContract(
    const SfzNormalizedSection& section,
    const SfzRegionSourceMetadata& sourceMetadata = {});
} // namespace drs::engine
