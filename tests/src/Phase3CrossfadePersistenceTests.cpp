#include "drs/engine/AuthoringSession.h"
#include "drs/engine/RuntimeLoader.h"
#include "shared/ProjectStorage.h"

#include <filesystem>
#include <fstream>
#include <iostream>
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

bool containsIssueFragment(const std::vector<std::string>& issues, const std::string& fragment)
{
    for (const auto& issue : issues)
    {
        if (issue.find(fragment) != std::string::npos)
            return true;
    }

    return false;
}

void requireCrossfadeEquals(const drs::engine::VelocityCrossfadeDescriptor& crossfade,
                            int fadeInLowVelocity,
                            int fadeInHighVelocity,
                            int fadeOutLowVelocity,
                            int fadeOutHighVelocity,
                            const std::string& context)
{
    require(crossfade.fadeInLowVelocity == fadeInLowVelocity,
            context + " should preserve fadeInLowVelocity.");
    require(crossfade.fadeInHighVelocity == fadeInHighVelocity,
            context + " should preserve fadeInHighVelocity.");
    require(crossfade.fadeOutLowVelocity == fadeOutLowVelocity,
            context + " should preserve fadeOutLowVelocity.");
    require(crossfade.fadeOutHighVelocity == fadeOutHighVelocity,
            context + " should preserve fadeOutHighVelocity.");
}
} // namespace

int main()
{
    using namespace drs::engine;

    try
    {
        const auto phase2ProjectPath = fs::path(getPhase2ReferenceProjectManifestPath());
        const auto phase2Project = loadPhase2ReferenceProjectManifest();
        require(phase2Project.loaded, "Phase 2 reference project must load for crossfade persistence tests.");
        require(!phase2Project.project.authoring.zones.empty(),
                "Phase 2 reference project must provide at least one authored zone.");

        const auto legacyJson = serializeRuntimeProjectManifest(phase2Project.project,
                                                                phase2ProjectPath.generic_string());
        require(legacyJson.find("\"velocityCrossfade\"") == std::string::npos,
                "Legacy Phase 2 authoring fixtures should not serialize crossfade metadata when none is authored.");
        for (const auto& zone : phase2Project.project.authoring.zones)
        {
            require(!hasAnyVelocityCrossfadeValue(zone.velocityCrossfade),
                    "Legacy Phase 2 authoring fixtures should default absent crossfade fields to zero semantics.");
        }

        auto crossfadeProject = phase2Project.project;
        require(crossfadeProject.authoring.zones.size() >= 2,
                "Crossfade persistence tests require at least two authored zones.");

        auto& lowerZone = crossfadeProject.authoring.zones.at(0);
        auto& upperZone = crossfadeProject.authoring.zones.at(1);
        crossfadeProject.authoring.selectedZoneId = lowerZone.id;

        lowerZone.rootKey = 57;
        lowerZone.keyLow = 36;
        lowerZone.keyHigh = 59;
        lowerZone.velocityLow = 1;
        lowerZone.velocityHigh = 60;
        lowerZone.roundRobinLength = 1;
        lowerZone.roundRobinPosition = 1;
        lowerZone.velocityCrossfade = {};
        lowerZone.velocityCrossfade.fadeOutLowVelocity = 25;
        lowerZone.velocityCrossfade.fadeOutHighVelocity = 60;

        upperZone.rootKey = lowerZone.rootKey;
        upperZone.keyLow = lowerZone.keyLow;
        upperZone.keyHigh = lowerZone.keyHigh;
        upperZone.velocityLow = 25;
        upperZone.velocityHigh = 127;
        upperZone.roundRobinLength = 1;
        upperZone.roundRobinPosition = 1;
        upperZone.velocityCrossfade = {};
        upperZone.velocityCrossfade.fadeInLowVelocity = 25;
        upperZone.velocityCrossfade.fadeInHighVelocity = 60;

        const auto tempDirectory = fs::temp_directory_path() / "drs-phase3-crossfade-persistence-tests";
        const auto projectPath = tempDirectory / "crossfade-roundtrip.drsproj";
        const auto streamPath = tempDirectory / "crossfade-roundtrip.drstrm";
        const auto instrumentPath = tempDirectory / "crossfade-roundtrip.drinst";

        const auto serializedProject = serializeRuntimeProjectManifest(crossfadeProject,
                                                                       projectPath.generic_string());
        require(serializedProject.find("\"velocityCrossfade\"") != std::string::npos,
                "Project serialization should emit velocityCrossfade metadata once authored.");
        require(serializedProject.find("\"curve\": \"linear\"") != std::string::npos,
                "Project serialization should emit the supported linear crossfade curve.");
        writeTextFile(projectPath, serializedProject);

        const auto roundTripProject = loadRuntimeProjectManifest(projectPath.generic_string());
        require(roundTripProject.loaded, "Crossfade-authored project should survive save/load round-tripping.");
        requireCrossfadeEquals(roundTripProject.project.authoring.zones.front().velocityCrossfade,
                               0,
                               0,
                               25,
                               60,
                               "Round-tripped lower project zone crossfade metadata");
        requireCrossfadeEquals(roundTripProject.project.authoring.zones.at(1).velocityCrossfade,
                               25,
                               60,
                               0,
                               0,
                               "Round-tripped upper project zone crossfade metadata");

        writeTextFile(streamPath, "phase3 crossfade persistence stream placeholder");
        const auto instrument = drs::app::buildInstrumentManifestForProject(crossfadeProject,
                                                                            juce::File(projectPath.generic_string()));
        require(instrument.zones.size() == crossfadeProject.authoring.zones.size(),
                "Project-to-instrument conversion should preserve every authored zone.");
        requireCrossfadeEquals(instrument.zones.front().velocityCrossfade,
                               0,
                               0,
                               25,
                               60,
                               "Built lower instrument zone crossfade metadata");
        requireCrossfadeEquals(instrument.zones.at(1).velocityCrossfade,
                               25,
                               60,
                               0,
                               0,
                               "Built upper instrument zone crossfade metadata");

        const auto serializedInstrument = serializeRuntimeInstrumentManifest(instrument,
                                                                            instrumentPath.generic_string());
        require(serializedInstrument.find("\"velocityCrossfade\"") != std::string::npos,
                "Instrument serialization should emit velocityCrossfade metadata once authored.");
        writeTextFile(instrumentPath, serializedInstrument);

        const auto roundTripInstrument = loadRuntimeInstrumentManifest(instrumentPath.generic_string());
        require(roundTripInstrument.loaded,
                "Crossfade-authored instrument manifest should survive save/load round-tripping.");
        requireCrossfadeEquals(roundTripInstrument.instrument.zones.front().velocityCrossfade,
                               0,
                               0,
                               25,
                               60,
                               "Round-tripped lower instrument zone crossfade metadata");
        requireCrossfadeEquals(roundTripInstrument.instrument.zones.at(1).velocityCrossfade,
                               25,
                               60,
                               0,
                               0,
                               "Round-tripped upper instrument zone crossfade metadata");

        // Exercise the actual authoring transactions, including the bulk-stack path,
        // through the transactional project + companion-instrument save boundary.
        auto authoredStackProject = crossfadeProject;
        auto thirdLayer = authoredStackProject.authoring.zones.at(1);
        thirdLayer.id += "-stack-third";
        thirdLayer.displayName += " stack third";
        thirdLayer.velocityLow = 85;
        thirdLayer.velocityHigh = 127;
        thirdLayer.velocityCrossfade = {};
        authoredStackProject.authoring.zones.at(0).velocityLow = 1;
        authoredStackProject.authoring.zones.at(0).velocityHigh = 42;
        authoredStackProject.authoring.zones.at(0).velocityCrossfade = {};
        authoredStackProject.authoring.zones.at(0).roundRobin.reset();
        authoredStackProject.authoring.zones.at(0).roundRobinLength = 0;
        authoredStackProject.authoring.zones.at(0).roundRobinPosition = 0;
        authoredStackProject.authoring.zones.at(1).velocityLow = 43;
        authoredStackProject.authoring.zones.at(1).velocityHigh = 84;
        authoredStackProject.authoring.zones.at(1).velocityCrossfade = {};
        authoredStackProject.authoring.zones.at(1).roundRobin.reset();
        authoredStackProject.authoring.zones.at(1).roundRobinLength = 0;
        authoredStackProject.authoring.zones.at(1).roundRobinPosition = 0;
        thirdLayer.roundRobin.reset();
        thirdLayer.roundRobinLength = 0;
        thirdLayer.roundRobinPosition = 0;
        for (std::size_t index = 2; index < authoredStackProject.authoring.zones.size(); ++index)
            authoredStackProject.authoring.zones[index].rootKey += 12;
        authoredStackProject.authoring.zones.push_back(thirdLayer);

        AuthoringSession stackSession(authoredStackProject);
        const auto stackResult = stackSession.createVelocityCrossfadeStack(
            { { lowerZone.id, upperZone.id, thirdLayer.id }, 12 }, "Create persisted crossfade stack");
        require(stackResult.applied,
                "UI-equivalent stack authoring must commit before the persistence boundary: "
                    + (stackResult.issues.empty() ? std::string("no issue") : stackResult.issues.front()));
        const auto authoredProjectFile = juce::File((tempDirectory / "authored-stack.drsproj").generic_string());
        require(drs::app::saveProjectFiles(stackSession.getProject(), authoredProjectFile).saved,
                "A UI-authored crossfade stack must save its project and companion instrument together.");
        writeTextFile(tempDirectory / "authored-stack.drstrm", "authored stack stream placeholder");
        const auto savedStackProject = loadRuntimeProjectManifest(authoredProjectFile.getFullPathName().toStdString());
        const auto savedStackInstrument = loadRuntimeInstrumentManifest(
            authoredProjectFile.withFileExtension(".drinst").getFullPathName().toStdString());
        require(savedStackProject.loaded && savedStackInstrument.loaded,
                "A UI-authored crossfade stack must reload through both manifests: project="
                    + (savedStackProject.issues.empty() ? std::string("ok") : savedStackProject.issues.front())
                    + "; instrument="
                    + (savedStackInstrument.issues.empty() ? std::string("ok") : savedStackInstrument.issues.front()));
        require(hasCompleteFadeOut(savedStackProject.project.authoring.zones.at(0).velocityCrossfade)
                    && hasCompleteFadeIn(savedStackProject.project.authoring.zones.at(1).velocityCrossfade)
                    && hasCompleteFadeOut(savedStackProject.project.authoring.zones.at(1).velocityCrossfade)
                    && hasCompleteFadeIn(savedStackProject.project.authoring.zones.back().velocityCrossfade),
                "Reloaded project must retain every adjacent UI-authored stack relationship.");
        require(hasAnyVelocityCrossfadeValue(savedStackInstrument.instrument.zones.at(0).velocityCrossfade)
                    && hasAnyVelocityCrossfadeValue(savedStackInstrument.instrument.zones.at(1).velocityCrossfade),
                "Companion instrument generation must retain UI-authored crossfade metadata.");

        auto invalidProject = crossfadeProject;
        invalidProject.authoring.zones.front().velocityCrossfade.fadeOutLowVelocity =
            invalidProject.authoring.zones.front().velocityCrossfade.fadeOutHighVelocity;
        const auto invalidProjectPath = tempDirectory / "crossfade-invalid.drsproj";
        writeTextFile(invalidProjectPath,
                      serializeRuntimeProjectManifest(invalidProject, invalidProjectPath.generic_string()));

        const auto invalidProjectLoad = loadRuntimeProjectManifest(invalidProjectPath.generic_string());
        require(!invalidProjectLoad.loaded,
                "Project load should reject invalid persisted velocity crossfade metadata.");
        require(containsIssueFragment(invalidProjectLoad.issues, "velocityCrossfade fade-in"),
                "Project load should report a useful velocityCrossfade validation message.");

        AuthoringSession session(crossfadeProject);
        const auto selectedZone = session.getSelectedZone();
        require(selectedZone.has_value(),
                "Crossfade persistence tests require a selected zone before edit-validation coverage.");

        auto invalidZone = *selectedZone;
        invalidZone.velocityCrossfade.fadeOutLowVelocity = invalidZone.velocityCrossfade.fadeOutHighVelocity;
        const auto rejectedEdit = session.updateSelectedZone(invalidZone, "Introduce invalid crossfade metadata");
        require(!rejectedEdit.applied,
                "Authoring edits should reject invalid velocity crossfade spans before they are persisted.");
        require(containsIssueFragment(rejectedEdit.issues, "velocityCrossfade"),
                "Rejected authoring edits should report a velocityCrossfade validation issue.");
        requireCrossfadeEquals(session.getSelectedZone()->velocityCrossfade,
                               0,
                               0,
                               25,
                               60,
                               "Rejected authoring edit rollback");

        std::cout << "Phase 3 crossfade persistence tests passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Phase 3 crossfade persistence tests failed: "
                  << exception.what() << std::endl;
        return 1;
    }
}
