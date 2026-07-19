#pragma once

#include "shared/authoring/CompactInspectorPrimitives.h"
#include "shared/authoring/AuthoringViewModels.h"

#include <juce_gui_basics/juce_gui_basics.h>

namespace drs::app::authoring
{
class ZoneMappingEditor final : public juce::Component
{
public:
    ZoneMappingEditor();

    void resized() override;
    void setViewModel(ZoneFieldValuesViewModel nextViewModel);
    void setCallbacks(ZoneFieldCallbacks nextCallbacks);

private:
    struct CommitValues
    {
        ZoneFieldValuesViewModel values;
        juce::String validationMessage;
    };

    CommitValues collectCurrentValues() const;
    void commitCurrentValues(const juce::String& label);
    void applyValuesToControls(const ZoneFieldValuesViewModel& values);
    void refreshValidationMessage(const juce::String& messageText);

    ZoneFieldValuesViewModel viewModel;
    ZoneFieldCallbacks callbacks;

    CompactInspectorMessage emptyStateMessage;
    CompactInspectorSection mapSection;
    CompactInspectorSection sampleSection;
    CompactInspectorSection mixSection;
    CompactInspectorSection advancedSection;

    juce::Component mapSectionContent;
    juce::Component sampleSectionContent;
    juce::Component mixSectionContent;
    juce::Component advancedSectionContent;

    CompactInspectorSliderRow rootKeyRow;
    CompactInspectorRangeRow keyRangeRow;
    CompactInspectorRangeRow velocityRangeRow;
    CompactInspectorSliderRow gainRow;
    CompactInspectorSliderRow panRow;
    CompactInspectorToggleRow loopToggleRow;
    CompactInspectorActionRow previewZoneRow;
    CompactInspectorActionRow restoreRootKeyRow;
    CompactInspectorMessage validationMessage;
};
} // namespace drs::app::authoring
