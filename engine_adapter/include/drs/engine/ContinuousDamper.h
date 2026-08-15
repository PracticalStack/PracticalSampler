#pragma once

#include "drs/engine/ContinuousDamperContract.h"

#include <array>
#include <string>
#include <vector>

namespace drs::engine
{
struct ContinuousDamperCurvePoint
{
    int controllerValue = 0;
    double normalizedValue = 0.0;
};

struct ContinuousDamperCurveCompileResult
{
    bool compiled = false;
    std::string findingCode;
    std::string detail;
    std::array<double, continuousDamperCurvePointCount> values {};
};

struct ContinuousDamperDefinition
{
    int sustainControllerNumber = legacySustainControllerNumber;
    double sustainThreshold = legacySustainThreshold;
    bool dynamicRelease = false;
    int releaseControllerNumber = halfPedalReleaseControllerNumber;
    double releaseAmountSeconds = 0.0;
    int releaseCurveIndex = -1;
    std::array<double, continuousDamperCurvePointCount> releaseCurve {};

    bool operator==(const ContinuousDamperDefinition& other) const noexcept
    {
        return sustainControllerNumber == other.sustainControllerNumber
            && sustainThreshold == other.sustainThreshold
            && dynamicRelease == other.dynamicRelease
            && releaseControllerNumber == other.releaseControllerNumber
            && releaseAmountSeconds == other.releaseAmountSeconds
            && releaseCurveIndex == other.releaseCurveIndex
            && releaseCurve == other.releaseCurve;
    }

    bool operator!=(const ContinuousDamperDefinition& other) const noexcept
    {
        return !(*this == other);
    }
};

ContinuousDamperCurveCompileResult compileContinuousDamperCurve(
    const std::vector<ContinuousDamperCurvePoint>& points);
bool validateContinuousDamperDefinition(const ContinuousDamperDefinition& damper,
                                        std::string& findingCode,
                                        std::string& detail) noexcept;
} // namespace drs::engine
