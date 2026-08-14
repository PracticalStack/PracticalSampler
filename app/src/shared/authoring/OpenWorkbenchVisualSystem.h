#pragma once

#include <juce_graphics/juce_graphics.h>

#include <array>
#include <cstdint>
#include <string_view>

namespace drs::app::authoring::visual
{
// Open Workbench is deliberately quiet and utilitarian: a neutral application
// shell, warm work surfaces, graphite text, and a small set of semantic accents.
// Keep authoring paint code on these roles so a future theme can be changed here.
inline const juce::Colour shell { 0xffe9ece8 };
inline const juce::Colour surface { 0xffffffff };
inline const juce::Colour surfaceRaised { 0xffffffff };
inline const juce::Colour surfaceSubtle { 0xfff7f8f5 };
inline const juce::Colour surfaceHover { 0xffedf0ec };
inline const juce::Colour border { 0xffbac2c1 };
inline const juce::Colour borderStrong { 0xff879395 };

inline const juce::Colour text { 0xff273035 };
inline const juce::Colour textMuted { 0xff5f686b };
inline const juce::Colour textDisabled { 0xff8a9293 };
inline const juce::Colour textOnAccent { 0xffffffff };

inline const juce::Colour selection { 0xffb6531d };
inline const juce::Colour selectionHover { 0xff9f4718 };
inline const juce::Colour selectionSecondary { 0xffb77951 };
inline const juce::Colour focus { 0xff28658f };
inline const juce::Colour information { 0xff2b76b7 };
inline const juce::Colour modulation { 0xff28786f };
inline const juce::Colour success { 0xff4f7e3b };
inline const juce::Colour warning { 0xff93601e };
inline const juce::Colour error { 0xffa6453e };

inline const juce::Colour mapSurface { 0xfffbfaf5 };
inline const juce::Colour mapGrid { 0xffdce1dc };
inline const juce::Colour dataLabelSurface { 0xd9202729 };

// Data-bearing crossfade directions are intentionally distinct from UI state.
inline const juce::Colour crossfadeIn { 0xff3f84ad };
inline const juce::Colour crossfadeOut { 0xffc37a33 };

constexpr float controlRadius = 2.0f;
constexpr float panelRadius = 3.0f;
constexpr float borderWidth = 1.0f;
constexpr float focusWidth = 2.0f;
constexpr int compactRowHeight = 24;
constexpr int controlHeight = 28;
constexpr int toolbarHeight = 28;
constexpr float titleTypeSize = 24.0f;
constexpr float sectionTypeSize = 16.0f;
constexpr float fieldTypeSize = 14.0f;
constexpr float bodyTypeSize = 13.0f;
constexpr float compactTypeSize = 12.5f;
constexpr float metadataTypeSize = 11.0f;

inline juce::Colour disabled(const juce::Colour colour) noexcept
{
    return colour.interpolatedWith(surface, 0.58f);
}

inline juce::Colour stableGroupTint(const std::string_view groupId) noexcept
{
    // These muted colors carry data identity, not UI state. Selection and focus
    // remain independently visible through orange and blue outlines.
    constexpr std::array<juce::uint32, 8> palette {
        0xff6f91aa, 0xff789581, 0xffad895c, 0xff8e7ea1,
        0xff699497, 0xff9f7474, 0xff838f68, 0xff8c7f75
    };
    std::uint32_t hash = 2166136261u;
    for (const auto character : groupId)
    {
        hash ^= static_cast<std::uint8_t>(character);
        hash *= 16777619u;
    }
    return juce::Colour(palette[hash % palette.size()]);
}

inline void drawFocusRing(juce::Graphics& g,
                          juce::Rectangle<float> bounds,
                          const float cornerSize = controlRadius)
{
    g.setColour(surfaceRaised.withAlpha(0.96f));
    g.drawRoundedRectangle(bounds.expanded(1.0f), cornerSize + 1.0f, focusWidth + 1.0f);
    g.setColour(focus);
    g.drawRoundedRectangle(bounds, cornerSize, focusWidth);
}
} // namespace drs::app::authoring::visual
