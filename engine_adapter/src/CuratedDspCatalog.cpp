#include "drs/engine/CuratedDspCatalog.h"

#include <algorithm>
#include <cmath>
#include <unordered_set>

namespace drs::engine
{
const std::vector<CuratedDspEffectDescriptor>& getCuratedDspCatalog()
{
    static const std::vector<CuratedDspEffectDescriptor> catalog {
        { "drs.gain", 1, { CuratedDspScope::zone, CuratedDspScope::group, CuratedDspScope::instrument },
          { { "gainDb", CuratedDspParameterUnit::decibels, -96.0, 24.0, 0.0, CuratedDspSmoothing::linear },
            { "polarity", CuratedDspParameterUnit::boolean, 0.0, 1.0, 0.0, CuratedDspSmoothing::none },
            { "mute", CuratedDspParameterUnit::boolean, 0.0, 1.0, 0.0, CuratedDspSmoothing::linear } },
          CuratedDspStateClass::stateless, { 0, 0, 0, 1 } },
        { "drs.saturator", 1, { CuratedDspScope::zone, CuratedDspScope::group, CuratedDspScope::instrument },
          { { "character", CuratedDspParameterUnit::normalized, 0.0, 2.0, 0.0, CuratedDspSmoothing::none },
            { "driveDb", CuratedDspParameterUnit::decibels, 0.0, 36.0, 6.0, CuratedDspSmoothing::linear },
            { "tone", CuratedDspParameterUnit::normalized, 0.0, 1.0, 0.5, CuratedDspSmoothing::linear },
            { "mix", CuratedDspParameterUnit::normalized, 0.0, 1.0, 1.0, CuratedDspSmoothing::linear },
            { "outputDb", CuratedDspParameterUnit::decibels, -24.0, 24.0, 0.0, CuratedDspSmoothing::linear } },
          CuratedDspStateClass::bounded, { 128, 0, 0, 3 } },
        { "drs.stereoDelay", 1, { CuratedDspScope::zone, CuratedDspScope::group, CuratedDspScope::instrument },
          { { "timeMs", CuratedDspParameterUnit::milliseconds, 1.0, 2000.0, 375.0, CuratedDspSmoothing::logarithmic },
            { "sync", CuratedDspParameterUnit::boolean, 0.0, 1.0, 0.0, CuratedDspSmoothing::none },
            { "divisionBeats", CuratedDspParameterUnit::ratio, 0.0625, 4.0, 0.5, CuratedDspSmoothing::logarithmic },
            { "feedback", CuratedDspParameterUnit::ratio, 0.0, 0.95, 0.35, CuratedDspSmoothing::linear },
            { "pingPong", CuratedDspParameterUnit::boolean, 0.0, 1.0, 0.0, CuratedDspSmoothing::linear },
            { "tone", CuratedDspParameterUnit::normalized, 0.0, 1.0, 0.7, CuratedDspSmoothing::linear },
            { "width", CuratedDspParameterUnit::normalized, 0.0, 1.0, 1.0, CuratedDspSmoothing::linear },
            { "mix", CuratedDspParameterUnit::normalized, 0.0, 1.0, 0.25, CuratedDspSmoothing::linear } },
          CuratedDspStateClass::delay, { 1536000u, 0, 30u * 96000u, 12 } },
        { "drs.algorithmicReverb", 1, { CuratedDspScope::zone, CuratedDspScope::group, CuratedDspScope::instrument },
          { { "preDelayMs", CuratedDspParameterUnit::milliseconds, 0.0, 250.0, 20.0, CuratedDspSmoothing::linear },
            { "size", CuratedDspParameterUnit::normalized, 0.0, 1.0, 0.5, CuratedDspSmoothing::linear },
            { "decaySeconds", CuratedDspParameterUnit::seconds, 0.1, 20.0, 2.5, CuratedDspSmoothing::logarithmic },
            { "mix", CuratedDspParameterUnit::normalized, 0.0, 1.0, 0.2, CuratedDspSmoothing::linear } },
          CuratedDspStateClass::reverb, { 524288u, 0, 30u * 96000u, 20 } }
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

bool validatesCuratedDspCatalog(std::vector<std::string>& findings)
{
    findings.clear();
    std::unordered_set<std::string_view> typeVersions;
    for (const auto& effect : getCuratedDspCatalog())
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
        }
        if (effect.cost.maximumTailFrames > 30u * 96000u)
            findings.push_back("catalog tail exceeds S0 ceiling");
    }
    return findings.empty();
}
} // namespace drs::engine
