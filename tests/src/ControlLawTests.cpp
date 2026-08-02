#include "drs/engine/ControlLaw.h"
#include "drs/engine/RuntimeLoader.h"

#include <array>
#include <cmath>
#include <limits>
#include <iostream>
#include <stdexcept>
#include <string>
#include <type_traits>

namespace
{
using namespace drs::engine;

struct MixerGainAnchor { double normalized; double decibels; };
constexpr std::array<MixerGainAnchor, 7> anchors {{
    { 0.00, -96.0 }, { 0.05, -60.0 }, { 0.25, -30.0 }, { 0.50, -15.0 },
    { 0.75, -6.0 }, { 0.85, 0.0 }, { 1.00, 6.0 }
}};

void require(const bool condition, const std::string& message)
{
    if (!condition) throw std::runtime_error(message);
}

CompiledControlLaw compile(const std::string_view id, const double minimum, const double maximum)
{
    CompiledControlLaw law;
    require(compileControlLaw(id, minimum, maximum, law), "Control law compilation failed.");
    require(isCompiledControlLawValid(law), "Compiled control law must validate.");
    return law;
}

double forward(const CompiledControlLaw& law, const double normalized)
{
    double physical = 0.0;
    require(normalizedToPhysical(law, normalized, physical), "Forward mapping must accept finite input.");
    return physical;
}

double inverse(const CompiledControlLaw& law, const double physical)
{
    double normalized = 0.0;
    require(physicalToNormalized(law, physical, normalized), "Inverse mapping must accept finite input.");
    return normalized;
}

void verifyLegacyFixture()
{
    const auto loaded = loadRuntimeProjectManifest(DRS_CONTROL_LAW_S0_FIXTURE_PATH);
    require(loaded.loaded && loaded.project.authoring.groups.size() == 3
                && loaded.project.authoring.macros.size() == 3,
            "The Sprint 0 three-group legacy fixture must load unchanged.");
    for (const auto& macro : loaded.project.authoring.macros)
    {
        require(macro.exposedInPerformance && macro.targets.size() == 1,
                "Each legacy fixture group must expose its gain control.");
        const auto& target = macro.targets.front();
        require(target.curve == "linear" && target.destinationMinimum == -96.0
                    && target.destinationMaximum == 24.0 && std::abs(macro.defaultValue - 0.8) < 1.0e-12,
                "The legacy fixture must preserve its 80 percent linear-unity behavior.");
    }
}

void verifyMixerGain()
{
    const auto law = compile(controlLawMixerGainV1, -96.0, 6.0);
    require(std::is_trivially_copyable<CompiledControlLaw>::value,
            "Compiled callback data must be trivially copyable.");
    require(law.kind == ControlLawKind::mixerGainV1 && law.version == 1,
            "Compiled mixer data must retain a stable kind and version.");
    for (const auto& anchor : anchors)
    {
        require(std::abs(forward(law, anchor.normalized) - anchor.decibels) < 1.0e-12,
                "Mixer gain anchor does not match the frozen contract.");
        require(std::abs(inverse(law, anchor.decibels) - anchor.normalized) < 1.0e-12,
                "Mixer gain anchor inverse does not match the frozen contract.");
    }
    auto previous = forward(law, 0.0);
    for (int index = 1; index <= 10000; ++index)
    {
        const auto normalized = static_cast<double>(index) / 10000.0;
        const auto physical = forward(law, normalized);
        require(physical >= previous, "Mixer gain must be monotonic.");
        require(std::abs(inverse(law, physical) - normalized) < 1.0e-12,
                "Mixer gain forward/inverse round trip must be stable.");
        previous = physical;
    }
    require(forward(law, -1.0) == -96.0 && forward(law, 2.0) == 6.0,
            "Mixer gain must clamp normalized endpoints.");
    require(inverse(law, -120.0) == 0.0 && inverse(law, 20.0) == 1.0,
            "Mixer gain inverse must clamp physical endpoints.");
}

void verifyOtherLawsAndProperties()
{
    const auto linear = compile(controlLawLinearDbV1, -24.0, 6.0);
    const auto logarithmic = compile(controlLawLogPositiveV1, 10.0, 1000.0);
    const auto centered = compile(controlLawBipolarCenteredV1, -18.0, 12.0);
    const auto stepped = compile(controlLawSteppedV1, 0.0, 4.0);
    const auto toggle = compile(controlLawToggleV1, 0.0, 1.0);
    require(std::abs(forward(linear, 0.5) + 9.0) < 1.0e-12,
            "Linear dB law must interpolate physical values.");
    require(std::abs(forward(logarithmic, 0.5) - 100.0) < 1.0e-12,
            "Positive-log midpoint must be the geometric mean.");
    require(forward(centered, 0.5) == 0.0 && inverse(centered, 0.0) == 0.5,
            "Bipolar centered law requires a zero detent.");
    require(forward(stepped, 0.63) == 3.0 && forward(toggle, 0.49) == 0.0
                && forward(toggle, 0.50) == 1.0,
            "Stepped and toggle laws must have deterministic discrete behavior.");

    std::uint32_t state = 0x4d595df4u;
    for (int index = 0; index < 10000; ++index)
    {
        state = state * 1664525u + 1013904223u;
        const auto normalized = static_cast<double>(state) / std::numeric_limits<std::uint32_t>::max();
        for (const auto* law : { &linear, &logarithmic, &centered })
        {
            const auto physical = forward(*law, normalized);
            require(std::isfinite(physical) && std::isfinite(inverse(*law, physical)),
                    "Randomized finite mappings must remain finite.");
            require(std::abs(inverse(*law, physical) - normalized) < 1.0e-11,
                    "Randomized finite mappings must round trip.");
        }
    }
}

void verifyRejectionAndFormatting()
{
    CompiledControlLaw law;
    require(!compileControlLaw("unknown", -96.0, 6.0, law)
                && !compileControlLaw(controlLawMixerGainV1, -96.0, 24.0, law)
                && !compileControlLaw(controlLawLogPositiveV1, 0.0, 100.0, law),
            "Unknown and incompatible law specifications must be rejected.");
    const auto mixer = compile(controlLawMixerGainV1, -96.0, 6.0);
    double output = 0.0;
    const auto nan = std::numeric_limits<double>::quiet_NaN();
    const auto infinity = std::numeric_limits<double>::infinity();
    require(!normalizedToPhysical(mixer, nan, output) && !normalizedToPhysical(mixer, infinity, output)
                && !physicalToNormalized(mixer, nan, output) && !physicalToNormalized(mixer, -infinity, output),
            "NaN and infinity must be rejected rather than sent to DSP.");

    require(formatControlLawValue(-96.0, ControlLawUnit::decibels, { true, -96.0, 1 })
                    == "\xE2\x88\x92\xE2\x88\x9E"
                && formatControlLawValue(0.0, ControlLawUnit::decibels) == "0.0 dB"
                && formatControlLawValue(6.0, ControlLawUnit::decibels) == "+6.0 dB"
                && formatControlLawValue(1200.0, ControlLawUnit::hertz, { false, -96.0, 2 }) == "1.20 kHz"
                && formatControlLawValue(2500.0, ControlLawUnit::milliseconds, { false, -96.0, 2 }) == "2.50 s"
                && formatControlLawValue(0.25, ControlLawUnit::percent) == "25.0%"
                && formatControlLawValue(-0.25, ControlLawUnit::pan) == "L 25"
                && formatControlLawValue(0.0, ControlLawUnit::pan) == "C",
            "Shared formatting must preserve the approved dB, time, percent, and pan labels.");
}
} // namespace

int main()
{
    try
    {
        verifyLegacyFixture();
        verifyMixerGain();
        verifyOtherLawsAndProperties();
        verifyRejectionAndFormatting();
        std::cout << "Control-law core tests passed.\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "Control-law core test failure: " << error.what() << '\n';
        return 1;
    }
}
