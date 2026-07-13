#include "drs/engine/AuthoringSession.h"
#include "drs/engine/RuntimeLoader.h"
#include "plugin/PluginProcessor.h"
#include "standalone/MainComponent.h"

#include <juce_audio_processors_headless/juce_audio_processors_headless.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

namespace
{
namespace fs = std::filesystem;

void require(bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

void writeTextFile(const fs::path& path, const std::string& text)
{
    fs::create_directories(path.parent_path());
    std::ofstream output(path, std::ios::binary);
    output << text;
}

juce::Component* findDescendantById(juce::Component& root, const juce::String& componentId)
{
    if (root.getComponentID() == componentId)
        return &root;

    for (int index = 0; index < root.getNumChildComponents(); ++index)
    {
        if (auto* match = findDescendantById(*root.getChildComponent(index), componentId))
            return match;
    }

    return nullptr;
}
} // namespace

int main()
{
    try
    {
        juce::ScopedJuceInitialiser_GUI gui;

        const auto projectLoad = drs::engine::loadPhase2ReferenceProjectManifest();
        require(projectLoad.loaded, "Phase 2 reference authoring project must load before Sprint 5 tests run.");

        drs::engine::AuthoringSession session(projectLoad.project);
        require(session.getProject().authoring.macros.size() == 2,
                "Phase 2 reference project macro count changed unexpectedly.");
        require(session.getProject().authoring.fxSlots.size() == 2,
                "Phase 2 reference project FX slot count changed unexpectedly.");
        require(session.getProject().authoring.routingBuses.size() == 2,
                "Phase 2 reference project routing-bus count changed unexpectedly.");

        auto editedMacro = session.getProject().authoring.macros.front();
        editedMacro.defaultValue = 0.62;
        editedMacro.minValue = 0.10;
        editedMacro.maxValue = 0.90;
        editedMacro.targets.front().parameterId = "zone-pan";
        editedMacro.targets.front().parameterPath = "authoring.zone.pan";
        editedMacro.targets.front().role = "placement";
        require(session.updateMacro(0, editedMacro, "Retarget macro for placement").applied,
                "Macro edit should commit successfully.");
        require(session.getProject().authoring.macros.front().targets.front().parameterId == "zone-pan",
                "Macro parameter assignment did not persist in the authoring session.");
        require(session.moveMacro(0, 1, "Move macro later on the compact surface").applied,
                "Macro reorder should commit successfully.");
        require(session.getProject().authoring.macros[1].id == editedMacro.id,
                "Macro reorder did not persist the new authoring layout.");

        auto editedFxSlot = session.getProject().authoring.fxSlots[1];
        editedFxSlot.effectType = "reverb";
        editedFxSlot.bypassed = false;
        require(session.updateFxSlot(1, editedFxSlot, "Swap shimmer slot to reverb").applied,
                "FX slot edit should commit successfully.");
        require(session.getProject().authoring.fxSlots[1].effectType == "reverb",
                "FX slot effect-type edit did not persist in the authoring session.");
        require(!session.getProject().authoring.fxSlots[1].bypassed,
                "FX slot bypass edit did not persist in the authoring session.");

        auto editedBus = session.getProject().authoring.routingBuses[1];
        editedBus.inputSourceId = "pad-a3-high";
        editedBus.fxSlotIds = {"color-eq", "shimmer-delay"};
        require(session.updateRoutingBus(1, editedBus, "Route pad bus through both curated FX slots").applied,
                "Routing-bus edit should commit successfully.");
        require(session.getProject().authoring.routingBuses[1].inputSourceId == "pad-a3-high",
                "Routing input-source edit did not persist in the authoring session.");
        require(session.getProject().authoring.routingBuses[1].fxSlotIds.size() == 2,
                "Routing insert-chain edit did not persist in the authoring session.");

        const auto tempDirectory = fs::temp_directory_path() / "drs-phase2-macro-routing-tests";
        const auto tempProjectPath = tempDirectory / "phase2-macro-routing-roundtrip.drsproj";
        writeTextFile(tempProjectPath,
                      drs::engine::serializeRuntimeProjectManifest(session.getProject(),
                                                                   tempProjectPath.generic_string()));

        const auto roundTripLoad = drs::engine::loadRuntimeProjectManifest(tempProjectPath.generic_string());
        require(roundTripLoad.loaded, "Saved Phase 2 Sprint 5 project should load successfully.");
        require(roundTripLoad.project.authoring.macros[1].targets.front().parameterId == "zone-pan",
                "Saved Sprint 5 project must preserve edited macro assignments.");
        require(roundTripLoad.project.authoring.fxSlots[1].effectType == "reverb",
                "Saved Sprint 5 project must preserve edited FX selections.");
        require(roundTripLoad.project.authoring.routingBuses[1].inputSourceId == "pad-a3-high",
                "Saved Sprint 5 project must preserve edited routing inputs.");
        require(roundTripLoad.project.authoring.routingBuses[1].fxSlotIds.size() == 2,
                "Saved Sprint 5 project must preserve edited routing chains.");

        drs::plugin::Processor processor;
        std::unique_ptr<juce::AudioProcessorEditor> editor(processor.createEditor());
        require(editor != nullptr, "Plugin editor creation failed during Sprint 5 validation.");
        auto* pluginTabs = dynamic_cast<juce::TabbedComponent*>(findDescendantById(*editor, "workspaceTabs"));
        require(pluginTabs != nullptr, "Plugin shell should expose the workspace tabs during Sprint 5 validation.");
        pluginTabs->setCurrentTabIndex(1);
        require(findDescendantById(*editor, "authoringModeSelector") != nullptr,
                "Plugin authoring shell should expose the Sprint 5 mode selector.");
        require(findDescendantById(*editor, "authoringMacroSelector") != nullptr,
                "Plugin authoring shell should expose the Sprint 5 macro selector.");
        require(findDescendantById(*editor, "authoringFxSelector") != nullptr,
                "Plugin authoring shell should expose the Sprint 5 FX selector.");
        require(findDescendantById(*editor, "authoringRoutingSelector") != nullptr,
                "Plugin authoring shell should expose the Sprint 5 routing selector.");

        drs::standalone::MainComponent standalone(false);
        auto* standaloneTabs = dynamic_cast<juce::TabbedComponent*>(findDescendantById(standalone, "workspaceTabs"));
        require(standaloneTabs != nullptr,
                "Standalone shell should expose the workspace tabs during Sprint 5 validation.");
        standaloneTabs->setCurrentTabIndex(1);
        require(findDescendantById(standalone, "authoringModeSelector") != nullptr,
                "Standalone authoring shell should expose the Sprint 5 mode selector.");
        require(findDescendantById(standalone, "authoringMacroSelector") != nullptr,
                "Standalone authoring shell should expose the Sprint 5 macro selector.");
        require(findDescendantById(standalone, "authoringFxSelector") != nullptr,
                "Standalone authoring shell should expose the Sprint 5 FX selector.");
        require(findDescendantById(standalone, "authoringRoutingSelector") != nullptr,
                "Standalone authoring shell should expose the Sprint 5 routing selector.");

        std::cout << "Phase 2 macro routing tests passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Phase 2 macro routing tests failed: " << exception.what() << std::endl;
        return 1;
    }
}
