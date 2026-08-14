#include "drs/engine/AuthoringSession.h"
#include "drs/engine/RuntimeLoader.h"
#include "plugin/PluginProcessor.h"
#include "shared/PerformanceBankImport.h"
#include "standalone/MainComponent.h"

#include <juce_audio_processors_headless/juce_audio_processors_headless.h>

#include <array>
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

void writeMidiPhraseFixture(const fs::path& path)
{
    fs::create_directories(path.parent_path());
    static constexpr std::array<unsigned char, 61> midiBytes
    {
        0x4d, 0x54, 0x68, 0x64, 0x00, 0x00, 0x00, 0x06,
        0x00, 0x00, 0x00, 0x01, 0x01, 0xe0,
        0x4d, 0x54, 0x72, 0x6b, 0x00, 0x00, 0x00, 0x27,
        0x00, 0x90, 0x3c, 0x60,
        0x00, 0x90, 0x40, 0x60,
        0x00, 0x90, 0x43, 0x64,
        0x83, 0x60, 0x80, 0x3c, 0x00,
        0x00, 0x80, 0x40, 0x00,
        0x00, 0x80, 0x43, 0x00,
        0x81, 0x70, 0x90, 0x48, 0x68,
        0x81, 0x70, 0x80, 0x48, 0x00,
        0x00, 0xff, 0x2f, 0x00
    };

    std::ofstream output(path, std::ios::binary);
    require(output.good(), "Temporary MIDI phrase fixture could not be created.");
    output.write(reinterpret_cast<const char*>(midiBytes.data()), static_cast<std::streamsize>(midiBytes.size()));
    require(output.good(), "Temporary MIDI phrase fixture could not be written.");
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
        require(projectLoad.loaded, "Phase 2 reference authoring project must load before Sprint 6 tests run.");
        require(projectLoad.project.authoring.performanceBanks.size() == 1,
                "Phase 2 reference project performance-bank count changed unexpectedly.");
        require(projectLoad.project.authoring.performanceBanks[0].phraseAssets.size() == 1,
                "Phase 2 reference project should ship one Sprint 6 phrase asset.");
        require(projectLoad.project.authoring.performanceBanks[0].triggerSlots[0].phraseAssetId == "writer-bank-c-major",
                "Phase 2 reference project phrase-trigger slot wiring changed unexpectedly.");

        const auto tempDirectory = fs::temp_directory_path() / "drs-phase2-performance-bank-tests";
        const auto midiPath = tempDirectory / "writer-bank-phrase.mid";
        writeMidiPhraseFixture(midiPath);

        const auto importResult = drs::app::importMidiPhraseAsset(midiPath.string(),
                                                                  "writer-bank-imported",
                                                                  "Writer Bank Imported");
        require(importResult.imported,
                "Sprint 6 MIDI phrase import should succeed. State="
                    + importResult.state
                    + (importResult.issues.empty() ? std::string{} : " issue=" + importResult.issues.front()));
        require(importResult.phraseAsset.notes.size() == 4, "Sprint 6 MIDI phrase fixture note count changed unexpectedly.");
        require(importResult.phraseAsset.lengthBeats == 2.0, "Sprint 6 MIDI phrase fixture beat length changed unexpectedly.");
        require(importResult.phraseAsset.chordHint == "C major", "Sprint 6 MIDI chord analysis changed unexpectedly.");

        drs::engine::AuthoringSession session(projectLoad.project);
        require(session.selectPerformanceBank("writer-bank").applied,
                "Selecting the reference performance bank should succeed.");
        auto editedBank = session.getSelectedPerformanceBank().value();
        editedBank.phraseAssets.push_back(importResult.phraseAsset);
        editedBank.triggerSlots[0].phraseAssetId = importResult.phraseAsset.id;
        editedBank.triggerSlots[0].chordMode = "preserve-intervals";
        editedBank.triggerSlots[0].targetArticulationId = "lead";
        require(session.updatePerformanceBank(0, editedBank, "Import sprint 6 phrase and retarget trigger slot").applied,
                "Performance-bank edit should commit successfully.");
        require(session.getSelectedPerformanceBank()->phraseAssets.size() == 2,
                "Imported phrase asset did not persist in the authoring session.");
        require(session.getSelectedPerformanceBank()->triggerSlots[0].phraseAssetId == "writer-bank-imported",
                "Trigger-slot phrase asset wiring did not persist in the authoring session.");
        require(session.getSelectedPerformanceBank()->triggerSlots[0].chordMode == "preserve-intervals",
                "Trigger-slot chord mode did not persist in the authoring session.");
        require(session.undo().applied,
                "Performance-bank edits should remain undoable after the UI25-402 workbench migration.");
        require(session.getSelectedPerformanceBank()->phraseAssets.size() == 1,
                "Undo should restore the pre-import performance-bank phrase library.");
        require(session.getSelectedPerformanceBank()->triggerSlots[0].phraseAssetId == "writer-bank-c-major",
                "Undo should restore the pre-import trigger-slot phrase wiring.");
        require(session.redo().applied,
                "Performance-bank edits should remain redoable after the UI25-402 workbench migration.");
        require(session.getSelectedPerformanceBank()->phraseAssets.size() == 2,
                "Redo should restore the imported performance-bank phrase library.");
        require(session.getSelectedPerformanceBank()->triggerSlots[0].phraseAssetId == "writer-bank-imported",
                "Redo should restore the imported trigger-slot phrase wiring.");

        const auto tempProjectPath = tempDirectory / "phase2-performance-bank-roundtrip.drsproj";
        writeTextFile(tempProjectPath,
                      drs::engine::serializeRuntimeProjectManifest(session.getProject(),
                                                                   tempProjectPath.generic_string()));

        const auto roundTripLoad = drs::engine::loadRuntimeProjectManifest(tempProjectPath.generic_string());
        require(roundTripLoad.loaded, "Saved Sprint 6 performance-bank project should load successfully.");
        require(roundTripLoad.project.authoring.performanceBanks[0].phraseAssets.size() == 2,
                "Saved Sprint 6 project must preserve imported phrase assets.");
        require(roundTripLoad.project.authoring.performanceBanks[0].triggerSlots[0].phraseAssetId == "writer-bank-imported",
                "Saved Sprint 6 project must preserve trigger-slot phrase wiring.");

        drs::plugin::Processor processor;
        std::unique_ptr<juce::AudioProcessorEditor> editor(processor.createEditor());
        require(editor != nullptr, "Plugin editor creation failed during Sprint 6 validation.");
        auto* pluginTabs = dynamic_cast<juce::TabbedComponent*>(findDescendantById(*editor, "workspaceTabs"));
        require(pluginTabs != nullptr, "Plugin shell should expose workspace tabs during Sprint 6 validation.");
        pluginTabs->setCurrentTabIndex(1);
        require(findDescendantById(*editor, "authoringModeSelector") == nullptr,
                "Plugin authoring shell should retire the temporary mode selector after UI25-402.");
        require(findDescendantById(*editor, "authoringWorkbenchPerformanceTab") != nullptr,
                "Plugin authoring shell should expose the performance workbench tab.");
        require(findDescendantById(*editor, "authoringPerformanceBankSelector") != nullptr,
                "Plugin authoring shell should expose the Sprint 6 performance-bank selector.");
        require(findDescendantById(*editor, "authoringTriggerSlotSelector") != nullptr,
                "Plugin authoring shell should expose the Sprint 6 trigger-slot selector.");
        require(findDescendantById(*editor, "authoringPhraseAssetSelector") != nullptr,
                "Plugin authoring shell should expose the Sprint 6 phrase selector.");
        require(findDescendantById(*editor, "authoringPhraseImportButton") != nullptr,
                "Plugin authoring shell should expose the Sprint 6 phrase import action.");

        drs::standalone::MainComponent standalone(false);
        auto* standaloneTabs = dynamic_cast<juce::TabbedComponent*>(findDescendantById(standalone, "workspaceTabs"));
        require(standaloneTabs != nullptr, "Standalone shell should expose workspace tabs during Sprint 6 validation.");
        standaloneTabs->setCurrentTabIndex(1);
        require(findDescendantById(standalone, "authoringModeSelector") == nullptr,
                "Standalone authoring shell should retire the temporary mode selector after UI25-402.");
        require(findDescendantById(standalone, "authoringWorkbenchPerformanceTab") != nullptr,
                "Standalone authoring shell should expose the performance workbench tab.");
        require(findDescendantById(standalone, "authoringPerformanceBankSelector") != nullptr,
                "Standalone authoring shell should expose the Sprint 6 performance-bank selector.");
        require(findDescendantById(standalone, "authoringTriggerSlotSelector") != nullptr,
                "Standalone authoring shell should expose the Sprint 6 trigger-slot selector.");
        require(findDescendantById(standalone, "authoringPhraseAssetSelector") != nullptr,
                "Standalone authoring shell should expose the Sprint 6 phrase selector.");
        require(findDescendantById(standalone, "authoringPhraseImportButton") != nullptr,
                "Standalone authoring shell should expose the Sprint 6 phrase import action.");

        std::cout << "Phase 2 performance bank tests passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Phase 2 performance bank tests failed: " << exception.what() << std::endl;
        return 1;
    }
}
