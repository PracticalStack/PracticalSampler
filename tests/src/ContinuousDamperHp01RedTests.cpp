#include <array>
#include <iostream>
#include <string_view>

namespace
{
constexpr std::array<std::string_view, 6> redSeams {
    "preserve-continuous-cc64",
    "dynamic-release-envelope",
    "repedal-still-audible",
    "release-trigger-uniqueness",
    "sustain-controller-reassignment",
    "generation-owned-damper-update"
};

constexpr std::string_view promotedHp02Seam = "salamander-half-pedal-projection";

bool isKnownSeam(const std::string_view value)
{
    for (const auto seam : redSeams)
        if (seam == value)
            return true;
    return false;
}
} // namespace

// HP-01 owns the contract and deterministic evidence only. These checks are
// direct-only and intentionally red until HP-02 through HP-04 replace each seam
// with registered behavioral coverage.
int main(int argc, char** argv)
{
    if (argc == 2 && std::string_view(argv[1]) == promotedHp02Seam)
    {
        std::cout << "PROMOTED GREEN: continuous damper seam '" << promotedHp02Seam
                  << "' is covered by drs.continuous_damper.hp02.\n";
        return 0;
    }

    if (argc != 2 || !isKnownSeam(argv[1]))
    {
        std::cerr << "Usage: drs_continuous_damper_hp01_red_tests <named-missing-seam>\n";
        for (const auto seam : redSeams)
            std::cerr << "  " << seam << '\n';
        std::cerr << "Promoted seams:\n  " << promotedHp02Seam << '\n';
        return 2;
    }

    std::cerr << "EXPECTED RED: missing continuous damper seam '" << argv[1]
              << "'. HP-01 reserves this behavior; production support belongs "
                 "to HP-02 through HP-04.\n";
    return 1;
}
