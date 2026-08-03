#include "drs/engine/AuthoringSession.h"
#include "drs/engine/RuntimeLoader.h"
#include "shared/AuthoringPanel.h"

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
    require(button != nullptr, "Expected articulation UI button is missing.");
    return *button;
}
} // namespace

int main()
{
    try
    {
        juce::ScopedJuceInitialiser_GUI gui;
        const auto loaded = drs::engine::loadPhase2ReferenceProjectManifest();
        require(loaded.loaded, "The reference authoring project must load for Sprint 10 UI coverage.");
        const auto curated = drs::engine::migrateRuntimeProjectToCuratedDspSchema(loaded.project);
        require(curated.valid, "The reference project must migrate through the current authoring schema.");
        const auto migrated = drs::engine::migrateRuntimeProjectToPerformanceArticulationSchema(curated.project);
        require(migrated.valid && migrated.project.authoring.articulations.size() >= 2,
                "The Sprint 10 UI fixture must provide explicit articulations.");

        drs::engine::AuthoringSession session(migrated.project);
        drs::app::AuthoringPanel panel(session, {}, {}, {}, drs::app::AuthoringPanel::LayoutMode::expanded);
        panel.setSize(1120, 800);
        panel.setVisible(true);
        panel.resized();

        requireButton(panel, "authoringDrawerArticulationsTab").onClick();
        require(findById(panel, "authoringArticulationList")->isVisible(),
                "Articulation workspace must be visible when its drawer tab is active.");
        require(findById(panel, "authoringArticulationKeySwitchPicker")->isVisible(),
                "The key-switch note picker must be visible in the articulation workspace.");
        require(findById(panel, "authoringZoneArticulationSelector") != nullptr,
                "Zone inspector must expose first-class articulation assignment.");

        requireButton(panel, "authoringArticulationMidiLearnButton").onClick();
        require(panel.applyLearnedKeySwitchMidiNote(12),
                "An active MIDI Learn request must accept a MIDI note from either shell.");
        const auto& learned = session.getProject().authoring.articulations.front();
        require(learned.activation.has_value() && learned.activation->midiNote == 12,
                "MIDI Learn must write the selected consuming key switch through the session transaction.");
        require(session.getDocumentState().dirty,
                "Key-switch authoring must mark the project dirty.");
        require(session.undo().applied,
                "Key-switch authoring must remain undoable.");
        requireButton(panel, "authoringArticulationKeyboardNote14").onClick();
        require(session.getProject().authoring.articulations.front().activation.has_value()
                    && session.getProject().authoring.articulations.front().activation->midiNote == 14,
                "The clickable key-switch keyboard must assign D0 through the same transaction path.");

        requireButton(panel, "authoringArticulationCreateButton").onClick();
        require(session.getProject().authoring.articulations.size() == migrated.project.authoring.articulations.size() + 1,
                "The Articulations workspace must create a first-class articulation without JSON editing.");

        std::cout << "Performance-engine Sprint 10 articulation UI tests passed.\n";
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Performance-engine Sprint 10 articulation UI tests failed: " << exception.what() << '\n';
        return 1;
    }
}
