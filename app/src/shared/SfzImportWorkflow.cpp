#include "shared/SfzImportWorkflow.h"

#include "drs/engine/SampleImport.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <sstream>

namespace drs::app
{
namespace
{
const auto reviewBackground = juce::Colour::fromRGB(245, 240, 232);
const auto reviewPanel = juce::Colour::fromRGB(255, 252, 246);
const auto reviewInk = juce::Colour::fromRGB(25, 31, 36);
const auto reviewMuted = juce::Colour::fromRGB(96, 103, 112);
const auto reviewAccent = juce::Colour::fromRGB(150, 78, 22);
const auto reviewSuccess = juce::Colour::fromRGB(28, 108, 88);
const auto reviewWarning = juce::Colour::fromRGB(138, 100, 0);
const auto reviewDanger = juce::Colour::fromRGB(139, 53, 53);

juce::String toDisplayString(const std::string& text)
{
    return juce::String::fromUTF8(text.c_str());
}

juce::String formatGainDb(const double gainDb)
{
    const auto roundedGain = std::abs(gainDb) < 1.0e-9 ? 0.0 : gainDb;
    return juce::String(roundedGain, 2) + " dB";
}

juce::String sourceSectionLabel(const drs::engine::SfzOpcodeScope scope)
{
    switch (scope)
    {
        case drs::engine::SfzOpcodeScope::control: return "<control>";
        case drs::engine::SfzOpcodeScope::global: return "<global>";
        case drs::engine::SfzOpcodeScope::master: return "<master>";
        case drs::engine::SfzOpcodeScope::group: return "<group>";
        case drs::engine::SfzOpcodeScope::region: return "<region>";
        case drs::engine::SfzOpcodeScope::curve: return "<curve>";
        case drs::engine::SfzOpcodeScope::effect: return "<effect>";
        case drs::engine::SfzOpcodeScope::midi: return "<midi>";
        case drs::engine::SfzOpcodeScope::sample: return "<sample>";
        case drs::engine::SfzOpcodeScope::unknown: break;
    }
    return "<unknown>";
}

std::string summarizeFinding(const drs::engine::SfzImportFinding& finding)
{
    std::ostringstream stream;
    stream << finding.code << ": " << finding.summary;
    if (!finding.detail.empty())
        stream << " " << finding.detail;
    if (!finding.location.sourcePath.empty())
        stream << " [" << finding.location.sourcePath << ":" << finding.location.lineNumber << "]";
    return stream.str();
}

struct GroupedFinding
{
    const drs::engine::SfzImportFinding* first = nullptr;
    std::size_t occurrenceCount = 0;
};

std::vector<GroupedFinding> groupFindings(
    const std::vector<drs::engine::SfzImportFinding>& findings)
{
    std::vector<GroupedFinding> groups;
    std::map<std::string, std::size_t> groupIndexes;

    for (const auto& finding : findings)
    {
        const auto key = std::to_string(static_cast<int>(finding.severity)) + "\n"
            + std::to_string(static_cast<int>(finding.disposition)) + "\n"
            + finding.code + "\n" + finding.summary;
        if (const auto iterator = groupIndexes.find(key); iterator != groupIndexes.end())
        {
            ++groups[iterator->second].occurrenceCount;
            continue;
        }

        groupIndexes.emplace(key, groups.size());
        groups.push_back({ &finding, 1 });
    }

    return groups;
}

void configureWrappedLabel(juce::Label& label,
                           const juce::String& componentId,
                           float fontHeight,
                           juce::Font::FontStyleFlags styleFlags,
                           juce::Colour colour)
{
    label.setComponentID(componentId);
    label.setColour(juce::Label::textColourId, colour);
    label.setFont(juce::FontOptions(fontHeight, styleFlags));
    label.setJustificationType(juce::Justification::topLeft);
}

juce::String buildIssueSummary(const std::vector<std::string>& issues, std::size_t maximumIssueCount)
{
    if (issues.empty())
        return "The SFZ review could not be prepared.";

    juce::String summary;
    for (std::size_t index = 0; index < issues.size() && index < maximumIssueCount; ++index)
        summary += "\n- " + toDisplayString(issues[index]);

    if (issues.size() > maximumIssueCount)
        summary += "\n- ...";

    return summary.trimStart();
}
} // namespace

SfzImportReviewPreparationResult prepareSfzImportReview(const drs::engine::RuntimeProjectModel& baseProject,
                                                        const std::string& sfzPath)
{
    return prepareSfzImportReview(baseProject,
                                   sfzPath,
                                   drs::engine::defaultSfzImportExecutionContext());
}

SfzImportReviewPreparationResult prepareSfzImportReview(
    const drs::engine::RuntimeProjectModel& baseProject,
    const std::string& sfzPath,
    const drs::engine::SfzImportExecutionContext& context)
{
    SfzImportReviewPreparationResult result;
    result.analysis = drs::engine::analyzeSfzImportDocument(sfzPath, context);
    result.reportModel = makeSfzImportReportModel(result.analysis);
    auto projectionContext = context;
    projectionContext.sourceRegionMetadataResolver = [](const std::string& samplePath)
        -> std::optional<drs::engine::SfzImportSourceRegionMetadata>
    {
        const auto inspection = drs::engine::inspectSampleFileMetadataOnly(samplePath);
        if (!inspection.inspected)
            return std::nullopt;
        drs::engine::SfzImportSourceRegionMetadata metadata;
        metadata.frameCount = inspection.metadata.frameCount;
        metadata.loopRangePresent = inspection.metadata.loopRangePresent;
        metadata.loopStartFrame = inspection.metadata.loopStartFrame;
        metadata.loopEndFrameInclusive = inspection.metadata.loopEndFrame;
        return metadata;
    };
    result.projection = drs::engine::projectSfzImportAnalysis(
        baseProject, result.analysis, projectionContext);
    result.commitAllowed = result.reportModel.commitAllowed && result.projection.projected;
    result.blocking = result.analysis.report.blocking || result.projection.blocking;
    result.state = result.projection.state.empty() ? result.analysis.report.state : result.projection.state;
    result.issues = result.projection.issues;

    if (!result.analysis.report.available)
    {
        result.issues.push_back("The SFZ report could not be prepared for review.");
    }

    if (!result.projection.projected && result.issues.empty())
    {
        result.issues.push_back("The SFZ document could not be projected into native authoring content.");
    }

    result.prepared = result.analysis.report.available
        && !result.analysis.execution.canceled()
        && !result.projection.execution.canceled();
    return result;
}

juce::String buildSfzImportIssueSummary(const SfzImportReviewPreparationResult& review,
                                        std::size_t maximumIssueCount)
{
    juce::String summary;
    if (!review.state.empty())
        summary = toDisplayString(review.state);

    if (!review.issues.empty())
    {
        if (summary.isNotEmpty())
            summary += "\n\n";
        summary += buildIssueSummary(review.issues, maximumIssueCount);
        return summary;
    }

    if (!review.analysis.report.findings.empty())
    {
        if (summary.isNotEmpty())
            summary += "\n\n";

        std::vector<std::string> findingLines;
        findingLines.reserve(review.analysis.report.findings.size());
        for (const auto& finding : review.analysis.report.findings)
            findingLines.push_back(summarizeFinding(finding));
        summary += buildIssueSummary(findingLines, maximumIssueCount);
        return summary;
    }

    return summary.isNotEmpty() ? summary : juce::String("The SFZ review could not be completed.");
}

juce::String buildSfzImportAppliedSummary(const SfzImportReviewPreparationResult& review)
{
    juce::String summary("Imported SFZ into the current project.");
    summary += "\nZones: " + juce::String(static_cast<int>(review.projection.zones.size()));
    summary += "\nGroups: " + juce::String(static_cast<int>(review.projection.groups.size()));
    summary += "\nSample sources: " + juce::String(static_cast<int>(review.projection.sampleSources.size()));
    summary += "\nMaster gain: " + formatGainDb(review.projection.masterGainDb);
    summary += "\nWarnings: " + juce::String(static_cast<int>(review.analysis.report.summary.warningFindingCount));
    if (review.projection.omittedUnsafeRegionCount > 0)
        summary += "\nSound-safe omissions: "
            + juce::String(static_cast<int>(review.projection.omittedUnsafeRegionCount));

    if (!review.projection.authoringNotes.empty())
        summary += "\nSaved review notes: " + juce::String(static_cast<int>(review.projection.authoringNotes.size()));

    return summary;
}

SfzImportReviewComponent::SfzImportReviewComponent(SfzImportReviewPreparationResult reviewToUse,
                                                   DecisionCallback callback)
    : review(std::move(reviewToUse)),
      decisionCallback(std::move(callback))
{
    setOpaque(true);

    configureWrappedLabel(headlineLabel, "sfzImportReviewHeadlineLabel", 22.0f, juce::Font::bold, reviewInk);
    configureWrappedLabel(guidanceLabel, "sfzImportReviewGuidanceLabel", 15.0f, juce::Font::plain, reviewMuted);
    configureWrappedLabel(pathLabel, "sfzImportReviewPathLabel", 13.0f, juce::Font::plain, reviewMuted);
    configureWrappedLabel(summaryLabel, "sfzImportReviewSummaryLabel", 14.0f, juce::Font::plain, reviewInk);
    configureWrappedLabel(projectionLabel, "sfzImportReviewProjectionLabel", 14.0f, juce::Font::plain, reviewInk);

    findingsEditor.setComponentID("sfzImportReviewFindingsEditor");
    findingsEditor.setReadOnly(true);
    findingsEditor.setMultiLine(true);
    findingsEditor.setReturnKeyStartsNewLine(true);
    findingsEditor.setScrollbarsShown(true);
    findingsEditor.setCaretVisible(false);
    findingsEditor.setColour(juce::TextEditor::backgroundColourId, reviewPanel);
    findingsEditor.setColour(juce::TextEditor::textColourId, reviewInk);
    findingsEditor.setColour(juce::TextEditor::outlineColourId, reviewMuted.withAlpha(0.25f));

    cancelButton.setComponentID("sfzImportReviewCancelButton");
    applyButton.setComponentID("sfzImportReviewApplyButton");
    applyButton.setEnabled(review.commitAllowed);
    applyButton.setButtonText(
        !review.commitAllowed ? "Import Unavailable"
                              : (review.projection.omittedUnsafeRegionCount > 0
                                     ? "Import Safe Zones"
                                     : "Import into Project"));

    cancelButton.onClick = [this]() { commitDecision(false); };
    applyButton.onClick = [this]() { commitDecision(true); };

    addAndMakeVisible(headlineLabel);
    addAndMakeVisible(guidanceLabel);
    addAndMakeVisible(pathLabel);
    addAndMakeVisible(summaryLabel);
    addAndMakeVisible(projectionLabel);
    addAndMakeVisible(findingsEditor);
    addAndMakeVisible(cancelButton);
    addAndMakeVisible(applyButton);

    refreshFromReview();
    setSize(760, 620);
}

SfzImportReviewComponent::~SfzImportReviewComponent()
{
    if (!decisionCommitted && decisionCallback)
        decisionCallback(false);
}

void SfzImportReviewComponent::resized()
{
    auto area = getLocalBounds().reduced(20);
    headlineLabel.setBounds(area.removeFromTop(34));
    guidanceLabel.setBounds(area.removeFromTop(54));
    area.removeFromTop(6);
    pathLabel.setBounds(area.removeFromTop(22));
    area.removeFromTop(10);
    summaryLabel.setBounds(area.removeFromTop(84));
    area.removeFromTop(8);
    projectionLabel.setBounds(area.removeFromTop(84));
    area.removeFromTop(10);
    auto buttonRow = area.removeFromBottom(42);
    findingsEditor.setBounds(area);

    applyButton.setBounds(buttonRow.removeFromRight(180));
    buttonRow.removeFromRight(12);
    cancelButton.setBounds(buttonRow.removeFromRight(120));
}

void SfzImportReviewComponent::refreshFromReview()
{
    headlineLabel.setText(toDisplayString(review.reportModel.headline), juce::dontSendNotification);
    guidanceLabel.setText(toDisplayString(review.reportModel.guidance), juce::dontSendNotification);
    pathLabel.setText("Document: " + toDisplayString(review.reportModel.documentPath), juce::dontSendNotification);
    summaryLabel.setText(buildSummaryText(), juce::dontSendNotification);
    projectionLabel.setText(buildProjectionText(), juce::dontSendNotification);
    findingsEditor.setText(buildFindingsText(), juce::dontSendNotification);

    auto headlineColour = reviewDanger;
    if (!review.blocking)
        headlineColour = review.reportModel.confirmationRequired ? reviewWarning : reviewSuccess;
    headlineLabel.setColour(juce::Label::textColourId, headlineColour);
}

void SfzImportReviewComponent::commitDecision(bool accepted)
{
    if (decisionCommitted)
        return;

    decisionCommitted = true;
    if (decisionCallback)
        decisionCallback(accepted);

    if (auto* dialogWindow = findParentComponentOfClass<juce::DialogWindow>())
        dialogWindow->exitModalState(accepted ? 1 : 0);
}

juce::String SfzImportReviewComponent::buildSummaryText() const
{
    juce::String text("Conversion summary");
    text += "\nConverted: " + juce::String(static_cast<int>(review.reportModel.convertedCount));
    text += " | Approximated: " + juce::String(static_cast<int>(review.reportModel.approximatedCount));
    text += " | Review-only: " + juce::String(static_cast<int>(review.reportModel.reportedOnlyCount));
    text += " | Blocking: " + juce::String(static_cast<int>(review.reportModel.blockingCount));
    for (const auto& section : review.reportModel.report.sections)
    {
        text += "\n" + juce::String::fromUTF8(section.name.c_str()) + ": "
            + juce::String(static_cast<int>(section.itemCount));
    }
    return text;
}

juce::String SfzImportReviewComponent::buildProjectionText() const
{
    juce::String text("Native projection");
    text += "\nZones: " + juce::String(static_cast<int>(review.projection.zones.size()));
    text += " | Groups: " + juce::String(static_cast<int>(review.projection.groups.size()));
    text += " | Sample sources: " + juce::String(static_cast<int>(review.projection.sampleSources.size()));
    text += "\nMaster gain: " + formatGainDb(review.projection.masterGainDb);
    text += " | Playable draft: " + juce::String(review.projection.playable ? "Yes" : "No");
    text += " | Saved notes: " + juce::String(static_cast<int>(review.projection.projectNotes.size()
                                                                 + review.projection.authoringNotes.size()));
    if (review.projection.omittedUnsafeRegionCount > 0)
    {
        text += "\nSound-safe omissions: "
            + juce::String(static_cast<int>(review.projection.omittedUnsafeRegionCount));
        text += " | Retained regions: "
            + juce::String(static_cast<int>(review.projection.zones.size()));
    }
    return text;
}

juce::String SfzImportReviewComponent::buildFindingsText() const
{
    constexpr auto maximumRenderedFindingGroups = std::size_t { 100 };
    juce::String text;

    if (!review.issues.empty())
    {
        text += "Projection issues:\n";
        text += buildIssueSummary(review.issues, 20) + "\n";
        text += "\n";
    }

    if (!review.projection.omittedRegionSummaries.empty())
    {
        text += "Sound-safe omissions:\n";
        for (const auto& omission : review.projection.omittedRegionSummaries)
        {
            text += "- " + toDisplayString(omission.feature)
                + " from " + sourceSectionLabel(omission.sourceScope)
                + ": " + juce::String(static_cast<int>(omission.affectedRegionCount))
                + " affected region";
            if (omission.affectedRegionCount != 1)
                text += "s";
            if (!omission.sourcePath.empty())
            {
                text += " [" + toDisplayString(omission.sourcePath);
                if (omission.firstSourceLineNumber > 0)
                    text += ":" + juce::String(static_cast<int>(omission.firstSourceLineNumber));
                text += "]";
            }
            text += "\n";
        }
        text += "\n";
    }

    if (review.analysis.report.findings.empty())
    {
        text += "No findings were reported for this SFZ document.";
        return text;
    }

    text += "Findings:\n";
    const auto groups = groupFindings(review.analysis.report.findings);
    const auto renderedGroupCount = std::min(groups.size(), maximumRenderedFindingGroups);
    for (std::size_t index = 0; index < renderedGroupCount; ++index)
    {
        const auto& group = groups[index];
        text += "- " + toDisplayString(summarizeFinding(*group.first));
        if (group.occurrenceCount > 1)
            text += " (" + juce::String(static_cast<int>(group.occurrenceCount)) + " occurrences)";
        text += "\n";
    }

    if (groups.size() > renderedGroupCount)
    {
        text += "- ... " + juce::String(static_cast<int>(groups.size() - renderedGroupCount))
            + " additional finding groups omitted from this view.\n";
    }

    if (review.analysis.report.summary.suppressedFindingCount > 0)
    {
        text += "- ... "
            + juce::String(static_cast<int>(review.analysis.report.summary.suppressedFindingCount))
            + " additional findings omitted by import safety limits.\n";
    }
    return text;
}

void showSfzImportReviewDialog(juce::Component* parentComponent,
                               SfzImportReviewPreparationResult review,
                               SfzImportReviewComponent::DecisionCallback decisionCallback)
{
    juce::DialogWindow::LaunchOptions options;
    options.dialogTitle = review.reportModel.headline.empty() ? "Review SFZ Import"
                                                              : toDisplayString(review.reportModel.headline);
    options.dialogBackgroundColour = reviewBackground;
    options.escapeKeyTriggersCloseButton = true;
    options.useNativeTitleBar = true;
    options.resizable = true;
    options.useBottomRightCornerResizer = false;
    options.content.setOwned(new SfzImportReviewComponent(std::move(review), std::move(decisionCallback)));
    options.componentToCentreAround = parentComponent;
    options.launchAsync();
}
} // namespace drs::app
