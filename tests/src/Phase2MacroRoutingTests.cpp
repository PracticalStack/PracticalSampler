#include "drs/engine/AuthoringSession.h"
#include "drs/engine/CuratedDspCatalog.h"
#include "drs/engine/RuntimeLoader.h"
#include "plugin/PluginProcessor.h"
#include "standalone/MainComponent.h"

#include <juce_audio_processors_headless/juce_audio_processors_headless.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <regex>
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

std::string removeMacroExposureFields(std::string text)
{
    static const std::regex exposureField(
        R"(\s*"exposedInPerformance": (true|false),\r?\n)");
    return std::regex_replace(text, exposureField, "\n");
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
        require(session.getSelectedMacroIndex().has_value() && *session.getSelectedMacroIndex() == 0,
                "Session should recover a stable default macro selection.");
        require(session.moveMacro(0, 1, "Move macro later on the compact surface").applied,
                "Macro reorder should commit successfully.");
        require(session.getProject().authoring.macros[1].id == editedMacro.id,
                "Macro reorder did not persist the new authoring layout.");
        require(session.getSelectedMacroIndex().has_value() && session.getProject().authoring.macros[*session.getSelectedMacroIndex()].id == editedMacro.id,
                "Macro selection should follow the selected macro by stable ID after reordering.");

        drs::engine::RuntimeProjectMacroDefinition createdMacro;
        createdMacro.name = "Layer Blend";
        createdMacro.defaultValue = 0.40;
        createdMacro.minValue = 0.0;
        createdMacro.maxValue = 1.0;
        createdMacro.exposedInPerformance = true;
        const auto createMacroResult = session.createMacro(createdMacro, "Create exposed layer macro");
        require(createMacroResult.applied,
                "Macro creation should commit successfully.");
        require(session.getProject().authoring.macros.size() == 3,
                "Macro creation should append a new authored macro.");
        const auto createdMacroIndex = session.getSelectedMacroIndex();
        require(createdMacroIndex.has_value(),
                "Macro creation should select the new macro.");
        const auto createdMacroId = session.getProject().authoring.macros[*createdMacroIndex].id;
        require(!createdMacroId.empty()
                    && session.getProject().authoring.macros[*createdMacroIndex].name == "Layer Blend"
                    && session.getProject().authoring.macros[*createdMacroIndex].exposedInPerformance,
                "Created macro should preserve its authored name and exposure metadata.");

        const auto duplicateMacroResult = session.duplicateMacro(createdMacroId, "Duplicate exposed layer macro");
        require(duplicateMacroResult.applied,
                "Macro duplication should commit successfully.");
        require(session.getProject().authoring.macros.size() == 4,
                "Macro duplication should append a distinct authored macro.");
        const auto duplicatedMacroIndex = session.getSelectedMacroIndex();
        require(duplicatedMacroIndex.has_value(),
                "Macro duplication should select the duplicate.");
        const auto& duplicatedMacro = session.getProject().authoring.macros[*duplicatedMacroIndex];
        require(duplicatedMacro.id != createdMacroId
                    && duplicatedMacro.name.find("Layer Blend Copy") != std::string::npos
                    && duplicatedMacro.exposedInPerformance,
                "Macro duplication should generate a stable new ID while preserving useful authored metadata.");

        auto invalidMacro = duplicatedMacro;
        invalidMacro.defaultValue = 1.5;
        const auto invalidMacroResult = session.updateMacro(*duplicatedMacroIndex,
                                                            invalidMacro,
                                                            "Break macro range validation");
        require(!invalidMacroResult.applied,
                "Macro edits should reject defaults outside the declared range.");

        auto renamedIdMacro = duplicatedMacro;
        renamedIdMacro.id = "renamed-id";
        const auto renamedIdResult = session.updateMacro(*duplicatedMacroIndex,
                                                         renamedIdMacro,
                                                         "Attempt illegal macro ID change");
        require(!renamedIdResult.applied,
                "Macro IDs should remain immutable once created.");

        require(session.deleteMacro(duplicatedMacro.id, "Delete duplicate macro").applied,
                "Macro deletion should commit successfully.");
        require(session.getProject().authoring.macros.size() == 3,
                "Macro deletion should remove the duplicate authored macro.");
        require(session.getSelectedMacroIndex().has_value()
                    && session.getProject().authoring.macros[*session.getSelectedMacroIndex()].id == createdMacroId,
                "Macro deletion should keep a predictable nearby selection.");

        auto editedFxSlot = session.getProject().authoring.fxSlots[1];
        editedFxSlot.effectType = "drs.algorithmicReverb";
        editedFxSlot.effectVersion = 1;
        editedFxSlot.unavailable = false;
        editedFxSlot.legacyInert = false;
        editedFxSlot.parameters.clear();
        const auto* reverbDescriptor = drs::engine::findCuratedDspEffect(editedFxSlot.effectType,
                                                                          editedFxSlot.effectVersion);
        require(reverbDescriptor != nullptr, "Curated reverb must resolve for routing-edit coverage.");
        for (const auto& parameter : reverbDescriptor->parameters)
            editedFxSlot.parameters.push_back({ std::string(parameter.id), parameter.defaultValue });
        editedFxSlot.bypassed = false;
        require(session.updateFxSlot(1, editedFxSlot, "Swap shimmer slot to reverb").applied,
                "FX slot edit should commit successfully.");
        require(session.getProject().authoring.fxSlots[1].effectType == "drs.algorithmicReverb",
                "FX slot effect-type edit did not persist in the authoring session.");
        require(!session.getProject().authoring.fxSlots[1].bypassed,
                "FX slot bypass edit did not persist in the authoring session.");
        require(session.duplicateFxSlot("color-eq", "color-eq-routing-copy", "Duplicate EQ for routing edit").applied,
                "Routing coverage requires a distinct owner slot before assigning two insert chains.");
        require(session.moveFxSlotToBus("color-eq-routing-copy",
                                        session.getProject().authoring.routingBuses[1].id,
                                        "Move duplicated EQ into the pad routing chain").applied,
                "Routing coverage must transfer the copied slot before editing its owner chain.");

        auto editedBus = session.getProject().authoring.routingBuses[1];
        editedBus.inputSourceId = "pad-a3-high";
        editedBus.fxSlotIds = {"color-eq-routing-copy", "shimmer-delay"};
        const auto routingEdit = session.updateRoutingBus(1, editedBus, "Route pad bus through both curated FX slots");
        require(routingEdit.applied,
                "Routing-bus edit should commit successfully: "
                    + (routingEdit.issues.empty() ? std::string("no issue") : routingEdit.issues.front()));
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
        const auto savedCreatedMacro = std::find_if(roundTripLoad.project.authoring.macros.begin(),
                                                    roundTripLoad.project.authoring.macros.end(),
                                                    [&](const auto& macro) { return macro.id == createdMacroId; });
        require(savedCreatedMacro != roundTripLoad.project.authoring.macros.end()
                    && savedCreatedMacro->exposedInPerformance,
                "Saved projects must preserve macro exposure metadata by stable macro ID.");
        const auto savedReverb = std::find_if(roundTripLoad.project.authoring.fxSlots.begin(),
                                              roundTripLoad.project.authoring.fxSlots.end(),
                                              [](const auto& slot) { return slot.id == "shimmer-delay"; });
        require(savedReverb != roundTripLoad.project.authoring.fxSlots.end()
                    && savedReverb->effectType == "drs.algorithmicReverb",
                "Saved Sprint 5 project must preserve edited FX selections by stable slot ID.");
        require(roundTripLoad.project.authoring.routingBuses[1].inputSourceId == "pad-a3-high",
                "Saved Sprint 5 project must preserve edited routing inputs.");
        require(roundTripLoad.project.authoring.routingBuses[1].fxSlotIds.size() == 2,
                "Saved Sprint 5 project must preserve edited routing chains.");

        auto legacyProject = session.getProject();
        legacyProject.authoring.macros = {
            { "legacy-close", "Legacy Close", 0.5, 0.0, 1.0, {} },
            { "legacy-room", "Legacy Room", 0.3, 0.0, 1.0, {} }
        };
        const auto legacyProjectPath = tempDirectory / "phase2-macro-routing-legacy-macros.drsproj";
        writeTextFile(legacyProjectPath,
                      removeMacroExposureFields(
                          drs::engine::serializeRuntimeProjectManifest(
                              legacyProject,
                              legacyProjectPath.generic_string())));
        const auto legacyLoad = drs::engine::loadRuntimeProjectManifest(legacyProjectPath.generic_string());
        require(legacyLoad.loaded,
                "Legacy macro project without exposure flags should still load successfully.");
        require(legacyLoad.project.authoring.macros.size() == 2
                    && legacyLoad.project.authoring.macros[0].id == "legacy-close"
                    && legacyLoad.project.authoring.macros[1].id == "legacy-room"
                    && legacyLoad.project.authoring.macros[0].exposedInPerformance
                    && legacyLoad.project.authoring.macros[1].exposedInPerformance,
                "Legacy macros without explicit exposure flags should preserve authored order and remain visible in Perform.");

        auto zeroMacroProject = session.getProject();
        zeroMacroProject.authoring.macros.clear();
        const auto zeroMacroProjectPath = tempDirectory / "phase2-macro-routing-zero-macros.drsproj";
        writeTextFile(zeroMacroProjectPath,
                      drs::engine::serializeRuntimeProjectManifest(
                          zeroMacroProject,
                          zeroMacroProjectPath.generic_string()));
        const auto zeroMacroLoad = drs::engine::loadRuntimeProjectManifest(zeroMacroProjectPath.generic_string());
        require(zeroMacroLoad.loaded && zeroMacroLoad.project.authoring.macros.empty(),
                "Older or migrated projects with zero macros should remain valid and load without synthetic authored controls.");

        auto invalidProject = roundTripLoad.project;
        invalidProject.authoring.macros.back().defaultValue = 5.0;
        const auto invalidProjectValidation = drs::engine::validateRuntimeProjectModel(invalidProject);
        require(!invalidProjectValidation.valid
                    && std::any_of(invalidProjectValidation.issues.begin(),
                                   invalidProjectValidation.issues.end(),
                                   [](const std::string& issue)
                                   {
                                       return issue.find("defaultValue outside minValue/maxValue") != std::string::npos;
                                   }),
                "Project validation should reject macros whose defaultValue falls outside the declared range.");

        drs::plugin::Processor processor;
        std::unique_ptr<juce::AudioProcessorEditor> editor(processor.createEditor());
        require(editor != nullptr, "Plugin editor creation failed during Sprint 5 validation.");
        auto* pluginTabs = dynamic_cast<juce::TabbedComponent*>(findDescendantById(*editor, "workspaceTabs"));
        require(pluginTabs != nullptr, "Plugin shell should expose the workspace tabs during Sprint 5 validation.");
        pluginTabs->setCurrentTabIndex(1);
        require(findDescendantById(*editor, "authoringModeSelector") == nullptr,
                "Plugin authoring shell should retire the temporary mode selector after UI25-402.");
        require(findDescendantById(*editor, "authoringDrawerMacrosTab") != nullptr,
                "Plugin authoring shell should expose the macros drawer tab.");
        require(findDescendantById(*editor, "authoringDrawerRoutingTab") != nullptr,
                "Plugin authoring shell should expose the routing drawer tab.");
        require(findDescendantById(*editor, "authoringMacroList") != nullptr,
                "Plugin authoring shell should expose the UI25-404 macro repeated-structure list.");
        require(findDescendantById(*editor, "authoringFxSelector") != nullptr,
                "Plugin authoring shell should expose the Sprint 5 FX selector.");
        require(findDescendantById(*editor, "authoringRoutingSelector") != nullptr,
                "Plugin authoring shell should expose the Sprint 5 routing selector.");

        drs::standalone::MainComponent standalone(false);
        auto* standaloneTabs = dynamic_cast<juce::TabbedComponent*>(findDescendantById(standalone, "workspaceTabs"));
        require(standaloneTabs != nullptr,
                "Standalone shell should expose the workspace tabs during Sprint 5 validation.");
        standaloneTabs->setCurrentTabIndex(1);
        require(findDescendantById(standalone, "authoringModeSelector") == nullptr,
                "Standalone authoring shell should retire the temporary mode selector after UI25-402.");
        require(findDescendantById(standalone, "authoringDrawerMacrosTab") != nullptr,
                "Standalone authoring shell should expose the macros drawer tab.");
        require(findDescendantById(standalone, "authoringDrawerRoutingTab") != nullptr,
                "Standalone authoring shell should expose the routing drawer tab.");
        require(findDescendantById(standalone, "authoringMacroList") != nullptr,
                "Standalone authoring shell should expose the UI25-404 macro repeated-structure list.");
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
