#include <array>
#include <iostream>
#include <string_view>

namespace
{
struct PromotedSeam
{
    std::string_view name;
    std::string_view registeredTest;
};

constexpr std::array<std::string_view, 2> redSeams {
    "repedal-still-audible",
    "release-trigger-uniqueness"
};

constexpr std::array<PromotedSeam, 5> promotedSeams {{
    { "salamander-half-pedal-projection", "drs.continuous_damper.hp02" },
    { "preserve-continuous-cc64", "drs.continuous_damper.hp03" },
    { "dynamic-release-envelope", "drs.continuous_damper.hp03" },
    { "sustain-controller-reassignment", "drs.continuous_damper.hp03" },
    { "generation-owned-damper-update", "drs.continuous_damper.hp03" }
}};

bool isKnownSeam(const std::string_view value)
{
    for (const auto seam : redSeams)
        if (seam == value)
            return true;
    return false;
}

const PromotedSeam* findPromotedSeam(const std::string_view value)
{
    for (const auto& seam : promotedSeams)
        if (seam.name == value)
            return &seam;
    return nullptr;
}
} // namespace

// HP-01 owns the contract and deterministic evidence only. These checks are
// direct-only and intentionally red until HP-02 through HP-04 replace each seam
// with registered behavioral coverage.
int main(int argc, char** argv)
{
    const auto* promoted = argc == 2 ? findPromotedSeam(argv[1]) : nullptr;
    if (promoted != nullptr)
    {
        std::cout << "PROMOTED GREEN: continuous damper seam '" << promoted->name
                  << "' is covered by " << promoted->registeredTest << ".\n";
        return 0;
    }

    if (argc != 2 || !isKnownSeam(argv[1]))
    {
        std::cerr << "Usage: drs_continuous_damper_hp01_red_tests <named-missing-seam>\n";
        for (const auto seam : redSeams)
            std::cerr << "  " << seam << '\n';
        std::cerr << "Promoted seams:\n";
        for (const auto& seam : promotedSeams)
            std::cerr << "  " << seam.name << '\n';
        return 2;
    }

    std::cerr << "EXPECTED RED: missing continuous damper seam '" << argv[1]
              << "'. HP-01 reserves this behavior; production support belongs "
                 "to HP-02 through HP-04.\n";
    return 1;
}
