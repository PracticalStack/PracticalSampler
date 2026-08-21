#include "shared/authoring/ScopedZoneProjection.h"
#include "shared/authoring/InstrumentStructureBrowser.h"
#include "shared/authoring/StructureScope.h"
#include "support/StructureViewerFixtures.h"

#include <chrono>
#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <vector>
#include <numeric>

namespace
{
void require(bool value, const char* message)
{
    if (!value) throw std::runtime_error(message);
}
}

int main()
{
    try
    {
        const auto project = drs::tests::makeStructureViewerFixture(1695);
        drs::app::authoring::AuthoringStructureSelection selection;
        selection.replace(drs::app::authoring::StructureSelectionKind::layer,
                          { "layer-piano" }, "layer-piano");
        std::vector<long long> timings;
        drs::app::authoring::ScopedZoneProjection projection;
        for (int iteration = 0; iteration < 20; ++iteration)
        {
            const auto start = std::chrono::steady_clock::now();
            projection = drs::app::authoring::buildScopedZoneProjection(
                project, { drs::app::authoring::StructureScopeKind::layer, "layer-piano" }, selection);
            timings.push_back(std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start).count());
        }
        std::sort(timings.begin(), timings.end());
        const auto average = std::accumulate(timings.begin(), timings.end(), 0LL)
            / static_cast<long long>(timings.size());
        const auto p95 = timings[static_cast<std::size_t>(timings.size() * 95 / 100)];
        const auto browserStart = std::chrono::steady_clock::now();
        const auto rows = drs::app::authoring::buildInstrumentStructureRows(
            project, selection, {}, {}, {});
        const auto browserElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - browserStart).count();
        require(projection.totalInScope > 800, "Large fixture should exercise production-sized scope projection.");
        require(projection.zones.size() == projection.totalInScope,
                "Default qualification projection should include visible fixture containers.");
        require(rows.size() > 1000 && browserElapsed < 250,
                "Hierarchy refresh should remain responsive for a 1,700-zone fixture.");
        require(p95 < 250, "Scope projection should remain responsive on the UI thread.");
        std::cout << "Unified workspace qualification passed for " << project.authoring.zones.size()
                  << " zones; projection average " << average << " ms, p95 " << p95
                  << " ms; browser " << rows.size() << " rows in " << browserElapsed << " ms.\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "Unified workspace qualification failed: " << error.what() << "\n";
        return 1;
    }
}
