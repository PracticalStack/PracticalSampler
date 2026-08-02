#include "drs/engine/RuntimeLoader.h"

#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace
{
struct MixerGainAnchor
{
    double normalized = 0.0;
    double decibels = 0.0;
};

constexpr std::array<MixerGainAnchor, 7> mixerGainV1Anchors {{
    { 0.00, -96.0 }, { 0.05, -60.0 }, { 0.25, -30.0 }, { 0.50, -15.0 },
    { 0.75, -6.0 }, { 0.85, 0.0 }, { 1.00, 6.0 }
}};

constexpr std::array<std::string_view, 7> redSeams {
    "anchors", "monotonicity", "endpoints", "inverse-round-trips", "clamping",
    "non-finite-rejection", "formatting"
};

struct RedContractCase
{
    std::string_view seam;
    std::string_view input;
    std::string_view expected;
};

// This is deliberately data, not a second local mapping implementation. Sprint
// 1 consumes the same cases with ControlLaw's public forward/inverse/format APIs.
constexpr std::array<RedContractCase, 14> redContractCases {{
    { "anchors", "0.05", "-60 dB" }, { "anchors", "0.50", "-15 dB" },
    { "anchors", "0.85", "0 dB" }, { "monotonicity", "all adjacent anchors", "strictly increasing dB" },
    { "endpoints", "0.00", "-96 dB exactly" }, { "endpoints", "1.00", "+6 dB exactly" },
    { "inverse-round-trips", "-30 dB", "0.25 within tolerance" },
    { "inverse-round-trips", "0 dB", "0.85 within tolerance" },
    { "clamping", "normalized < 0", "-96 dB" }, { "clamping", "normalized > 1", "+6 dB" },
    { "non-finite-rejection", "NaN", "reject before publication" },
    { "non-finite-rejection", "+/-Inf", "reject before publication" },
    { "formatting", "-96 dB endpoint", "-inf display" },
    { "formatting", "0 dB", "0.0 dB display" }
}};

void require(const bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

bool isKnownSeam(const std::string_view value)
{
    for (const auto seam : redSeams)
        if (seam == value)
            return true;
    return false;
}

double legacyLinearDb(const double normalized) noexcept
{
    return -96.0 + (24.0 - -96.0) * normalized;
}

void verifyRegressionFixture()
{
    const auto loaded = drs::engine::loadRuntimeProjectManifest(DRS_CONTROL_LAW_S0_FIXTURE_PATH);
    require(loaded.loaded, "The three-group legacy-linear fixture must load.");
    require(loaded.project.authoring.groups.size() == 3
                && loaded.project.authoring.macros.size() == 3,
            "The fixture must contain Bell, EPiano, and Plucks Perform groups.");
    for (const auto& macro : loaded.project.authoring.macros)
    {
        require(macro.exposedInPerformance && macro.targets.size() == 1,
                "Every fixture group must expose exactly one gain macro.");
        const auto& target = macro.targets.front();
        require(target.role == "mix" && target.dspParameterId == "gainDb"
                    && target.curve == "linear"
                    && target.sourceMinimum == 0.0 && target.sourceMaximum == 1.0
                    && target.destinationMinimum == -96.0 && target.destinationMaximum == 24.0,
                "The fixture must preserve the legacy linear -96 to +24 dB mapping.");
        require(std::abs(macro.defaultValue - 0.8) < 1.0e-12,
                "The fixture must expose the historic 80 percent unity position.");
    }
}

[[noreturn]] void reportExpectedRed(const std::string_view seam)
{
    std::cerr << "EXPECTED RED [" << seam << "]: shared ControlLaw core is not implemented.\n";
    for (const auto& contractCase : redContractCases)
    {
        if (contractCase.seam == seam)
            std::cerr << "  input=" << contractCase.input << ", expected="
                      << contractCase.expected << '\n';
    }
    if (seam == "anchors")
    {
        for (const auto& anchor : mixerGainV1Anchors)
            std::cerr << "  normalized=" << anchor.normalized << " expected=" << anchor.decibels
                      << " dB, legacy-linear=" << legacyLinearDb(anchor.normalized) << " dB\n";
    }
    std::cerr << "Sprint 1 must replace this direct-only red seam with the shared, bidirectional "
                 "ControlLaw implementation and registered green tests.\n";
    std::exit(1);
}
} // namespace

int main(int argc, char** argv)
{
    if (argc != 2 || !isKnownSeam(argv[1]))
    {
        std::cerr << "Usage: drs_control_law_s0_red_tests <named-missing-seam>\n";
        for (const auto seam : redSeams)
            std::cerr << "  " << seam << '\n';
        return 2;
    }

    try
    {
        verifyRegressionFixture();
        reportExpectedRed(argv[1]);
    }
    catch (const std::exception& error)
    {
        std::cerr << "Fixture contract failure: " << error.what() << '\n';
        return 1;
    }
}
