#include "drs/engine/AuthoringSession.h"
#include "drs/engine/RuntimeLoader.h"
#include "shared/AuthoringPanel.h"
#include "shared/AuthoringPreviewModel.h"
#include "shared/authoring/AuthoringSummaryStrip.h"
#include "shared/authoring/RepeatedStructureList.h"
#include "shared/authoring/AuthoringWorkspaceLayout.h"
#include "shared/authoring/ZoneMappingEditor.h"

#include <juce_gui_extra/juce_gui_extra.h>

#include <filesystem>
#include <fstream>
#include <cmath>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

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

void requireComponentVisibleWithin(juce::Component& root,
                                   const juce::String& componentId,
                                   const juce::Rectangle<int>& containerBounds)
{
    auto* component = findDescendantById(root, componentId);
    require(component != nullptr, "Missing component ID: " + componentId.toStdString());
    require(component->isVisible(), "Component should be visible: " + componentId.toStdString());
    require(!component->getBounds().isEmpty(), "Component bounds should not be empty: " + componentId.toStdString());
    require(containerBounds.contains(component->getBounds()),
            "Component should remain inside the panel bounds: " + componentId.toStdString());
}

void requireComponentVisible(juce::Component& root, const juce::String& componentId)
{
    auto* component = findDescendantById(root, componentId);
    require(component != nullptr, "Missing component ID: " + componentId.toStdString());
    require(component->isVisible(), "Component should be visible: " + componentId.toStdString());
    require(!component->getBounds().isEmpty(), "Component bounds should not be empty: " + componentId.toStdString());
}

juce::Button& requireButton(juce::Component& root, const juce::String& componentId)
{
    auto* button = dynamic_cast<juce::Button*>(findDescendantById(root, componentId));
    require(button != nullptr, "Missing button ID: " + componentId.toStdString());
    return *button;
}

juce::Slider& requireSlider(juce::Component& root, const juce::String& componentId)
{
    auto* slider = dynamic_cast<juce::Slider*>(findDescendantById(root, componentId));
    require(slider != nullptr, "Missing slider ID: " + componentId.toStdString());
    return *slider;
}

juce::Label& requireLabel(juce::Component& root, const juce::String& componentId)
{
    auto* label = dynamic_cast<juce::Label*>(findDescendantById(root, componentId));
    require(label != nullptr, "Missing label ID: " + componentId.toStdString());
    return *label;
}

juce::Label& requireSliderTextBoxLabel(juce::Slider& slider)
{
    for (int childIndex = 0; childIndex < slider.getNumChildComponents(); ++childIndex)
    {
        if (auto* label = dynamic_cast<juce::Label*>(slider.getChildComponent(childIndex)))
            return *label;
    }

    throw std::runtime_error("Slider is missing its text-box label child.");
}

std::string requireMessageText(juce::Component& root, const juce::String& componentId)
{
    auto* message = findDescendantById(root, componentId);
    require(message != nullptr, "Missing message ID: " + componentId.toStdString());

    for (int childIndex = 0; childIndex < message->getNumChildComponents(); ++childIndex)
    {
        if (auto* label = dynamic_cast<juce::Label*>(message->getChildComponent(childIndex)))
            return label->getText().toStdString();
    }

    throw std::runtime_error("Message component is missing its label child.");
}

juce::ToggleButton& requireToggleButton(juce::Component& root, const juce::String& componentId)
{
    auto* toggle = dynamic_cast<juce::ToggleButton*>(findDescendantById(root, componentId));
    require(toggle != nullptr, "Missing toggle ID: " + componentId.toStdString());
    return *toggle;
}

juce::ComboBox& requireComboBox(juce::Component& root, const juce::String& componentId)
{
    auto* combo = dynamic_cast<juce::ComboBox*>(findDescendantById(root, componentId));
    require(combo != nullptr, "Missing combo box ID: " + componentId.toStdString());
    return *combo;
}

juce::ListBox& requireListBox(juce::Component& root, const juce::String& componentId)
{
    auto* listBox = dynamic_cast<juce::ListBox*>(findDescendantById(root, componentId));
    require(listBox != nullptr, "Missing list box ID: " + componentId.toStdString());
    return *listBox;
}

drs::app::authoring::RepeatedStructureList& requireRepeatedStructureList(juce::Component& root,
                                                                         const juce::String& componentId)
{
    auto* list = dynamic_cast<drs::app::authoring::RepeatedStructureList*>(findDescendantById(root, componentId));
    require(list != nullptr, "Missing repeated-structure list ID: " + componentId.toStdString());
    return *list;
}

drs::app::authoring::ZoneMapCanvas& requireZoneMapCanvas(juce::Component& root, const juce::String& componentId)
{
    auto* zoneMap = dynamic_cast<drs::app::authoring::ZoneMapCanvas*>(findDescendantById(root, componentId));
    require(zoneMap != nullptr, "Missing zone map ID: " + componentId.toStdString());
    return *zoneMap;
}

bool isDescendantOrSelf(const juce::Component* component, const juce::Component& ancestor)
{
    for (auto* current = component; current != nullptr; current = current->getParentComponent())
    {
        if (current == &ancestor)
            return true;
    }

    return false;
}

void requireFocusedWithin(const juce::Component& component, const std::string& message)
{
    const auto* focusedComponent = juce::Component::getCurrentlyFocusedComponent();
    require(focusedComponent != nullptr && isDescendantOrSelf(focusedComponent, component), message);
}

void requireAccessibilityAction(juce::Component& component,
                                juce::AccessibilityActionType actionType,
                                const std::string& message)
{
    auto* handler = component.getAccessibilityHandler();
    require(handler != nullptr, message + " should expose an accessibility handler.");
    require(handler->getActions().contains(actionType),
            message + " should expose the expected accessibility action.");
}

double toLinearChannel(double channel)
{
    return channel <= 0.04045 ? (channel / 12.92)
                              : std::pow((channel + 0.055) / 1.055, 2.4);
}

double relativeLuminance(juce::Colour colour)
{
    const auto red = toLinearChannel(colour.getFloatRed());
    const auto green = toLinearChannel(colour.getFloatGreen());
    const auto blue = toLinearChannel(colour.getFloatBlue());
    return (0.2126 * red) + (0.7152 * green) + (0.0722 * blue);
}

double contrastRatio(juce::Colour first, juce::Colour second)
{
    const auto lighter = std::max(relativeLuminance(first), relativeLuminance(second));
    const auto darker = std::min(relativeLuminance(first), relativeLuminance(second));
    return (lighter + 0.05) / (darker + 0.05);
}

void requireContrastAtLeast(juce::Colour foreground,
                            juce::Colour background,
                            double minimumRatio,
                            const std::string& message)
{
    require(contrastRatio(foreground, background) >= minimumRatio, message);
}

void requireNonEmptyAccessibilityTitle(juce::Component& root, const juce::String& componentId)
{
    auto* component = findDescendantById(root, componentId);
    require(component != nullptr, "Missing component ID for accessibility title: " + componentId.toStdString());
    require(component->getTitle().isNotEmpty(),
            "Component should expose a non-empty accessibility title: " + componentId.toStdString());
}

void requireNonEmptyAccessibilityDescription(juce::Component& root, const juce::String& componentId)
{
    auto* component = findDescendantById(root, componentId);
    require(component != nullptr, "Missing component ID for accessibility description: " + componentId.toStdString());
    require(component->getDescription().isNotEmpty(),
            "Component should expose a non-empty accessibility description: " + componentId.toStdString());
}

void requireNonEmptyAccessibilityHelpText(juce::Component& root, const juce::String& componentId)
{
    auto* component = findDescendantById(root, componentId);
    require(component != nullptr, "Missing component ID for accessibility help text: " + componentId.toStdString());
    require(component->getHelpText().isNotEmpty(),
            "Component should expose non-empty accessibility help text: " + componentId.toStdString());
}

void requireAccessibilityTitleEquals(juce::Component& root,
                                     const juce::String& componentId,
                                     const juce::String& expectedTitle)
{
    auto* component = findDescendantById(root, componentId);
    require(component != nullptr, "Missing component ID for accessibility title equality: " + componentId.toStdString());
    require(component->getTitle() == expectedTitle,
            "Component accessibility title should match the live text: " + componentId.toStdString());
}

void requireAccessibilityDescriptionContains(juce::Component& root,
                                             const juce::String& componentId,
                                             const juce::String& expectedFragment)
{
    auto* component = findDescendantById(root, componentId);
    require(component != nullptr,
            "Missing component ID for accessibility description check: " + componentId.toStdString());
    require(component->getDescription().contains(expectedFragment),
            "Component accessibility description should include the expected fragment: " + componentId.toStdString());
}

void requireAccessibilityHandlerState(juce::Component& root,
                                      const juce::String& componentId,
                                      bool expectedHandler)
{
    auto* component = findDescendantById(root, componentId);
    require(component != nullptr, "Missing component ID for accessibility handler: " + componentId.toStdString());
    require(component->isAccessible() == expectedHandler,
            "Unexpected accessibility visibility state for: " + componentId.toStdString());
}

void requireIncreasingFocusOrder(juce::Component& root,
                                 std::initializer_list<juce::String> componentIds)
{
    auto previousFocusOrder = -1;

    for (const auto& componentId : componentIds)
    {
        auto* component = findDescendantById(root, componentId);
        require(component != nullptr, "Missing component ID for focus-order check: " + componentId.toStdString());
        const auto currentFocusOrder = component->getExplicitFocusOrder();
        require(currentFocusOrder > 0,
                "Component should expose an explicit positive focus order: " + componentId.toStdString());
        require(currentFocusOrder > previousFocusOrder,
                "Component focus order should increase monotonically: " + componentId.toStdString());
        previousFocusOrder = currentFocusOrder;
    }
}

int countDescendantsById(juce::Component& root, const juce::String& componentId)
{
    auto count = root.getComponentID() == componentId ? 1 : 0;

    for (int index = 0; index < root.getNumChildComponents(); ++index)
        count += countDescendantsById(*root.getChildComponent(index), componentId);

    return count;
}

void collectComponentIdCounts(juce::Component& root,
                              std::map<std::string, int>& componentIdCounts)
{
    if (!root.getComponentID().isEmpty())
        ++componentIdCounts[root.getComponentID().toStdString()];

    for (int index = 0; index < root.getNumChildComponents(); ++index)
        collectComponentIdCounts(*root.getChildComponent(index), componentIdCounts);
}

void requireUniqueNonEmptyComponentIds(juce::Component& root)
{
    std::map<std::string, int> componentIdCounts;
    collectComponentIdCounts(root, componentIdCounts);

    std::vector<std::string> duplicateIds;
    for (const auto& [componentId, count] : componentIdCounts)
    {
        if (count > 1)
            duplicateIds.push_back(componentId + " x" + std::to_string(count));
    }

    require(duplicateIds.empty(),
            "Authoring panel should not expose duplicate non-empty component IDs. Duplicates: "
                + (duplicateIds.empty() ? std::string{} : duplicateIds.front()));
}

void requireRetiredTemporaryIdsAbsent(juce::Component& root)
{
    for (const auto& componentId : {
             juce::String("authoringModeSelector"),
             juce::String("authoringDrawerPlaceholder"),
             juce::String("authoringMacroSelector")
         })
    {
        require(findDescendantById(root, componentId) == nullptr,
                "Retired temporary component ID should remain absent: " + componentId.toStdString());
    }
}

bool componentTreeContainsLabelText(juce::Component& root, const juce::String& expectedText)
{
    if (auto* label = dynamic_cast<juce::Label*>(&root))
    {
        if (label->getText() == expectedText)
            return true;
    }

    for (int index = 0; index < root.getNumChildComponents(); ++index)
    {
        if (componentTreeContainsLabelText(*root.getChildComponent(index), expectedText))
            return true;
    }

    return false;
}

juce::Point<float> computeZoneMapPoint(const juce::Component& zoneMap,
                                       const drs::engine::AuthoringZoneSummary& zone)
{
    const auto inner = zoneMap.getLocalBounds().toFloat().reduced(12.0f);
    const auto x = inner.getX() + inner.getWidth() * (static_cast<float>(zone.keyLow) / 127.0f);
    const auto width = std::max(10.0f,
                                inner.getWidth() * (static_cast<float>(zone.keyHigh - zone.keyLow + 1) / 128.0f));
    const auto normalizedVelocityLow = 1.0f - (static_cast<float>(zone.velocityHigh) / 127.0f);
    const auto normalizedVelocityHigh = 1.0f - (static_cast<float>(zone.velocityLow) / 127.0f);
    const auto y = inner.getY() + inner.getHeight() * normalizedVelocityLow;
    const auto height = std::max(14.0f, inner.getHeight() * (normalizedVelocityHigh - normalizedVelocityLow));
    return {x + (width * 0.5f), y + (height * 0.5f)};
}

drs::engine::AuthoringZoneSummary makeZoneSummary(const drs::engine::RuntimeProjectZoneDefinition& zone)
{
    drs::engine::AuthoringZoneSummary summary;
    summary.id = zone.id;
    summary.displayName = zone.displayName;
    summary.sampleSourceId = zone.sampleSourceId;
    summary.articulationId = zone.articulationId;
    summary.rootKey = zone.rootKey;
    summary.keyLow = zone.keyLow;
    summary.keyHigh = zone.keyHigh;
    summary.velocityLow = zone.velocityLow;
    summary.velocityHigh = zone.velocityHigh;
    summary.velocityCrossfade = zone.velocityCrossfade;
    summary.gainDb = zone.gainDb;
    summary.pan = zone.pan;
    summary.loopEnabled = zone.loopEnabled;
    summary.triggerMode = zone.triggerMode;
    summary.selected = true;
    return summary;
}

juce::Point<float> computeZoneMapHandlePoint(const juce::Component& zoneMap,
                                             const drs::engine::AuthoringZoneSummary& zone,
                                             drs::app::authoring::ZoneMapCanvas::RangeHandle handle)
{
    const auto inner = zoneMap.getLocalBounds().toFloat().reduced(12.0f);
    const auto x = inner.getX() + inner.getWidth() * (static_cast<float>(zone.keyLow) / 127.0f);
    const auto width = std::max(10.0f,
                                inner.getWidth() * (static_cast<float>(zone.keyHigh - zone.keyLow + 1) / 128.0f));
    const auto normalizedVelocityLow = 1.0f - (static_cast<float>(zone.velocityHigh) / 127.0f);
    const auto normalizedVelocityHigh = 1.0f - (static_cast<float>(zone.velocityLow) / 127.0f);
    const auto y = inner.getY() + inner.getHeight() * normalizedVelocityLow;
    const auto height = std::max(14.0f, inner.getHeight() * (normalizedVelocityHigh - normalizedVelocityLow));
    const auto bounds = juce::Rectangle<float>(x, y, width, height);

    switch (handle)
    {
        case drs::app::authoring::ZoneMapCanvas::RangeHandle::keyLow:
            return {bounds.getX(), bounds.getCentreY()};
        case drs::app::authoring::ZoneMapCanvas::RangeHandle::keyHigh:
            return {bounds.getRight(), bounds.getCentreY()};
        case drs::app::authoring::ZoneMapCanvas::RangeHandle::velocityHigh:
            return {bounds.getCentreX(), bounds.getY()};
        case drs::app::authoring::ZoneMapCanvas::RangeHandle::velocityLow:
            return {bounds.getCentreX(), bounds.getBottom()};
        case drs::app::authoring::ZoneMapCanvas::RangeHandle::none:
        default:
            return bounds.getCentre();
    }
}

juce::Point<float> computeZoneMapTargetPointForKey(const juce::Component& zoneMap,
                                                   const drs::engine::AuthoringZoneSummary& zone,
                                                   int midiKey)
{
    const auto inner = zoneMap.getLocalBounds().toFloat().reduced(12.0f);
    const auto normalizedKey = juce::jlimit(0.0f, 127.0f, static_cast<float>(midiKey)) / 127.0f;
    const auto x = inner.getX() + inner.getWidth() * normalizedKey;
    return {x, computeZoneMapPoint(zoneMap, zone).y};
}

juce::Point<float> computeZoneMapTargetPointForVelocity(const juce::Component& zoneMap,
                                                        const drs::engine::AuthoringZoneSummary& zone,
                                                        int velocity)
{
    const auto inner = zoneMap.getLocalBounds().toFloat().reduced(12.0f);
    const auto clampedVelocity = juce::jlimit(1.0f, 127.0f, static_cast<float>(velocity));
    const auto normalizedVelocity = 1.0f - (clampedVelocity / 127.0f);
    const auto y = inner.getY() + inner.getHeight() * normalizedVelocity;
    return {computeZoneMapPoint(zoneMap, zone).x, y};
}

std::string describeBounds(juce::Component& root, const juce::String& componentId)
{
    auto* component = findDescendantById(root, componentId);
    if (component == nullptr)
        return componentId.toStdString() + ": missing";

    const auto bounds = component->getBounds();
    return componentId.toStdString()
        + ": x=" + std::to_string(bounds.getX())
        + ", y=" + std::to_string(bounds.getY())
        + ", w=" + std::to_string(bounds.getWidth())
        + ", h=" + std::to_string(bounds.getHeight())
        + ", visible=" + std::string(component->isVisible() ? "true" : "false");
}

drs::app::AuthoringWaveformPreview makePreviewFixture()
{
    drs::app::AuthoringWaveformPreview preview;
    preview.available = true;
    preview.state = "ready";
    preview.sourcePath = "fixtures/phase2/lead-a4-sustain.wav";
    preview.formatName = "WAV";
    preview.durationSeconds = 2.418;
    preview.sampleRate = 48000.0;
    preview.frameCount = 116064;
    preview.channelCount = 2;
    preview.loopEnabled = true;
    preview.loopStartFrame = 12000;
    preview.loopEndFrame = 88000;

    preview.points.reserve(96);
    for (int index = 0; index < 96; ++index)
    {
        const auto phase = static_cast<float>(index % 16) / 15.0f;
        const auto amplitude = 0.2f + static_cast<float>(index % 5) * 0.12f;
        preview.points.push_back({-amplitude * (0.3f + phase * 0.7f), amplitude});
    }

    return preview;
}

drs::app::AuthoringImportResponsivenessSnapshot makeImportMetricsFixture()
{
    drs::app::AuthoringImportResponsivenessSnapshot metrics;
    metrics.available = true;
    metrics.state = "ready";
    metrics.totalItemCount = 24;
    metrics.processedCount = 24;
    metrics.warningItemCount = 1;
    metrics.failedItemCount = 0;
    metrics.lastProcessDurationMicros = 410;
    metrics.averageProcessDurationMicros = 320;
    metrics.maxProcessDurationMicros = 870;
    metrics.lastProcessedItemId = "lead-a4-sustain";
    return metrics;
}

drs::engine::DraftPlaybackStatus makeDraftPlaybackStatusFixture()
{
    drs::engine::DraftPlaybackStatus status;
    status.draftRevision = 4;
    status.preview.available = true;
    status.preview.revision = 3;
    status.preview.state = "Stale";
    status.performance.available = true;
    status.performance.revision = 2;
    status.performance.state = "Active";
    return status;
}

drs::app::AuthoringPreviewStatusSnapshot makeAuthoringPreviewStatusFixture()
{
    drs::app::AuthoringPreviewStatusSnapshot status;
    status.available = true;
    status.draftRevision = 4;
    status.activeRevision = 3;
    status.stateLabel = "Failed";
    status.failureState = "Selected authoring sample could not be prepared.";
    status.blockingPrerequisite = "Relink or re-import the selected sample file.";
    status.blockingGuidance = "Restore the sample file for the selected zone, then prepare the authoring preview again.";
    return status;
}

void saveComponentPng(juce::Component& component, const fs::path& path)
{
    juce::Image image(juce::Image::ARGB, component.getWidth(), component.getHeight(), true);
    juce::Graphics graphics(image);
    component.paintEntireComponent(graphics, true);

    juce::PNGImageFormat pngFormat;
    juce::File targetFile(path.string());
    targetFile.getParentDirectory().createDirectory();
    juce::FileOutputStream output(targetFile);
    require(output.openedOk(), "Could not create diagnostic image: " + path.string());
    require(pngFormat.writeImageToStream(image, output), "Could not write diagnostic image: " + path.string());
}

void exerciseSummaryStripLeaf(const fs::path& outputDirectory)
{
    drs::app::authoring::AuthoringSummaryStrip strip;

    int previewRequests = 0;
    int prepareDraftRequests = 0;
    int publishDraftRequests = 0;
    int undoRequests = 0;
    int redoRequests = 0;
    int saveRequests = 0;
    drs::app::authoring::SelectionSummaryCallbacks callbacks;
    callbacks.onPreviewRequested = [&previewRequests] { ++previewRequests; };
    callbacks.onPrepareDraftPlaybackRequested = [&prepareDraftRequests] { ++prepareDraftRequests; };
    callbacks.onPublishDraftPlaybackRequested = [&publishDraftRequests] { ++publishDraftRequests; };
    callbacks.onUndoRequested = [&undoRequests] { ++undoRequests; };
    callbacks.onRedoRequested = [&redoRequests] { ++redoRequests; };
    callbacks.onMarkSavedRequested = [&saveRequests] { ++saveRequests; };
    strip.setCallbacks(std::move(callbacks));

    drs::app::authoring::SelectionSummaryViewModel viewModel;
    viewModel.title = "Lead Sustain";
    viewModel.statusText = "Selected zone is ready to preview";
    viewModel.sourceText = "Source: fixtures/phase2/lead-a4-sustain.wav";
    viewModel.articulationText = "Articulation: sustain";
    viewModel.playbackText = "Draft playback: draft r4 | preview r3 (Stale) | published r2 (Active)";
    viewModel.canPreview = true;
    viewModel.canPrepareDraftPlayback = true;
    viewModel.canPublishDraftPlayback = false;
    viewModel.canUndo = true;
    viewModel.canRedo = true;
    viewModel.dirty = true;

    strip.setViewModel(viewModel);
    strip.setTopLeftPosition(0, 0);
    strip.setSize(760, drs::app::authoring::heroHeight);
    strip.setVisible(true);
    strip.resized();

    const auto bounds = strip.getLocalBounds();
    requireComponentVisibleWithin(strip, "authoringSummaryStrip", bounds);
    requireComponentVisibleWithin(strip, "authoringPreviewButton", bounds);
    requireComponentVisibleWithin(strip, "authoringPrepareDraftButton", bounds);
    requireComponentVisibleWithin(strip, "authoringPublishDraftButton", bounds);
    requireComponentVisibleWithin(strip, "authoringUndoButton", bounds);
    requireComponentVisibleWithin(strip, "authoringRedoButton", bounds);
    requireComponentVisibleWithin(strip, "authoringSaveButton", bounds);
    requireAccessibilityTitleEquals(strip, "authoringSummaryTitleLabel", "Lead Sustain");
    requireAccessibilityTitleEquals(strip, "authoringSummaryStatusLabel", "Selected zone is ready to preview");
    requireAccessibilityTitleEquals(strip,
                                    "authoringSummarySourceLabel",
                                    "Source: fixtures/phase2/lead-a4-sustain.wav");
    requireAccessibilityTitleEquals(strip, "authoringSummaryArticulationLabel", "Articulation: sustain");
    requireAccessibilityTitleEquals(strip,
                                    "authoringSummaryPlaybackLabel",
                                    "Draft playback: draft r4 | preview r3 (Stale) | published r2 (Active)");
    requireAccessibilityDescriptionContains(strip, "authoringSummaryTitleLabel", "Selected zone title: Lead Sustain");
    requireAccessibilityDescriptionContains(strip,
                                            "authoringSummaryStatusLabel",
                                            "Selection status: Selected zone is ready to preview");
    requireAccessibilityDescriptionContains(strip,
                                            "authoringSummarySourceLabel",
                                            "Selected zone source: Source: fixtures/phase2/lead-a4-sustain.wav");
    requireAccessibilityDescriptionContains(strip,
                                            "authoringSummaryArticulationLabel",
                                            "Selected zone articulation: Articulation: sustain");
    requireAccessibilityDescriptionContains(strip,
                                            "authoringSummaryPlaybackLabel",
                                            "Playback revision state: Draft playback: draft r4 | preview r3 (Stale) | published r2 (Active)");
    requireAccessibilityDescriptionContains(strip, "authoringPreviewButton", "Previews the selected zone.");
    requireAccessibilityDescriptionContains(strip, "authoringPrepareDraftButton", "Builds the latest draft for playback preview.");
    requireAccessibilityDescriptionContains(strip, "authoringPublishDraftButton", "Unavailable because the latest draft is not ready to publish yet.");
    requireAccessibilityDescriptionContains(strip, "authoringUndoButton", "Reverts the most recent authoring change.");
    requireAccessibilityDescriptionContains(strip, "authoringRedoButton", "Reapplies the most recently undone authoring change.");
    requireAccessibilityDescriptionContains(strip, "authoringSaveButton", "Marks the current authoring state as saved.");
    requireNonEmptyAccessibilityHelpText(strip, "authoringPreviewButton");
    requireNonEmptyAccessibilityHelpText(strip, "authoringPrepareDraftButton");
    requireNonEmptyAccessibilityHelpText(strip, "authoringPublishDraftButton");
    requireNonEmptyAccessibilityHelpText(strip, "authoringUndoButton");
    requireNonEmptyAccessibilityHelpText(strip, "authoringRedoButton");
    requireNonEmptyAccessibilityHelpText(strip, "authoringSaveButton");
    require(requireButton(strip, "authoringPreviewButton").isEnabled(),
            "Summary strip preview button should reflect the fixture view model.");
    require(requireButton(strip, "authoringPrepareDraftButton").isEnabled(),
            "Summary strip prepare-draft button should reflect the fixture view model.");
    require(!requireButton(strip, "authoringPublishDraftButton").isEnabled(),
            "Summary strip publish-draft button should reflect the fixture view model.");
    require(requireButton(strip, "authoringUndoButton").isEnabled(),
            "Summary strip undo button should reflect the fixture view model.");
    require(requireButton(strip, "authoringRedoButton").isEnabled(),
            "Summary strip redo button should reflect the fixture view model.");

    require(static_cast<bool>(requireButton(strip, "authoringPreviewButton").onClick),
            "Summary strip preview button should expose an onClick callback.");
    requireButton(strip, "authoringPreviewButton").onClick();
    requireButton(strip, "authoringPrepareDraftButton").onClick();
    requireButton(strip, "authoringUndoButton").onClick();
    requireButton(strip, "authoringRedoButton").onClick();
    requireButton(strip, "authoringSaveButton").onClick();

    require(previewRequests == 1, "Summary strip should emit exactly one preview callback.");
    require(prepareDraftRequests == 1, "Summary strip should emit exactly one prepare-draft callback.");
    require(publishDraftRequests == 0, "Summary strip should not emit publish-draft callbacks while disabled.");
    require(undoRequests == 1, "Summary strip should emit exactly one undo callback.");
    require(redoRequests == 1, "Summary strip should emit exactly one redo callback.");
    require(saveRequests == 1, "Summary strip should emit exactly one save callback.");

    viewModel.canPreview = false;
    viewModel.canPrepareDraftPlayback = false;
    viewModel.canPublishDraftPlayback = true;
    viewModel.canUndo = false;
    viewModel.canRedo = false;
    viewModel.dirty = false;
    strip.setViewModel(viewModel);
    require(!requireButton(strip, "authoringPreviewButton").isEnabled(),
            "Summary strip preview button should disable when preview is unavailable.");
    require(!requireButton(strip, "authoringPrepareDraftButton").isEnabled(),
            "Summary strip prepare-draft button should disable when preparation is unavailable.");
    require(requireButton(strip, "authoringPublishDraftButton").isEnabled(),
            "Summary strip publish-draft button should enable when the latest draft is ready to publish.");
    require(!requireButton(strip, "authoringUndoButton").isEnabled(),
            "Summary strip undo button should disable when undo is unavailable.");
    require(!requireButton(strip, "authoringRedoButton").isEnabled(),
            "Summary strip redo button should disable when redo is unavailable.");
    requireAccessibilityDescriptionContains(strip,
                                            "authoringPreviewButton",
                                            "Unavailable because no zone preview is available.");
    requireAccessibilityDescriptionContains(strip,
                                            "authoringPrepareDraftButton",
                                            "Unavailable because the current draft cannot be prepared for playback yet.");
    requireAccessibilityDescriptionContains(strip,
                                            "authoringPublishDraftButton",
                                            "Publishes the latest prepared draft to the performance path.");
    requireAccessibilityDescriptionContains(strip,
                                            "authoringUndoButton",
                                            "Unavailable because there is no change to undo.");
    requireAccessibilityDescriptionContains(strip,
                                            "authoringRedoButton",
                                            "Unavailable because there is no change to redo.");
    requireAccessibilityDescriptionContains(strip,
                                            "authoringSaveButton",
                                            "Project is already marked saved.");
    requireAccessibilityDescriptionContains(strip,
                                            "authoringSaveButton",
                                            "Project is already marked saved.");
    require(requireButton(strip, "authoringSaveButton").getHelpText().contains("Make a change before marking a new saved state."),
            "Summary strip save button should explain when the project is already saved.");
    requireButton(strip, "authoringPublishDraftButton").onClick();
    require(publishDraftRequests == 1, "Summary strip should emit exactly one publish-draft callback once enabled.");

    saveComponentPng(strip, outputDirectory / "leaf-summary-strip.png");
}

void exerciseZoneMappingEditorLeaf(const fs::path& outputDirectory)
{
    drs::app::authoring::ZoneMappingEditor editor;
    editor.setTopLeftPosition(0, 0);
    editor.setSize(360, 560);
    editor.setVisible(true);

    drs::app::authoring::ZoneFieldValuesViewModel emptyViewModel;
    emptyViewModel.hasSelection = false;
    emptyViewModel.emptyStateText = "Select a zone to edit mapping fields.";
    editor.setViewModel(emptyViewModel);
    editor.resized();

    requireComponentVisibleWithin(editor, "authoringZoneFieldEditor", editor.getLocalBounds());
    requireComponentVisible(editor, "authoringZoneFieldEmptyState");
    require(!requireSlider(editor, "authoringRootKeySlider").isShowing(),
            "Zone mapping editor should hide mapping controls when no zone is selected.");
    requireAccessibilityHandlerState(editor, "authoringZoneFieldEmptyState", true);
    requireAccessibilityHandlerState(editor, "authoringRootKeySlider", false);

    int commitRequests = 0;
    int restoreRequests = 0;
    std::string lastCommitLabel;
    drs::app::authoring::ZoneFieldValuesViewModel lastCommittedValues;

    drs::app::authoring::ZoneFieldCallbacks callbacks;
    callbacks.onCommitRequested = [&](const drs::app::authoring::ZoneFieldValuesViewModel& values,
                                      const std::string& label)
    {
        ++commitRequests;
        lastCommitLabel = label;
        lastCommittedValues = values;
    };
    callbacks.onRestoreRootKeyRequested = [&restoreRequests]
    {
        ++restoreRequests;
    };
    editor.setCallbacks(std::move(callbacks));

    drs::app::authoring::ZoneFieldValuesViewModel populatedViewModel;
    populatedViewModel.hasSelection = true;
    populatedViewModel.rootKey = 60;
    populatedViewModel.keyLow = 48;
    populatedViewModel.keyHigh = 72;
    populatedViewModel.velocityLow = 8;
    populatedViewModel.velocityHigh = 120;
    populatedViewModel.gainDb = -3.5;
    populatedViewModel.pan = 0.25;
    populatedViewModel.loopEnabled = false;
    editor.setViewModel(populatedViewModel);
    editor.resized();

    const auto bounds = editor.getLocalBounds();
    require(countDescendantsById(editor, "authoringRootKeySlider") == 1,
            "Zone mapping editor should expose one root-key control after removing old mapping rows.");
    require(countDescendantsById(editor, "authoringKeyLowSlider") == 1,
            "Zone mapping editor should expose one key-low control after removing old mapping rows.");
    require(countDescendantsById(editor, "authoringKeyHighSlider") == 1,
            "Zone mapping editor should expose one key-high control after removing old mapping rows.");
    require(countDescendantsById(editor, "authoringVelocityLowSlider") == 1,
            "Zone mapping editor should expose one velocity-low control after removing old mapping rows.");
    require(countDescendantsById(editor, "authoringVelocityHighSlider") == 1,
            "Zone mapping editor should expose one velocity-high control after removing old mapping rows.");
    require(countDescendantsById(editor, "authoringGainSlider") == 1,
            "Zone mapping editor should expose one gain control after removing old mapping rows.");
    require(countDescendantsById(editor, "authoringPanSlider") == 1,
            "Zone mapping editor should expose one pan control after removing old mapping rows.");
    require(countDescendantsById(editor, "authoringLoopEnabledToggle") == 1,
            "Zone mapping editor should expose one loop toggle after removing old mapping rows.");
    require(countDescendantsById(editor, "authoringTriggerModeSelector") == 1,
            "Zone mapping editor should expose one trigger-mode selector.");
    require(countDescendantsById(editor, "authoringRestoreRootKeyButton") == 1,
            "Zone mapping editor should expose one restore-root-key action after removing old mapping rows.");

    for (const auto& componentId : {
             juce::String("authoringMapInspectorSection"),
             juce::String("authoringMapInspectorSectionDisclosure"),
             juce::String("authoringRootKeySlider"),
             juce::String("authoringKeyLowSlider"),
             juce::String("authoringKeyHighSlider"),
             juce::String("authoringSampleInspectorSection"),
             juce::String("authoringSampleInspectorSectionDisclosure"),
             juce::String("authoringMixInspectorSection"),
             juce::String("authoringMixInspectorSectionDisclosure"),
             juce::String("authoringAdvancedInspectorSection"),
             juce::String("authoringAdvancedInspectorSectionDisclosure"),
             juce::String("authoringTriggerModeSelector"),
             juce::String("authoringRestoreRootKeyButton")
         })
    {
        requireNonEmptyAccessibilityTitle(editor, componentId);
        requireNonEmptyAccessibilityDescription(editor, componentId);
    }

    requireNonEmptyAccessibilityHelpText(editor, "authoringRootKeySlider");
    requireNonEmptyAccessibilityHelpText(editor, "authoringKeyLowSlider");
    requireNonEmptyAccessibilityHelpText(editor, "authoringKeyHighSlider");
    requireNonEmptyAccessibilityHelpText(editor, "authoringTriggerModeSelector");
    requireNonEmptyAccessibilityHelpText(editor, "authoringRestoreRootKeyButton");
    requireIncreasingFocusOrder(editor,
                                {
                                    "authoringMapInspectorSectionDisclosure",
                                    "authoringRootKeySlider",
                                    "authoringKeyLowSlider",
                                    "authoringKeyHighSlider",
                                    "authoringSampleInspectorSectionDisclosure",
                                    "authoringVelocityLowSlider",
                                    "authoringVelocityHighSlider",
                                    "authoringMixInspectorSectionDisclosure",
                                    "authoringGainSlider",
                                    "authoringPanSlider",
                                    "authoringAdvancedInspectorSectionDisclosure",
                                    "authoringLoopEnabledToggle",
                                    "authoringTriggerModeSelector",
                                    "authoringRestoreRootKeyButton"
                                });

    requireAccessibilityHandlerState(editor, "authoringMapInspectorSection", true);
    requireAccessibilityHandlerState(editor, "authoringMapInspectorSectionDisclosure", true);
    requireAccessibilityHandlerState(editor, "authoringRootKeySlider", true);
    requireAccessibilityHandlerState(editor, "authoringKeyLowSlider", true);
    requireAccessibilityHandlerState(editor, "authoringKeyHighSlider", true);
    requireAccessibilityHandlerState(editor, "authoringVelocityLowSlider", false);
    requireAccessibilityHandlerState(editor, "authoringVelocityHighSlider", false);
    requireAccessibilityHandlerState(editor, "authoringGainSlider", false);
    requireAccessibilityHandlerState(editor, "authoringPanSlider", false);
    requireAccessibilityHandlerState(editor, "authoringLoopEnabledToggle", false);
    requireAccessibilityHandlerState(editor, "authoringTriggerModeSelector", false);
    requireAccessibilityHandlerState(editor, "authoringRestoreRootKeyButton", false);

    for (const auto& componentId : {
             juce::String("authoringMapInspectorSection"),
             juce::String("authoringSampleInspectorSection"),
             juce::String("authoringMixInspectorSection"),
             juce::String("authoringAdvancedInspectorSection"),
             juce::String("authoringMapInspectorSectionDisclosure"),
             juce::String("authoringSampleInspectorSectionDisclosure"),
             juce::String("authoringMixInspectorSectionDisclosure"),
             juce::String("authoringAdvancedInspectorSectionDisclosure")
         })
    {
        requireComponentVisibleWithin(editor, componentId, bounds);
    }

    for (const auto& componentId : {
             juce::String("authoringRootKeySlider"),
             juce::String("authoringKeyLowSlider"),
             juce::String("authoringKeyHighSlider")
         })
    {
        requireComponentVisibleWithin(editor, componentId, bounds);
    }

    if (findDescendantById(editor, "authoringVelocityLowSlider")->getBounds().isEmpty())
        requireButton(editor, "authoringSampleInspectorSectionDisclosure").onClick();
    requireComponentVisibleWithin(editor, "authoringVelocityLowSlider", bounds);
    requireComponentVisibleWithin(editor, "authoringVelocityHighSlider", bounds);
    requireAccessibilityHandlerState(editor, "authoringVelocityLowSlider", true);
    requireAccessibilityHandlerState(editor, "authoringVelocityHighSlider", true);
    requireButton(editor, "authoringSampleInspectorSectionDisclosure").onClick();
    requireAccessibilityHandlerState(editor, "authoringVelocityLowSlider", false);
    requireAccessibilityHandlerState(editor, "authoringVelocityHighSlider", false);

    if (findDescendantById(editor, "authoringGainSlider")->getBounds().isEmpty())
        requireButton(editor, "authoringMixInspectorSectionDisclosure").onClick();
    requireComponentVisibleWithin(editor, "authoringGainSlider", bounds);
    requireComponentVisibleWithin(editor, "authoringPanSlider", bounds);
    requireAccessibilityHandlerState(editor, "authoringGainSlider", true);
    requireAccessibilityHandlerState(editor, "authoringPanSlider", true);

    if (findDescendantById(editor, "authoringRestoreRootKeyButton")->getBounds().isEmpty())
        requireButton(editor, "authoringAdvancedInspectorSectionDisclosure").onClick();
    requireComponentVisibleWithin(editor, "authoringLoopEnabledToggle", bounds);
    requireComponentVisibleWithin(editor, "authoringTriggerModeSelector", bounds);
    requireComponentVisibleWithin(editor, "authoringRestoreRootKeyButton", bounds);
    requireComponentVisibleWithin(editor, "authoringZoneValidationMessage", bounds);
    requireAccessibilityHandlerState(editor, "authoringLoopEnabledToggle", true);
    requireAccessibilityHandlerState(editor, "authoringTriggerModeSelector", true);
    requireAccessibilityHandlerState(editor, "authoringRestoreRootKeyButton", true);

    auto& rootKeySlider = requireSlider(editor, "authoringRootKeySlider");
    require(static_cast<bool>(rootKeySlider.onDragStart),
            "Zone mapping editor root key slider should expose a drag-start commit callback.");
    rootKeySlider.setValue(67.0, juce::dontSendNotification);
    require(static_cast<bool>(rootKeySlider.onDragEnd),
            "Zone mapping editor root key slider should expose a drag-end commit callback.");
    rootKeySlider.onDragStart();
    rootKeySlider.onDragEnd();
    require(commitRequests == 1, "Zone mapping editor should commit root key edits.");
    require(lastCommitLabel == "Update zone root key", "Zone mapping editor should label root key edits.");
    require(lastCommittedValues.rootKey == 67, "Zone mapping editor should report the edited root key.");

    auto& panSlider = requireSlider(editor, "authoringPanSlider");
    panSlider.setValue(-0.4, juce::dontSendNotification);
    require(static_cast<bool>(panSlider.onDragEnd),
            "Zone mapping editor pan slider should expose a drag-end commit callback.");
    panSlider.onDragStart();
    panSlider.onDragEnd();
    require(commitRequests == 2, "Zone mapping editor should commit pan edits.");
    require(lastCommitLabel == "Update zone pan", "Zone mapping editor should label pan edits.");
    require(std::abs(lastCommittedValues.pan - (-0.4)) < 0.001,
            "Zone mapping editor should report the edited pan value.");

    auto& rootKeyTextBox = requireSliderTextBoxLabel(rootKeySlider);
    rootKeySlider.showTextBox();
    auto* rootKeyEditor = rootKeyTextBox.getCurrentTextEditor();
    require(rootKeyEditor != nullptr, "Zone mapping editor root key slider should show an editable text box.");
    rootKeyEditor->setText("64", false);
    require(commitRequests == 2,
            "Zone mapping editor should not commit root key text edits until the editor is dismissed.");
    rootKeyTextBox.hideEditor(false);
    require(commitRequests == 3, "Zone mapping editor should commit root key text edits when editing completes.");
    require(lastCommitLabel == "Update zone root key", "Zone mapping editor should preserve root key commit labels.");
    require(lastCommittedValues.rootKey == 64, "Zone mapping editor should report committed text-entry values.");

    auto& keyLowSlider = requireSlider(editor, "authoringKeyLowSlider");
    keyLowSlider.setValue(90.0, juce::dontSendNotification);
    keyLowSlider.onDragStart();
    keyLowSlider.onDragEnd();
    require(commitRequests == 4, "Zone mapping editor should commit key-range edits.");
    require(lastCommitLabel == "Update zone key range", "Zone mapping editor should label key-range edits.");
    require(lastCommittedValues.keyLow == 72 && lastCommittedValues.keyHigh == 90,
            "Zone mapping editor should normalize inverted key ranges before commit.");
    require(static_cast<int>(keyLowSlider.getValue()) == 72
                && static_cast<int>(requireSlider(editor, "authoringKeyHighSlider").getValue()) == 90,
            "Zone mapping editor should reflect normalized key ranges in the visible controls.");
    require(requireMessageText(editor, "authoringZoneValidationMessage")
                == "Key range was normalized to keep Low <= High.",
            "Zone mapping editor should explain when key ranges are normalized.");

    auto& velocityLowSlider = requireSlider(editor, "authoringVelocityLowSlider");
    velocityLowSlider.setValue(126.0, juce::dontSendNotification);
    velocityLowSlider.onDragStart();
    velocityLowSlider.onDragEnd();
    require(commitRequests == 5, "Zone mapping editor should commit velocity-range edits.");
    require(lastCommitLabel == "Update zone velocity range", "Zone mapping editor should label velocity-range edits.");
    require(lastCommittedValues.velocityLow == 120 && lastCommittedValues.velocityHigh == 126,
            "Zone mapping editor should normalize inverted velocity ranges before commit.");
    require(static_cast<int>(velocityLowSlider.getValue()) == 120
                && static_cast<int>(requireSlider(editor, "authoringVelocityHighSlider").getValue()) == 126,
            "Zone mapping editor should reflect normalized velocity ranges in the visible controls.");
    require(requireMessageText(editor, "authoringZoneValidationMessage")
                == "Velocity range was normalized to keep Low <= High.",
            "Zone mapping editor should explain when velocity ranges are normalized.");

    auto& loopToggle = requireToggleButton(editor, "authoringLoopEnabledToggle");
    loopToggle.setToggleState(true, juce::dontSendNotification);
    require(static_cast<bool>(loopToggle.onClick),
            "Zone mapping editor loop toggle should expose an onClick callback.");
    loopToggle.onClick();
    require(commitRequests == 6, "Zone mapping editor should commit loop toggle edits.");
    require(lastCommitLabel == "Toggle zone loop", "Zone mapping editor should label loop edits.");
    require(lastCommittedValues.loopEnabled, "Zone mapping editor should report the toggled loop state.");

    auto& triggerModeSelector = requireComboBox(editor, "authoringTriggerModeSelector");
    require(triggerModeSelector.getSelectedId() == 1,
            "Zone mapping editor should default selected zones to gated trigger mode.");
    triggerModeSelector.setSelectedId(2, juce::sendNotificationSync);
    require(commitRequests == 7, "Zone mapping editor should commit trigger-mode edits.");
    require(lastCommitLabel == "Update zone trigger mode",
            "Zone mapping editor should label trigger-mode edits.");
    require(lastCommittedValues.triggerMode == drs::engine::ZoneTriggerMode::oneShot,
            "Zone mapping editor should report one-shot trigger mode.");

    requireButton(editor, "authoringRestoreRootKeyButton").onClick();
    require(restoreRequests == 1, "Zone mapping editor should emit restore-root-key callbacks.");

    saveComponentPng(editor, outputDirectory / "leaf-zone-mapping-editor.png");
}

void writeReachabilityChecklist(std::ostream& inventory)
{
    inventory << "Reachability checklist\n";
    inventory << "- Summary strip: authoringSummaryStrip, authoringPreviewButton, authoringPrepareDraftButton, authoringPublishDraftButton, authoringUndoButton, authoringRedoButton, authoringSaveButton\n";
    inventory << "- Playback banner: authoringPlaybackBanner, authoringPlaybackBannerLabel, authoringPlaybackBannerPrepareButton, authoringPlaybackBannerPublishButton\n";
    inventory << "- Toolbar row: authoringZoneSelector\n";
    inventory << "- Persistent map: authoringZoneMap\n";
    inventory << "- Drawer host: authoringDrawer, authoringDrawerTabStrip, authoringDrawerToggleButton\n";
    inventory << "- Drawer tabs: authoringDrawerWaveformTab, authoringDrawerMacrosTab, authoringDrawerRoutingTab, authoringDrawerPerformanceTab\n";
    inventory << "- Mapping inspector: authoringRootKeySlider, authoringKeyLowSlider, authoringKeyHighSlider, authoringVelocityLowSlider, authoringVelocityHighSlider, authoringGainSlider, authoringPanSlider, authoringLoopEnabledToggle, authoringTriggerModeSelector, authoringRestoreRootKeyButton\n";
    inventory << "- Drawer context: authoringDrawerTitleLabel, authoringDrawerScopeLabel, authoringDrawerBreadcrumbLabel\n";
    inventory << "- Waveform drawer content: authoringWaveformPreview, authoringWaveformStatusLabel, authoringWaveformInfoLabel, authoringWaveformLoopLabel, authoringWaveformImportLabel\n";
    inventory << "- Macros drawer content: authoringMacroList, authoringMacroListBox, authoringMacroAssignmentSelector, authoringMacroRoleSelector, authoringMacroDefaultSlider, authoringMacroMinSlider, authoringMacroMaxSlider, authoringMacroMoveUpButton, authoringMacroMoveDownButton\n";
    inventory << "- Routing drawer content: authoringFxSelector, authoringFxTypeSelector, authoringFxBypassedToggle, authoringRoutingSelector, authoringRoutingInputSelector, authoringRoutingInsertOneSelector, authoringRoutingInsertTwoSelector\n";
    inventory << "- Performance drawer content: authoringPerformanceBankSelector, authoringTriggerSlotSelector, authoringTriggerEventSelector, authoringTargetArticulationSelector, authoringPhraseAssetSelector, authoringChordModeSelector, authoringPhraseImportPath, authoringPhraseImportButton\n";
    inventory << "- Retired temporary IDs absent: authoringModeSelector, authoringDrawerPlaceholder, authoringMacroSelector\n";
    inventory << "\n";
}

void exerciseRepeatedStructureListComponent()
{
    drs::app::authoring::RepeatedStructureList list("testRepeatedStructureList",
                                                    "testRepeatedStructureListBox",
                                                    "testRepeatedStructureEmptyState");
    list.setBounds(0, 0, 320, 160);

    auto& listBox = list.getListBox();
    int selectionCallbackCount = 0;
    int lastSelection = -1;
    list.setOnSelectionChanged([&](int selectedIndex)
    {
        ++selectionCallbackCount;
        lastSelection = selectedIndex;
    });

    drs::app::authoring::RepeatedStructureListViewModel emptyStateModel;
    emptyStateModel.emptyStateText = "No project rows yet.";
    list.setViewModel(emptyStateModel);

    require(!listBox.isVisible(),
            "Repeated structure list should expose its empty-state message when no rows exist.");
    require(requireMessageText(list, "testRepeatedStructureEmptyState") == "No project rows yet.",
            "Repeated structure list should surface the configured empty-state message.");

    drs::app::authoring::RepeatedStructureListViewModel populatedModel;
    populatedModel.selectedIndex = 0;
    populatedModel.rows = {
        {"macro-a", "Macro A", "timbre | Filter cutoff", true},
        {"macro-b", "Macro B", "motion | Voice pitch", true}
    };
    list.setViewModel(populatedModel);

    require(listBox.isVisible(),
            "Repeated structure list should show rows once real project entities are provided.");
    require(list.getRowCount() == 2,
            "Repeated structure list should report the real project row count.");
    require(listBox.getSelectedRow() == 0,
            "Repeated structure list should honor the selected row from its view model.");
    require(listBox.keyPressed(juce::KeyPress(juce::KeyPress::downKey)),
            "Repeated structure list should consume down-arrow keyboard navigation.");
    require(listBox.getSelectedRow() == 1,
            "Repeated structure list should advance selection when navigating with the keyboard.");
    require(selectionCallbackCount >= 1 && lastSelection == 1,
            "Repeated structure list should report keyboard-driven selection changes.");
}

void exerciseSurface(drs::app::AuthoringPanel& panel,
                     int surfaceId,
                     const std::string& shellName,
                     const std::string& surfaceName,
                     const fs::path& outputDirectory,
                     std::ostream& inventory,
                     std::vector<std::string>& baselineFindings)
{
    const auto panelBounds = panel.getLocalBounds();
    requireComponentVisibleWithin(panel, "authoringWorkspace", panelBounds);
    requireComponentVisibleWithin(panel, "authoringZoneSelector", panelBounds);
    requireComponentVisibleWithin(panel, "authoringPlaybackBanner", panelBounds);
    requireComponentVisibleWithin(panel, "authoringPlaybackBannerLabel", panelBounds);
    requireComponentVisibleWithin(panel, "authoringPlaybackBannerPrepareButton", panelBounds);
    requireComponentVisibleWithin(panel, "authoringZoneMap", panelBounds);
    requireComponentVisibleWithin(panel, "authoringDrawer", panelBounds);
    requireComponentVisibleWithin(panel, "authoringDrawerTabStrip", panelBounds);
    requireComponentVisibleWithin(panel, "authoringDrawerToggleButton", panelBounds);
    requireComponentVisibleWithin(panel, "authoringDrawerWaveformTab", panelBounds);
    requireComponentVisibleWithin(panel, "authoringDrawerMacrosTab", panelBounds);
    requireComponentVisibleWithin(panel, "authoringDrawerRoutingTab", panelBounds);
    requireComponentVisibleWithin(panel, "authoringDrawerPerformanceTab", panelBounds);
    requireComponentVisibleWithin(panel, "authoringPreviewButton", panelBounds);
    requireComponentVisibleWithin(panel, "authoringUndoButton", panelBounds);
    requireComponentVisibleWithin(panel, "authoringRedoButton", panelBounds);
    requireComponentVisibleWithin(panel, "authoringSaveButton", panelBounds);
    require(findDescendantById(panel, "authoringZoneMap")->getBounds().getHeight()
                >= drs::app::authoring::minimumMapVisibleHeight,
            "Authoring shell must preserve the minimum visible map height.");

    if (surfaceId == 2)
        requireButton(panel, "authoringDrawerMacrosTab").onClick();
    else if (surfaceId == 3)
        requireButton(panel, "authoringDrawerRoutingTab").onClick();
    else if (surfaceId == 4)
        requireButton(panel, "authoringDrawerPerformanceTab").onClick();
    else if (surfaceId == 1
             && requireButton(panel, "authoringDrawerToggleButton").getButtonText() == "Hide Drawer")
    {
        requireButton(panel, "authoringDrawerToggleButton").onClick();
    }

    switch (surfaceId)
    {
        case 1:
        {
            auto ensureControlsVisible = [&](const juce::String& firstComponentId,
                                             const juce::String& secondComponentId,
                                             const juce::String& disclosureButtonId)
            {
                for (int attempt = 0; attempt < 2; ++attempt)
                {
                    const auto firstVisible = !findDescendantById(panel, firstComponentId)->getBounds().isEmpty();
                    const auto secondVisible = !findDescendantById(panel, secondComponentId)->getBounds().isEmpty();
                    if (firstVisible && secondVisible)
                        return;

                    requireButton(panel, disclosureButtonId).onClick();
                }
            };

            auto collapseSectionIfOpen = [&](const juce::String& firstComponentId,
                                             const juce::String& secondComponentId,
                                             const juce::String& disclosureButtonId)
            {
                juce::ignoreUnused(firstComponentId, secondComponentId);
                auto& disclosureButton = requireButton(panel, disclosureButtonId);
                if (disclosureButton.getButtonText() == "Hide")
                    disclosureButton.onClick();
            };

            collapseSectionIfOpen("authoringVelocityLowSlider",
                                  "authoringVelocityHighSlider",
                                  "authoringSampleInspectorSectionDisclosure");
            collapseSectionIfOpen("authoringGainSlider",
                                  "authoringPanSlider",
                                  "authoringMixInspectorSectionDisclosure");
            collapseSectionIfOpen("authoringRestoreRootKeyButton",
                                  "authoringLoopEnabledToggle",
                                  "authoringAdvancedInspectorSectionDisclosure");

            requireComponentVisibleWithin(panel, "authoringRootKeySlider", panelBounds);
            requireComponentVisibleWithin(panel, "authoringKeyLowSlider", panelBounds);
            requireComponentVisibleWithin(panel, "authoringKeyHighSlider", panelBounds);
            require(countDescendantsById(panel, "authoringRootKeySlider") == 1,
                    "Authoring panel should not expose duplicate root-key controls after mapping-row removal.");
            require(countDescendantsById(panel, "authoringKeyLowSlider") == 1,
                    "Authoring panel should not expose duplicate key-low controls after mapping-row removal.");
            require(countDescendantsById(panel, "authoringKeyHighSlider") == 1,
                    "Authoring panel should not expose duplicate key-high controls after mapping-row removal.");

            ensureControlsVisible("authoringVelocityLowSlider",
                                  "authoringVelocityHighSlider",
                                  "authoringSampleInspectorSectionDisclosure");
            requireComponentVisibleWithin(panel, "authoringVelocityLowSlider", panelBounds);
            requireComponentVisibleWithin(panel, "authoringVelocityHighSlider", panelBounds);
            require(countDescendantsById(panel, "authoringVelocityLowSlider") == 1,
                    "Authoring panel should not expose duplicate velocity-low controls after mapping-row removal.");
            require(countDescendantsById(panel, "authoringVelocityHighSlider") == 1,
                    "Authoring panel should not expose duplicate velocity-high controls after mapping-row removal.");
            requireButton(panel, "authoringSampleInspectorSectionDisclosure").onClick();

            ensureControlsVisible("authoringGainSlider",
                                  "authoringPanSlider",
                                  "authoringMixInspectorSectionDisclosure");
            requireComponentVisibleWithin(panel, "authoringGainSlider", panelBounds);
            requireComponentVisibleWithin(panel, "authoringPanSlider", panelBounds);
            require(countDescendantsById(panel, "authoringGainSlider") == 1,
                    "Authoring panel should not expose duplicate gain controls after mapping-row removal.");
            require(countDescendantsById(panel, "authoringPanSlider") == 1,
                    "Authoring panel should not expose duplicate pan controls after mapping-row removal.");
            requireButton(panel, "authoringMixInspectorSectionDisclosure").onClick();

            ensureControlsVisible("authoringRestoreRootKeyButton",
                                  "authoringLoopEnabledToggle",
                                  "authoringAdvancedInspectorSectionDisclosure");
            requireComponentVisibleWithin(panel, "authoringLoopEnabledToggle", panelBounds);
            requireComponentVisible(panel, "authoringRestoreRootKeyButton");
            require(countDescendantsById(panel, "authoringLoopEnabledToggle") == 1,
                    "Authoring panel should not expose duplicate loop toggles after mapping-row removal.");
            require(countDescendantsById(panel, "authoringRestoreRootKeyButton") == 1,
                    "Authoring panel should not expose duplicate restore-root-key actions after mapping-row removal.");
            break;
        }
        case 2:
            requireComponentVisibleWithin(panel, "authoringMacroList", panelBounds);
            requireComponentVisibleWithin(panel, "authoringMacroAssignmentSelector", panelBounds);
            requireComponentVisibleWithin(panel, "authoringMacroRoleSelector", panelBounds);
            require(requireLabel(panel, "authoringDrawerScopeLabel").getText().toStdString().find("Project-scoped")
                        != std::string::npos,
                    "Macros drawer should expose explicit project scope vocabulary.");
            require(requireLabel(panel, "authoringDrawerBreadcrumbLabel").getText().toStdString().find("Project > Macros >")
                        != std::string::npos,
                    "Macros drawer should expose a breadcrumb for the selected macro.");
            break;
        case 3:
            requireComponentVisibleWithin(panel, "authoringFxSelector", panelBounds);
            requireComponentVisibleWithin(panel, "authoringRoutingSelector", panelBounds);
            require(requireLabel(panel, "authoringDrawerScopeLabel").getText().toStdString().find("Project-scoped")
                        != std::string::npos,
                    "Routing drawer should expose explicit project scope vocabulary.");
            require(requireLabel(panel, "authoringDrawerBreadcrumbLabel").getText().toStdString().find("Project > Routing >")
                        != std::string::npos,
                    "Routing drawer should expose a breadcrumb for the selected FX and bus.");
            break;
        case 4:
            requireComponentVisibleWithin(panel, "authoringPerformanceBankSelector", panelBounds);
            requireComponentVisibleWithin(panel, "authoringTriggerSlotSelector", panelBounds);
            require(requireLabel(panel, "authoringDrawerScopeLabel").getText().toStdString().find("Bank-scoped")
                        != std::string::npos,
                    "Performance drawer should expose explicit bank/trigger scope vocabulary.");
            require(requireLabel(panel, "authoringDrawerBreadcrumbLabel").getText().toStdString().find("Project > Performance >")
                        != std::string::npos,
                    "Performance drawer should expose a breadcrumb for the selected bank and trigger.");
            break;
        default:
            require(false, "Unexpected authoring mode ID.");
            break;
    }

    inventory << shellName << " / " << surfaceName << "\n";
    for (const auto& componentId : {
             juce::String("authoringWorkspace"),
             juce::String("authoringZoneSelector"),
             juce::String("authoringDrawer"),
             juce::String("authoringDrawerTabStrip"),
             juce::String("authoringWaveformPreview"),
             juce::String("authoringZoneMap"),
             juce::String("authoringGainSlider"),
             juce::String("authoringPanSlider"),
             juce::String("authoringRestoreRootKeyButton"),
             juce::String("authoringMacroList"),
             juce::String("authoringFxSelector"),
             juce::String("authoringRoutingSelector"),
             juce::String("authoringPerformanceBankSelector"),
             juce::String("authoringTriggerSlotSelector")
         })
    {
        inventory << "  " << describeBounds(panel, componentId) << "\n";
    }

    std::vector<juce::String> activeModeComponents;
    switch (surfaceId)
    {
        case 1:
            activeModeComponents = {
                "authoringZoneMap",
                "authoringRootKeySlider",
                "authoringKeyLowSlider",
                "authoringKeyHighSlider"
            };
            break;
        case 2:
            activeModeComponents = {
                "authoringMacroList",
                "authoringMacroAssignmentSelector",
                "authoringMacroRoleSelector",
                "authoringMacroDefaultSlider",
                "authoringMacroMinSlider",
                "authoringMacroMaxSlider",
                "authoringMacroMoveUpButton",
                "authoringMacroMoveDownButton"
            };
            break;
        case 3:
            activeModeComponents = {
                "authoringFxSelector",
                "authoringFxTypeSelector",
                "authoringFxBypassedToggle",
                "authoringRoutingSelector",
                "authoringRoutingInputSelector",
                "authoringRoutingInsertOneSelector",
                "authoringRoutingInsertTwoSelector"
            };
            break;
        case 4:
            activeModeComponents = {
                "authoringPerformanceBankSelector",
                "authoringTriggerSlotSelector",
                "authoringTriggerEventSelector",
                "authoringTargetArticulationSelector",
                "authoringPhraseAssetSelector",
                "authoringChordModeSelector",
                "authoringPhraseImportPath",
                "authoringPhraseImportButton"
            };
            break;
        default:
            break;
    }

    for (const auto& componentId : activeModeComponents)
    {
        if (auto* component = findDescendantById(panel, componentId);
            component == nullptr)
        {
            baselineFindings.push_back(shellName + " / " + surfaceName + " missing: " + componentId.toStdString());
        }
        else if (!component->isVisible())
        {
            baselineFindings.push_back(shellName + " / " + surfaceName + " hidden: " + componentId.toStdString());
        }
        else if (component->getBounds().isEmpty())
        {
            baselineFindings.push_back(shellName + " / " + surfaceName + " empty-bounds: " + componentId.toStdString());
        }
        else if (!panelBounds.contains(component->getBounds()))
        {
            baselineFindings.push_back(shellName + " / " + surfaceName + " clipped: " + componentId.toStdString());
        }
    }

    inventory << "\n";

    saveComponentPng(panel, outputDirectory / (shellName + "-" + surfaceName + ".png"));
}

void exerciseDrawerBehavior(drs::app::AuthoringPanel& panel,
                            const std::string& shellName,
                            std::vector<std::string>& baselineFindings)
{
    const auto panelBounds = panel.getLocalBounds();
    auto& toggleButton = requireButton(panel, "authoringDrawerToggleButton");
    auto& waveformTabButton = requireButton(panel, "authoringDrawerWaveformTab");
    auto& macrosTabButton = requireButton(panel, "authoringDrawerMacrosTab");
    auto* zoneSelector = dynamic_cast<juce::ComboBox*>(findDescendantById(panel, "authoringZoneSelector"));

    require(zoneSelector != nullptr, "Drawer behavior checks require the zone selector.");

    if (shellName == "compact")
    {
        require(!requireSlider(panel, "authoringRootKeySlider").getBounds().isEmpty(),
                "Compact shell should still lay out mapping controls before drawer checks.");
        require(!findDescendantById(panel, "authoringWaveformPreview")->isVisible(),
                "Compact shell should begin with the drawer content hidden.");
        toggleButton.onClick();
        requireComponentVisibleWithin(panel, "authoringWaveformPreview", panelBounds);
    }
    else
    {
        requireComponentVisibleWithin(panel, "authoringWaveformPreview", panelBounds);
    }

    requireComponentVisibleWithin(panel, "authoringDrawerTitleLabel", panelBounds);
    requireComponentVisibleWithin(panel, "authoringDrawerScopeLabel", panelBounds);
    requireComponentVisibleWithin(panel, "authoringDrawerBreadcrumbLabel", panelBounds);
    requireComponentVisibleWithin(panel, "authoringWaveformStatusLabel", panelBounds);
    requireComponentVisibleWithin(panel, "authoringWaveformInfoLabel", panelBounds);
    requireComponentVisibleWithin(panel, "authoringWaveformLoopLabel", panelBounds);
    requireComponentVisibleWithin(panel, "authoringWaveformImportLabel", panelBounds);

    const auto initialZoneId = zoneSelector->getSelectedId();
    const auto alternateZoneId = initialZoneId == 2 ? 1 : 2;
    require(alternateZoneId != initialZoneId,
            "Drawer behavior checks require at least two selectable zones.");
    const auto initialWaveformScope = requireLabel(panel, "authoringDrawerScopeLabel").getText().toStdString();
    const auto initialWaveformBreadcrumb = requireLabel(panel, "authoringDrawerBreadcrumbLabel").getText().toStdString();
    const auto initialWaveformInfo = requireLabel(panel, "authoringWaveformInfoLabel").getText().toStdString();
    require(initialWaveformScope.find("Zone-scoped") != std::string::npos,
            "Waveform drawer should expose explicit zone scope vocabulary.");
    require(initialWaveformBreadcrumb.find("Project > Zones >") != std::string::npos,
            "Waveform drawer should expose a breadcrumb for the selected zone.");

    zoneSelector->setSelectedId(alternateZoneId, juce::sendNotificationSync);
    require(waveformTabButton.getToggleState(),
            "Changing zones should not change the active waveform drawer tab.");
    require(findDescendantById(panel, "authoringWaveformPreview")->isVisible(),
            "Changing zones should not collapse an open waveform drawer.");
    require(requireLabel(panel, "authoringWaveformInfoLabel").getText().toStdString() != initialWaveformInfo
                || requireLabel(panel, "authoringDrawerBreadcrumbLabel").getText().toStdString() != initialWaveformBreadcrumb,
            "Waveform drawer content should update when the selected zone changes.");

    macrosTabButton.onClick();
    requireComponentVisibleWithin(panel, "authoringMacroList", panelBounds);
    requireComponentVisibleWithin(panel, "authoringMacroAssignmentSelector", panelBounds);
    require(!findDescendantById(panel, "authoringWaveformPreview")->isVisible(),
            "Non-waveform drawer tabs should hide waveform content during Sprint 4.");
    require(requireLabel(panel, "authoringDrawerScopeLabel").getText().toStdString().find("Project-scoped")
                != std::string::npos,
            "Macros drawer should expose explicit project scope vocabulary.");
    require(requireLabel(panel, "authoringDrawerBreadcrumbLabel").getText().toStdString().find("Project > Macros >")
                != std::string::npos,
            "Macros drawer should expose the selected macro breadcrumb.");
    auto& macroList = requireRepeatedStructureList(panel, "authoringMacroList");
    auto& macroListBox = macroList.getListBox();
    if (macroList.getRowCount() > 1)
    {
        const auto initialMacroBreadcrumb = requireLabel(panel, "authoringDrawerBreadcrumbLabel").getText().toStdString();
        macroListBox.selectRow(1);
        require(requireLabel(panel, "authoringDrawerBreadcrumbLabel").getText().toStdString() != initialMacroBreadcrumb,
                "Selecting a repeated-structure row should rebind the macro drawer breadcrumb.");
        macroListBox.selectRow(0);
    }

    requireButton(panel, "authoringDrawerRoutingTab").onClick();
    requireComponentVisibleWithin(panel, "authoringFxSelector", panelBounds);
    requireComponentVisibleWithin(panel, "authoringRoutingSelector", panelBounds);
    require(requireLabel(panel, "authoringDrawerScopeLabel").getText().toStdString().find("Project-scoped")
                != std::string::npos,
            "Routing drawer should expose explicit project scope vocabulary.");
    require(requireLabel(panel, "authoringDrawerBreadcrumbLabel").getText().toStdString().find("Project > Routing >")
                != std::string::npos,
            "Routing drawer should expose the selected routing breadcrumb.");

    requireButton(panel, "authoringDrawerPerformanceTab").onClick();
    requireComponentVisibleWithin(panel, "authoringPerformanceBankSelector", panelBounds);
    requireComponentVisibleWithin(panel, "authoringTriggerSlotSelector", panelBounds);
    require(requireLabel(panel, "authoringDrawerScopeLabel").getText().toStdString().find("Bank-scoped")
                != std::string::npos,
            "Performance drawer should expose explicit bank scope vocabulary.");
    require(requireLabel(panel, "authoringDrawerBreadcrumbLabel").getText().toStdString().find("Project > Performance >")
                != std::string::npos,
            "Performance drawer should expose the selected performance breadcrumb.");

    waveformTabButton.onClick();
    requireComponentVisibleWithin(panel, "authoringWaveformPreview", panelBounds);

    macrosTabButton.onClick();
    zoneSelector->setSelectedId(initialZoneId, juce::sendNotificationSync);
    require(macrosTabButton.getToggleState(),
            "Changing zones should not replace the active non-waveform drawer tab.");
    if (auto* macroListComponent = findDescendantById(panel, "authoringMacroList");
        macroListComponent == nullptr || !macroListComponent->isVisible())
    {
        baselineFindings.push_back(shellName + " / drawer state did not survive zone selection");
    }

    waveformTabButton.onClick();
    require(requireLabel(panel, "authoringDrawerScopeLabel").getText().toStdString() == initialWaveformScope,
            "Waveform drawer should preserve explicit scope vocabulary when revisited.");
    require(requireLabel(panel, "authoringDrawerBreadcrumbLabel").getText().toStdString() == initialWaveformBreadcrumb,
            "Waveform drawer should restore its original breadcrumb when the original zone is reselected.");
    require(requireLabel(panel, "authoringWaveformInfoLabel").getText().toStdString() == initialWaveformInfo,
            "Waveform drawer should restore its original metadata when the original zone is reselected.");

    if (shellName == "compact")
    {
        toggleButton.onClick();
        require(!findDescendantById(panel, "authoringWaveformPreview")->isVisible(),
                "Compact shell drawer should close back to its default collapsed state.");
    }
}

void exerciseDrawerEditorTransactions(drs::app::AuthoringPanel& panel,
                                      drs::engine::AuthoringSession& session)
{
    requireButton(panel, "authoringDrawerMacrosTab").onClick();
    auto& macroDefaultSlider = requireSlider(panel, "authoringMacroDefaultSlider");
    auto& macroList = requireRepeatedStructureList(panel, "authoringMacroList");
    auto& macroListBox = macroList.getListBox();
    if (macroList.getRowCount() > 1)
    {
        macroListBox.selectRow(1);
        require(std::abs(macroDefaultSlider.getValue() - session.getProject().authoring.macros[1].defaultValue) < 0.001,
                "Selecting a repeated-structure macro row should bind the detail editor to that macro.");
        macroListBox.selectRow(0);
    }

    const auto macroUndoDepth = session.getDocumentState().undoDepth;
    const auto macroTargetValue = juce::jlimit(macroDefaultSlider.getMinimum(),
                                               macroDefaultSlider.getMaximum(),
                                               macroDefaultSlider.getValue() + 0.05);
    macroDefaultSlider.setValue(macroTargetValue, juce::dontSendNotification);
    macroDefaultSlider.onDragEnd();
    require(session.getDocumentState().undoDepth == macroUndoDepth + 1,
            "Macro drawer edits should create one undo transaction per completed gesture.");
    require(std::abs(session.getProject().authoring.macros.front().defaultValue - macroTargetValue) < 0.001,
            "Macro drawer edits should persist through the authoring session.");

    requireButton(panel, "authoringDrawerRoutingTab").onClick();
    auto& fxTypeSelector = requireComboBox(panel, "authoringFxTypeSelector");
    const auto routingUndoDepth = session.getDocumentState().undoDepth;
    const auto nextFxId = fxTypeSelector.getSelectedId() == fxTypeSelector.getNumItems() ? 1 : fxTypeSelector.getSelectedId() + 1;
    fxTypeSelector.setSelectedId(nextFxId, juce::sendNotificationSync);
    require(session.getDocumentState().undoDepth == routingUndoDepth + 1,
            "Routing drawer edits should create one undo transaction per committed selection.");
    require(session.getProject().authoring.fxSlots.front().effectType == fxTypeSelector.getText().toStdString(),
            "Routing drawer edits should persist through the authoring session.");
}

void exerciseAccessibilityAndFocusBehavior(drs::app::AuthoringPanel& panel,
                                           const std::string& shellName)
{
    auto& toggleButton = requireButton(panel, "authoringDrawerToggleButton");
    auto& waveformTabButton = requireButton(panel, "authoringDrawerWaveformTab");
    auto& macrosTabButton = requireButton(panel, "authoringDrawerMacrosTab");
    auto& routingTabButton = requireButton(panel, "authoringDrawerRoutingTab");
    auto& performanceTabButton = requireButton(panel, "authoringDrawerPerformanceTab");
    auto& macroAssignmentSelector = requireComboBox(panel, "authoringMacroAssignmentSelector");
    auto& macroMoveUpButton = requireButton(panel, "authoringMacroMoveUpButton");
    auto& macroMoveDownButton = requireButton(panel, "authoringMacroMoveDownButton");
    auto& fxSelector = requireComboBox(panel, "authoringFxSelector");
    auto& fxTypeSelector = requireComboBox(panel, "authoringFxTypeSelector");
    auto& fxBypassedToggle = requireButton(panel, "authoringFxBypassedToggle");
    auto& routingBusSelector = requireComboBox(panel, "authoringRoutingSelector");
    auto& routingInputSelector = requireComboBox(panel, "authoringRoutingInputSelector");
    auto& routingInsertOneSelector = requireComboBox(panel, "authoringRoutingInsertOneSelector");
    auto& routingInsertTwoSelector = requireComboBox(panel, "authoringRoutingInsertTwoSelector");
    auto& performanceBankSelector = requireComboBox(panel, "authoringPerformanceBankSelector");
    auto& triggerSlotSelector = requireComboBox(panel, "authoringTriggerSlotSelector");
    auto& triggerEventSelector = requireComboBox(panel, "authoringTriggerEventSelector");
    auto& targetArticulationSelector = requireComboBox(panel, "authoringTargetArticulationSelector");
    auto& phraseAssetSelector = requireComboBox(panel, "authoringPhraseAssetSelector");
    auto& chordModeSelector = requireComboBox(panel, "authoringChordModeSelector");
    auto* phraseImportPath = findDescendantById(panel, "authoringPhraseImportPath");
    auto& phraseImportButton = requireButton(panel, "authoringPhraseImportButton");
    auto& zoneSelector = requireComboBox(panel, "authoringZoneSelector");
    auto& playbackBannerPrepareButton = requireButton(panel, "authoringPlaybackBannerPrepareButton");
    auto& playbackBannerPublishButton = requireButton(panel, "authoringPlaybackBannerPublishButton");
    auto& zoneMap = requireZoneMapCanvas(panel, "authoringZoneMap");
    auto& macroList = requireRepeatedStructureList(panel, "authoringMacroList");
    require(phraseImportPath != nullptr, "Performance drawer accessibility checks require the phrase import path.");

    for (const auto& componentId : {
             juce::String("authoringPreviewButton"),
             juce::String("authoringPlaybackBanner"),
             juce::String("authoringPlaybackBannerLabel"),
             juce::String("authoringPlaybackBannerPrepareButton"),
             juce::String("authoringUndoButton"),
             juce::String("authoringRedoButton"),
             juce::String("authoringSaveButton"),
             juce::String("authoringZoneSelector"),
             juce::String("authoringZoneMap"),
             juce::String("authoringDrawerToggleButton"),
             juce::String("authoringDrawerWaveformTab"),
             juce::String("authoringDrawerMacrosTab"),
             juce::String("authoringDrawerRoutingTab"),
             juce::String("authoringDrawerPerformanceTab")
         })
    {
        requireNonEmptyAccessibilityTitle(panel, componentId);
        requireAccessibilityHandlerState(panel, componentId, true);
    }

    requireContrastAtLeast(zoneSelector.findColour(juce::ComboBox::focusedOutlineColourId),
                           zoneSelector.findColour(juce::ComboBox::backgroundColourId),
                           3.0,
                           "Zone selector focus styling should remain visually distinct from its background.");
    requireContrastAtLeast(zoneMap.findColour(juce::TextEditor::focusedOutlineColourId),
                           zoneMap.findColour(juce::ListBox::backgroundColourId),
                           3.0,
                           "Zone map focus styling should remain visually distinct from the map background.");
    requireContrastAtLeast(macroList.getListBox().findColour(juce::TextEditor::focusedOutlineColourId),
                           macroList.getListBox().findColour(juce::ListBox::backgroundColourId),
                           3.0,
                           "Repeated-structure list focus styling should remain visually distinct from the list background.");
    requireContrastAtLeast(phraseImportPath->findColour(juce::TextEditor::focusedOutlineColourId),
                           phraseImportPath->findColour(juce::TextEditor::backgroundColourId),
                           3.0,
                           "Text-entry focus styling should remain visually distinct from the editor background.");

    requireIncreasingFocusOrder(panel,
                                {
                                    "authoringUndoButton",
                                    "authoringRedoButton",
                                    "authoringSaveButton",
                                    "authoringPreviewButton",
                                    "authoringPlaybackBannerPrepareButton",
                                    "authoringZoneSelector",
                                    "authoringZoneMap",
                                    "authoringDrawerToggleButton",
                                    "authoringDrawerWaveformTab",
                                    "authoringDrawerMacrosTab",
                                    "authoringDrawerRoutingTab",
                                    "authoringDrawerPerformanceTab"
                                });

    if (shellName == "compact" && toggleButton.getButtonText() == "Show Drawer")
    {
        toggleButton.onClick();
    }

    const auto initialSummarySource = requireLabel(panel, "authoringSummarySourceLabel").getText();
    const auto initialSummaryStatus = requireLabel(panel, "authoringSummaryStatusLabel").getText();
    const auto initialSummaryArticulation = requireLabel(panel, "authoringSummaryArticulationLabel").getText();
    const auto initialSummaryPlayback = requireLabel(panel, "authoringSummaryPlaybackLabel").getText();
    const auto initialPlaybackBannerText = requireLabel(panel, "authoringPlaybackBannerLabel").getText();
    require(initialSummaryStatus.contains("preview blocked: Selected authoring sample could not be prepared."),
            "Summary strip should surface the authoring preview failure detail in the status line.");
    require(initialSummaryStatus.contains("fix: Relink or re-import the selected sample file."),
            "Summary strip should surface the next authoring prerequisite when preview preparation fails.");
    require(initialSummaryStatus.contains("playback action: Prepare the latest draft for preview."),
            "Summary strip should surface the next draft-playback action when the performance preview is stale.");
    require(initialSummaryPlayback.contains("authoring preview r4 (Failed)"),
            "Summary strip should surface the authoring preview revision state in the playback line.");
    requireComponentVisible(panel, "authoringPlaybackBanner");
    requireComponentVisible(panel, "authoringPlaybackBannerLabel");
    requireComponentVisible(panel, "authoringPlaybackBannerPrepareButton");
    require(initialPlaybackBannerText.contains("playback action: Prepare the latest draft for preview."),
            "Workspace banner should surface the next draft-playback action when the performance preview is stale.");
    require(playbackBannerPrepareButton.isVisible() && playbackBannerPrepareButton.isEnabled(),
            "Workspace banner should expose an enabled prepare-draft action when the current draft is stale.");
    require(!playbackBannerPublishButton.isVisible(),
            "Workspace banner should hide publish-draft actions until the latest draft is ready to publish.");
    requireAccessibilityTitleEquals(panel, "authoringSummarySourceLabel", initialSummarySource);
    requireAccessibilityTitleEquals(panel, "authoringSummaryStatusLabel", initialSummaryStatus);
    requireAccessibilityTitleEquals(panel, "authoringSummaryArticulationLabel", initialSummaryArticulation);
    requireAccessibilityTitleEquals(panel, "authoringSummaryPlaybackLabel", initialSummaryPlayback);
    requireAccessibilityDescriptionContains(panel, "authoringSummarySourceLabel", initialSummarySource);
    requireAccessibilityDescriptionContains(panel, "authoringSummaryStatusLabel", initialSummaryStatus);
    requireAccessibilityDescriptionContains(panel, "authoringSummaryArticulationLabel", initialSummaryArticulation);
    requireAccessibilityDescriptionContains(panel, "authoringSummaryPlaybackLabel", initialSummaryPlayback);
    requireAccessibilityTitleEquals(panel, "authoringPlaybackBannerLabel", initialPlaybackBannerText);
    requireAccessibilityDescriptionContains(panel, "authoringPlaybackBanner", initialPlaybackBannerText);
    requireAccessibilityDescriptionContains(panel, "authoringPlaybackBannerLabel", initialPlaybackBannerText);
    requireAccessibilityDescriptionContains(panel,
                                            "authoringPlaybackBannerPrepareButton",
                                            "Builds the latest draft for playback preview from the workspace banner.");
    requireNonEmptyAccessibilityHelpText(panel, "authoringPlaybackBannerPrepareButton");
    requireAccessibilityHandlerState(panel, "authoringPlaybackBannerPublishButton", false);

    auto summarySourceChanged = false;
    for (int candidateId = 1; candidateId <= zoneSelector.getNumItems(); ++candidateId)
    {
        if (candidateId == zoneSelector.getSelectedId())
            continue;

        zoneSelector.setSelectedId(candidateId, juce::sendNotificationSync);
        if (requireLabel(panel, "authoringSummarySourceLabel").getText() != initialSummarySource)
        {
            summarySourceChanged = true;
            break;
        }
    }

    const auto updatedSummarySource = requireLabel(panel, "authoringSummarySourceLabel").getText();
    const auto updatedSummaryStatus = requireLabel(panel, "authoringSummaryStatusLabel").getText();
    const auto updatedSummaryArticulation = requireLabel(panel, "authoringSummaryArticulationLabel").getText();
    const auto updatedSummaryPlayback = requireLabel(panel, "authoringSummaryPlaybackLabel").getText();
    require(summarySourceChanged && updatedSummarySource != initialSummarySource,
            "Changing the selected zone should refresh the summary-strip source text.");
    requireAccessibilityTitleEquals(panel, "authoringSummarySourceLabel", updatedSummarySource);
    requireAccessibilityTitleEquals(panel, "authoringSummaryStatusLabel", updatedSummaryStatus);
    requireAccessibilityTitleEquals(panel, "authoringSummaryArticulationLabel", updatedSummaryArticulation);
    requireAccessibilityTitleEquals(panel, "authoringSummaryPlaybackLabel", updatedSummaryPlayback);
    requireAccessibilityDescriptionContains(panel, "authoringSummarySourceLabel", updatedSummarySource);
    requireAccessibilityDescriptionContains(panel, "authoringSummaryStatusLabel", updatedSummaryStatus);
    requireAccessibilityDescriptionContains(panel, "authoringSummaryArticulationLabel", updatedSummaryArticulation);
    requireAccessibilityDescriptionContains(panel, "authoringSummaryPlaybackLabel", updatedSummaryPlayback);

    requireAccessibilityTitleEquals(panel, "authoringDrawerTitleLabel", requireLabel(panel, "authoringDrawerTitleLabel").getText());
    requireAccessibilityTitleEquals(panel, "authoringDrawerScopeLabel", requireLabel(panel, "authoringDrawerScopeLabel").getText());
    requireAccessibilityTitleEquals(panel,
                                    "authoringDrawerBreadcrumbLabel",
                                    requireLabel(panel, "authoringDrawerBreadcrumbLabel").getText());
    requireAccessibilityDescriptionContains(panel,
                                            "authoringDrawerScopeLabel",
                                            requireLabel(panel, "authoringDrawerScopeLabel").getText());
    requireAccessibilityDescriptionContains(panel,
                                            "authoringDrawerBreadcrumbLabel",
                                            requireLabel(panel, "authoringDrawerBreadcrumbLabel").getText());
    requireAccessibilityTitleEquals(panel,
                                    "authoringWaveformStatusLabel",
                                    requireLabel(panel, "authoringWaveformStatusLabel").getText());
    require(requireLabel(panel, "authoringWaveformStatusLabel").getText().contains("Fix: Relink or re-import the selected sample file."),
            "Waveform drawer status should include the next prerequisite for failed authoring preview preparation.");
    requireAccessibilityDescriptionContains(panel,
                                            "authoringWaveformStatusLabel",
                                            requireLabel(panel, "authoringWaveformStatusLabel").getText());
    requireAccessibilityDescriptionContains(panel,
                                            "authoringWaveformStatusLabel",
                                            "Next step: Restore the sample file for the selected zone, then prepare the authoring preview again.");
    requireAccessibilityTitleEquals(panel, "authoringWaveformInfoLabel", requireLabel(panel, "authoringWaveformInfoLabel").getText());
    requireAccessibilityDescriptionContains(panel,
                                            "authoringWaveformInfoLabel",
                                            requireLabel(panel, "authoringWaveformInfoLabel").getText());

    macrosTabButton.onClick();
    require(macroAssignmentSelector.isVisible(),
            "Macro drawer controls should be visible when the macros tab is active.");
    requireAccessibilityHandlerState(panel, "authoringMacroAssignmentSelector", true);
    requireAccessibilityTitleEquals(panel, "authoringDrawerTitleLabel", "Macro Assignment");
    requireAccessibilityTitleEquals(panel,
                                    "authoringDrawerScopeLabel",
                                    requireLabel(panel, "authoringDrawerScopeLabel").getText());
    requireAccessibilityDescriptionContains(panel, "authoringDrawerScopeLabel", "Project-scoped");
    requireAccessibilityDescriptionContains(panel, "authoringDrawerBreadcrumbLabel", "Project > Macros");
    requireAccessibilityDescriptionContains(panel,
                                            "authoringMacroAssignmentSelector",
                                            "Chooses the parameter assigned to");
    requireAccessibilityDescriptionContains(panel,
                                            "authoringMacroRoleSelector",
                                            "Chooses the semantic role for");
    requireAccessibilityDescriptionContains(panel,
                                            "authoringMacroDefaultSlider",
                                            "Adjusts the default value for");
    require(!macroMoveUpButton.isEnabled(),
            "The first macro should not be movable upward.");
    requireAccessibilityDescriptionContains(panel,
                                            "authoringMacroMoveUpButton",
                                            "already the first macro");
    auto& macroListBox = macroList.getListBox();
    if (macroList.getRowCount() > 1)
    {
        macroListBox.selectRow(macroList.getRowCount() - 1);
        require(macroMoveUpButton.isEnabled(),
                "The last macro should still be movable upward.");
        require(!macroMoveDownButton.isEnabled(),
                "The last macro should not be movable downward.");
        requireAccessibilityDescriptionContains(panel,
                                                "authoringMacroMoveUpButton",
                                                "earlier in the list");
        requireAccessibilityDescriptionContains(panel,
                                                "authoringMacroMoveDownButton",
                                                "already the last macro");
        macroListBox.selectRow(0);
    }

    routingTabButton.onClick();
    require(!macroAssignmentSelector.isVisible(),
            "Macro drawer controls should be hidden after switching to routing.");
    requireAccessibilityHandlerState(panel, "authoringMacroAssignmentSelector", false);
    requireAccessibilityTitleEquals(panel, "authoringDrawerTitleLabel", "Routing Detail");
    requireAccessibilityDescriptionContains(panel, "authoringDrawerScopeLabel", "Project-scoped");
    requireAccessibilityDescriptionContains(panel, "authoringDrawerBreadcrumbLabel", "Project > Routing >");
    require(fxSelector.isVisible(),
            "FX selector should be visible when the routing tab is active.");
    require(fxBypassedToggle.isVisible(),
            "FX bypass toggle should be visible when the routing tab is active.");
    require(routingBusSelector.isVisible(),
            "Routing bus selector should be visible when the routing tab is active.");
    require(routingInputSelector.isVisible(),
            "Routing drawer controls should be visible when the routing tab is active.");
    require(routingInsertOneSelector.isVisible(),
            "Routing insert A selector should be visible when the routing tab is active.");
    require(routingInsertTwoSelector.isVisible(),
            "Routing insert B selector should be visible when the routing tab is active.");
    requireAccessibilityDescriptionContains(panel,
                                            "authoringFxSelector",
                                            "Current FX slot: " + fxSelector.getText());
    requireAccessibilityDescriptionContains(panel,
                                            "authoringFxTypeSelector",
                                            fxSelector.getText());
    requireAccessibilityDescriptionContains(panel,
                                            "authoringFxTypeSelector",
                                            fxTypeSelector.getText());
    requireAccessibilityDescriptionContains(panel,
                                            "authoringFxBypassedToggle",
                                            fxSelector.getText());
    requireAccessibilityDescriptionContains(panel,
                                            "authoringRoutingSelector",
                                            "Current bus: " + routingBusSelector.getText());
    requireAccessibilityDescriptionContains(panel,
                                            "authoringRoutingInputSelector",
                                            routingBusSelector.getText());
    requireAccessibilityDescriptionContains(panel,
                                            "authoringRoutingInputSelector",
                                            routingInputSelector.getText());
    requireAccessibilityDescriptionContains(panel,
                                            "authoringRoutingInsertOneSelector",
                                            routingBusSelector.getText());
    requireAccessibilityDescriptionContains(panel,
                                            "authoringRoutingInsertTwoSelector",
                                            routingBusSelector.getText());
    const auto initialFxId = fxSelector.getSelectedId();
    const auto initialRoutingBusId = routingBusSelector.getSelectedId();
    if (fxSelector.getNumItems() > 1)
    {
        const auto initialFxName = fxSelector.getText();
        const auto nextFxId = fxSelector.getSelectedId() == fxSelector.getNumItems() ? 1 : fxSelector.getSelectedId() + 1;
        fxSelector.setSelectedId(nextFxId, juce::sendNotificationSync);
        pumpMessages();
        require(fxSelector.getText() != initialFxName,
                "Selecting a different FX slot should update the active routing detail.");
        requireAccessibilityDescriptionContains(panel,
                                                "authoringFxSelector",
                                                "Current FX slot: " + fxSelector.getText());
        requireAccessibilityDescriptionContains(panel,
                                                "authoringFxTypeSelector",
                                                fxSelector.getText());
        requireAccessibilityDescriptionContains(panel,
                                                "authoringFxBypassedToggle",
                                                fxSelector.getText());
    }
    if (routingBusSelector.getNumItems() > 1)
    {
        const auto initialBusName = routingBusSelector.getText();
        const auto nextBusId = routingBusSelector.getSelectedId() == routingBusSelector.getNumItems()
            ? 1
            : routingBusSelector.getSelectedId() + 1;
        routingBusSelector.setSelectedId(nextBusId, juce::sendNotificationSync);
        pumpMessages();
        require(routingBusSelector.getText() != initialBusName,
                "Selecting a different routing bus should update the active routing detail.");
        requireAccessibilityDescriptionContains(panel,
                                                "authoringRoutingSelector",
                                                "Current bus: " + routingBusSelector.getText());
        requireAccessibilityDescriptionContains(panel,
                                                "authoringRoutingInputSelector",
                                                routingBusSelector.getText());
        requireAccessibilityDescriptionContains(panel,
                                                "authoringRoutingInsertOneSelector",
                                                routingBusSelector.getText());
        requireAccessibilityDescriptionContains(panel,
                                                "authoringRoutingInsertTwoSelector",
                                                routingBusSelector.getText());
    }
    if (fxSelector.getSelectedId() != initialFxId)
    {
        fxSelector.setSelectedId(initialFxId, juce::sendNotificationSync);
        pumpMessages();
    }
    if (routingBusSelector.getSelectedId() != initialRoutingBusId)
    {
        routingBusSelector.setSelectedId(initialRoutingBusId, juce::sendNotificationSync);
        pumpMessages();
    }
    toggleButton.onClick();
    require(!routingInputSelector.isVisible(),
            "Routing drawer controls should be hidden after closing the drawer.");
    requireAccessibilityHandlerState(panel, "authoringRoutingInputSelector", false);

    performanceTabButton.onClick();
    requireAccessibilityTitleEquals(panel, "authoringDrawerTitleLabel", "Performance Detail");
    requireAccessibilityDescriptionContains(panel, "authoringDrawerScopeLabel", "Bank-scoped");
    requireAccessibilityDescriptionContains(panel, "authoringDrawerBreadcrumbLabel", "Project > Performance");
    require(performanceBankSelector.isVisible(),
            "Performance bank selector should be visible when the performance tab is active.");
    require(triggerSlotSelector.isVisible(),
            "Trigger slot selector should be visible when the performance tab is active.");
    require(triggerEventSelector.isVisible(),
            "Trigger event selector should be visible when the performance tab is active.");
    require(targetArticulationSelector.isVisible(),
            "Target articulation selector should be visible when the performance tab is active.");
    require(phraseAssetSelector.isVisible(),
            "Phrase asset selector should be visible when the performance tab is active.");
    require(chordModeSelector.isVisible(),
            "Chord mode selector should be visible when the performance tab is active.");
    require(phraseImportPath->isVisible(),
            "Phrase import path should be visible when the performance tab is active.");
    require(phraseImportButton.isVisible(),
            "Phrase import button should be visible when the performance tab is active.");
    for (const auto& componentId : {
             juce::String("authoringPerformanceBankSelector"),
             juce::String("authoringTriggerSlotSelector"),
             juce::String("authoringTriggerEventSelector"),
             juce::String("authoringTargetArticulationSelector"),
             juce::String("authoringPhraseAssetSelector"),
             juce::String("authoringChordModeSelector"),
             juce::String("authoringPhraseImportPath"),
             juce::String("authoringPhraseImportButton")
         })
    {
        requireNonEmptyAccessibilityTitle(panel, componentId);
        requireNonEmptyAccessibilityDescription(panel, componentId);
        requireAccessibilityHandlerState(panel, componentId, true);
    }
    requireIncreasingFocusOrder(panel,
                                {
                                    "authoringPerformanceBankSelector",
                                    "authoringTriggerSlotSelector",
                                    "authoringTriggerEventSelector",
                                    "authoringTargetArticulationSelector",
                                    "authoringPhraseAssetSelector",
                                    "authoringChordModeSelector",
                                    "authoringPhraseImportPath",
                                    "authoringPhraseImportButton"
                                });
    requireAccessibilityDescriptionContains(panel,
                                            "authoringPerformanceBankSelector",
                                            "Current bank: " + performanceBankSelector.getText());
    requireAccessibilityDescriptionContains(panel,
                                            "authoringTriggerSlotSelector",
                                            performanceBankSelector.getText());
    requireAccessibilityDescriptionContains(panel,
                                            "authoringTriggerSlotSelector",
                                            triggerSlotSelector.getText());
    requireAccessibilityDescriptionContains(panel,
                                            "authoringTriggerEventSelector",
                                            triggerSlotSelector.getText());
    requireAccessibilityDescriptionContains(panel,
                                            "authoringTriggerEventSelector",
                                            triggerEventSelector.getText());
    requireAccessibilityDescriptionContains(panel,
                                            "authoringTargetArticulationSelector",
                                            triggerSlotSelector.getText());
    requireAccessibilityDescriptionContains(panel,
                                            "authoringTargetArticulationSelector",
                                            targetArticulationSelector.getText());
    requireAccessibilityDescriptionContains(panel,
                                            "authoringPhraseAssetSelector",
                                            triggerSlotSelector.getText());
    requireAccessibilityDescriptionContains(panel,
                                            "authoringPhraseAssetSelector",
                                            phraseAssetSelector.getText());
    requireAccessibilityDescriptionContains(panel,
                                            "authoringChordModeSelector",
                                            triggerSlotSelector.getText());
    requireAccessibilityDescriptionContains(panel,
                                            "authoringChordModeSelector",
                                            chordModeSelector.getText());
    const auto initialPhraseImportPathDescription = phraseImportPath->getDescription();
    const auto initialPhraseImportButtonDescription = phraseImportButton.getDescription();
    require(!initialPhraseImportPathDescription.isEmpty(),
            "Phrase import path should expose a contextual accessibility description.");
    require(!initialPhraseImportButtonDescription.isEmpty(),
            "Phrase import button should expose a contextual accessibility description.");
    const auto initialTriggerSlotId = triggerSlotSelector.getSelectedId();
    if (triggerSlotSelector.getNumItems() > 1)
    {
        const auto initialTriggerName = triggerSlotSelector.getText();
        const auto nextTriggerId = triggerSlotSelector.getSelectedId() == triggerSlotSelector.getNumItems()
            ? 1
            : triggerSlotSelector.getSelectedId() + 1;
        triggerSlotSelector.setSelectedId(nextTriggerId, juce::sendNotificationSync);
        pumpMessages();
        require(triggerSlotSelector.getText() != initialTriggerName,
                "Selecting a different trigger slot should update the active performance detail.");
        requireAccessibilityDescriptionContains(panel,
                                                "authoringTriggerSlotSelector",
                                                triggerSlotSelector.getText());
        requireAccessibilityDescriptionContains(panel,
                                                "authoringTriggerEventSelector",
                                                triggerSlotSelector.getText());
        requireAccessibilityDescriptionContains(panel,
                                                "authoringTargetArticulationSelector",
                                                triggerSlotSelector.getText());
        requireAccessibilityDescriptionContains(panel,
                                                "authoringPhraseAssetSelector",
                                                triggerSlotSelector.getText());
        requireAccessibilityDescriptionContains(panel,
                                                "authoringChordModeSelector",
                                                triggerSlotSelector.getText());
    }
    if (triggerSlotSelector.getSelectedId() != initialTriggerSlotId)
    {
        triggerSlotSelector.setSelectedId(initialTriggerSlotId, juce::sendNotificationSync);
        pumpMessages();
    }
    auto* phraseImportTextEditor = dynamic_cast<juce::TextEditor*>(phraseImportPath);
    require(phraseImportTextEditor != nullptr,
            "Performance drawer accessibility checks require the phrase import path editor.");
    phraseImportTextEditor->setText("C:/fixtures/phase2/accessibility-check.mid");
    if (phraseImportTextEditor->onTextChange)
        phraseImportTextEditor->onTextChange();
    pumpMessages();
    requireAccessibilityDescriptionContains(panel,
                                            "authoringPhraseImportPath",
                                            "Current path: C:/fixtures/phase2/accessibility-check.mid");
    requireAccessibilityDescriptionContains(panel,
                                            "authoringPhraseImportButton",
                                            "C:/fixtures/phase2/accessibility-check.mid");
    if (auto* performanceSummary = findDescendantById(panel, "authoringPerformanceSummaryLabel");
        performanceSummary != nullptr && performanceSummary->isVisible())
    {
        requireAccessibilityTitleEquals(panel,
                                        "authoringPerformanceSummaryLabel",
                                        requireLabel(panel, "authoringPerformanceSummaryLabel").getText());
        requireAccessibilityDescriptionContains(panel,
                                                "authoringPerformanceSummaryLabel",
                                                requireLabel(panel, "authoringPerformanceSummaryLabel").getText());
    }
    if (auto* phraseSummary = findDescendantById(panel, "authoringPhraseSummaryLabel");
        phraseSummary != nullptr && phraseSummary->isVisible())
    {
        requireAccessibilityTitleEquals(panel,
                                        "authoringPhraseSummaryLabel",
                                        requireLabel(panel, "authoringPhraseSummaryLabel").getText());
        requireAccessibilityDescriptionContains(panel,
                                                "authoringPhraseSummaryLabel",
                                                requireLabel(panel, "authoringPhraseSummaryLabel").getText());
    }

    waveformTabButton.onClick();
    requireAccessibilityHandlerState(panel, "authoringWaveformPreview", true);
    requireAccessibilityTitleEquals(panel, "authoringDrawerTitleLabel", "Waveform Detail");
    requireAccessibilityDescriptionContains(panel, "authoringDrawerScopeLabel", "Zone-scoped");
    requireAccessibilityDescriptionContains(panel, "authoringDrawerBreadcrumbLabel", "Project > Zones >");
    requireAccessibilityDescriptionContains(panel,
                                            "authoringWaveformStatusLabel",
                                            "Preview Failed");
    requireAccessibilityDescriptionContains(panel,
                                            "authoringWaveformStatusLabel",
                                            "Fix: Relink or re-import the selected sample file.");
    requireAccessibilityDescriptionContains(panel,
                                            "authoringWaveformStatusLabel",
                                            "Selected authoring sample could not be prepared.");
    requireAccessibilityDescriptionContains(panel,
                                            "authoringWaveformInfoLabel",
                                            requireLabel(panel, "authoringWaveformInfoLabel").getText());
    requireAccessibilityHandlerState(panel, "authoringPerformanceBankSelector", false);
    requireAccessibilityHandlerState(panel, "authoringTriggerSlotSelector", false);
    requireAccessibilityHandlerState(panel, "authoringTriggerEventSelector", false);
    requireAccessibilityHandlerState(panel, "authoringTargetArticulationSelector", false);
    requireAccessibilityHandlerState(panel, "authoringPhraseAssetSelector", false);
    requireAccessibilityHandlerState(panel, "authoringChordModeSelector", false);
    requireAccessibilityHandlerState(panel, "authoringPhraseImportPath", false);
    requireAccessibilityHandlerState(panel, "authoringPhraseImportButton", false);
}

void exerciseHostedFocusTransitions(drs::app::AuthoringPanel& panel, const std::string& shellName)
{
    DesktopHostedComponent hostedPanel(panel);
    pumpMessages();

    auto& toggleButton = requireButton(panel, "authoringDrawerToggleButton");
    auto& waveformTabButton = requireButton(panel, "authoringDrawerWaveformTab");
    auto& macrosTabButton = requireButton(panel, "authoringDrawerMacrosTab");
    auto& routingTabButton = requireButton(panel, "authoringDrawerRoutingTab");
    auto& performanceTabButton = requireButton(panel, "authoringDrawerPerformanceTab");
    auto& macroAssignmentSelector = requireComboBox(panel, "authoringMacroAssignmentSelector");
    auto& macroList = requireRepeatedStructureList(panel, "authoringMacroList");
    auto& phraseImportButton = requireButton(panel, "authoringPhraseImportButton");
    auto& sampleDisclosureButton = requireButton(panel, "authoringSampleInspectorSectionDisclosure");
    auto& velocityLowSlider = requireSlider(panel, "authoringVelocityLowSlider");

    if (shellName == "compact" && toggleButton.getButtonText() == "Show Drawer")
    {
        toggleButton.onClick();
        pumpMessages();
    }

    macrosTabButton.onClick();
    pumpMessages();
    macroList.getListBox().grabKeyboardFocus();
    pumpMessages();
    requireFocusedWithin(macroList.getListBox(),
                         "Repeated-structure lists should expose a visible keyboard focus target in a desktop host.");
    macroAssignmentSelector.grabKeyboardFocus();
    pumpMessages();
    requireFocusedWithin(macroAssignmentSelector,
                         "Visible macro controls should be able to take keyboard focus in a desktop host.");

    routingTabButton.onClick();
    pumpMessages();
    requireFocusedWithin(routingTabButton,
                         "Switching away from a focused macro control should redirect focus to the routing tab.");

    performanceTabButton.onClick();
    pumpMessages();
    phraseImportButton.grabKeyboardFocus();
    pumpMessages();
    requireFocusedWithin(phraseImportButton,
                         "Visible performance drawer controls should be able to take keyboard focus in a desktop host.");

    waveformTabButton.onClick();
    pumpMessages();
    requireFocusedWithin(waveformTabButton,
                         "Switching away from a focused performance control should redirect focus to the waveform tab.");

    performanceTabButton.onClick();
    pumpMessages();
    phraseImportButton.grabKeyboardFocus();
    pumpMessages();
    requireFocusedWithin(phraseImportButton,
                         "Performance drawer import button should regain focus when revisited.");

    toggleButton.onClick();
    pumpMessages();
    requireFocusedWithin(toggleButton,
                         "Collapsing the drawer while a drawer control is focused should redirect focus to the toggle.");

    if (findDescendantById(panel, "authoringVelocityLowSlider")->getBounds().isEmpty())
    {
        sampleDisclosureButton.onClick();
        pumpMessages();
    }

    velocityLowSlider.grabKeyboardFocus();
    pumpMessages();
    requireFocusedWithin(velocityLowSlider,
                         "Expanded inspector controls should be able to take keyboard focus in a desktop host.");

    sampleDisclosureButton.onClick();
    pumpMessages();
    requireFocusedWithin(sampleDisclosureButton,
                         "Collapsing a section while one of its controls is focused should redirect focus to the disclosure button.");
}

void exerciseKeyboardOnlyWorkflowSmoke(drs::app::AuthoringPanel& panel,
                                       drs::engine::AuthoringSession& session,
                                       const std::string& shellName)
{
    DesktopHostedComponent host(panel);
    auto& waveformTabButton = requireButton(panel, "authoringDrawerWaveformTab");
    auto& macrosTabButton = requireButton(panel, "authoringDrawerMacrosTab");
    auto& undoButton = requireButton(panel, "authoringUndoButton");
    auto& redoButton = requireButton(panel, "authoringRedoButton");
    auto& saveButton = requireButton(panel, "authoringSaveButton");
    auto& previewButton = requireButton(panel, "authoringPreviewButton");
    auto& zoneSelector = requireComboBox(panel, "authoringZoneSelector");
    auto& zoneMap = requireZoneMapCanvas(panel, "authoringZoneMap");
    auto& drawerToggleButton = requireButton(panel, "authoringDrawerToggleButton");
    auto& restoreRootKeyButton = requireButton(panel, "authoringRestoreRootKeyButton");
    auto& macroList = requireRepeatedStructureList(panel, "authoringMacroList");
    auto ensureSectionOpen = [&](const juce::String& targetComponentId, const juce::String& disclosureButtonId)
    {
        if (findDescendantById(panel, targetComponentId)->getBounds().isEmpty())
            requireButton(panel, disclosureButtonId).onClick();
    };

    zoneSelector.setSelectedId(2, juce::sendNotificationSync);
    pumpMessages();
    require(session.getSelectedZone()->id == "pad-a3-high",
            "Keyboard workflow smoke test requires the Phase 2 pad-a3-high zone.");
    ensureSectionOpen("authoringRestoreRootKeyButton", "authoringAdvancedInspectorSectionDisclosure");
    pumpMessages();
    const auto originalZoneId = session.getSelectedZone()->id;
    const auto originalMacroIndex = macroList.getSelectedIndex();

    previewButton.grabKeyboardFocus();
    pumpMessages();
    requireFocusedWithin(previewButton,
                         "Keyboard workflow should begin on a reachable summary action.");
    zoneSelector.grabKeyboardFocus();
    pumpMessages();
    requireFocusedWithin(zoneSelector,
                         "Keyboard workflow should allow the zone selector to take keyboard focus.");
    zoneMap.grabKeyboardFocus();
    pumpMessages();
    requireFocusedWithin(zoneMap,
                         "Keyboard workflow should allow the zone map to take keyboard focus.");
    drawerToggleButton.grabKeyboardFocus();
    pumpMessages();
    requireFocusedWithin(drawerToggleButton,
                         "Keyboard workflow should allow the drawer toggle to take keyboard focus.");

    macrosTabButton.onClick();
    pumpMessages();
    auto& macroListBox = macroList.getListBox();
    macroListBox.grabKeyboardFocus();
    pumpMessages();
    requireFocusedWithin(macroListBox,
                         "Keyboard workflow should allow the macro list to take keyboard focus.");
    if (macroList.getRowCount() > 1)
    {
        require(macroListBox.keyPressed(juce::KeyPress(juce::KeyPress::downKey)),
                "Keyboard workflow should allow the macro list to respond to the down arrow.");
        require(macroList.getSelectedIndex() != originalMacroIndex,
                "Keyboard workflow should move the macro selection when navigating the list by keyboard.");
    }

    waveformTabButton.onClick();
    pumpMessages();
    zoneMap.grabKeyboardFocus();
    pumpMessages();
    requireFocusedWithin(zoneMap,
                         "Keyboard workflow should restore focus to the zone map after returning to waveform detail.");
    const auto zoneBeforeKeyNavigation = session.getSelectedZone()->id;
    require(zoneMap.keyPressed(juce::KeyPress(juce::KeyPress::rightKey)),
            "Keyboard workflow should allow the zone map to respond to arrow-key navigation.");
    require(session.getSelectedZone()->id != zoneBeforeKeyNavigation,
            "Keyboard workflow should change the selected zone after zone-map keyboard navigation.");

    requireAccessibilityAction(previewButton, juce::AccessibilityActionType::press, "Preview button");
    requireAccessibilityAction(undoButton, juce::AccessibilityActionType::press, "Undo button");
    requireAccessibilityAction(redoButton, juce::AccessibilityActionType::press, "Redo button");
    requireAccessibilityAction(saveButton, juce::AccessibilityActionType::press, "Mark saved button");
    requireAccessibilityAction(restoreRootKeyButton,
                               juce::AccessibilityActionType::press,
                               "Restore root key button");

    if (originalMacroIndex >= 0 && originalMacroIndex < macroList.getRowCount())
        macroList.getListBox().selectRow(originalMacroIndex);
    if (session.getSelectedZone()->id != originalZoneId)
        zoneSelector.setSelectedId(2, juce::sendNotificationSync);

    waveformTabButton.onClick();
    pumpMessages();
    if (shellName == "compact" && findDescendantById(panel, "authoringWaveformPreview")->isVisible())
    {
        drawerToggleButton.onClick();
        pumpMessages();
    }
}

void exerciseMapSelectionBehavior(drs::app::AuthoringPanel& panel,
                                  drs::engine::AuthoringSession& session)
{
    auto& zoneMap = requireZoneMapCanvas(panel, "authoringZoneMap");
    auto& zoneSelector = requireComboBox(panel, "authoringZoneSelector");
    auto& rootKeySlider = requireSlider(panel, "authoringRootKeySlider");

    const auto initialUndoDepth = session.getDocumentState().undoDepth;
    const auto summaries = session.getZoneSummaries();
    require(summaries.size() >= 3, "Map selection checks require the Phase 2 reference zones.");

    const auto padIterator = std::find_if(summaries.begin(),
                                          summaries.end(),
                                          [](const auto& zone)
                                          {
                                              return zone.id == "pad-a3-high";
                                          });
    require(padIterator != summaries.end(), "Map selection checks require the pad-a3-high zone.");

    const auto padIndex = static_cast<int>(std::distance(summaries.begin(), padIterator));
    require(zoneMap.requestSelectionAt(computeZoneMapPoint(zoneMap, *padIterator)),
            "Zone map should select a zone when clicked within its bounds.");
    require(session.getSelectedZone()->id == "pad-a3-high",
            "Zone map click selection should retarget the selected session zone.");
    require(zoneSelector.getSelectedId() == padIndex + 1,
            "Zone selector should stay synchronized with map click selection.");
    require(static_cast<int>(rootKeySlider.getValue()) == session.getSelectedZone()->rootKey,
            "Zone inspector should refresh to the map-selected zone values.");
    require(componentTreeContainsLabelText(panel,
                                           "Sample source: "
                                               + juce::String::fromUTF8(session.getSelectedZone()->sampleSourceId.c_str())),
            "Summary strip should refresh its sample-source text after map click selection.");

    require(zoneMap.keyPressed(juce::KeyPress(juce::KeyPress::rightKey)),
            "Zone map should accept right-arrow keyboard navigation.");
    require(session.getSelectedZone()->id == "lead-a4-sustain",
            "Zone map keyboard navigation should advance to the next zone.");
    require(zoneSelector.getSelectedId() == static_cast<int>(summaries.size()),
            "Zone selector should stay synchronized with map keyboard navigation.");
    require(static_cast<int>(rootKeySlider.getValue()) == session.getSelectedZone()->rootKey,
            "Zone inspector should refresh after keyboard-driven map selection changes.");
    require(componentTreeContainsLabelText(panel,
                                           "Sample source: "
                                               + juce::String::fromUTF8(session.getSelectedZone()->sampleSourceId.c_str())),
            "Summary strip should refresh after keyboard-driven map selection changes.");
    require(session.getDocumentState().undoDepth == initialUndoDepth + 2,
            "Map click and keyboard selection should each create one selection transaction.");

    const auto selectedZoneBeforeDrag = makeZoneSummary(*session.getSelectedZone());
    const auto keyLowHandle = computeZoneMapHandlePoint(zoneMap,
                                                        selectedZoneBeforeDrag,
                                                        drs::app::authoring::ZoneMapCanvas::RangeHandle::keyLow);
    require(zoneMap.beginRangeGestureAt(keyLowHandle),
            "Zone map should begin a key-range gesture from the selected zone handle.");
    require(zoneMap.isRangeGestureActive(),
            "Zone map should report an active gesture while a range drag is in progress.");
    const auto keyDragTarget = computeZoneMapTargetPointForKey(zoneMap, selectedZoneBeforeDrag, 72);
    require(zoneMap.updateActiveRangeGesture(keyDragTarget),
            "Zone map should update the live key-range gesture preview.");
    require(zoneMap.endActiveRangeGesture(keyDragTarget),
            "Zone map should finish a key-range gesture on release.");
    require(!zoneMap.isRangeGestureActive(),
            "Zone map should clear its active gesture state after commit.");
    require(session.getDocumentState().undoDepth == initialUndoDepth + 3,
            "Completing one map range drag should create exactly one additional undo transaction.");
    require(session.getSelectedZone()->keyLow == 72,
            "Zone map key-range drags should persist the committed low-key boundary.");
    require(static_cast<int>(rootKeySlider.getValue()) == session.getSelectedZone()->rootKey,
            "Zone inspector should remain synchronized after map range edits.");
    require(static_cast<int>(requireSlider(panel, "authoringKeyLowSlider").getValue()) == 72,
            "Zone inspector key-low control should refresh after a committed map drag.");

    const auto zoneBeforeCancel = makeZoneSummary(*session.getSelectedZone());
    const auto velocityLowHandle = computeZoneMapHandlePoint(zoneMap,
                                                             zoneBeforeCancel,
                                                             drs::app::authoring::ZoneMapCanvas::RangeHandle::velocityLow);
    require(zoneMap.beginRangeGestureAt(velocityLowHandle),
            "Zone map should begin a velocity-range gesture from the selected zone handle.");
    const auto velocityDragTarget = computeZoneMapTargetPointForVelocity(zoneMap, zoneBeforeCancel, 32);
    require(zoneMap.updateActiveRangeGesture(velocityDragTarget),
            "Zone map should update the live velocity-range gesture preview.");
    require(zoneMap.cancelActiveRangeGesture(),
            "Zone map should cancel an in-flight drag without committing.");
    require(!zoneMap.isRangeGestureActive(),
            "Zone map should clear its active gesture state after cancel.");
    require(session.getDocumentState().undoDepth == initialUndoDepth + 3,
            "Cancelling a map range drag should not create a new undo transaction.");
    require(session.getSelectedZone()->velocityLow == zoneBeforeCancel.velocityLow,
            "Cancelling a map range drag should preserve the original velocity range.");
    require(static_cast<int>(requireSlider(panel, "authoringVelocityLowSlider").getValue())
                == zoneBeforeCancel.velocityLow,
            "Zone inspector should remain unchanged after a cancelled map drag.");
}

void exerciseGateAWorkflow(drs::app::AuthoringPanel& panel,
                           drs::engine::AuthoringSession& session,
                           const std::string& shellName,
                           const fs::path& outputDirectory,
                           std::ostream& inventory,
                           int& previewStartCount,
                           int& previewEndCount,
                           int& restoreRootKeyCount,
                           int& lastPreviewMidiNote,
                           float& lastPreviewVelocity)
{
    auto& zoneSelector = requireComboBox(panel, "authoringZoneSelector");
    auto& zoneMap = requireZoneMapCanvas(panel, "authoringZoneMap");
    auto& previewButton = requireButton(panel, "authoringPreviewButton");
    auto& undoButton = requireButton(panel, "authoringUndoButton");
    auto& redoButton = requireButton(panel, "authoringRedoButton");
    auto& saveButton = requireButton(panel, "authoringSaveButton");

    zoneSelector.setSelectedId(2, juce::sendNotificationSync);
    require(session.getSelectedZone()->id == "pad-a3-high",
            "Gate A workflow zone selection should retarget the selected zone.");

    const auto originalRootKey = session.getSelectedZone()->rootKey;
    const auto undoDepthAfterSelection = session.getDocumentState().undoDepth;

    auto ensureSectionOpen = [&](const juce::String& targetComponentId, const juce::String& disclosureButtonId)
    {
        if (findDescendantById(panel, targetComponentId)->getBounds().isEmpty())
            requireButton(panel, disclosureButtonId).onClick();
    };

    const auto selectedZone = makeZoneSummary(*session.getSelectedZone());
    const auto keyHighHandle = computeZoneMapHandlePoint(zoneMap,
                                                         selectedZone,
                                                         drs::app::authoring::ZoneMapCanvas::RangeHandle::keyHigh);
    const auto keyHighTarget = computeZoneMapTargetPointForKey(zoneMap, selectedZone, 84);
    require(zoneMap.beginRangeGestureAt(keyHighHandle),
            "Gate A workflow should begin the map key-range gesture.");
    require(zoneMap.updateActiveRangeGesture(keyHighTarget),
            "Gate A workflow should update the map key-range gesture.");
    require(zoneMap.endActiveRangeGesture(keyHighTarget),
            "Gate A workflow should commit the map key-range gesture.");
    require(session.getSelectedZone()->keyHigh == 84,
            "Gate A workflow key-range drag should persist the edited key-high boundary.");

    const auto zoneAfterKeyDrag = makeZoneSummary(*session.getSelectedZone());
    const auto velocityHighHandle = computeZoneMapHandlePoint(zoneMap,
                                                              zoneAfterKeyDrag,
                                                              drs::app::authoring::ZoneMapCanvas::RangeHandle::velocityHigh);
    const auto velocityHighTarget = computeZoneMapTargetPointForVelocity(zoneMap, zoneAfterKeyDrag, 112);
    require(zoneMap.beginRangeGestureAt(velocityHighHandle),
            "Gate A workflow should begin the map velocity-range gesture.");
    require(zoneMap.updateActiveRangeGesture(velocityHighTarget),
            "Gate A workflow should update the map velocity-range gesture.");
    require(zoneMap.endActiveRangeGesture(velocityHighTarget),
            "Gate A workflow should commit the map velocity-range gesture.");
    require(session.getSelectedZone()->velocityHigh == 112,
            "Gate A workflow velocity-range drag should persist the edited velocity-high boundary.");

    auto& rootKeySlider = requireSlider(panel, "authoringRootKeySlider");
    rootKeySlider.onDragStart();
    rootKeySlider.setValue(64.0, juce::dontSendNotification);
    rootKeySlider.onDragEnd();
    require(session.getSelectedZone()->rootKey == 64,
            "Gate A workflow should commit root-key edits through the compact inspector.");

    ensureSectionOpen("authoringGainSlider", "authoringMixInspectorSectionDisclosure");
    auto& gainSlider = requireSlider(panel, "authoringGainSlider");
    gainSlider.onDragStart();
    gainSlider.setValue(1.5, juce::dontSendNotification);
    gainSlider.onDragEnd();
    require(std::abs(session.getSelectedZone()->gainDb - 1.5) < 0.001,
            "Gate A workflow should commit gain edits through the compact inspector.");

    auto& panSlider = requireSlider(panel, "authoringPanSlider");
    panSlider.onDragStart();
    panSlider.setValue(-0.2, juce::dontSendNotification);
    panSlider.onDragEnd();
    require(std::abs(session.getSelectedZone()->pan - (-0.2)) < 0.001,
            "Gate A workflow should commit pan edits through the compact inspector.");

    ensureSectionOpen("authoringRestoreRootKeyButton", "authoringAdvancedInspectorSectionDisclosure");
    auto& loopToggle = requireToggleButton(panel, "authoringLoopEnabledToggle");
    loopToggle.setToggleState(true, juce::dontSendNotification);
    loopToggle.onClick();
    require(session.getSelectedZone()->loopEnabled,
            "Gate A workflow should commit loop-toggle edits through the compact inspector.");

    auto& triggerModeSelector = requireComboBox(panel, "authoringTriggerModeSelector");
    triggerModeSelector.setSelectedId(2, juce::sendNotificationSync);
    require(session.getSelectedZone()->triggerMode == drs::engine::ZoneTriggerMode::oneShot,
            "Gate A workflow should commit one-shot trigger mode through the compact inspector.");

    const auto expectedPreview = session.buildSelectedZonePreviewRequest();
    require(expectedPreview.available, "Gate A workflow preview should be available after zone edits.");
    previewButton.onClick();
    require(previewStartCount == 1, "Gate A workflow should emit one preview-start callback.");
    require(lastPreviewMidiNote == expectedPreview.midiNote,
            "Gate A workflow preview callback should use the selected-zone preview note.");
    require(std::abs(lastPreviewVelocity - (static_cast<float>(expectedPreview.velocity) / 127.0f)) < 0.001f,
            "Gate A workflow preview callback should use the selected-zone preview velocity.");

    const auto undoDepthBeforeRestore = session.getDocumentState().undoDepth;
    requireButton(panel, "authoringRestoreRootKeyButton").onClick();
    require(restoreRootKeyCount == 1, "Gate A workflow should invoke restore-root-key exactly once.");
    require(session.getSelectedZone()->rootKey == originalRootKey,
            "Gate A workflow restore-root-key action should restore the zone root key.");
    require(session.getDocumentState().undoDepth == undoDepthBeforeRestore + 1,
            "Gate A workflow restore-root-key action should create one undo transaction.");

    undoButton.onClick();
    require(session.getSelectedZone()->rootKey == 64,
            "Gate A workflow undo should restore the pre-restore root key.");
    redoButton.onClick();
    require(session.getSelectedZone()->rootKey == originalRootKey,
            "Gate A workflow redo should reapply the restored root key.");

    require(session.getDocumentState().dirty,
            "Gate A workflow should leave the project dirty before marking a save checkpoint.");
    saveButton.onClick();
    require(!session.getDocumentState().dirty,
            "Gate A workflow mark-saved action should clear the dirty flag.");
    require(session.getDocumentState().undoDepth >= undoDepthAfterSelection + 6,
            "Gate A workflow should leave behind the expected edit history depth.");

    inventory << shellName << " / gate-a\n";
    inventory << "  zone=" << session.getSelectedZone()->id
              << " key=" << session.getSelectedZone()->keyLow << "-" << session.getSelectedZone()->keyHigh
              << " vel=" << session.getSelectedZone()->velocityLow << "-" << session.getSelectedZone()->velocityHigh
              << " root=" << session.getSelectedZone()->rootKey
              << " gain=" << session.getSelectedZone()->gainDb
              << " pan=" << session.getSelectedZone()->pan
              << " loop=" << (session.getSelectedZone()->loopEnabled ? "true" : "false")
              << " trigger=" << (session.getSelectedZone()->triggerMode == drs::engine::ZoneTriggerMode::oneShot
                                      ? "one-shot" : "gated")
              << " dirty=" << (session.getDocumentState().dirty ? "true" : "false")
              << " undo=" << session.getDocumentState().undoDepth
              << " redo=" << session.getDocumentState().redoDepth
              << " previewMidi=" << lastPreviewMidiNote
              << " previewVelocity=" << lastPreviewVelocity
              << " previewEndCallbacks=" << previewEndCount
              << "\n\n";

    saveComponentPng(panel, outputDirectory / (shellName + "-gate-a-final.png"));
}

void exerciseCrossfadeDebugVisibility()
{
    const auto projectLoad = drs::engine::loadPhase2ReferenceProjectManifest();
    require(projectLoad.loaded, "Phase 2 reference project must load for crossfade debug visibility coverage.");

    auto project = projectLoad.project;
    require(!project.authoring.zones.empty(),
            "Crossfade debug visibility coverage requires at least one authored zone.");

    auto& crossfadeZone = project.authoring.zones.front();
    crossfadeZone.velocityLow = 1;
    crossfadeZone.velocityHigh = 127;
    crossfadeZone.velocityCrossfade.fadeInLowVelocity = 1;
    crossfadeZone.velocityCrossfade.fadeInHighVelocity = 32;
    crossfadeZone.velocityCrossfade.fadeOutLowVelocity = 96;
    crossfadeZone.velocityCrossfade.fadeOutHighVelocity = 127;
    project.authoring.selectedZoneId = crossfadeZone.id;

    drs::engine::AuthoringSession session(project);
    drs::app::AuthoringPanel panel(session,
                                   []()
                                   {
                                       return makePreviewFixture();
                                   },
                                   []()
                                   {
                                       return makeAuthoringPreviewStatusFixture();
                                   },
                                   []()
                                   {
                                       return makeImportMetricsFixture();
                                   },
                                   drs::app::AuthoringPanel::LayoutMode::expanded,
                                   []() {},
                                   []()
                                   {
                                       return makeDraftPlaybackStatusFixture();
                                   });
    panel.setTopLeftPosition(0, 0);
    panel.setSize(1400, 900);
    panel.setVisible(true);
    panel.resized();
    panel.reloadFromSession();

    auto& zoneSelector = requireComboBox(panel, "authoringZoneSelector");
    auto crossfadeVisible = false;
    for (int index = 0; index < zoneSelector.getNumItems(); ++index)
    {
        if (zoneSelector.getItemText(index).contains("Xfade in 1-32 out 96-127"))
        {
            crossfadeVisible = true;
            break;
        }
    }

    require(crossfadeVisible,
            "Zone selector debug text should surface authored velocity crossfade ranges for inspection.");
}
} // namespace

int main()
{
    try
    {
        juce::ScopedJuceInitialiser_GUI gui;

        drs::app::authoring::ZoneMapCanvas dropTargetZoneMap;
        std::vector<juce::File> droppedSampleFiles;
        dropTargetZoneMap.setOnSampleFilesDropped([&droppedSampleFiles](std::vector<juce::File> files)
        {
            droppedSampleFiles = std::move(files);
        });
        juce::StringArray unsupportedDrop { "C:\\samples\\notes.txt", "C:\\samples\\kick.mp3" };
        require(!dropTargetZoneMap.isInterestedInFileDrag(unsupportedDrop),
                "Zone Map should reject file drops that contain no WAV or FLAC samples.");
        juce::StringArray mixedDrop {
            "C:\\samples\\Piano_C3.WAV",
            "C:\\samples\\Piano_E3.flac",
            "C:\\samples\\readme.txt"
        };
        require(dropTargetZoneMap.isInterestedInFileDrag(mixedDrop),
                "Zone Map should accept a drop containing WAV or FLAC samples.");
        dropTargetZoneMap.filesDropped(mixedDrop, 0, 0);
        require(droppedSampleFiles.size() == 2
                    && droppedSampleFiles[0].hasFileExtension(".wav")
                    && droppedSampleFiles[1].hasFileExtension(".flac"),
                "Zone Map should forward only dropped WAV and FLAC files to the import callback.");

        const auto projectLoad = drs::engine::loadPhase2ReferenceProjectManifest();
        require(projectLoad.loaded, "Phase 2 reference project must load for authoring UI characterization.");

        const auto outputDirectory = fs::temp_directory_path() / "drs-phase2-authoring-ui-tests";
        fs::create_directories(outputDirectory);
        std::ofstream inventory(outputDirectory / "component-bounds.txt", std::ios::binary);
        require(inventory.good(), "Could not create authoring UI component inventory output.");
        std::vector<std::string> baselineFindings;

        exerciseSummaryStripLeaf(outputDirectory);
        exerciseZoneMappingEditorLeaf(outputDirectory);
        exerciseCrossfadeDebugVisibility();
        exerciseRepeatedStructureListComponent();
        writeReachabilityChecklist(inventory);

        auto runShell = [&](const std::string& shellName,
                            drs::app::AuthoringPanel::LayoutMode layoutMode,
                            int width,
                            int height)
        {
            drs::engine::AuthoringSession session(projectLoad.project);
            int previewStartCount = 0;
            int previewEndCount = 0;
            int restoreRootKeyCount = 0;
            int lastPreviewMidiNote = -1;
            float lastPreviewVelocity = -1.0f;
            drs::app::AuthoringPanel* panelPtr = nullptr;
            drs::app::AuthoringPanel panel(session,
                                           []()
                                           {
                                               return makePreviewFixture();
                                           },
                                           []()
                                           {
                                               return makeAuthoringPreviewStatusFixture();
                                           },
                                           []()
                                           {
                                               return makeImportMetricsFixture();
                                           },
                                           layoutMode,
                                           [&session, &restoreRootKeyCount, &panelPtr]()
                                           {
                                               ++restoreRootKeyCount;

                                               const auto selectedZone = session.getSelectedZone();
                                               if (!selectedZone.has_value())
                                                   return;

                                               auto restoredZone = *selectedZone;
                                               restoredZone.rootKey = restoredZone.id == "pad-a3-high" ? 57 : 69;
                                               session.updateSelectedZone(restoredZone, "Restore zone root key");

                                               if (panelPtr != nullptr)
                                                   panelPtr->reloadFromSession();
                                           },
                                           []()
                                           {
                                               return makeDraftPlaybackStatusFixture();
                                           },
                                           {},
                                           {},
                                           [&previewStartCount,
                                            &previewEndCount,
                                            &lastPreviewMidiNote,
                                            &lastPreviewVelocity](const drs::engine::AuthoringPreviewCommand& command)
                                           {
                                               if (command.type == drs::engine::AuthoringPreviewCommandType::auditionSelectedZone)
                                               {
                                                   ++previewStartCount;
                                                   lastPreviewMidiNote = command.midiNote;
                                                   lastPreviewVelocity = command.velocity;
                                               }
                                               else if (command.type == drs::engine::AuthoringPreviewCommandType::noteOff)
                                               {
                                                   ++previewEndCount;
                                               }
                                           });
            panelPtr = &panel;
            panel.setTopLeftPosition(0, 0);
            panel.setSize(width, height);
            panel.setVisible(true);
            panel.resized();
            panel.reloadFromSession();

            require(panel.getWidth() == width && panel.getHeight() == height,
                    "Authoring panel size did not match the requested shell baseline.");
            requireRetiredTemporaryIdsAbsent(panel);
            requireUniqueNonEmptyComponentIds(panel);

            exerciseMapSelectionBehavior(panel, session);
            exerciseGateAWorkflow(panel,
                                  session,
                                  shellName,
                                  outputDirectory,
                                  inventory,
                                  previewStartCount,
                                  previewEndCount,
                                  restoreRootKeyCount,
                                  lastPreviewMidiNote,
                                  lastPreviewVelocity);
            exerciseKeyboardOnlyWorkflowSmoke(panel, session, shellName);
            exerciseDrawerBehavior(panel, shellName, baselineFindings);
            exerciseAccessibilityAndFocusBehavior(panel, shellName);
            exerciseDrawerEditorTransactions(panel, session);
            exerciseSurface(panel, 1, shellName, "mapping", outputDirectory, inventory, baselineFindings);
            exerciseSurface(panel, 2, shellName, "macros", outputDirectory, inventory, baselineFindings);
            exerciseSurface(panel, 3, shellName, "routing", outputDirectory, inventory, baselineFindings);
            exerciseSurface(panel, 4, shellName, "performance", outputDirectory, inventory, baselineFindings);
            exerciseHostedFocusTransitions(panel, shellName);
        };

        runShell("compact",
                 drs::app::AuthoringPanel::LayoutMode::compact,
                 drs::app::authoring::compactShellWidth,
                 drs::app::authoring::compactShellHeight);

        runShell("expanded-target",
                 drs::app::AuthoringPanel::LayoutMode::expanded,
                 drs::app::authoring::expandedTargetShellWidth,
                 drs::app::authoring::expandedTargetShellHeight);

        runShell("expanded-min",
                 drs::app::AuthoringPanel::LayoutMode::expanded,
                 drs::app::authoring::expandedMinimumShellWidth,
                 drs::app::authoring::minimumShellHeight);

        inventory << "Baseline findings\n";
        for (const auto& finding : baselineFindings)
            inventory << "- " << finding << "\n";
        require(baselineFindings.empty(),
                "Authoring UI baseline findings must be empty after Sprint 1 layout hardening.");

        std::cout << "Phase 2 authoring UI characterization tests passed. Output: "
                  << outputDirectory.string() << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Phase 2 authoring UI characterization tests failed: "
                  << exception.what() << std::endl;
        return 1;
    }
}
