#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace drs::engine
{
enum class CuratedDspScope : std::uint8_t { zone, group, layer, instrument };
enum class CuratedDspParameterUnit : std::uint8_t { decibels, normalized, boolean, milliseconds, seconds, hertz, ratio, semitones };
enum class CuratedDspSmoothing : std::uint8_t { none, linear, logarithmic };
enum class CuratedDspStateClass : std::uint8_t { stateless, bounded, delay, reverb };

struct CuratedDspControlRecommendation
{
    std::string_view role;
    std::string_view controlLawId;
    double recommendedMinimum = 0.0;
    double recommendedMaximum = 1.0;
};

struct CuratedDspParameterDescriptor
{
    std::string_view id;
    CuratedDspParameterUnit unit = CuratedDspParameterUnit::normalized;
    double minimum = 0.0;
    double maximum = 1.0;
    double defaultValue = 0.0;
    CuratedDspSmoothing smoothing = CuratedDspSmoothing::linear;
    // Descriptor metadata recommends interaction behavior. The resolved law and
    // range remain project-owned beginning with the Sprint 3 schema work.
    std::string_view defaultControlLawId;
    std::vector<std::string_view> allowedControlLawIds;
    std::vector<CuratedDspControlRecommendation> recommendations;
};

enum class CuratedDspControlLawResolutionSource : std::uint8_t
{
    none = 0,
    explicitOverride,
    roleRecommendation,
    descriptorDefault,
    compatibilityFallback
};

struct CuratedDspControlLawResolveRequest
{
    const CuratedDspParameterDescriptor* descriptor = nullptr;
    std::string_view role;
    std::string_view explicitControlLawId;
    // Used only for legacy/incomplete descriptors where descriptor is null.
    CuratedDspParameterUnit fallbackUnit = CuratedDspParameterUnit::normalized;
    double fallbackMinimum = 0.0;
    double fallbackMaximum = 1.0;
};

struct CuratedDspControlLawResolution
{
    bool resolved = false;
    CuratedDspControlLawResolutionSource source = CuratedDspControlLawResolutionSource::none;
    std::string_view controlLawId;
    double minimum = 0.0;
    double maximum = 1.0;
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
bool validatesCuratedDspCatalog(const std::vector<CuratedDspEffectDescriptor>& catalog,
                                std::vector<std::string>& findings);
bool validatesCuratedDspCatalog(std::vector<std::string>& findings);
CuratedDspControlLawResolution resolveCuratedDspControlLaw(
    const CuratedDspControlLawResolveRequest& request) noexcept;
} // namespace drs::engine
