#pragma once

#include "shared/authoring/OpenWorkbenchVisualSystem.h"

#include <juce_graphics/juce_graphics.h>

#include <string_view>

namespace drs::app::authoring
{
enum class ZoneMapDetailLevel
{
    overview,
    working,
    detail
};

struct ZoneMapRenderPolicy
{
    ZoneMapDetailLevel level = ZoneMapDetailLevel::overview;
    bool drawZoneOutlines = true;
    bool drawCrossfades = false;
    bool drawHoverLabel = false;
    bool drawSelectedLabel = false;
    bool drawRangeHandles = false;
    bool drawCrossfadeHandles = false;
    bool drawSelectionAggregate = true;

    static ZoneMapRenderPolicy forDisplayedZoom(const int displayedZoom) noexcept
    {
        if (displayedZoom <= 34)
            return {};

        if (displayedZoom <= 89)
            return { ZoneMapDetailLevel::working, true, true, true, true, true, false, false };

        return { ZoneMapDetailLevel::detail, true, true, true, true, true, true, false };
    }
};

inline juce::Colour stableZoneGroupTint(const std::string_view groupId) noexcept
{
    return visual::stableGroupTint(groupId);
}
} // namespace drs::app::authoring
