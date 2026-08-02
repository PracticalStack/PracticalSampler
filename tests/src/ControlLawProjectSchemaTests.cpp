#include "drs/engine/AuthoringSession.h"
#include "drs/engine/RuntimeLoader.h"

#include <iostream>
#include <stdexcept>
#include <string>

namespace
{
using namespace drs::engine;

void require(const bool condition, const std::string& message)
{
    if (!condition) throw std::runtime_error(message);
}

RuntimeProjectModel loadLegacyFixture()
{
    const auto loaded = loadRuntimeProjectManifest(DRS_CONTROL_LAW_S0_FIXTURE_PATH);
    require(loaded.loaded, "The control-law legacy fixture must load.");
    return loaded.project;
}

RuntimeProjectMacroDefinition makeNewGainMacro()
{
    RuntimeProjectMacroDefinition macro;
    macro.id = "new-group-gain";
    macro.name = "New Group Gain";
    macro.defaultValue = 0.5;
    macro.minValue = 0.0;
    macro.maxValue = 1.0;
    macro.exposedInPerformance = true;
    RuntimeProjectMacroTargetDefinition target;
    target.parameterId = "dsp.bell-gain-insert.gainDb";
    target.parameterPath = "groups/bell/gainDb";
    target.role = "mix";
    target.dspSlotId = "bell-gain-insert";
    target.dspParameterId = "gainDb";
    target.sourceMinimum = 0.0;
    target.sourceMaximum = 1.0;
    target.destinationMinimum = -96.0;
    target.destinationMaximum = 24.0;
    target.curve = "linear";
    macro.targets.push_back(std::move(target));
    return macro;
}

void verifyLegacyRoundTripAndNewDefaults()
{
    auto legacy = loadLegacyFixture();
    for (const auto& macro : legacy.authoring.macros)
    {
        require(macro.targets.front().controlLaw.id.empty()
                    && macro.targets.front().controlLaw.version == 0
                    && macro.targets.front().curve == "linear",
                "Existing linear macro targets must remain explicit legacy targets.");
    }
    const auto legacySerialized = serializeRuntimeProjectManifest(legacy, "legacy.drsproj");
    require(legacySerialized.find("\"controlLaw\"") == std::string::npos,
            "Saving an untouched legacy project must not silently add a control law.");

    AuthoringSession session(legacy);
    const auto created = session.createMacro(makeNewGainMacro(), "Create resolved group gain");
    require(created.applied, "A new group-gain macro must be accepted.");
    const auto macro = session.getSelectedMacro();
    require(macro.has_value() && macro->targets.size() == 1
                && macro->targets.front().controlLaw.id == "drs.mixerGain.v1"
                && macro->targets.front().controlLaw.version == 1
                && macro->targets.front().destinationMinimum == -96.0
                && macro->targets.front().destinationMaximum == 6.0,
            "New mix-role gain targets must persist the resolved mixer-gain v1 range.");

    const auto serialized = serializeRuntimeProjectManifest(session.getProject(), "resolved.drsproj");
    require(serialized.find("\"controlLaw\": {") != std::string::npos
                && serialized.find("\"id\": \"drs.mixerGain.v1\"") != std::string::npos,
            "New targets must serialize an explicit versioned control-law identity.");
    const auto parsed = parseRuntimeProjectManifest(serialized, "resolved.drsproj", false);
    require(parsed.loaded && parsed.project.authoring.macros.back().targets.front().controlLaw.id
                    == "drs.mixerGain.v1",
            "Versioned control-law identity must survive a project save/load round trip.");

    auto incompleteIdentity = serialized;
    const auto versionPosition = incompleteIdentity.find("\"version\": 1");
    require(versionPosition != std::string::npos, "The serialized control law must include a version.");
    incompleteIdentity.replace(versionPosition, std::string("\"version\": 1").size(), "\"version\": 0");
    require(!parseRuntimeProjectManifest(incompleteIdentity, "incomplete-law.drsproj", false).loaded,
            "A present control-law object requires a non-zero version.");

    auto malformedIdentity = serialized;
    const auto lawStart = malformedIdentity.find("\"controlLaw\": {");
    require(lawStart != std::string::npos, "The serialized project must contain a control-law object.");
    const auto lawEnd = malformedIdentity.find('}', lawStart);
    require(lawEnd != std::string::npos, "The serialized control-law object must close.");
    malformedIdentity.replace(lawStart, lawEnd - lawStart + 1, "\"controlLaw\": 1");
    require(!parseRuntimeProjectManifest(malformedIdentity, "malformed-law.drsproj", false).loaded,
            "A malformed non-object control law must reject project load rather than guessing a curve.");

    auto futureIdentity = serialized;
    futureIdentity.replace(versionPosition, std::string("\"version\": 1").size(), "\"version\": 2");
    const auto futureParsed = parseRuntimeProjectManifest(futureIdentity, "future-law.drsproj", false);
    require(futureParsed.loaded && futureParsed.project.authoring.macros.back().targets.front().controlLaw.version == 2,
            "Future law versions must be retained by storage for later publication rejection.");
}

void verifyExplicitMigration()
{
    AuthoringSession session(loadLegacyFixture());
    const auto preview = session.previewMixerTaperUpgrade();
    require(preview.affectedMacroIds.size() == 3 && preview.affectedTargetPaths.size() == 3,
            "The upgrade preview must enumerate all and only the three legacy group gain targets.");
    require(!session.getDocumentState().dirty,
            "Previewing the mixer taper upgrade must not modify the project.");

    const auto upgrade = session.upgradeMixerTaper("Upgrade mixer taper");
    require(upgrade.applied && session.getDocumentState().dirty,
            "The explicit upgrade must be a dirty, undoable authoring transaction.");
    for (const auto& macro : session.getProject().authoring.macros)
    {
        const auto& target = macro.targets.front();
        require(target.controlLaw.id == "drs.mixerGain.v1" && target.controlLaw.version == 1
                    && target.destinationMinimum == -96.0 && target.destinationMaximum == 6.0,
                "The upgrade must change each eligible target to the approved mixer law.");
    }

    require(session.undo().applied && !session.getDocumentState().dirty,
            "Undo must restore the untouched legacy project state.");
    for (const auto& macro : session.getProject().authoring.macros)
        require(macro.targets.front().controlLaw.id.empty()
                    && macro.targets.front().destinationMaximum == 24.0,
                "Undo must restore legacy linear response exactly.");
    require(session.redo().applied
                && session.getProject().authoring.macros.front().targets.front().controlLaw.id
                    == "drs.mixerGain.v1",
            "Redo must restore the explicit mixer taper upgrade.");
}
} // namespace

int main()
{
    try
    {
        verifyLegacyRoundTripAndNewDefaults();
        verifyExplicitMigration();
        std::cout << "Control-law project schema tests passed.\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "Control-law project schema test failure: " << error.what() << '\n';
        return 1;
    }
}
