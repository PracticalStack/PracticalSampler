#include "drs/engine/ControlLaw.h"
#include "drs/engine/CuratedDspCatalog.h"

#include <algorithm>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace
{
using namespace drs::engine;

void require(const bool condition, const std::string& message)
{
    if (!condition) throw std::runtime_error(message);
}

const CuratedDspParameterDescriptor& findParameter(const std::string_view effectId,
                                                    const std::string_view parameterId)
{
    const auto* effect = findCuratedDspEffect(effectId, 1);
    require(effect != nullptr, "Expected Wave 1 catalog effect is absent.");
    const auto found = std::find_if(effect->parameters.begin(), effect->parameters.end(),
                                    [&](const auto& parameter) { return parameter.id == parameterId; });
    require(found != effect->parameters.end(), "Expected catalog parameter is absent.");
    return *found;
}

std::string snapshotCatalogControlMetadata()
{
    std::ostringstream stream;
    stream << std::setprecision(17);
    for (const auto& effect : getCuratedDspCatalog())
    {
        for (const auto& parameter : effect.parameters)
        {
            stream << effect.typeId << '@' << effect.algorithmVersion << '/' << parameter.id << '|'
                   << static_cast<unsigned int>(parameter.unit) << '|'
                   << parameter.minimum << '|' << parameter.maximum << '|'
                   << parameter.defaultValue << '|' << static_cast<unsigned int>(parameter.smoothing)
                   << '|' << parameter.defaultControlLawId << "|allowed=";
            for (const auto law : parameter.allowedControlLawIds) stream << law << ',';
            stream << "|recommendations=";
            for (const auto& recommendation : parameter.recommendations)
                stream << recommendation.role << ':' << recommendation.controlLawId << ':'
                       << recommendation.recommendedMinimum << ':'
                       << recommendation.recommendedMaximum << ',';
            stream << '\n';
        }
    }
    return stream.str();
}

std::uint64_t fnv1a(const std::string& text)
{
    std::uint64_t value = 14695981039346656037ull;
    for (const auto character : text)
    {
        value ^= static_cast<unsigned char>(character);
        value *= 1099511628211ull;
    }
    return value;
}

void verifyCatalogAndGoldenSnapshot()
{
    std::vector<std::string> findings;
    if (!validatesCuratedDspCatalog(findings))
    {
        std::string message = "Curated control-law metadata must validate:";
        for (const auto& finding : findings) message += " [" + finding + ']';
        throw std::runtime_error(message);
    }

    std::size_t parameterCount = 0;
    for (const auto& effect : getCuratedDspCatalog())
    {
        for (const auto& parameter : effect.parameters)
        {
            ++parameterCount;
            require(!parameter.defaultControlLawId.empty()
                        && !parameter.allowedControlLawIds.empty()
                        && std::find(parameter.allowedControlLawIds.begin(), parameter.allowedControlLawIds.end(),
                                     parameter.defaultControlLawId) != parameter.allowedControlLawIds.end(),
                    "Every curated parameter must have an allowed default control law.");
        }
    }
    require(parameterCount == 32, "The Wave 1 catalog inventory must remain complete.");

    constexpr std::uint64_t goldenSnapshotHash = 0x430e490609c7f153ull;
    const auto snapshot = snapshotCatalogControlMetadata();
    const auto actualHash = fnv1a(snapshot);
    if (actualHash != goldenSnapshotHash)
    {
        std::ostringstream message;
        message << "Catalog control-law golden changed. hash=0x" << std::hex << actualHash
                << "\n" << snapshot;
        throw std::runtime_error(message.str());
    }
}

void verifyResolver()
{
    const auto& gain = findParameter("drs.gain", "gainDb");
    const auto mixer = resolveCuratedDspControlLaw({ &gain, "mix", {} });
    require(mixer.resolved && mixer.source == CuratedDspControlLawResolutionSource::roleRecommendation
                && mixer.controlLawId == controlLawMixerGainV1
                && mixer.minimum == -96.0 && mixer.maximum == 6.0,
            "Group/bus gain with the mix role must resolve to mixer-gain v1.");

    const auto explicitLinear = resolveCuratedDspControlLaw(
        { &gain, "mix", controlLawLinearDbV1 });
    require(explicitLinear.resolved
                && explicitLinear.source == CuratedDspControlLawResolutionSource::explicitOverride
                && explicitLinear.controlLawId == controlLawLinearDbV1
                && explicitLinear.minimum == -96.0 && explicitLinear.maximum == 24.0,
            "An allowed explicit override must take priority and retain descriptor range.");

    const auto explicitMixer = resolveCuratedDspControlLaw(
        { &gain, {}, controlLawMixerGainV1 });
    require(explicitMixer.resolved
                && explicitMixer.source == CuratedDspControlLawResolutionSource::explicitOverride
                && explicitMixer.minimum == -96.0 && explicitMixer.maximum == 6.0,
            "An allowed override with a role-specific range must resolve that validated range.");

    const auto defaultLaw = resolveCuratedDspControlLaw({ &gain, "unclassified-role", {} });
    require(defaultLaw.resolved
                && defaultLaw.source == CuratedDspControlLawResolutionSource::descriptorDefault
                && defaultLaw.controlLawId == controlLawLinearDbV1,
            "Unknown roles must fall back to the descriptor default.");

    require(!resolveCuratedDspControlLaw({ &gain, "mix", controlLawToggleV1 }).resolved,
            "Overrides not approved by the descriptor must be rejected.");

    const auto hertzFallback = resolveCuratedDspControlLaw(
        { nullptr, {}, {}, CuratedDspParameterUnit::hertz, 40.0, 18000.0 });
    const auto booleanFallback = resolveCuratedDspControlLaw(
        { nullptr, {}, {}, CuratedDspParameterUnit::boolean, 0.0, 1.0 });
    require(hertzFallback.resolved
                && hertzFallback.source == CuratedDspControlLawResolutionSource::compatibilityFallback
                && hertzFallback.controlLawId == controlLawLogPositiveV1
                && booleanFallback.resolved && booleanFallback.controlLawId == controlLawToggleV1,
            "Incomplete legacy descriptors must use the documented unit-based fallback.");
}

void verifyInvalidCatalogRejection()
{
    const auto catalog = getCuratedDspCatalog();
    std::vector<std::string> findings;

    auto duplicateRole = catalog;
    duplicateRole.front().parameters.front().recommendations.push_back(
        { "mix", controlLawMixerGainV1, -96.0, 6.0 });
    require(!validatesCuratedDspCatalog(duplicateRole, findings),
            "Duplicate descriptor recommendation roles must be rejected.");

    auto invalidRange = catalog;
    invalidRange.front().parameters.front().recommendations.front().recommendedMaximum = 30.0;
    require(!validatesCuratedDspCatalog(invalidRange, findings),
            "Recommendation ranges outside the DSP descriptor range must be rejected.");

    auto unitMismatch = catalog;
    unitMismatch.front().parameters.front().allowedControlLawIds.push_back(controlLawToggleV1);
    require(!validatesCuratedDspCatalog(unitMismatch, findings),
            "A control law incompatible with the parameter unit must be rejected.");

    auto unknownDefault = catalog;
    unknownDefault.front().parameters.front().defaultControlLawId = "vendor.unknown.v1";
    require(!validatesCuratedDspCatalog(unknownDefault, findings),
            "Unknown descriptor defaults must be rejected.");
}
} // namespace

int main()
{
    try
    {
        verifyCatalogAndGoldenSnapshot();
        verifyResolver();
        verifyInvalidCatalogRejection();
        std::cout << "Curated DSP control-law tests passed.\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "Curated DSP control-law test failure: " << error.what() << '\n';
        return 1;
    }
}
