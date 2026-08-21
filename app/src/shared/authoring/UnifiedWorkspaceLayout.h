#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace drs::app::authoring
{
struct UnifiedWorkspaceLayoutInput
{
    juce::Rectangle<int> content;
    int browserWidth = 300;
    int inspectorWidth = 340;
    int gap = 12;
    bool mapVisible = true;
};

struct UnifiedWorkspaceLayoutResult
{
    juce::Rectangle<int> browser;
    juce::Rectangle<int> map;
    juce::Rectangle<int> inspector;
};

// Shared geometry policy for standalone/plugin shells. Hidden Map space is
// reclaimed by the inspector; no invisible placeholder is retained.
UnifiedWorkspaceLayoutResult calculateUnifiedWorkspaceLayout(const UnifiedWorkspaceLayoutInput& input);
} // namespace drs::app::authoring
