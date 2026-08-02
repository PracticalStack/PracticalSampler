#include <array>
#include <iostream>
#include <string_view>

namespace
{
constexpr std::array<std::string_view, 7> redSeams {
    "compiled-event-index-tables",
    "lane-local-attack-context",
    "latch-switch-note-consumption",
    "physical-note-off-and-effective-release",
    "pedal-transition-trigger-routes",
    "compiled-choke-target-mask",
    "rr-reset-before-route-selection"
};

bool isKnownSeam(const std::string_view value)
{
    for (const auto seam : redSeams)
        if (seam == value)
            return true;
    return false;
}
} // namespace

// Sprint 0 deliberately owns only the contract and fixture baseline. These
// named checks are direct-only and intentionally red until their later sprint
// turns each seam into a registered behavioural test.
int main(int argc, char** argv)
{
    if (argc != 2 || !isKnownSeam(argv[1]))
    {
        std::cerr << "Usage: drs_performance_engine_s0_red_tests <named-missing-seam>\n";
        for (const auto seam : redSeams)
            std::cerr << "  " << seam << '\n';
        return 2;
    }

    std::cerr << "EXPECTED RED: missing declarative performance-engine seam '"
              << argv[1] << "'. The Sprint 0 contract reserves this behavior; "
              << "production support belongs to its later sprint.\n";
    return 1;
}
