#include "drs/engine/ContinuousDamper.h"

#include <algorithm>
#include <cmath>
#include <map>

namespace drs::engine
{
ContinuousDamperCurveCompileResult compileContinuousDamperCurve(
    const std::vector<ContinuousDamperCurvePoint>& points)
{
    ContinuousDamperCurveCompileResult result;
    if (points.empty())
    {
        result.findingCode = "damper.curve.empty";
        result.detail = "A continuous damper curve must declare at least one point.";
        return result;
    }

    std::map<int, double> ordered;
    for (const auto& point : points)
    {
        if (point.controllerValue < 0
            || point.controllerValue >= static_cast<int>(continuousDamperCurvePointCount))
        {
            result.findingCode = "damper.curve.point_index_out_of_range";
            result.detail = "Continuous damper curve point indices must be between 0 and 127.";
            return result;
        }
        if (!std::isfinite(point.normalizedValue))
        {
            result.findingCode = "damper.curve.point_non_finite";
            result.detail = "Continuous damper curve points must be finite.";
            return result;
        }
        if (point.normalizedValue < 0.0 || point.normalizedValue > 1.0)
        {
            result.findingCode = "damper.curve.point_out_of_range";
            result.detail = "Continuous damper curve points must be normalized to 0 through 1.";
            return result;
        }
        if (!ordered.emplace(point.controllerValue, point.normalizedValue).second)
        {
            result.findingCode = "damper.curve.point_duplicate";
            result.detail = "Continuous damper curves must not declare the same vNNN point twice.";
            return result;
        }
    }

    const auto last = std::prev(ordered.end());
    for (int index = 0; index < static_cast<int>(continuousDamperCurvePointCount); ++index)
    {
        const auto upper = ordered.lower_bound(index);
        if (upper == ordered.begin())
        {
            result.values[static_cast<std::size_t>(index)] = upper->second;
            continue;
        }
        if (upper == ordered.end())
        {
            result.values[static_cast<std::size_t>(index)] = last->second;
            continue;
        }
        if (upper->first == index)
        {
            result.values[static_cast<std::size_t>(index)] = upper->second;
            continue;
        }

        const auto lower = std::prev(upper);
        const auto proportion = static_cast<double>(index - lower->first)
            / static_cast<double>(upper->first - lower->first);
        result.values[static_cast<std::size_t>(index)] = lower->second
            + ((upper->second - lower->second) * proportion);
    }

    result.compiled = true;
    return result;
}

bool validateContinuousDamperDefinition(const ContinuousDamperDefinition& damper,
                                        std::string& findingCode,
                                        std::string& detail) noexcept
{
    const auto fail = [&](const char* code, const char* message)
    {
        findingCode = code;
        detail = message;
        return false;
    };
    if (damper.sustainControllerNumber < 0 || damper.sustainControllerNumber > 127)
        return fail("damper.sustain_controller_out_of_range", "Damper sustainControllerNumber must be between 0 and 127.");
    if (!std::isfinite(damper.sustainThreshold)
        || damper.sustainThreshold < 0.0 || damper.sustainThreshold > 127.0)
        return fail("damper.sustain_threshold_out_of_range", "Damper sustainThreshold must be finite and between 0 and 127.");
    if (damper.releaseControllerNumber != halfPedalReleaseControllerNumber)
        return fail("damper.release_controller_unsupported", "HP-02 supports dynamic amplitude release on CC64 only.");
    if (!std::isfinite(damper.releaseAmountSeconds)
        || damper.releaseAmountSeconds < 0.0 || damper.releaseAmountSeconds > maximumDynamicReleaseSeconds)
        return fail("damper.release_amount_out_of_range", "Damper releaseAmountSeconds must be finite and between 0 and 100 seconds.");
    if (!damper.dynamicRelease)
    {
        if (damper.releaseAmountSeconds != 0.0 || damper.releaseCurveIndex != -1)
            return fail("damper.disabled_release_metadata", "Disabled dynamic release must not carry an amount or curve reference.");
        return true;
    }
    if (damper.releaseCurveIndex < 0 || damper.releaseCurveIndex > 255)
        return fail("damper.curve_reference_out_of_range", "Dynamic release curve references must be between 0 and 255.");
    for (const auto value : damper.releaseCurve)
        if (!std::isfinite(value) || value < 0.0 || value > 1.0)
            return fail("damper.curve_value_invalid", "Compiled damper curve values must be finite and normalized to 0 through 1.");
    return true;
}
} // namespace drs::engine
