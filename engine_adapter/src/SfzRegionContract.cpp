#include "drs/engine/SfzRegionContract.h"

#include <algorithm>
#include <charconv>
#include <limits>
#include <string_view>

namespace drs::engine
{
namespace
{
std::string toLowerAscii(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](const unsigned char character)
    {
        return character >= 'A' && character <= 'Z'
            ? static_cast<char>(character - 'A' + 'a')
            : static_cast<char>(character);
    });
    return value;
}

std::optional<std::uint64_t> parseFrame(const std::string& text) noexcept
{
    if (text.empty())
        return std::nullopt;

    std::uint64_t value = 0;
    const auto* begin = text.data();
    const auto* end = begin + text.size();
    const auto result = std::from_chars(begin, end, value, 10);
    if (result.ec != std::errc {} || result.ptr != end)
        return std::nullopt;
    return value;
}

SfzRegionValueProvenance provenanceFor(const SfzResolvedOpcode& opcode)
{
    return {
        opcode.inherited ? SfzRegionValueOrigin::inheritedOpcode
                         : SfzRegionValueOrigin::localOpcode,
        opcode.name,
        opcode.location
    };
}

SfzImportFinding makeFinding(SfzImportFindingSeverity severity,
                             SfzImportSupportDisposition disposition,
                             std::string code,
                             std::string summary,
                             std::string detail,
                             const SfzImportSourceLocation& location = {})
{
    return { severity,
             disposition,
             std::move(code),
             std::move(summary),
             std::move(detail),
             location };
}

void promoteDisposition(SfzRegionResolutionResult& result,
                        const SfzRegionMappingDisposition candidate) noexcept
{
    if (static_cast<int>(candidate) > static_cast<int>(result.disposition))
        result.disposition = candidate;
}

bool resolveDirectFrame(const SfzResolvedOpcode* opcode,
                        const char* displayName,
                        SfzRegionFrameValue& destination,
                        SfzRegionResolutionResult& result)
{
    if (opcode == nullptr)
        return true;

    const auto value = parseFrame(opcode->value);
    if (!value.has_value())
    {
        result.findings.push_back(makeFinding(
            SfzImportFindingSeverity::error,
            SfzImportSupportDisposition::blocking,
            std::string("sfz.region.") + opcode->name + ".invalid",
            std::string("Invalid SFZ ") + displayName,
            "The value must be a non-negative integer source-frame position.",
            opcode->location));
        promoteDisposition(result, SfzRegionMappingDisposition::invalid);
        return false;
    }

    destination.present = true;
    destination.frame = *value;
    destination.provenance = provenanceFor(*opcode);
    return true;
}

bool resolveInclusiveEnd(const SfzResolvedOpcode* opcode,
                         const char* displayName,
                         SfzRegionFrameValue& destination,
                         SfzRegionResolutionResult& result)
{
    if (opcode == nullptr)
        return true;

    const auto value = parseFrame(opcode->value);
    if (!value.has_value())
    {
        result.findings.push_back(makeFinding(
            SfzImportFindingSeverity::error,
            SfzImportSupportDisposition::blocking,
            std::string("sfz.region.") + opcode->name + ".invalid",
            std::string("Invalid SFZ ") + displayName,
            "The value must be a non-negative integer inclusive source-frame position.",
            opcode->location));
        promoteDisposition(result, SfzRegionMappingDisposition::invalid);
        return false;
    }

    if (*value == std::numeric_limits<std::uint64_t>::max())
    {
        result.findings.push_back(makeFinding(
            SfzImportFindingSeverity::error,
            SfzImportSupportDisposition::blocking,
            std::string("sfz.region.") + opcode->name + ".overflow",
            std::string("SFZ ") + displayName + " is too large",
            "The inclusive endpoint cannot be represented as a native exclusive endpoint.",
            opcode->location));
        promoteDisposition(result, SfzRegionMappingDisposition::invalid);
        return false;
    }

    destination.present = true;
    destination.frame = *value + 1;
    destination.provenance = provenanceFor(*opcode);
    promoteDisposition(result, SfzRegionMappingDisposition::normalized);
    return true;
}

void resolveLoopMetadataFallback(SfzRegionContract& region,
                                 const SfzRegionSourceMetadata& metadata,
                                 SfzRegionResolutionResult& result)
{
    if (!metadata.firstLoop.has_value())
        return;

    const auto& loop = *metadata.firstLoop;
    if (!region.loopStart.present)
    {
        region.loopStart = {
            true,
            loop.startFrame,
            { SfzRegionValueOrigin::wavLoopMetadata, "loop_start", {} }
        };
    }

    if (!region.loopEndExclusive.present)
    {
        if (loop.endFrameInclusive == std::numeric_limits<std::uint64_t>::max())
        {
            result.findings.push_back(makeFinding(
                SfzImportFindingSeverity::error,
                SfzImportSupportDisposition::blocking,
                "sfz.region.wav_loop_end.overflow",
                "WAV loop end is too large",
                "The WAV metadata endpoint cannot be represented as a native exclusive endpoint."));
            promoteDisposition(result, SfzRegionMappingDisposition::invalid);
        }
        else
        {
            region.loopEndExclusive = {
                true,
                loop.endFrameInclusive + 1,
                { SfzRegionValueOrigin::wavLoopMetadata, "loop_end", {} }
            };
            promoteDisposition(result, SfzRegionMappingDisposition::normalized);
        }
    }
}

bool validateResolvedRanges(SfzRegionResolutionResult& result)
{
    const auto& region = result.region;
    auto valid = result.disposition != SfzRegionMappingDisposition::invalid;

    if (!region.playbackSuppressed
        && region.playbackEndExclusive.present
        && region.playbackStart.frame >= region.playbackEndExclusive.frame)
    {
        result.findings.push_back(makeFinding(
            SfzImportFindingSeverity::error,
            SfzImportSupportDisposition::blocking,
            "sfz.region.playback_range.invalid",
            "Invalid SFZ playback region",
            "The resolved half-open playback range must contain at least one source frame.",
            region.playbackEndExclusive.provenance.location));
        valid = false;
    }

    if (!region.playbackSuppressed && region.loopEnabledCompatibility())
    {
        if (!region.loopStart.present || !region.loopEndExclusive.present)
        {
            result.findings.push_back(makeFinding(
                SfzImportFindingSeverity::warning,
                SfzImportSupportDisposition::reportedOnly,
                "sfz.region.loop_range.missing",
                "Active SFZ loop has no resolvable range",
                "The loop mode is retained, but playback cannot enable the loop until boundaries are resolved from SFZ, WAV metadata, or known source length.",
                region.loopMode.provenance.location));
            promoteDisposition(result, SfzRegionMappingDisposition::unsupported);
        }
        else if (region.loopStart.frame >= region.loopEndExclusive.frame)
        {
            result.findings.push_back(makeFinding(
                SfzImportFindingSeverity::error,
                SfzImportSupportDisposition::blocking,
                "sfz.region.loop_range.invalid",
                "Invalid SFZ loop region",
                "The resolved half-open loop range must contain at least one source frame.",
                region.loopEndExclusive.provenance.location));
            valid = false;
        }
        else if (region.loopStart.frame < region.playbackStart.frame
                 || (region.playbackEndExclusive.present
                     && region.loopEndExclusive.frame > region.playbackEndExclusive.frame))
        {
            result.findings.push_back(makeFinding(
                SfzImportFindingSeverity::error,
                SfzImportSupportDisposition::blocking,
                "sfz.region.loop_range.outside_playback",
                "SFZ loop lies outside the playback region",
                "The resolved loop must be fully contained by the resolved playback range.",
                region.loopEndExclusive.provenance.location));
            valid = false;
        }
    }

    if (!valid)
        promoteDisposition(result, SfzRegionMappingDisposition::invalid);
    return valid;
}
} // namespace

std::optional<SfzRegionLoopMode> parseSfzRegionLoopMode(const std::string& value) noexcept
{
    const auto lowered = toLowerAscii(value);
    if (lowered == "no_loop")
        return SfzRegionLoopMode::noLoop;
    if (lowered == "one_shot")
        return SfzRegionLoopMode::oneShot;
    if (lowered == "loop_continuous")
        return SfzRegionLoopMode::loopContinuous;
    if (lowered == "loop_sustain")
        return SfzRegionLoopMode::loopSustain;
    return std::nullopt;
}

const char* sfzRegionLoopModeName(const SfzRegionLoopMode mode) noexcept
{
    switch (mode)
    {
        case SfzRegionLoopMode::noLoop: return "no_loop";
        case SfzRegionLoopMode::oneShot: return "one_shot";
        case SfzRegionLoopMode::loopContinuous: return "loop_continuous";
        case SfzRegionLoopMode::loopSustain: return "loop_sustain";
    }
    return "no_loop";
}

SfzRegionResolutionResult resolveSfzRegionContract(
    const SfzNormalizedSection& section,
    const SfzRegionSourceMetadata& sourceMetadata)
{
    SfzRegionResolutionResult result;
    auto& region = result.region;
    region.playbackStart = {
        true,
        0,
        { SfzRegionValueOrigin::defaultValue, "offset", {} }
    };

    const auto* offset = findEffectiveOpcode(section, "offset");
    const auto* playbackEnd = findEffectiveOpcode(section, "end");
    const auto* loopMode = findEffectiveOpcode(section, "loop_mode");
    const auto* loopStart = findEffectiveOpcode(section, "loop_start");
    const auto* loopEnd = findEffectiveOpcode(section, "loop_end");

    resolveDirectFrame(offset, "offset", region.playbackStart, result);
    if (playbackEnd != nullptr && playbackEnd->value == "-1")
    {
        region.playbackSuppressed = true;
        region.playbackEndExclusive.provenance = provenanceFor(*playbackEnd);
        promoteDisposition(result, SfzRegionMappingDisposition::normalized);
    }
    else
    {
        resolveInclusiveEnd(playbackEnd, "end", region.playbackEndExclusive, result);
    }
    resolveDirectFrame(loopStart, "loop start", region.loopStart, result);
    resolveInclusiveEnd(loopEnd, "loop end", region.loopEndExclusive, result);

    if (!region.playbackSuppressed
        && !region.playbackEndExclusive.present
        && sourceMetadata.frameCount.has_value())
    {
        region.playbackEndExclusive = {
            true,
            *sourceMetadata.frameCount,
            { SfzRegionValueOrigin::sourceAudioMetadata, "end", {} }
        };
    }

    resolveLoopMetadataFallback(region, sourceMetadata, result);

    if (loopMode != nullptr)
    {
        const auto parsedMode = parseSfzRegionLoopMode(loopMode->value);
        if (parsedMode.has_value())
        {
            region.loopMode.mode = *parsedMode;
            region.loopMode.provenance = provenanceFor(*loopMode);
        }
        else
        {
            region.loopMode.mode = SfzRegionLoopMode::noLoop;
            region.loopMode.provenance = provenanceFor(*loopMode);
            result.findings.push_back(makeFinding(
                SfzImportFindingSeverity::warning,
                SfzImportSupportDisposition::reportedOnly,
                "sfz.region.loop_mode.unsupported",
                "Unsupported SFZ loop mode",
                "The loop_mode value is recognized but is outside the portable SFZ v1 region contract.",
                loopMode->location));
            promoteDisposition(result, SfzRegionMappingDisposition::unsupported);
        }
    }
    else if (region.loopStart.present && region.loopEndExclusive.present)
    {
        region.loopMode = {
            SfzRegionLoopMode::loopContinuous,
            { sourceMetadata.firstLoop.has_value()
                  ? SfzRegionValueOrigin::wavLoopMetadata
                  : SfzRegionValueOrigin::defaultValue,
              "loop_mode",
              {} }
        };
    }
    else
    {
        region.loopMode = {
            SfzRegionLoopMode::noLoop,
            { SfzRegionValueOrigin::defaultValue, "loop_mode", {} }
        };
    }

    if (region.loopEnabledCompatibility())
    {
        if (!region.loopStart.present)
        {
            region.loopStart = {
                true,
                region.playbackStart.frame,
                { SfzRegionValueOrigin::defaultValue, "loop_start", {} }
            };
        }
        if (!region.loopEndExclusive.present && region.playbackEndExclusive.present)
        {
            region.loopEndExclusive = {
                true,
                region.playbackEndExclusive.frame,
                { SfzRegionValueOrigin::defaultValue, "loop_end", {} }
            };
        }
    }

    result.valid = validateResolvedRanges(result);
    return result;
}
} // namespace drs::engine
