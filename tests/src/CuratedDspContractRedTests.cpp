#include <array>
#include <iostream>
#include <string_view>

namespace
{
constexpr std::array<std::string_view, 6> redSeams {
    "schema-5-effect-version-and-parameters",
    "immutable-dsp-graph-plan",
    "precompiled-scoped-destination-index",
    "generation-owned-dsp-state",
    "generation-tagged-parameter-control",
    "bounded-tail-retirement"
};

bool isKnownSeam(std::string_view value)
{
    for (const auto seam : redSeams)
        if (seam == value)
            return true;
    return false;
}
} // namespace

// These are deliberately direct-only S0 tests. Invoke one named seam at a time;
// each exits red until its production contract is implemented and this target is
// converted into the matching registered green suite in later sprints.
int main(int argc, char** argv)
{
    if (argc != 2 || !isKnownSeam(argv[1]))
    {
        std::cerr << "Usage: drs_curated_dsp_contract_red_tests <named-missing-seam>\n";
        for (const auto seam : redSeams)
            std::cerr << "  " << seam << '\n';
        return 2;
    }

    std::cerr << "EXPECTED RED: missing curated DSP seam '" << argv[1]
              << "'. This direct-only contract check must fail until its named behavior exists.\n";
    return 1;
}
