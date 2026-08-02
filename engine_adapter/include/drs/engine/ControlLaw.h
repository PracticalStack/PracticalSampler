#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <string_view>

namespace drs::engine
{
// IDs are persisted contract identifiers.  Do not alter the behavior associated
// with a released ID; introduce a new versioned ID instead.
inline constexpr std::string_view controlLawLinearDbV1 = "drs.linearDb.v1";
inline constexpr std::string_view controlLawLogPositiveV1 = "drs.logPositive.v1";
inline constexpr std::string_view controlLawMixerGainV1 = "drs.mixerGain.v1";
inline constexpr std::string_view controlLawBipolarLinearV1 = "drs.bipolarLinear.v1";
inline constexpr std::string_view controlLawBipolarCenteredV1 = "drs.bipolarCentered.v1";
inline constexpr std::string_view controlLawSteppedV1 = "drs.stepped.v1";
inline constexpr std::string_view controlLawToggleV1 = "drs.toggle.v1";

enum class ControlLawKind : std::uint8_t
{
    invalid = 0,
    linear,
    positiveLog,
    mixerGainV1,
    bipolarCentered,
    stepped,
    toggle
};

struct ControlLawPoint
{
    double normalized = 0.0;
    double physical = 0.0;
};

// This type crosses the publication/audio boundary. It is deliberately bounded,
// trivially copyable, and contains no dynamically owned data.
struct CompiledControlLaw
{
    ControlLawKind kind = ControlLawKind::invalid;
    std::uint32_t version = 0;
    std::uint8_t pointCount = 0;
    std::array<ControlLawPoint, 8> points {};
};

enum class ControlLawUnit : std::uint8_t
{
    generic,
    decibels,
    hertz,
    milliseconds,
    seconds,
    percent,
    pan
};

struct ControlLawFormatOptions
{
    bool renderMinimumAsNegativeInfinity = false;
    double negativeInfinityThreshold = -96.0;
    unsigned int precision = 1;
};

// Builds a fully resolved immutable law. The mixer law accepts only its frozen
// -96 to +6 dB range; positive-log laws require a strictly positive range.
bool compileControlLaw(std::string_view id,
                       double physicalMinimum,
                       double physicalMaximum,
                       CompiledControlLaw& result) noexcept;
bool isCompiledControlLawValid(const CompiledControlLaw& law) noexcept;

// Return false instead of manufacturing a physical value for invalid/non-finite
// input. Callers reject those values before publishing to DSP.
bool normalizedToPhysical(const CompiledControlLaw& law,
                          double normalized,
                          double& physical) noexcept;
bool physicalToNormalized(const CompiledControlLaw& law,
                          double physical,
                          double& normalized) noexcept;

// Formatting is intentionally separate from compiled realtime data. It is for
// Perform/authoring presentation and accessibility strings only.
std::string formatControlLawValue(double physical,
                                  ControlLawUnit unit,
                                  ControlLawFormatOptions options = {});
} // namespace drs::engine
