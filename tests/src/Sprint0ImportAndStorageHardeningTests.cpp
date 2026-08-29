#include "drs/engine/SfzImportProjection.h"
#include "drs/engine/SfzImportReport.h"
#include "drs/engine/RuntimeLoader.h"
#include "shared/ProjectStorage.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

namespace
{
namespace fs = std::filesystem;
const std::string legacyIdentityNote = "Created in Decent Rhapsody Studio before the presentation rename.";
const std::string currentInstrumentProvenance =
    "Generated from the current Practical Sampler authoring project when the project was saved.";

void require(const bool condition, const std::string& message)
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

bool hasFinding(const std::vector<drs::engine::SfzImportFinding>& findings,
                const std::string& code)
{
    return std::any_of(findings.begin(), findings.end(), [&code](const auto& finding)
    {
        return finding.code == code;
    });
}

fs::path resolveSmDrumsPath()
{
    const auto relativePath = fs::path(
        "DemoSFVInstruments/SMDrums_Sforzando_1.2/Programs/SM_Drums_kit.sfz");
    const auto sourceRoot = fs::path(DRS_SOURCE_ROOT);
    for (const auto& candidate : { sourceRoot / relativePath, sourceRoot.parent_path() / relativePath })
    {
        if (fs::exists(candidate))
            return candidate;
    }

    throw std::runtime_error("Could not locate the permanent SM Drums regression corpus.");
}

drs::engine::RuntimeProjectModel makeStorageProject(const juce::File& projectFile,
                                                     const std::string& displayName)
{
    drs::engine::RuntimeProjectModel project;
    project.schemaName = "drs.project";
    project.schemaVersion = 2;
    project.projectId = "sprint0-storage";
    project.displayName = displayName;
    project.contentRootPath = projectFile.getParentDirectory().getFullPathName().toStdString();
    project.defaultInstrumentManifestPath = projectFile.withFileExtension(".drinst")
                                                .getFullPathName().toStdString();
    project.authoring.schemaName = "drs.authoring";
    project.authoring.schemaVersion = 1;
    project.notes = { legacyIdentityNote };
    return project;
}

void verifyMacrosWhitespaceAndRootRelativeSamples(const fs::path& testRoot)
{
    const auto programPath = testRoot / "Macro Package/Programs/Macro Kit.sfz";
    const auto mappingPath = testRoot / "Macro Package/Programs/mappings/kick.sfz";
    const auto samplePath = testRoot / "Macro Package/Samples/Kick Drum.wav";
    writeTextFile(samplePath, {});
    writeTextFile(programPath,
                  "#define $KEY 36\n"
                  "#define $VOLUME 70\n"
                  "<control> label_cc$VOLUME=Kick Volume\n"
                  "<global> loop_mode=one_shot\n"
                  "#include \"mappings/kick.sfz\"\n");
    writeTextFile(mappingPath,
                  "<group> key=$KEY amplitude_oncc$VOLUME=100\n"
                  "<region> sample=..\\Samples\\Kick Drum.wav lovel=1 hivel=127\n");

    const auto analysis = drs::engine::analyzeSfzImportDocument(programPath.generic_string());
    require(analysis.parseResult.parsed && analysis.normalizeResult.normalized,
            "Macro fixture should parse and normalize.");
    require(!hasFinding(analysis.report.findings, "sfz.sample.missing"),
            "An included mapping must resolve sample paths from the root SFZ directory.");

    const auto control = std::find_if(analysis.parseResult.document.sections.begin(),
                                      analysis.parseResult.document.sections.end(),
                                      [](const auto& section)
                                      {
                                          return section.scope == drs::engine::SfzOpcodeScope::control;
                                      });
    require(control != analysis.parseResult.document.sections.end()
                && control->opcodes.front().name == "label_cc70"
                && control->opcodes.front().value == "Kick Volume",
            "#define substitution and whitespace-safe control labels must be preserved.");

    const auto group = std::find_if(analysis.parseResult.document.sections.begin(),
                                    analysis.parseResult.document.sections.end(),
                                    [](const auto& section)
                                    {
                                        return section.scope == drs::engine::SfzOpcodeScope::group;
                                    });
    require(group != analysis.parseResult.document.sections.end()
                && group->opcodes.size() == 2
                && group->opcodes[0].name == "key"
                && group->opcodes[0].value == "36"
                && group->opcodes[1].name == "amplitude_oncc70",
            "#define substitution must apply to opcode values and opcode names in includes.");

    const auto region = std::find_if(analysis.parseResult.document.sections.begin(),
                                     analysis.parseResult.document.sections.end(),
                                     [](const auto& section)
                                     {
                                         return section.scope == drs::engine::SfzOpcodeScope::region;
                                     });
    require(region != analysis.parseResult.document.sections.end()
                && region->opcodes.front().value == "..\\Samples\\Kick Drum.wav"
                && fs::path(region->opcodes.front().resolutionBasePath)
                       == programPath.parent_path().lexically_normal(),
            "Unquoted sample values with spaces and their root resolution base must survive parsing.");
}

void verifySfzBudgets(const fs::path& testRoot)
{
    using namespace drs::engine;
    const auto budgetRoot = testRoot / "budgets";

    const auto bytesFile = budgetRoot / "bytes.sfz";
    writeTextFile(bytesFile, "<region> sample=sample.wav\n");
    auto context = defaultSfzImportExecutionContext();
    context.budgets.maximumTotalSourceBytes = 4;
    auto result = parseSfzDocument(bytesFile.generic_string(), context);
    require(result.execution.failureReason == SfzImportFailureReason::budgetExceeded
                && hasFinding(result.findings, "budget.source_bytes_exceeded"),
            "The expanded SFZ byte budget must block oversized input.");

    const auto includeRoot = budgetRoot / "includes.sfz";
    writeTextFile(includeRoot, "#include \"included.sfz\"\n");
    writeTextFile(budgetRoot / "included.sfz", "<region> sample=sample.wav\n");
    context = defaultSfzImportExecutionContext();
    context.budgets.maximumIncludeCount = 0;
    result = parseSfzDocument(includeRoot.generic_string(), context);
    require(hasFinding(result.findings, "budget.include_count_exceeded"),
            "The include expansion budget must be enforced.");

    const auto sectionFile = budgetRoot / "sections.sfz";
    writeTextFile(sectionFile, "<group> key=36\n<region> sample=sample.wav\n");
    context = defaultSfzImportExecutionContext();
    context.budgets.maximumSectionCount = 1;
    result = parseSfzDocument(sectionFile.generic_string(), context);
    require(hasFinding(result.findings, "budget.section_count_exceeded"),
            "The section budget must be enforced.");

    const auto regionFile = budgetRoot / "regions.sfz";
    writeTextFile(regionFile,
                  "<region> sample=one.wav\n"
                  "<region> sample=two.wav\n");
    context = defaultSfzImportExecutionContext();
    context.budgets.maximumRegionCount = 1;
    result = parseSfzDocument(regionFile.generic_string(), context);
    require(hasFinding(result.findings, "budget.region_count_exceeded"),
            "The region budget must be enforced.");

    const auto findingFile = budgetRoot / "findings.sfz";
    writeTextFile(findingFile, "bad-token\nbad-token\nbad-token\nbad-token\n");
    context = defaultSfzImportExecutionContext();
    context.budgets.maximumFindingCount = 2;
    result = parseSfzDocument(findingFile.generic_string(), context);
    require(result.findings.size() == 2 && result.suppressedFindingCount == 2,
            "Findings must be capped while retaining an omitted-finding count.");
}

void verifySmDrumsCorpus()
{
    using namespace drs::engine;
    const auto smDrumsPath = resolveSmDrumsPath();
    const auto analysis = analyzeSfzImportDocument(smDrumsPath.generic_string());
    require(analysis.parseResult.parsed && analysis.parseResult.complete,
            "SM_Drums_kit.sfz must parse completely without freezing.");
    const auto kickBus = parseSfzDocument((smDrumsPath.parent_path() / "Kick_bus.sfz").generic_string());
    require(kickBus.parsed && kickBus.complete,
            "Kick_bus.sfz must accept SM Drums #define key and CC substitutions.");
    require(analysis.parseResult.document.sourceFiles.size() == 31,
            "SM Drums should expand the root plus its 30 includes.");
    require(std::count_if(analysis.parseResult.document.sections.begin(),
                          analysis.parseResult.document.sections.end(),
                          [](const auto& section)
                          {
                              return section.scope == SfzOpcodeScope::region;
                          }) == 3358,
            "SM Drums region count changed unexpectedly.");
    const auto maximumFindingCount
        = defaultSfzImportExecutionContext().budgets.maximumFindingCount;
    require(analysis.report.findings.size() <= maximumFindingCount
                && (analysis.report.summary.suppressedFindingCount == 0
                    || analysis.report.findings.size() == maximumFindingCount),
            "SM Drums diagnostics must respect the cap and retain any omitted-finding count.");
    require(!hasFinding(analysis.report.findings, "sfz.sample.missing"),
            "SM Drums samples must resolve relative to the root SFZ, not included mapping files.");
}

void verifyRecoverableProjectPairSave()
{
    const auto testRoot = juce::File::getSpecialLocation(juce::File::tempDirectory)
                              .getNonexistentChildFile("drs-sprint0-storage", {}, false);
    const auto projectFile = testRoot.getChildFile("Transaction.drsproj");
    const auto instrumentFile = projectFile.withFileExtension(".drinst");
    const auto original = makeStorageProject(projectFile, "Original Generation");
    require(drs::app::saveProjectFiles(original, projectFile).saved,
            "The initial project pair should save.");
    const auto originalProjectText = projectFile.loadFileAsString();
    const auto originalInstrumentText = instrumentFile.loadFileAsString();
    const auto legacyProjectLoad = drs::engine::loadRuntimeProjectManifest(
        projectFile.getFullPathName().toStdString());
    require(legacyProjectLoad.loaded
                && legacyProjectLoad.project.schemaName == "drs.project"
                && legacyProjectLoad.project.schemaVersion == 2
                && legacyProjectLoad.project.notes == std::vector<std::string> { legacyIdentityNote }
                && drs::engine::serializeRuntimeProjectManifest(
                       legacyProjectLoad.project, projectFile.getFullPathName().toStdString())
                       .find(legacyIdentityNote) != std::string::npos,
            "A project note containing the former presentation name must load and reserialize unchanged.");

    const auto generatedInstrument = drs::app::buildInstrumentManifestForProject(original, projectFile);
    require(generatedInstrument.schemaName == "drs.instrument"
                && generatedInstrument.schemaVersion == 2
                && generatedInstrument.validationNotes
                    == std::vector<std::string> { currentInstrumentProvenance }
                && originalInstrumentText.toStdString().find(currentInstrumentProvenance)
                    != std::string::npos,
            "A newly saved instrument must use Practical Sampler provenance without a schema change.");

    const auto legacyInstrumentFile = testRoot.getChildFile("Legacy Identity.drinst");
    const auto referenceInstrumentLoad = drs::engine::loadPhase1ReferenceInstrumentManifest();
    require(referenceInstrumentLoad.loaded,
            "The legacy identity compatibility fixture requires the valid reference instrument.");
    auto legacyInstrument = referenceInstrumentLoad.instrument;
    legacyInstrument.validationNotes = { legacyIdentityNote };
    require(legacyInstrumentFile.replaceWithText(
                drs::engine::serializeRuntimeInstrumentManifest(
                    legacyInstrument, legacyInstrumentFile.getFullPathName().toStdString()),
                false,
                false,
                "\n"),
            "The legacy identity instrument fixture should be writable.");
    const auto legacyInstrumentLoad = drs::engine::parseRuntimeInstrumentManifest(
        legacyInstrumentFile.loadFileAsString().toStdString(),
        legacyInstrumentFile.getFullPathName().toStdString(),
        false);
    require(legacyInstrumentLoad.loaded
                && legacyInstrumentLoad.instrument.schemaName == legacyInstrument.schemaName
                && legacyInstrumentLoad.instrument.schemaVersion == legacyInstrument.schemaVersion
                && legacyInstrumentLoad.instrument.validationNotes
                    == std::vector<std::string> { legacyIdentityNote },
            "An instrument note containing the former presentation name must remain readable and unchanged.");

    auto replacement = makeStorageProject(projectFile, "Replacement Generation");
    drs::app::ProjectFilesSaveOptions options;
    options.allowCommitAtCheckpoint = [](const auto checkpoint)
    {
        return checkpoint != drs::app::ProjectFilesSaveCheckpoint::beforeProjectCommit;
    };
    const auto interruptedSave = drs::app::saveProjectFiles(replacement, projectFile, options);
    require(!interruptedSave.saved && interruptedSave.recoveredPreviousGeneration,
            "A failure between pair commits must roll back the old generation.");
    require(projectFile.loadFileAsString() == originalProjectText
                && instrumentFile.loadFileAsString() == originalInstrumentText,
            "Rollback must keep .drsproj and .drinst on the same generation.");

    const auto journal = projectFile.getSiblingFile(projectFile.getFileName() + ".save-journal");
    const auto projectBackup = projectFile.getSiblingFile(projectFile.getFileName() + ".save-backup");
    const auto instrumentBackup = instrumentFile.getSiblingFile(instrumentFile.getFileName() + ".save-backup");
    require(!journal.exists() && !projectBackup.exists() && !instrumentBackup.exists(),
            "A completed rollback must clean its journal and backup artifacts.");
    require(projectFile.copyFileTo(projectBackup) && instrumentFile.copyFileTo(instrumentBackup),
            "The recovery test should prepare an interrupted transaction generation.");
    require(instrumentFile.replaceWithText("interrupted instrument generation", false, false, "\n")
                && journal.replaceWithText(
                    "drs-project-save-transaction=1\nproject-existed=1\ninstrument-existed=1\n",
                    false,
                    false,
                    "\n"),
            "The recovery test should persist an interrupted transaction journal.");

    const auto recovery = drs::app::recoverProjectFilesTransaction(projectFile);
    require(recovery.recoveryNeeded && recovery.recovered,
            "A journaled interrupted save must recover on the next open/save boundary.");
    require(projectFile.loadFileAsString() == originalProjectText
                && instrumentFile.loadFileAsString() == originalInstrumentText,
            "Journal recovery must restore both files from the previous generation.");
    require(testRoot.deleteRecursively(), "Sprint 0 storage test cleanup failed.");
}
} // namespace

int main()
{
    try
    {
        const auto testRoot = fs::temp_directory_path() / "drs-sprint0-sfz-hardening";
        fs::remove_all(testRoot);
        fs::create_directories(testRoot);
        verifyMacrosWhitespaceAndRootRelativeSamples(testRoot);
        verifySfzBudgets(testRoot);
        verifySmDrumsCorpus();
        fs::remove_all(testRoot);
        verifyRecoverableProjectPairSave();
        std::cout << "Sprint 0 import and storage hardening tests passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Sprint 0 import and storage hardening tests failed: "
                  << exception.what() << std::endl;
        return 1;
    }
}
