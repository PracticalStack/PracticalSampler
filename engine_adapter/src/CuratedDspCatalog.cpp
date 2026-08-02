#include "drs/engine/CuratedDspCatalog.h"
#include "drs/engine/ControlLaw.h"
#include "drs/engine/DspAlgorithmicReverb.h"
#include "drs/engine/DspCompactEq.h"
#include "drs/engine/DspChorus.h"
#include "drs/engine/DspStereoDelay.h"

#include <algorithm>
#include <cmath>
#include <unordered_set>
#include <utility>

namespace drs::engine
{
namespace
{
constexpr std::size_t stereoDelayMemoryBytes = 2u * DspStereoDelayState::maximumDelayFrames * sizeof(float);
constexpr std::size_t algorithmicReverbMemoryBytes = DspAlgorithmicReverbState::maximumStateBytes;
constexpr std::size_t compactEqStateBytes = DspCompactEqState::maximumStateBytes;
constexpr std::size_t chorusStateBytes = DspChorusState::maximumStateBytes;

CuratedDspParameterDescriptor parameter(
    const std::string_view id,
    const CuratedDspParameterUnit unit,
    const double minimum,
    const double maximum,
    const double defaultValue,
    const CuratedDspSmoothing smoothing,
    const std::string_view defaultLaw,
    std::vector<std::string_view> allowed,
    std::vector<CuratedDspControlRecommendation> recommendations = {})
{
    return { id, unit, minimum, maximum, defaultValue, smoothing, defaultLaw,
             std::move(allowed), std::move(recommendations) };
}

bool containsLaw(const CuratedDspParameterDescriptor& parameter, const std::string_view id)
{
    return std::find(parameter.allowedControlLawIds.begin(), parameter.allowedControlLawIds.end(), id)
        != parameter.allowedControlLawIds.end();
}

bool canCompile(const std::string_view id, const double minimum, const double maximum) noexcept
{
    CompiledControlLaw law;
    return compileControlLaw(id, minimum, maximum, law);
}

bool isCompatibleWithUnit(const std::string_view law,
                          const CuratedDspParameterUnit unit) noexcept
{
    if (law == controlLawToggleV1) return unit == CuratedDspParameterUnit::boolean;
    if (law == controlLawMixerGainV1 || law == controlLawBipolarLinearV1)
        return unit == CuratedDspParameterUnit::decibels;
    if (law == controlLawLogPositiveV1)
    {
        return unit == CuratedDspParameterUnit::hertz
            || unit == CuratedDspParameterUnit::milliseconds
            || unit == CuratedDspParameterUnit::seconds
            || unit == CuratedDspParameterUnit::ratio;
    }
    if (law == controlLawSteppedV1) return unit == CuratedDspParameterUnit::normalized;
    // The current shared linear law is named for its primary dB use but is the
    // product's compatible continuous fallback for other non-boolean units.
    if (law == controlLawLinearDbV1) return unit != CuratedDspParameterUnit::boolean;
    return law == controlLawBipolarCenteredV1 && unit == CuratedDspParameterUnit::normalized;
}

CuratedDspControlLawResolution fallbackResolution(const CuratedDspParameterUnit unit,
                                                  const double minimum,
                                                  const double maximum) noexcept
{
    std::string_view law = controlLawLinearDbV1;
    if (unit == CuratedDspParameterUnit::boolean)
        law = controlLawToggleV1;
    else if ((unit == CuratedDspParameterUnit::hertz
              || unit == CuratedDspParameterUnit::seconds
              || unit == CuratedDspParameterUnit::milliseconds)
             && minimum > 0.0)
        law = controlLawLogPositiveV1;
    else if (unit == CuratedDspParameterUnit::decibels && minimum < 0.0 && maximum > 0.0)
        law = controlLawBipolarLinearV1;

    CuratedDspControlLawResolution result;
    if (canCompile(law, minimum, maximum))
    {
        result.resolved = true;
        result.source = CuratedDspControlLawResolutionSource::compatibilityFallback;
        result.controlLawId = law;
        result.minimum = minimum;
        result.maximum = maximum;
    }
    return result;
}
}

const std::vector<CuratedDspEffectDescriptor>& getCuratedDspCatalog()
{
    static const std::vector<CuratedDspEffectDescriptor> catalog {
        { "drs.gain", 1, { CuratedDspScope::zone, CuratedDspScope::group, CuratedDspScope::instrument },
          { parameter("gainDb", CuratedDspParameterUnit::decibels, -96.0, 24.0, 0.0, CuratedDspSmoothing::linear,
                      controlLawLinearDbV1, { controlLawLinearDbV1, controlLawMixerGainV1 },
                      { { "mix", controlLawMixerGainV1, -96.0, 6.0 } }),
            parameter("polarity", CuratedDspParameterUnit::boolean, 0.0, 1.0, 0.0, CuratedDspSmoothing::none,
                      controlLawToggleV1, { controlLawToggleV1 }),
            parameter("mute", CuratedDspParameterUnit::boolean, 0.0, 1.0, 0.0, CuratedDspSmoothing::linear,
                      controlLawToggleV1, { controlLawToggleV1 }) },
          CuratedDspStateClass::stateless, { 0, 0, 0, 1 } },
        { "drs.saturator", 1, { CuratedDspScope::zone, CuratedDspScope::group, CuratedDspScope::instrument },
          { parameter("character", CuratedDspParameterUnit::normalized, 0.0, 2.0, 0.0, CuratedDspSmoothing::none,
                      controlLawSteppedV1, { controlLawSteppedV1 }),
            parameter("driveDb", CuratedDspParameterUnit::decibels, 0.0, 36.0, 6.0, CuratedDspSmoothing::linear,
                      controlLawLinearDbV1, { controlLawLinearDbV1 }),
            parameter("tone", CuratedDspParameterUnit::normalized, 0.0, 1.0, 0.5, CuratedDspSmoothing::linear,
                      controlLawLinearDbV1, { controlLawLinearDbV1 }),
            parameter("mix", CuratedDspParameterUnit::normalized, 0.0, 1.0, 1.0, CuratedDspSmoothing::linear,
                      controlLawLinearDbV1, { controlLawLinearDbV1 }),
            parameter("outputDb", CuratedDspParameterUnit::decibels, -24.0, 24.0, 0.0, CuratedDspSmoothing::linear,
                      controlLawBipolarLinearV1, { controlLawBipolarLinearV1, controlLawLinearDbV1 }) },
          CuratedDspStateClass::bounded, { 128, 0, 0, 3 } },
        { "drs.stereoDelay", 1, { CuratedDspScope::zone, CuratedDspScope::group, CuratedDspScope::instrument },
          { parameter("timeMs", CuratedDspParameterUnit::milliseconds, 1.0, 2000.0, 375.0, CuratedDspSmoothing::logarithmic,
                      controlLawLogPositiveV1, { controlLawLogPositiveV1, controlLawLinearDbV1 }),
            parameter("sync", CuratedDspParameterUnit::boolean, 0.0, 1.0, 0.0, CuratedDspSmoothing::none,
                      controlLawToggleV1, { controlLawToggleV1 }),
            parameter("divisionBeats", CuratedDspParameterUnit::ratio, 0.0625, 4.0, 0.5, CuratedDspSmoothing::logarithmic,
                      controlLawLogPositiveV1, { controlLawLogPositiveV1, controlLawLinearDbV1 }),
            parameter("feedback", CuratedDspParameterUnit::ratio, 0.0, 0.95, 0.35, CuratedDspSmoothing::linear,
                      controlLawLinearDbV1, { controlLawLinearDbV1 }),
            parameter("pingPong", CuratedDspParameterUnit::boolean, 0.0, 1.0, 0.0, CuratedDspSmoothing::linear,
                      controlLawToggleV1, { controlLawToggleV1 }),
            parameter("tone", CuratedDspParameterUnit::normalized, 0.0, 1.0, 0.7, CuratedDspSmoothing::linear,
                      controlLawLinearDbV1, { controlLawLinearDbV1 }),
            parameter("width", CuratedDspParameterUnit::normalized, 0.0, 1.0, 1.0, CuratedDspSmoothing::linear,
                      controlLawLinearDbV1, { controlLawLinearDbV1 }),
            parameter("mix", CuratedDspParameterUnit::normalized, 0.0, 1.0, 0.25, CuratedDspSmoothing::linear,
                      controlLawLinearDbV1, { controlLawLinearDbV1 }) },
          CuratedDspStateClass::delay, { stereoDelayMemoryBytes, 0, 30u * 96000u, 12 } },
        { "drs.algorithmicReverb", 1, { CuratedDspScope::zone, CuratedDspScope::group, CuratedDspScope::instrument },
          { parameter("preDelayMs", CuratedDspParameterUnit::milliseconds, 0.0, 250.0, 20.0, CuratedDspSmoothing::linear,
                      controlLawLinearDbV1, { controlLawLinearDbV1 }),
            parameter("size", CuratedDspParameterUnit::normalized, 0.0, 1.0, 0.5, CuratedDspSmoothing::linear,
                      controlLawLinearDbV1, { controlLawLinearDbV1 }),
            parameter("decaySeconds", CuratedDspParameterUnit::seconds, 0.1, 20.0, 2.5, CuratedDspSmoothing::logarithmic,
                      controlLawLogPositiveV1, { controlLawLogPositiveV1, controlLawLinearDbV1 }),
            parameter("damping", CuratedDspParameterUnit::normalized, 0.0, 1.0, 0.5, CuratedDspSmoothing::linear,
                      controlLawLinearDbV1, { controlLawLinearDbV1 }),
            parameter("width", CuratedDspParameterUnit::normalized, 0.0, 1.0, 1.0, CuratedDspSmoothing::linear,
                      controlLawLinearDbV1, { controlLawLinearDbV1 }),
            parameter("mix", CuratedDspParameterUnit::normalized, 0.0, 1.0, 0.2, CuratedDspSmoothing::linear,
                      controlLawLinearDbV1, { controlLawLinearDbV1 }) },
          CuratedDspStateClass::reverb, { algorithmicReverbMemoryBytes, 0, 30u * 96000u, 20 } },
        { "drs.compactEq", 1, { CuratedDspScope::zone, CuratedDspScope::group, CuratedDspScope::instrument },
          { parameter("mode", CuratedDspParameterUnit::normalized, 0.0, 2.0, 1.0, CuratedDspSmoothing::none,
                      controlLawSteppedV1, { controlLawSteppedV1 }),
            parameter("frequencyHz", CuratedDspParameterUnit::hertz, 40.0, 18000.0, 1000.0, CuratedDspSmoothing::logarithmic,
                      controlLawLogPositiveV1, { controlLawLogPositiveV1, controlLawLinearDbV1 }),
            parameter("q", CuratedDspParameterUnit::ratio, .25, 12.0, .707, CuratedDspSmoothing::logarithmic,
                      controlLawLogPositiveV1, { controlLawLogPositiveV1, controlLawLinearDbV1 }),
            parameter("gainDb", CuratedDspParameterUnit::decibels, -18.0, 18.0, 0.0, CuratedDspSmoothing::linear,
                      controlLawBipolarLinearV1, { controlLawBipolarLinearV1, controlLawLinearDbV1 }),
            parameter("mix", CuratedDspParameterUnit::normalized, 0.0, 1.0, 1.0, CuratedDspSmoothing::linear,
                      controlLawLinearDbV1, { controlLawLinearDbV1 }) },
          CuratedDspStateClass::bounded, { compactEqStateBytes, 0, 0, 5 } },
        { "drs.chorus", 1, { CuratedDspScope::zone, CuratedDspScope::group, CuratedDspScope::instrument },
          { parameter("rateHz", CuratedDspParameterUnit::hertz, .05, 5.0, .8, CuratedDspSmoothing::logarithmic,
                      controlLawLogPositiveV1, { controlLawLogPositiveV1, controlLawLinearDbV1 }),
            parameter("depthMs", CuratedDspParameterUnit::milliseconds, .1, 12.0, 5.0, CuratedDspSmoothing::linear,
                      controlLawLinearDbV1, { controlLawLinearDbV1 }),
            parameter("baseDelayMs", CuratedDspParameterUnit::milliseconds, 5.0, 30.0, 15.0, CuratedDspSmoothing::linear,
                      controlLawLogPositiveV1, { controlLawLogPositiveV1, controlLawLinearDbV1 }),
            parameter("width", CuratedDspParameterUnit::normalized, 0.0, 1.0, 1.0, CuratedDspSmoothing::linear,
                      controlLawLinearDbV1, { controlLawLinearDbV1 }),
            parameter("mix", CuratedDspParameterUnit::normalized, 0.0, 1.0, .35, CuratedDspSmoothing::linear,
                      controlLawLinearDbV1, { controlLawLinearDbV1 }) },
          CuratedDspStateClass::delay, { chorusStateBytes, 0, 0, 9 } }
    };
    return catalog;
}

const CuratedDspEffectDescriptor* findCuratedDspEffect(std::string_view typeId,
                                                        std::uint32_t algorithmVersion) noexcept
{
    const auto& catalog = getCuratedDspCatalog();
    const auto it = std::find_if(catalog.begin(), catalog.end(), [&](const auto& descriptor)
    {
        return descriptor.typeId == typeId && descriptor.algorithmVersion == algorithmVersion;
    });
    return it == catalog.end() ? nullptr : &*it;
}

bool validatesCuratedDspCatalog(const std::vector<CuratedDspEffectDescriptor>& catalog,
                                std::vector<std::string>& findings)
{
    findings.clear();
    std::unordered_set<std::string_view> typeVersions;
    for (const auto& effect : catalog)
    {
        const auto identity = effect.typeId.empty() ? std::string_view {} : effect.typeId;
        if (identity.empty() || effect.algorithmVersion == 0 || !typeVersions.insert(identity).second)
            findings.push_back("invalid or duplicate catalog effect identity");
        if (effect.supportedScopes.empty()) findings.push_back("catalog effect has no supported scope");
        std::unordered_set<std::string_view> parameterIds;
        for (const auto& parameter : effect.parameters)
        {
            if (parameter.id.empty() || !parameterIds.insert(parameter.id).second
                || !std::isfinite(parameter.minimum) || !std::isfinite(parameter.maximum)
                || !std::isfinite(parameter.defaultValue) || parameter.minimum > parameter.maximum
                || parameter.defaultValue < parameter.minimum || parameter.defaultValue > parameter.maximum)
                findings.push_back("invalid catalog parameter descriptor");
            if (parameter.defaultControlLawId.empty()
                || !containsLaw(parameter, parameter.defaultControlLawId)
                || !isCompatibleWithUnit(parameter.defaultControlLawId, parameter.unit)
                || !canCompile(parameter.defaultControlLawId, parameter.minimum, parameter.maximum))
                findings.push_back("invalid catalog parameter default control law");

            std::unordered_set<std::string_view> allowedLaws;
            for (const auto law : parameter.allowedControlLawIds)
            {
                const auto hasCompatibleRange = canCompile(law, parameter.minimum, parameter.maximum)
                    || std::any_of(parameter.recommendations.begin(), parameter.recommendations.end(),
                                   [&](const auto& recommendation)
                                   {
                                       return recommendation.controlLawId == law
                                           && canCompile(law, recommendation.recommendedMinimum,
                                                         recommendation.recommendedMaximum);
                                   });
                if (law.empty() || !allowedLaws.insert(law).second
                    || !isCompatibleWithUnit(law, parameter.unit)
                    || !hasCompatibleRange)
                    findings.push_back("invalid catalog parameter allowed control law");
            }

            std::unordered_set<std::string_view> recommendationRoles;
            for (const auto& recommendation : parameter.recommendations)
            {
                const auto rangeIsLegal = std::isfinite(recommendation.recommendedMinimum)
                    && std::isfinite(recommendation.recommendedMaximum)
                    && recommendation.recommendedMinimum < recommendation.recommendedMaximum
                    && recommendation.recommendedMinimum >= parameter.minimum
                    && recommendation.recommendedMaximum <= parameter.maximum;
                if (recommendation.role.empty() || !recommendationRoles.insert(recommendation.role).second
                    || !containsLaw(parameter, recommendation.controlLawId)
                    || !isCompatibleWithUnit(recommendation.controlLawId, parameter.unit) || !rangeIsLegal
                    || !canCompile(recommendation.controlLawId, recommendation.recommendedMinimum,
                                   recommendation.recommendedMaximum))
                    findings.push_back("invalid catalog parameter control recommendation");
            }
        }
        if (effect.cost.maximumTailFrames > 30u * 96000u)
            findings.push_back("catalog tail exceeds S0 ceiling");
    }
    return findings.empty();
}

bool validatesCuratedDspCatalog(std::vector<std::string>& findings)
{
    return validatesCuratedDspCatalog(getCuratedDspCatalog(), findings);
}

CuratedDspControlLawResolution resolveCuratedDspControlLaw(
    const CuratedDspControlLawResolveRequest& request) noexcept
{
    if (request.descriptor == nullptr)
    {
        if (!request.explicitControlLawId.empty())
            return {};
        return fallbackResolution(request.fallbackUnit, request.fallbackMinimum, request.fallbackMaximum);
    }

    const auto& parameter = *request.descriptor;
    const auto resolved = [&](const std::string_view id,
                              const double minimum,
                              const double maximum,
                              const CuratedDspControlLawResolutionSource source)
    {
        CuratedDspControlLawResolution result;
        if (canCompile(id, minimum, maximum))
        {
            result.resolved = true;
            result.source = source;
            result.controlLawId = id;
            result.minimum = minimum;
            result.maximum = maximum;
        }
        return result;
    };

    if (!request.explicitControlLawId.empty())
    {
        if (!containsLaw(parameter, request.explicitControlLawId)) return {};
        if (canCompile(request.explicitControlLawId, parameter.minimum, parameter.maximum))
            return resolved(request.explicitControlLawId, parameter.minimum, parameter.maximum,
                            CuratedDspControlLawResolutionSource::explicitOverride);
        const auto recommendation = std::find_if(parameter.recommendations.begin(),
                                                 parameter.recommendations.end(),
                                                 [&](const auto& candidate)
                                                 { return candidate.controlLawId == request.explicitControlLawId; });
        return recommendation == parameter.recommendations.end() ? CuratedDspControlLawResolution {}
            : resolved(recommendation->controlLawId, recommendation->recommendedMinimum,
                       recommendation->recommendedMaximum,
                       CuratedDspControlLawResolutionSource::explicitOverride);
    }
    if (!request.role.empty())
    {
        const auto recommendation = std::find_if(parameter.recommendations.begin(),
                                                 parameter.recommendations.end(),
                                                 [&](const auto& candidate)
                                                 { return candidate.role == request.role; });
        if (recommendation != parameter.recommendations.end())
            return resolved(recommendation->controlLawId, recommendation->recommendedMinimum,
                            recommendation->recommendedMaximum,
                            CuratedDspControlLawResolutionSource::roleRecommendation);
    }
    if (!parameter.defaultControlLawId.empty())
        return resolved(parameter.defaultControlLawId, parameter.minimum, parameter.maximum,
                        CuratedDspControlLawResolutionSource::descriptorDefault);
    return fallbackResolution(parameter.unit, parameter.minimum, parameter.maximum);
}
} // namespace drs::engine
