#pragma once

#include "shared/SfzImportReportModel.h"
#include "drs/engine/SfzImportProjection.h"

#include <juce_gui_extra/juce_gui_extra.h>

#include <functional>
#include <string>
#include <vector>

namespace drs::app
{
struct SfzImportReviewPreparationResult
{
    bool prepared = false;
    bool commitAllowed = false;
    bool blocking = false;
    std::string state;
    std::vector<std::string> issues;
    drs::engine::SfzImportAnalysisResult analysis;
    drs::engine::SfzImportProjectionResult projection;
    SfzImportReportModel reportModel;
};

SfzImportReviewPreparationResult prepareSfzImportReview(const drs::engine::RuntimeProjectModel& baseProject,
                                                        const std::string& sfzPath);
juce::String buildSfzImportIssueSummary(const SfzImportReviewPreparationResult& review,
                                        std::size_t maximumIssueCount = 8);
juce::String buildSfzImportAppliedSummary(const SfzImportReviewPreparationResult& review);

class SfzImportReviewComponent final : public juce::Component
{
public:
    using DecisionCallback = std::function<void(bool accepted)>;

    SfzImportReviewComponent(SfzImportReviewPreparationResult review,
                             DecisionCallback decisionCallback);
    ~SfzImportReviewComponent() override;

    void resized() override;

private:
    void refreshFromReview();
    void commitDecision(bool accepted);
    juce::String buildSummaryText() const;
    juce::String buildProjectionText() const;
    juce::String buildFindingsText() const;

    SfzImportReviewPreparationResult review;
    DecisionCallback decisionCallback;
    bool decisionCommitted = false;

    juce::Label headlineLabel;
    juce::Label guidanceLabel;
    juce::Label pathLabel;
    juce::Label summaryLabel;
    juce::Label projectionLabel;
    juce::TextEditor findingsEditor;
    juce::TextButton cancelButton { "Cancel" };
    juce::TextButton applyButton { "Import into Project" };
};

void showSfzImportReviewDialog(juce::Component* parentComponent,
                               SfzImportReviewPreparationResult review,
                               SfzImportReviewComponent::DecisionCallback decisionCallback);
} // namespace drs::app
