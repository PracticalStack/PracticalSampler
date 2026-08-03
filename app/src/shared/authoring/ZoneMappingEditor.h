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
    void updateSampleSectionContentHeight();
    void invokeCrossfadeAction(bool create);

    ZoneFieldValuesViewModel viewModel;
    ZoneFieldCallbacks callbacks;

    CompactInspectorMessage emptyStateMessage;
    CompactInspectorSection mapSection;
    CompactInspectorSection sampleSection;
    CompactInspectorSection crossfadeSection;
    CompactInspectorSection roundRobinSection;
    CompactInspectorSection mixSection;
    CompactInspectorSection advancedSection;
    CompactInspectorSection performanceSection;

    juce::Component mapSectionContent;
    juce::Component sampleSectionContent;
    juce::Component crossfadeSectionContent;
    juce::Component roundRobinSectionContent;
    juce::Component mixSectionContent;
    juce::Component advancedSectionContent;
    juce::Component performanceSectionContent;

    CompactInspectorSliderRow rootKeyRow;
    CompactInspectorRangeRow keyRangeRow;
    CompactInspectorComboRow articulationRow;
    CompactInspectorRangeRow velocityRangeRow;
    CompactInspectorMessage crossfadeFadeInMessage;
    CompactInspectorMessage crossfadeFadeOutMessage;
    CompactInspectorRangeRow crossfadeOverlapRow;
    CompactInspectorActionRow createCrossfadeRow;
    CompactInspectorActionRow updateCrossfadeRow;
    CompactInspectorActionRow removeCrossfadeRow;
    CompactInspectorMessage crossfadeGuidanceMessage;
    CompactInspectorMessage roundRobinPoolMessage;
    CompactInspectorMessage roundRobinSlotMessage;
    CompactInspectorMessage roundRobinHintMessage;
    CompactInspectorActionRow createRoundRobinPoolRow;
    CompactInspectorActionRow addCompatibleZonesRow;
    CompactInspectorActionRow normalizeRoundRobinPoolRow;
    CompactInspectorActionRow removeRoundRobinPoolRow;
    CompactInspectorSliderRow gainRow;
    CompactInspectorSliderRow panRow;
    CompactInspectorToggleRow loopToggleRow;
    CompactInspectorComboRow triggerModeRow;
    CompactInspectorComboRow performanceEventRow;
    CompactInspectorComboRow sustainConditionRow;
    CompactInspectorComboRow pitchSourceRow;
    CompactInspectorComboRow chokeGroupRow;
    CompactInspectorComboRow chokeTargetRow;
    CompactInspectorSliderRow chokeFadeRow;
    CompactInspectorActionRow createChokeGroupRow;
    CompactInspectorMessage performanceHintMessage;
    CompactInspectorActionRow previewZoneRow;
    CompactInspectorActionRow restoreRootKeyRow;
    CompactInspectorMessage validationMessage;
};
} // namespace drs::app::authoring
