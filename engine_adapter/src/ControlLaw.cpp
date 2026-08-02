#include "drs/engine/ControlLaw.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <limits>
#include <sstream>

namespace drs::engine
{
namespace
{
constexpr std::array<ControlLawPoint, 7> mixerGainV1Points {{
    { 0.00, -96.0 }, { 0.05, -60.0 }, { 0.25, -30.0 }, { 0.50, -15.0 },
    { 0.75, -6.0 }, { 0.85, 0.0 }, { 1.00, 6.0 }
}};

bool isFinite(const double value) noexcept
{
    return std::isfinite(value);
}

void setTwoPointLaw(CompiledControlLaw& result,
                    const ControlLawKind kind,
                    const double minimum,
                    const double maximum) noexcept
{
    result.kind = kind;
    result.version = 1;
    result.pointCount = 2;
    result.points[0] = { 0.0, minimum };
    result.points[1] = { 1.0, maximum };
}

bool mapPiecewiseForward(const CompiledControlLaw& law,
                         const double normalized,
                         double& physical) noexcept
{
    const auto clamped = std::clamp(normalized, 0.0, 1.0);
    if (clamped <= law.points.front().normalized)
    {
        physical = law.points.front().physical;
        return true;
    }
    for (std::uint8_t index = 1; index < law.pointCount; ++index)
    {
        const auto& right = law.points[index];
        const auto& left = law.points[index - 1];
        if (clamped <= right.normalized)
        {
            const auto fraction = (clamped - left.normalized)
                / (right.normalized - left.normalized);
            physical = left.physical + (right.physical - left.physical) * fraction;
            return true;
        }
    }
    physical = law.points[law.pointCount - 1].physical;
    return true;
}

bool mapPiecewiseInverse(const CompiledControlLaw& law,
                         const double physical,
                         double& normalized) noexcept
{
    const auto clamped = std::clamp(physical, law.points.front().physical,
                                    law.points[law.pointCount - 1].physical);
    if (clamped <= law.points.front().physical)
    {
        normalized = law.points.front().normalized;
        return true;
    }
    for (std::uint8_t index = 1; index < law.pointCount; ++index)
    {
        const auto& right = law.points[index];
        const auto& left = law.points[index - 1];
        if (clamped <= right.physical)
        {
            const auto fraction = (clamped - left.physical) / (right.physical - left.physical);
            normalized = left.normalized + (right.normalized - left.normalized) * fraction;
            return true;
        }
    }
    normalized = law.points[law.pointCount - 1].normalized;
    return true;
}

std::string fixed(double value, const unsigned int precision)
{
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(static_cast<int>(precision)) << value;
    return stream.str();
}
} // namespace

bool compileControlLaw(const std::string_view id,
                       const double physicalMinimum,
                       const double physicalMaximum,
                       CompiledControlLaw& result) noexcept
{
    result = {};
    if (!isFinite(physicalMinimum) || !isFinite(physicalMaximum)
        || !(physicalMinimum < physicalMaximum))
        return false;

    if (id == controlLawMixerGainV1)
    {
        if (physicalMinimum != -96.0 || physicalMaximum != 6.0)
            return false;
        result.kind = ControlLawKind::mixerGainV1;
        result.version = 1;
        result.pointCount = static_cast<std::uint8_t>(mixerGainV1Points.size());
        result.points = {};
        for (std::size_t index = 0; index < mixerGainV1Points.size(); ++index)
            result.points[index] = mixerGainV1Points[index];
        return true;
    }
    if (id == controlLawLinearDbV1 || id == controlLawBipolarLinearV1)
    {
        setTwoPointLaw(result, ControlLawKind::linear, physicalMinimum, physicalMaximum);
        return true;
    }
    if (id == controlLawLogPositiveV1)
    {
        if (!(physicalMinimum > 0.0)) return false;
        setTwoPointLaw(result, ControlLawKind::positiveLog, physicalMinimum, physicalMaximum);
        return true;
    }
    if (id == controlLawBipolarCenteredV1)
    {
        if (!(physicalMinimum < 0.0 && physicalMaximum > 0.0)) return false;
        result.kind = ControlLawKind::bipolarCentered;
        result.version = 1;
        result.pointCount = 3;
        result.points[0] = { 0.0, physicalMinimum };
        result.points[1] = { 0.5, 0.0 };
        result.points[2] = { 1.0, physicalMaximum };
        return true;
    }
    if (id == controlLawSteppedV1)
    {
        setTwoPointLaw(result, ControlLawKind::stepped, physicalMinimum, physicalMaximum);
        return true;
    }
    if (id == controlLawToggleV1)
    {
        setTwoPointLaw(result, ControlLawKind::toggle, physicalMinimum, physicalMaximum);
        return true;
    }
    return false;
}

bool isCompiledControlLawValid(const CompiledControlLaw& law) noexcept
{
    if (law.kind == ControlLawKind::invalid || law.version != 1 || law.pointCount < 2
        || law.pointCount > law.points.size())
        return false;
    for (std::uint8_t index = 0; index < law.pointCount; ++index)
    {
        const auto& point = law.points[index];
        if (!isFinite(point.normalized) || !isFinite(point.physical)
            || (index > 0 && (point.normalized <= law.points[index - 1].normalized
                              || point.physical <= law.points[index - 1].physical)))
            return false;
    }
    return law.points.front().normalized == 0.0
        && law.points[law.pointCount - 1].normalized == 1.0;
}

bool normalizedToPhysical(const CompiledControlLaw& law,
                          const double normalized,
                          double& physical) noexcept
{
    if (!isFinite(normalized) || !isCompiledControlLawValid(law)) return false;
    if (law.kind == ControlLawKind::positiveLog)
    {
        const auto clamped = std::clamp(normalized, 0.0, 1.0);
        physical = law.points[0].physical * std::pow(
            law.points[1].physical / law.points[0].physical, clamped);
        return isFinite(physical);
    }
    if (law.kind == ControlLawKind::stepped)
    {
        double linear = 0.0;
        mapPiecewiseForward(law, normalized, linear);
        physical = std::round(linear);
        return true;
    }
    if (law.kind == ControlLawKind::toggle)
    {
        physical = normalized < 0.5 ? law.points[0].physical : law.points[1].physical;
        return true;
    }
    return mapPiecewiseForward(law, normalized, physical);
}

bool physicalToNormalized(const CompiledControlLaw& law,
                          const double physical,
                          double& normalized) noexcept
{
    if (!isFinite(physical) || !isCompiledControlLawValid(law)) return false;
    if (law.kind == ControlLawKind::positiveLog)
    {
        const auto clamped = std::clamp(physical, law.points[0].physical, law.points[1].physical);
        normalized = std::log(clamped / law.points[0].physical)
            / std::log(law.points[1].physical / law.points[0].physical);
        return isFinite(normalized);
    }
    if (law.kind == ControlLawKind::toggle)
    {
        normalized = physical < (law.points[0].physical + law.points[1].physical) * 0.5 ? 0.0 : 1.0;
        return true;
    }
    return mapPiecewiseInverse(law, physical, normalized);
}

std::string formatControlLawValue(const double physical,
                                  const ControlLawUnit unit,
                                  const ControlLawFormatOptions options)
{
    if (!isFinite(physical)) return "Invalid";
    const auto precision = std::min(options.precision, 6u);
    switch (unit)
    {
        case ControlLawUnit::decibels:
            if (options.renderMinimumAsNegativeInfinity
                && physical <= options.negativeInfinityThreshold)
                return "\xE2\x88\x92\xE2\x88\x9E";
            return (physical > 0.0 ? "+" : "") + fixed(physical, precision) + " dB";
        case ControlLawUnit::hertz:
            if (std::abs(physical) >= 1000.0)
                return fixed(physical / 1000.0, precision) + " kHz";
            return fixed(physical, precision) + " Hz";
        case ControlLawUnit::milliseconds:
            if (std::abs(physical) >= 1000.0)
                return fixed(physical / 1000.0, precision) + " s";
            return fixed(physical, precision) + " ms";
        case ControlLawUnit::seconds:
            return fixed(physical, precision) + " s";
        case ControlLawUnit::percent:
            return fixed(physical * 100.0, precision) + "%";
        case ControlLawUnit::pan:
            if (std::abs(physical) < 1.0e-12) return "C";
            return std::string(physical < 0.0 ? "L " : "R ")
                + fixed(std::abs(physical) * 100.0, 0);
        case ControlLawUnit::generic:
            return fixed(physical, precision);
    }
    return "Invalid";
}
} // namespace drs::engine
