#include "shared/SfzImportWorkflow.h"

#include <juce_gui_extra/juce_gui_extra.h>

#include <filesystem>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>

namespace
{
namespace fs = std::filesystem;

class DesktopHostedComponent final : public juce::Component
{
public:
    explicit DesktopHostedComponent(juce::Component& contentToHost)
        : hostedContent(contentToHost)
    {
        addAndMakeVisible(hostedContent);
        setSize(hostedContent.getWidth(), hostedContent.getHeight());
        addToDesktop(0);
        setVisible(true);
        toFront(true);
        resized();
    }

    ~DesktopHostedComponent() override
    {
        removeChildComponent(&hostedContent);
        setVisible(false);
        removeFromDesktop();
    }

    void resized() override
    {
        hostedContent.setBounds(getLocalBounds());
    }

private:
    juce::Component& hostedContent;
};

void require(bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

void pumpMessages(int millis = 20)
{
   #if JUCE_MODAL_LOOPS_PERMITTED
    if (auto* messageManager = juce::MessageManager::getInstanceWithoutCreating())
        messageManager->runDispatchLoopUntil(millis);
    else
        juce::Thread::sleep(millis);
   #else
    juce::Thread::sleep(millis);
   #endif
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

juce::Label& requireLabel(juce::Component& root, const juce::String& componentId)
{
    auto* label = dynamic_cast<juce::Label*>(findDescendantById(root, componentId));
    require(label != nullptr, "Missing label ID: " + componentId.toStdString());
    return *label;
}

juce::Button& requireButton(juce::Component& root, const juce::String& componentId)
{
    auto* button = dynamic_cast<juce::Button*>(findDescendantById(root, componentId));
    require(button != nullptr, "Missing button ID: " + componentId.toStdString());
    return *button;
}

juce::TextEditor& requireEditor(juce::Component& root, const juce::String& componentId)
{
    auto* editor = dynamic_cast<juce::TextEditor*>(findDescendantById(root, componentId));
    require(editor != nullptr, "Missing editor ID: " + componentId.toStdString());
    return *editor;
}

void requireLabelContains(juce::Component& root,
                          const juce::String& componentId,
                          const std::string& expectedFragment,
                          const std::string& message)
{
    const auto text = requireLabel(root, componentId).getText().toStdString();
    require(text.find(expectedFragment) != std::string::npos, message + " Text: " + text);
}

fs::path resolveFirstFixturePath()
{
    const auto sourceRoot = fs::path(DRS_SOURCE_ROOT);
    const auto relativeFixturePath =
        fs::path("DemoSFVInstruments/jlearman.jRhodes3d-master-rr/jRhodes3d-mono/_jRhodes3d-mono-flac.sfz");

    const auto localFixturePath = sourceRoot / relativeFixturePath;
    if (fs::exists(localFixturePath))
        return localFixturePath;

    const auto workspaceFixturePath = sourceRoot.parent_path() / relativeFixturePath;
    if (fs::exists(workspaceFixturePath))
        return workspaceFixturePath;

    throw std::runtime_error("Could not locate " + relativeFixturePath.generic_string());
}

drs::engine::RuntimeProjectModel makeBlankPhase2Project(const fs::path& fixturePath)
{
    drs::engine::RuntimeProjectModel project;
    project.schemaName = "drs.project";
    project.schemaVersion = 2;
    project.projectId = "sprint31.sfz-review-ui";
    project.displayName = "Sprint 3.1.5 Review UI";
    project.contentRootPath = fixturePath.parent_path().generic_string();
    project.defaultInstrumentManifestPath = (fixturePath.parent_path() / "review-ui-test.drstrm").generic_string();
    project.authoring.schemaName = "drs.authoring";
    project.authoring.schemaVersion = 1;
    return project;
}
} // namespace

int main()
{
    try
    {
        juce::ScopedJuceInitialiser_GUI gui;

        const auto fixturePath = resolveFirstFixturePath();
        const auto baseProject = makeBlankPhase2Project(fixturePath);
        const auto review = drs::app::prepareSfzImportReview(baseProject, fixturePath.generic_string());
        require(review.prepared, "Sprint 3.1.5 review UI needs the first SFZ fixture review to prepare.");
        require(review.commitAllowed, "Sprint 3.1.5 review UI expects the first SFZ fixture to remain importable.");

        std::optional<bool> acceptedDecision;
        {
            drs::app::SfzImportReviewComponent component(
                review,
                [&acceptedDecision](bool accepted)
                {
                    acceptedDecision = accepted;
                });
            component.setSize(760, 620);
            DesktopHostedComponent host(component);
            pumpMessages(30);

            requireLabelContains(component,
                                 "sfzImportReviewPathLabel",
                                 ".sfz",
                                 "Review UI should surface the imported SFZ document path.");
            requireLabelContains(component,
                                 "sfzImportReviewSummaryLabel",
                                 "Approximated: 0",
                                 "Review UI should surface conversion counts.");
            requireLabelContains(component,
                                 "sfzImportReviewGuidanceLabel",
                                 "master, group, and zone gains",
                                 "Review UI should explain scoped gain preservation.");
            requireLabelContains(component,
                                 "sfzImportReviewProjectionLabel",
                                 "Zones:",
                                 "Review UI should surface native projection counts.");
            requireLabelContains(component,
                                 "sfzImportReviewProjectionLabel",
                                 "Groups:",
                                 "Review UI should surface projected group counts.");
            requireLabelContains(component,
                                 "sfzImportReviewProjectionLabel",
                                 "Master gain:",
                                 "Review UI should surface projected master gain.");
            require(requireEditor(component, "sfzImportReviewFindingsEditor").getText().contains("Findings:"),
                    "Review UI should show the SFZ findings list.");
            require(requireButton(component, "sfzImportReviewApplyButton").isEnabled(),
                    "Review UI should keep apply enabled for the first SFZ fixture.");

            auto& applyButton = requireButton(component, "sfzImportReviewApplyButton");
            require(static_cast<bool>(applyButton.onClick),
                    "Review UI should wire the import button callback.");
            applyButton.onClick();
            pumpMessages(30);
        }

        require(acceptedDecision.has_value() && *acceptedDecision,
                "Review UI should report an accepted decision when the import button is used.");

        auto omissionReview = review;
        omissionReview.analysis.report.summary.unsafeUnconditionalRegionCount = 2;
        omissionReview.reportModel = drs::app::makeSfzImportReportModel(omissionReview.analysis);
        omissionReview.projection.omittedUnsafeRegionCount = 2;
        drs::engine::SfzImportOmittedRegionSummary omissionSummary;
        omissionSummary.dependencyKind
            = drs::engine::SfzImportSemanticDependencyKind::controllerRange;
        omissionSummary.controllerNumber = 23;
        omissionSummary.sourceScope = drs::engine::SfzOpcodeScope::group;
        omissionSummary.sourcePath = fixturePath.generic_string();
        omissionSummary.firstSourceLineNumber = 42;
        omissionSummary.feature = "MIDI controller range (CC23)";
        omissionSummary.affectedRegionCount = 2;
        omissionReview.projection.omittedRegionSummaries = { omissionSummary };

        std::optional<bool> omissionDecision;
        {
            drs::app::SfzImportReviewComponent component(
                omissionReview,
                [&omissionDecision](bool accepted)
                {
                    omissionDecision = accepted;
                });
            component.setSize(760, 620);
            DesktopHostedComponent host(component);
            pumpMessages(30);

            requireLabelContains(component,
                                 "sfzImportReviewGuidanceLabel",
                                 "will be omitted",
                                 "Sound-safe review should explain that unsafe regions will be omitted.");
            requireLabelContains(component,
                                 "sfzImportReviewProjectionLabel",
                                 "Sound-safe omissions: 2",
                                 "Sound-safe review should surface the omitted-region count.");
            require(requireButton(component, "sfzImportReviewApplyButton").getButtonText()
                        == "Import Safe Zones",
                    "Sound-safe review should offer the explicit safe import choice.");
            const auto omissionText = requireEditor(
                component, "sfzImportReviewFindingsEditor").getText();
            require(omissionText.contains("Sound-safe omissions:")
                        && omissionText.contains("MIDI controller range (CC23)")
                        && omissionText.contains("<group>")
                        && omissionText.contains("2 affected regions"),
                    "Sound-safe review should summarize omissions by feature and source section.");

            requireButton(component, "sfzImportReviewCancelButton").onClick();
            pumpMessages(30);
        }
        require(omissionDecision.has_value() && !*omissionDecision,
                "Sound-safe review should preserve Cancel as the explicit no-mutation choice.");

        auto blockedReview = review;
        blockedReview.commitAllowed = false;
        blockedReview.blocking = true;
        blockedReview.state = "Blocking review";
        blockedReview.issues = { "Blocking example issue" };
        blockedReview.reportModel.headline = "Blocked SFZ import";
        blockedReview.reportModel.guidance = "Unsupported content must remain review-only.";
        blockedReview.analysis.report.findings.clear();
        drs::engine::SfzImportFinding repeatedFinding;
        repeatedFinding.severity = drs::engine::SfzImportFindingSeverity::warning;
        repeatedFinding.disposition = drs::engine::SfzImportSupportDisposition::reportedOnly;
        repeatedFinding.code = "sfz.opcode.unmapped";
        repeatedFinding.summary = "Repeated compatibility finding";
        repeatedFinding.detail = "Only the first occurrence should be rendered.";
        for (int index = 0; index < 150; ++index)
            blockedReview.analysis.report.findings.push_back(repeatedFinding);
        blockedReview.analysis.report.summary.suppressedFindingCount = 27;

        std::optional<bool> cancelledDecision;
        {
            drs::app::SfzImportReviewComponent component(
                blockedReview,
                [&cancelledDecision](bool accepted)
                {
                    cancelledDecision = accepted;
                });
            component.setSize(760, 620);
            DesktopHostedComponent host(component);
            pumpMessages(30);

            require(!requireButton(component, "sfzImportReviewApplyButton").isEnabled(),
                    "Blocked SFZ review should disable the apply button.");
            require(requireEditor(component, "sfzImportReviewFindingsEditor").getText().contains("Projection issues:"),
                    "Blocked SFZ review should surface projection issues in the findings panel.");
            const auto findingsText = requireEditor(component, "sfzImportReviewFindingsEditor").getText();
            require(findingsText.contains("150 occurrences")
                        && findingsText.contains("27 additional findings omitted by import safety limits"),
                    "Review UI should group duplicate findings and disclose safety-limit omissions.");

            auto& cancelButton = requireButton(component, "sfzImportReviewCancelButton");
            require(static_cast<bool>(cancelButton.onClick),
                    "Review UI should wire the cancel button callback.");
            cancelButton.onClick();
            pumpMessages(30);
        }

        require(cancelledDecision.has_value() && !*cancelledDecision,
                "Review UI should report a cancelled decision when the cancel button is used.");

        std::cout << "Sprint 3.1.5 SFZ review UI tests passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Sprint 3.1.5 SFZ review UI tests failed: "
                  << exception.what() << std::endl;
        return 1;
    }
}
