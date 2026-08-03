#include "shared/authoring/ZoneMappingEditor.h"

#include <iostream>
#include <stdexcept>

namespace
{
void require(const bool condition, const char* message)
{
    if (!condition)
        throw std::runtime_error(message);
}

juce::Component* findById(juce::Component& root, const juce::String& componentId)
{
    if (root.getComponentID() == componentId)
        return &root;
    for (int index = 0; index < root.getNumChildComponents(); ++index)
        if (auto* found = findById(*root.getChildComponent(index), componentId))
            return found;
    return nullptr;
}

juce::Button& requireButton(juce::Component& root, const juce::String& componentId)
{
    auto* button = dynamic_cast<juce::Button*>(findById(root, componentId));
    require(button != nullptr, "Expected crossfade inspector button is missing.");
    return *button;
}

juce::Slider& requireSlider(juce::Component& root, const juce::String& componentId)
{
    auto* slider = dynamic_cast<juce::Slider*>(findById(root, componentId));
    require(slider != nullptr, "Expected crossfade inspector slider is missing.");
    return *slider;
}
} // namespace

int main()
{
    try
    {
        juce::ScopedJuceInitialiser_GUI gui;
        drs::app::authoring::ZoneMappingEditor editor;
        editor.setSize(420, 720);
        editor.setVisible(true);

        std::string requestedLower;
        std::string requestedUpper;
        int requestedLow = 0;
        int requestedHigh = 0;
        int createCount = 0;
        int updateCount = 0;
        int removeCount = 0;
        int stackCreateCount = 0;
        int stackRemoveCount = 0;
        int requestedStackWidth = 0;
        std::vector<std::string> requestedStackIds;
        drs::app::authoring::ZoneFieldCallbacks callbacks;
        callbacks.onCreateVelocityCrossfadeRequested = [&](const std::string& lower,
                                                            const std::string& upper,
                                                            const int low,
                                                            const int high)
        {
            ++createCount;
            requestedLower = lower;
            requestedUpper = upper;
            requestedLow = low;
            requestedHigh = high;
        };
        callbacks.onUpdateVelocityCrossfadeRequested = [&](const std::string& lower,
                                                            const std::string& upper,
                                                            const int low,
                                                            const int high)
        {
            ++updateCount;
            requestedLower = lower;
            requestedUpper = upper;
            requestedLow = low;
            requestedHigh = high;
        };
        callbacks.onRemoveVelocityCrossfadeRequested = [&](const std::string& lower, const std::string& upper)
        {
            ++removeCount;
            requestedLower = lower;
            requestedUpper = upper;
        };
        callbacks.onCreateVelocityCrossfadeStackRequested = [&](const std::vector<std::string>& ids, const int width)
        {
            ++stackCreateCount;
            requestedStackIds = ids;
            requestedStackWidth = width;
        };
        callbacks.onRemoveVelocityCrossfadeStackRequested = [&](const std::vector<std::string>& ids)
        {
            ++stackRemoveCount;
            requestedStackIds = ids;
        };
        editor.setCallbacks(std::move(callbacks));

        drs::app::authoring::ZoneFieldValuesViewModel values;
        values.hasSelection = true;
        values.crossfadeCanCreate = true;
        values.crossfadeOverlapLow = 55;
        values.crossfadeOverlapHigh = 72;
        values.crossfadeLowerZoneId = "lower";
        values.crossfadeUpperZoneId = "upper";
        values.crossfadeFadeInText = "Fade In: none";
        values.crossfadeFadeOutText = "Fade Out: none";
        values.crossfadeGuidanceText = "Create a Linear 55-72 overlap for the two selected layers.";
        editor.setViewModel(values);
        editor.resized();

        requireButton(editor, "authoringSampleInspectorSectionDisclosure").onClick();
        requireButton(editor, "authoringVelocityCrossfadeInspectorSectionDisclosure").onClick();
        const auto* section = findById(editor, "authoringVelocityCrossfadeInspectorSection");
        require(section != nullptr && !section->getBounds().isEmpty() && section->isVisible(),
                "Crossfade inspector subsection should expand beneath Sample.");
        require(!requireSlider(editor, "authoringCrossfadeOverlapLowSlider").getBounds().isEmpty(),
                "Crossfade overlap controls should receive a concrete layout when expanded.");
        require(requireButton(editor, "authoringCreateCrossfadeButton").isEnabled(),
                "An eligible two-zone selection should enable Create Crossfade.");

        requireButton(editor, "authoringCreateCrossfadeButton").onClick();
        require(createCount == 1 && requestedLower == "lower" && requestedUpper == "upper"
                    && requestedLow == 55 && requestedHigh == 72,
                "Create Crossfade should forward exact selected pair IDs and numeric overlap.");

        values.crossfadeCanCreate = false;
        values.crossfadeCanEdit = true;
        values.crossfadeCanRemove = true;
        values.crossfadeHasFadeIn = true;
        values.crossfadeFadeInText = "Fade In: Linear 55-72 with Lower layer";
        values.crossfadeGuidanceText = "Edit or remove the complete Fade In relationship with its lower layer.";
        editor.setViewModel(values);
        editor.resized();
        require(requireButton(editor, "authoringUpdateCrossfadeButton").isEnabled()
                    && requireButton(editor, "authoringRemoveCrossfadeButton").isEnabled(),
                "An existing relationship should expose Apply Overlap and Remove Crossfade.");

        requireSlider(editor, "authoringCrossfadeOverlapLowSlider").setValue(60, juce::dontSendNotification);
        requireSlider(editor, "authoringCrossfadeOverlapHighSlider").setValue(80, juce::dontSendNotification);
        requireButton(editor, "authoringUpdateCrossfadeButton").onClick();
        require(updateCount == 1 && requestedLow == 60 && requestedHigh == 80,
                "Apply Overlap should commit only the explicit numeric edit action.");
        requireButton(editor, "authoringRemoveCrossfadeButton").onClick();
        require(removeCount == 1 && requestedLower == "lower" && requestedUpper == "upper",
                "Remove Crossfade should target the complete relationship rather than one descriptor side.");

        values.crossfadeCanEdit = false;
        values.crossfadeCanRemove = false;
        values.crossfadeCanCreateStack = true;
        values.crossfadeCanRemoveStack = true;
        values.crossfadeStackZoneIds = { "layer-1", "layer-2", "layer-3" };
        values.crossfadeOverlapLow = 48;
        values.crossfadeOverlapHigh = 64;
        values.crossfadeGuidanceText = "Preview: 3 layers, 2 adjacent overlaps: 48-64, 80-96.";
        editor.setViewModel(values);
        editor.resized();
        require(requireButton(editor, "authoringCreateCrossfadeStackButton").isEnabled(),
                "A valid layer stack should expose Create Stack Crossfades.");
        require(requireButton(editor, "authoringRemoveCrossfadeStackButton").isEnabled(),
                "A valid layer stack should expose Remove Stack Crossfades.");
        requireButton(editor, "authoringCreateCrossfadeStackButton").onClick();
        require(stackCreateCount == 1 && requestedStackIds.size() == 3 && requestedStackWidth == 16,
                "Create Stack Crossfades should forward every selected layer and the requested width.");
        requireButton(editor, "authoringRemoveCrossfadeStackButton").onClick();
        require(stackRemoveCount == 1 && requestedStackIds.size() == 3,
                "Remove Stack Crossfades should target the complete selected stack.");

        std::cout << "Velocity crossfade inspector UI tests passed.\n";
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Velocity crossfade inspector UI tests failed: " << exception.what() << '\n';
        return 1;
    }
}
