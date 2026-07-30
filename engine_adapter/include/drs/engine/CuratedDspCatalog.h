#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace drs::engine
{
enum class CuratedDspScope : std::uint8_t { zone, group, instrument };
enum class CuratedDspParameterUnit : std::uint8_t { decibels, normalized, boolean, milliseconds, seconds, hertz, ratio, semitones };
enum class CuratedDspSmoothing : std::uint8_t { none, linear, logarithmic };
enum class CuratedDspStateClass : std::uint8_t { stateless, bounded, delay, reverb };

struct CuratedDspParameterDescriptor
{
    std::string_view id;
    CuratedDspParameterUnit unit = CuratedDspParameterUnit::normalized;
    double minimum = 0.0;
    double maximum = 1.0;
    double defaultValue = 0.0;
    CuratedDspSmoothing smoothing = CuratedDspSmoothing::linear;
};

struct CuratedDspCostMetadata
{
    std::size_t stateBytes = 0;
    std::size_t scratchBytes = 0;
    std::size_t maximumTailFrames = 0;
    std::uint32_t costUnits = 0;
};

struct CuratedDspEffectDescriptor
{
    std::string_view typeId;
    std::uint32_t algorithmVersion = 1;
    std::vector<CuratedDspScope> supportedScopes;
    std::vector<CuratedDspParameterDescriptor> parameters;
    CuratedDspStateClass stateClass = CuratedDspStateClass::stateless;
    CuratedDspCostMetadata cost;
};

const std::vector<CuratedDspEffectDescriptor>& getCuratedDspCatalog();
const CuratedDspEffectDescriptor* findCuratedDspEffect(std::string_view typeId,
                                                       std::uint32_t algorithmVersion) noexcept;
bool validatesCuratedDspCatalog(std::vector<std::string>& findings);
} // namespace drs::engine
