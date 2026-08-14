#pragma once

#include <juce_graphics/juce_graphics.h>

#include <array>
#include <cstdint>
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
    constexpr std::array<juce::uint32, 8> palette {
        0xff6689a6, 0xff71937d, 0xffb58a55, 0xff8d78a5,
        0xff5e9598, 0xffa16f70, 0xff7f8d5c, 0xff8a7b70
    };
    std::uint32_t hash = 2166136261u;
    for (const auto character : groupId)
    {
        hash ^= static_cast<std::uint8_t>(character);
        hash *= 16777619u;
    }
    return juce::Colour(palette[hash % palette.size()]);
}
} // namespace drs::app::authoring
