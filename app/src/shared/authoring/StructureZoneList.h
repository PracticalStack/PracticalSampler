#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace drs::app::authoring
{
// Dedicated zone-column seam. JUCE ListBox provides recycled row components;
// StructureViewer supplies the stable-ID model and range renderer.
class StructureZoneList final : public juce::ListBox
{
public:
    StructureZoneList()
    {
        setTitle("Structure zones");
        setDescription("Virtualized zone rows with key, velocity, articulation, and diagnostic metadata.");
        setRowHeight(48);
        setMultipleSelectionEnabled(true);
    }
};
} // namespace drs::app::authoring
