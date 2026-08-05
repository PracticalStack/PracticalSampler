#include "drs/engine/PackageReader.h"
#include "drs/engine/PerformancePackage.h"
#include "plugin/PluginEditor.h"
#include "plugin/PluginProcessor.h"
#include "shared/WorkspaceMenuPolicy.h"
#include "standalone/MainComponent.h"
#include "Phase1PerformancePackageSupport.h"

#include <juce_audio_processors_headless/juce_audio_processors_headless.h>

#include <json/json.hpp>

#include <algorithm>
#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

namespace
{
namespace fs = std::filesystem;
using ordered_json = nlohmann::ordered_json;
namespace package_support = drs::tests::performance_package;
using Clock = std::chrono::steady_clock;

void require(bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

std::string summarizeIssues(const std::vector<std::string>& issues)
{
    if (issues.empty())
        return "(none)";

    std::string summary;
    for (std::size_t index = 0; index < issues.size(); ++index)
    {
        if (index != 0)
            summary += " | ";
        summary += issues[index];
    }
    return summary;
}

void writeTextFile(const fs::path& path, const std::string& text)
{
    fs::create_directories(path.parent_path());
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    require(output.good(), "Could not open performance package release gate artifact for writing: " + path.generic_string());
    output << text;
    require(output.good(), "Could not finish writing performance package release gate artifact: " + path.generic_string());
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

float renderQueuedPerformanceSurfaceMagnitude(drs::plugin::Processor& processor,
                                              const int midiNoteNumber,
                                              const float velocity,
                                              const int blockCount = 8)
{
    processor.queuePerformanceSurfaceNoteOn(midiNoteNumber, velocity);

    float maxMagnitude = 0.0f;
    juce::MidiBuffer emptyMidiBuffer;
    for (int blockIndex = 0; blockIndex < blockCount; ++blockIndex)
    {
        juce::AudioBuffer<float> buffer(2, 512);
        buffer.clear();
        processor.processBlock(buffer, emptyMidiBuffer);
        maxMagnitude = std::max(maxMagnitude, buffer.getMagnitude(0, buffer.getNumSamples()));
    }

    processor.queuePerformanceSurfaceNoteOff(midiNoteNumber);
    return maxMagnitude;
}

ordered_json buildCompatibilityPolicySection()
{
    ordered_json section;
    section["policyId"] = drs::engine::performancePackageCompatibilityPolicyId;
    section["schemaName"] = drs::engine::performancePackageSchemaName;
    section["schemaVersion"] = drs::engine::performancePackageSchemaVersion;
    section["schemaMajorVersion"] = drs::engine::performancePackageSchemaMajorVersion;
    section["schemaMinorVersion"] = drs::engine::performancePackageSchemaMinorVersion;
    section["futureMinorPolicy"] = drs::engine::performancePackageFutureMinorPolicy;
    section["futureMajorPolicy"] = drs::engine::performancePackageFutureMajorPolicy;
    section["passed"] = drs::engine::performancePackageSchemaMajorVersion == 1
        && drs::engine::performancePackageSchemaMinorVersion == 0
        && drs::engine::performancePackageSchemaVersion == 1;
    return section;
}

ordered_json buildDeterminismSection(const fs::path& scratchDirectory)
{
    const auto& cryptoProvider = drs::engine::getDeterministicPackageCryptoProvider();
    const auto firstPlan = package_support::buildPackagePlan(scratchDirectory / "first",
                                                             scratchDirectory / "first" / "deterministic-a.drpkg");
    const auto secondPlan = package_support::buildPackagePlan(scratchDirectory / "second",
                                                              scratchDirectory / "second" / "deterministic-b.drpkg");

    const auto firstWrite = drs::engine::writePerformancePackage(firstPlan, cryptoProvider);
    const auto secondWrite = drs::engine::writePerformancePackage(secondPlan, cryptoProvider);
    require(firstWrite.written, "First deterministic package write should succeed.");
    require(secondWrite.written, "Second deterministic package write should succeed.");

    const auto firstBytes = package_support::readBinaryFile(firstPlan.outputPackagePath);
    const auto secondBytes = package_support::readBinaryFile(secondPlan.outputPackagePath);

    ordered_json section;
    section["firstPackagePath"] = firstPlan.outputPackagePath;
    section["secondPackagePath"] = secondPlan.outputPackagePath;
    section["firstPackageBytes"] = firstBytes.size();
    section["secondPackageBytes"] = secondBytes.size();
    section["byteIdentical"] = firstBytes == secondBytes;
    section["passed"] = firstBytes == secondBytes;
    return section;
}

ordered_json buildFailureReportingSection()
{
    const auto& cryptoProvider = drs::engine::getDeterministicPackageCryptoProvider();
    const auto corpusPaths = package_support::getCheckedInCorpusPaths();
    const auto indexRoot = ordered_json::parse(package_support::readTextFile(corpusPaths.index));

    ordered_json fixtures = ordered_json::array();
    bool passed = true;
    for (const auto& fixture : indexRoot.at("fixtures"))
    {
        const auto fixtureId = fixture.at("id").get<std::string>();
        const auto fixturePath = corpusPaths.root / fixture.at("path").get<std::string>();
        const auto expectedFailureCategory = fixture.at("expectedFailureCategory").get<std::string>();
        const auto expectedIssueSubstring = fixture.at("expectedIssueSubstring").get<std::string>();

        const auto load = drs::engine::loadPerformancePackage(fixturePath.generic_string(),
                                                              cryptoProvider,
                                                              drs::engine::performancePackageSchemaVersion);
        const bool shouldLoad = fixtureId == "valid";
        const bool stateMatched = shouldLoad ? load.loaded : !load.loaded;
        const bool categoryMatched = std::string(drs::engine::toString(load.failureCategory))
            == expectedFailureCategory;
        const bool issueMatched = expectedIssueSubstring.empty()
            || package_support::containsIssue(load.issues, expectedIssueSubstring);

        ordered_json caseResult;
        caseResult["id"] = fixtureId;
        caseResult["path"] = fixturePath.generic_string();
        caseResult["loaded"] = load.loaded;
        caseResult["failureCategory"] = drs::engine::toString(load.failureCategory);
        caseResult["expectedFailureCategory"] = expectedFailureCategory;
        caseResult["issueMatched"] = issueMatched;
        caseResult["state"] = load.state;
        caseResult["issues"] = load.issues;
        caseResult["passed"] = stateMatched && categoryMatched && issueMatched;
        fixtures.push_back(std::move(caseResult));
        passed = passed && stateMatched && categoryMatched && issueMatched;
    }

    ordered_json section;
    section["corpusIndexPath"] = corpusPaths.index.generic_string();
    section["fixtures"] = std::move(fixtures);
    section["passed"] = passed;
    return section;
}

ordered_json buildMenuCutoverSection()
{
    ordered_json section;
    section["saveProjectLabel"] = drs::app::saveProjectMenuLabel;
    section["saveProjectAsLabel"] = drs::app::saveProjectAsMenuLabel;
    section["exportPlayableInstrumentLabel"] = drs::app::exportPerformancePackageMenuLabel;
    section["openPlayablePackageLabel"] = drs::app::openPerformancePackageMenuLabel;
    section["helpText"] = drs::app::projectDirectoryHelpText;
    section["labelsDistinct"]
        = std::string(drs::app::saveProjectMenuLabel) != std::string(drs::app::exportPerformancePackageMenuLabel);
    section["helpMentionsProjectExtension"]
        = std::string(drs::app::projectDirectoryHelpText).find(".drsproj") != std::string::npos;
    section["helpMentionsPackageExtension"]
        = std::string(drs::app::projectDirectoryHelpText).find(".drpkg") != std::string::npos;
    section["passed"] = section["labelsDistinct"].get<bool>()
        && section["helpMentionsProjectExtension"].get<bool>()
        && section["helpMentionsPackageExtension"].get<bool>();
    return section;
}

ordered_json buildReopenAndUxSection(const fs::path& scratchDirectory)
{
    juce::ScopedJuceInitialiser_GUI gui;

    const auto exportedPackagePath = scratchDirectory / "release-gate-exported.drpkg";
    const auto packageFile = juce::File(juce::String::fromUTF8(exportedPackagePath.generic_string().c_str()));

    drs::standalone::MainComponent standalone(false);
    standalone.addToDesktop(0);
    standalone.setVisible(true);
    require(standalone.getProcessor().replaceAuthoringProject(package_support::buildAuthoringProjectFixture()),
            "Release gate standalone shell should accept the authoring export fixture.");

    const auto standaloneExportStarted = Clock::now();
    const auto exportResult = standalone.getProcessor().exportPerformancePackage(packageFile);
    const auto standaloneExportElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        Clock::now() - standaloneExportStarted);
    require(exportResult.exported,
            "Release gate package export should succeed. state="
                + exportResult.state + " issues=" + summarizeIssues(exportResult.issues));

    const auto standaloneOpenStarted = Clock::now();
    const auto standaloneLoad = standalone.getProcessor().loadPerformancePackageWorkspace(packageFile);
    const auto standaloneOpenElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        Clock::now() - standaloneOpenStarted);
    require(standaloneLoad.loaded,
            "Release gate standalone reopen should succeed. state="
                + standaloneLoad.state + " issues=" + summarizeIssues(standaloneLoad.issues));

    standalone.resized();
    standalone.getProcessor().prepareToPlay(44100.0, 512);
    standalone.getProcessor().serviceMessageThreadWork();
    const auto standaloneMagnitude = renderQueuedPerformanceSurfaceMagnitude(standalone.getProcessor(), 69, 0.8f);
    auto* standaloneTabs = dynamic_cast<juce::TabbedComponent*>(findDescendantById(standalone, "workspaceTabs"));
    const bool standaloneProjectBindingSuppressed
        = standalone.exportStateJson().find("\"projectBinding\"") == std::string::npos;

    drs::plugin::Processor processor;
    std::unique_ptr<juce::AudioProcessorEditor> editor(processor.createEditor());
    require(editor != nullptr, "Release gate plugin editor should construct.");
    editor->addToDesktop(0);
    editor->setVisible(true);

    const auto pluginOpenStarted = Clock::now();
    const auto pluginLoad = processor.loadPerformancePackageWorkspace(packageFile);
    const auto pluginOpenElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        Clock::now() - pluginOpenStarted);
    require(pluginLoad.loaded,
            "Release gate plugin reopen should succeed. state="
                + pluginLoad.state + " issues=" + summarizeIssues(pluginLoad.issues));

    editor->resized();
    processor.prepareToPlay(44100.0, 512);
    processor.serviceMessageThreadWork();
    const auto pluginMagnitude = renderQueuedPerformanceSurfaceMagnitude(processor, 69, 0.8f);
    auto* pluginTabs = dynamic_cast<juce::TabbedComponent*>(findDescendantById(*editor, "workspaceTabs"));
    juce::MemoryBlock pluginState;
    processor.getStateInformation(pluginState);
    const std::string serializedPluginState(static_cast<const char*>(pluginState.getData()), pluginState.getSize());
    const bool pluginProjectBindingSuppressed = serializedPluginState.find("\"projectBinding\"") == std::string::npos;

    ordered_json section;
    section["exportedPackagePath"] = exportedPackagePath.generic_string();
    section["exportedPackageBytes"] = exportResult.packageBytes;
    section["payloadCount"] = exportResult.payloadCount;
    section["standaloneExportMs"] = standaloneExportElapsed.count();
    section["standaloneOpenMs"] = standaloneOpenElapsed.count();
    section["standaloneAudibleMagnitude"] = standaloneMagnitude;
    section["standalonePerformanceOnly"] = standalone.getProcessor().getWorkspaceDocumentState().workspaceMode
        == drs::engine::WorkspaceMode::performanceOnly;
    section["standaloneAuthoringSuppressed"] = !standalone.getProcessor().getWorkspaceDocumentState().authoringAvailable;
    section["standaloneTabCount"] = standaloneTabs != nullptr ? standaloneTabs->getNumTabs() : 0;
    section["standaloneHasAuthoringZoneSelector"] = findDescendantById(standalone, "authoringZoneSelector") != nullptr;
    section["standaloneProjectBindingSuppressed"] = standaloneProjectBindingSuppressed;
    section["pluginOpenMs"] = pluginOpenElapsed.count();
    section["pluginAudibleMagnitude"] = pluginMagnitude;
    section["pluginPerformanceOnly"] = processor.getWorkspaceDocumentState().workspaceMode
        == drs::engine::WorkspaceMode::performanceOnly;
    section["pluginAuthoringSuppressed"] = !processor.getWorkspaceDocumentState().authoringAvailable;
    section["pluginTabCount"] = pluginTabs != nullptr ? pluginTabs->getNumTabs() : 0;
    section["pluginHasAuthoringZoneSelector"] = findDescendantById(*editor, "authoringZoneSelector") != nullptr;
    section["pluginProjectBindingSuppressed"] = pluginProjectBindingSuppressed;
    section["passed"] = standaloneMagnitude > 0.0001f
        && pluginMagnitude > 0.0001f
        && section["standalonePerformanceOnly"].get<bool>()
        && section["pluginPerformanceOnly"].get<bool>()
        && section["standaloneAuthoringSuppressed"].get<bool>()
        && section["pluginAuthoringSuppressed"].get<bool>()
        && section["standaloneTabCount"].get<int>() == 1
        && section["pluginTabCount"].get<int>() == 1
        && !section["standaloneHasAuthoringZoneSelector"].get<bool>()
        && !section["pluginHasAuthoringZoneSelector"].get<bool>()
        && standaloneProjectBindingSuppressed
        && pluginProjectBindingSuppressed;
    return section;
}

std::string currentIsoDate()
{
    const auto now = std::chrono::system_clock::now();
    const auto currentTime = std::chrono::system_clock::to_time_t(now);
    std::tm localTime {};
    localtime_s(&localTime, &currentTime);

    char buffer[11] {};
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d", &localTime);
    return buffer;
}
} // namespace

int main(int argc, char* argv[])
{
    try
    {
        const auto outputPath = argc > 1
            ? fs::path(argv[1])
            : fs::temp_directory_path() / "drs-phase1-performance-package-release-gate.json";
        const auto scratchDirectory = fs::temp_directory_path() / "drs-phase1-performance-package-release-gate";
        std::error_code errorCode;
        fs::remove_all(scratchDirectory, errorCode);
        fs::create_directories(scratchDirectory);

        ordered_json report;
        report["schemaName"] = "drs.performancePackageReleaseGate";
        report["schemaVersion"] = 1;
        report["capturedOn"] = currentIsoDate();
        report["compatibilityPolicy"] = buildCompatibilityPolicySection();
        report["determinism"] = buildDeterminismSection(scratchDirectory / "determinism");
        report["failureReporting"] = buildFailureReportingSection();
        report["menuCutover"] = buildMenuCutoverSection();
        report["reopenAndPerformanceOnlyUx"] = buildReopenAndUxSection(scratchDirectory / "reopen");
        report["passed"] = report["compatibilityPolicy"].at("passed").get<bool>()
            && report["determinism"].at("passed").get<bool>()
            && report["failureReporting"].at("passed").get<bool>()
            && report["menuCutover"].at("passed").get<bool>()
            && report["reopenAndPerformanceOnlyUx"].at("passed").get<bool>();

        writeTextFile(outputPath, report.dump(2) + "\n");
        require(report["passed"].get<bool>(),
                "Performance package release gate did not pass every section. See "
                    + outputPath.generic_string());
        std::cout << "Performance package release gate passed. Artifact: "
                  << outputPath.generic_string() << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Performance package release gate failed: " << exception.what() << std::endl;
        return 1;
    }
}
