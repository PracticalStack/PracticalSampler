#pragma once

#include "drs/engine/AuthoringSession.h"

#include <string>
#include <vector>

namespace drs::app::authoring
{
struct ZoneMapOverlapCandidate
{
    std::string zoneId;
    std::string displayName;
    int keyLow = 0;
    int keyHigh = 127;
    int velocityLow = 1;
    int velocityHigh = 127;
    int overlapArea = 0;
    bool exactStack = false;
};

// UI-side hit index for intentional Map overlap. It never changes paint order
// or authored data; it only answers which zones share the hit coordinate.
class ZoneMapOverlapIndex
{
public:
    void rebuild(const std::vector<drs::engine::AuthoringZoneSummary>& zones);
    std::vector<ZoneMapOverlapCandidate> query(int key, int velocity,
                                               const std::string& excludeZoneId = {}) const;
    std::size_t size() const noexcept { return zones.size(); }

private:
    std::vector<drs::engine::AuthoringZoneSummary> zones;
};
} // namespace drs::app::authoring
