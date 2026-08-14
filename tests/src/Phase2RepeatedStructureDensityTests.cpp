#include "drs/engine/AuthoringSession.h"
#include "drs/engine/RuntimeLoader.h"
#include "shared/AuthoringPanel.h"
#include "shared/authoring/RepeatedStructureAdapters.h"
#include "shared/authoring/AuthoringWorkspaceLayout.h"
#include "shared/authoring/RepeatedStructureList.h"

#include <juce_gui_extra/juce_gui_extra.h>

#include <array>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace
{
namespace fs = std::filesystem;

void require(bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
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
            "Component should remain inside the harness bounds: " + componentId.toStdString());
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

drs::app::authoring::RepeatedStructureList& requireRepeatedStructureList(juce::Component& root,
                                                                         const juce::String& componentId)
{
    auto* list = dynamic_cast<drs::app::authoring::RepeatedStructureList*>(findDescendantById(root, componentId));
    require(list != nullptr, "Missing repeated-structure list ID: " + componentId.toStdString());
    return *list;
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

struct FixtureScopeRow
{
    std::string key;
    std::string scopeLabel;
    std::string title;
    std::string statusText;
    std::string detailText;
    bool enabled = true;
};

struct FixtureScopeCase
{
    std::string outputStem;
    std::string fixtureTitle;
    std::string selectionPathRoot;
    std::vector<FixtureScopeRow> rows;
    int missingRowIndex = -1;
    int disabledRowIndex = -1;
};

class DensityFixtureHarness final : public juce::Component
{
public:
    DensityFixtureHarness()
        : repeatedStructureList("densityFixtureList",
                                "densityFixtureListBox",
                                "densityFixtureEmptyState")
    {
        setComponentID("densityFixtureHarness");

        fixtureTitleLabel.setComponentID("densityFixtureTitleLabel");
        selectionPathLabel.setComponentID("densityFixturePathLabel");
        scopeLabel.setComponentID("densityFixtureScopeLabel");
        detailTitleLabel.setComponentID("densityFixtureDetailTitleLabel");
        detailStatusLabel.setComponentID("densityFixtureDetailStatusLabel");
        detailBodyLabel.setComponentID("densityFixtureDetailBodyLabel");

        for (auto* label : { &fixtureTitleLabel,
                             &selectionPathLabel,
                             &scopeLabel,
                             &detailTitleLabel,
                             &detailStatusLabel,
                             &detailBodyLabel })
        {
            label->setJustificationType(juce::Justification::topLeft);
            addAndMakeVisible(label);
        }

        fixtureTitleLabel.setFont(juce::FontOptions(16.0f, juce::Font::bold));
        selectionPathLabel.setFont(juce::FontOptions(12.5f));
        scopeLabel.setFont(juce::FontOptions(12.5f, juce::Font::bold));
        detailTitleLabel.setFont(juce::FontOptions(14.0f, juce::Font::bold));
        detailStatusLabel.setFont(juce::FontOptions(12.0f));
        detailBodyLabel.setFont(juce::FontOptions(12.0f));

        detailBodyLabel.setMinimumHorizontalScale(0.9f);
        detailBodyLabel.setColour(juce::Label::textColourId, juce::Colour::fromRGB(82, 86, 94));
        detailStatusLabel.setColour(juce::Label::textColourId, juce::Colour::fromRGB(82, 86, 94));
        selectionPathLabel.setColour(juce::Label::textColourId, juce::Colour::fromRGB(82, 86, 94));

        repeatedStructureList.setOnSelectionChanged([this](int selectedIndex)
        {
            if (!paneBuilder || suppressSelectionRefresh)
                return;

            applyPaneViewModel(paneBuilder(selectedIndex));
        });
        addAndMakeVisible(repeatedStructureList);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(12);
        fixtureTitleLabel.setBounds(area.removeFromTop(22));
        area.removeFromTop(2);
        selectionPathLabel.setBounds(area.removeFromTop(16));
        area.removeFromTop(8);

        auto content = area;
        auto listArea = content.removeFromLeft(std::min(360, content.getWidth() / 2));
        repeatedStructureList.setBounds(listArea);
        content.removeFromLeft(12);

        scopeLabel.setBounds(content.removeFromTop(18));
        content.removeFromTop(6);
        detailTitleLabel.setBounds(content.removeFromTop(42));
        content.removeFromTop(4);
        detailStatusLabel.setBounds(content.removeFromTop(18));
        content.removeFromTop(6);
        detailBodyLabel.setBounds(content);
    }

    void setPaneViewModel(drs::app::authoring::RepeatedStructurePaneViewModel nextPane)
    {
        pane = std::move(nextPane);
        applyPaneViewModel(pane);
    }

    template <typename Adapter>
    void setAdapter(Adapter adapter)
    {
        paneBuilder = [adapter = std::move(adapter)](int selectedIndexOverride)
        {
            return drs::app::authoring::buildRepeatedStructurePaneViewModel(adapter,
                                                                            selectedIndexOverride);
        };

        applyPaneViewModel(paneBuilder(-1));
    }

    const drs::app::authoring::RepeatedStructurePaneViewModel& getPaneViewModel() const { return pane; }
    drs::app::authoring::RepeatedStructureList& getRepeatedStructureList() { return repeatedStructureList; }

private:
    void applyPaneViewModel(const drs::app::authoring::RepeatedStructurePaneViewModel& nextPane)
    {
        pane = nextPane;
        fixtureTitleLabel.setText(juce::String::fromUTF8(pane.title.c_str()), juce::dontSendNotification);
        selectionPathLabel.setText(juce::String::fromUTF8(pane.selectionPath.breadcrumbText.c_str()),
                                   juce::dontSendNotification);
        scopeLabel.setText(juce::String::fromUTF8(pane.selectionPath.scopeLabel.c_str()),
                           juce::dontSendNotification);
        detailTitleLabel.setText(juce::String::fromUTF8(pane.detail.title.c_str()),
                                 juce::dontSendNotification);
        detailStatusLabel.setText(juce::String::fromUTF8(pane.detail.statusText.c_str()),
                                  juce::dontSendNotification);
        detailBodyLabel.setText(juce::String::fromUTF8(pane.detail.bodyText.c_str()),
                                juce::dontSendNotification);

        const auto scopedSelectionRefresh = juce::ScopedValueSetter<bool>(suppressSelectionRefresh, true);
        repeatedStructureList.setViewModel(pane.list);
    }

    drs::app::authoring::RepeatedStructurePaneViewModel pane;
    std::function<drs::app::authoring::RepeatedStructurePaneViewModel(int)> paneBuilder;
    bool suppressSelectionRefresh = false;
    juce::Label fixtureTitleLabel;
    juce::Label selectionPathLabel;
    juce::Label scopeLabel;
    juce::Label detailTitleLabel;
    juce::Label detailStatusLabel;
    juce::Label detailBodyLabel;
    drs::app::authoring::RepeatedStructureList repeatedStructureList;
};

class FixtureScopeAdapter
{
public:
    explicit FixtureScopeAdapter(const FixtureScopeCase& nextFixture,
                                 int nextSelectedIndex = 0,
                                 drs::app::authoring::RepeatedStructureAdapterCallbacks nextCallbacks = {})
        : fixture(nextFixture),
          selectedIndex(nextSelectedIndex),
          callbacks(std::move(nextCallbacks))
    {
    }

    std::string getPaneTitle() const { return fixture.fixtureTitle; }
    std::string getListEmptyStateText() const { return "No repeated rows in this fixture."; }
    int getSelectedIndex() const { return selectedIndex; }
    int getItemCount() const { return static_cast<int>(fixture.rows.size()); }

    drs::app::authoring::RepeatedStructureRowViewModel getRowViewModel(int index) const
    {
        const auto& row = fixture.rows.at(static_cast<std::size_t>(index));

        drs::app::authoring::RepeatedStructureRowViewModel viewModel;
        viewModel.key = row.key;
        viewModel.title = row.title;
        viewModel.statusText = row.statusText;
        viewModel.enabled = row.enabled;
        return viewModel;
    }

    drs::app::authoring::RepeatedStructureSelectionPathViewModel getSelectionPathViewModel(int index) const
    {
        const auto& row = fixture.rows.at(static_cast<std::size_t>(index));
        return {
            row.scopeLabel,
            fixture.selectionPathRoot + " > " + row.title
        };
    }

    drs::app::authoring::RepeatedStructureDetailViewModel getDetailViewModel(int index) const
    {
        const auto& row = fixture.rows.at(static_cast<std::size_t>(index));
        return {
            row.title,
            row.statusText,
            row.detailText
        };
    }

    drs::app::authoring::RepeatedStructureSelectionPathViewModel getEmptySelectionPathViewModel() const
    {
        return {
            "No scope row selected",
            fixture.selectionPathRoot
        };
    }

    drs::app::authoring::RepeatedStructureDetailViewModel getEmptyDetailViewModel() const
    {
        return {
            "No repeated row selected",
            "Selection unavailable",
            "Fixture detail binding is unavailable without a selected row."
        };
    }

    const drs::app::authoring::RepeatedStructureAdapterCallbacks& getCallbacks() const
    {
        return callbacks;
    }

private:
    FixtureScopeCase fixture;
    int selectedIndex = 0;
    drs::app::authoring::RepeatedStructureAdapterCallbacks callbacks;
};

FixtureScopeCase makeVelocityLayerFixture()
{
    FixtureScopeCase fixture;
    fixture.outputStem = "density-velocity-layers";
    fixture.fixtureTitle = "Velocity Layer Density Fixture";
    fixture.selectionPathRoot = "Project > Layers";
    fixture.rows = {
        {"vl-01", "Velocity layer detail", "Layer 01 - Whisper Sustain Legato (Studio Ribbon A)", "Vel 1-8 | ready", "Primary pianissimo sustain layer with the warmest transient envelope.", true},
        {"vl-02", "Velocity layer detail", "Layer 02 - Whisper Sustain Legato (Studio Ribbon B)", "Vel 9-16 | ready", "Secondary pianissimo layer with alternate onset balance for repeated phrasing.", true},
        {"vl-03", "Velocity layer detail", "Layer 03 - Delicate Swell Sustain with Long Editorial Name", "Vel 17-24 | ready", "Long-name layer fixture to pressure compact labels without inventing a Phase 3 schema.", true},
        {"vl-04", "Velocity layer detail", "Layer 04 - Mezzo Sustain Bloom with Printed Release Tail", "Vel 25-40 | ready", "Mid dynamic layer that keeps the detail card in the same compact surface.", true},
        {"vl-05", "Velocity layer detail", "Layer 05 - Forte Sustain Bloom with Extra Room Description", "Vel 41-56 | ready", "Higher dynamic layer with a deliberately long descriptor.", true},
        {"vl-06", "Velocity layer detail", "Layer 06 - Accent Sustain Missing Asset Placeholder", "Vel 57-72 | missing source", "This fixture row simulates a missing imported asset while remaining selectable.", true},
        {"vl-07", "Velocity layer detail", "Layer 07 - Marcato Lifted Bow Composite", "Vel 73-88 | ready", "Accent layer used to verify selection can move beyond the initial viewport.", true},
        {"vl-08", "Velocity layer detail", "Layer 08 - Fortissimo Stack Disabled by Articulation Rule", "Vel 89-104 | disabled", "This fixture row simulates a disabled layer that still exposes its scope detail.", false},
        {"vl-09", "Velocity layer detail", "Layer 09 - Aggressive Sustain Stack for Scroll Audit", "Vel 105-112 | ready", "Late-list selection should scroll into view and keep detail binding stable.", true},
        {"vl-10", "Velocity layer detail", "Layer 10 - Final Safety Layer for Compact Height Audit", "Vel 113-127 | ready", "Last layer row verifies bottom-of-list navigation in the compact harness.", true}
    };
    fixture.missingRowIndex = 5;
    fixture.disabledRowIndex = 7;
    return fixture;
}

FixtureScopeCase makeRoundRobinFixture()
{
    FixtureScopeCase fixture;
    fixture.outputStem = "density-round-robin";
    fixture.fixtureTitle = "Round Robin Density Fixture";
    fixture.selectionPathRoot = "Project > Round Robin";
    fixture.rows = {
        {"rr-01", "Round-robin detail", "Round Robin 01 - Primary Attack Capture", "Take 1 | ready", "Base attack row used for compact repeated-structure navigation.", true},
        {"rr-02", "Round-robin detail", "Round Robin 02 - Secondary Attack Capture", "Take 2 | ready", "Second alternation row keeps status text concise.", true},
        {"rr-03", "Round-robin detail", "Round Robin 03 - Long Editorial Note About Bow Noise Balance", "Take 3 | ready", "Long-name round-robin row ensures future alternation labels remain readable.", true},
        {"rr-04", "Round-robin detail", "Round Robin 04 - Close Mic Attack with Extended Descriptor", "Take 4 | ready", "Row title intentionally exceeds simple preset naming.", true},
        {"rr-05", "Round-robin detail", "Round Robin 05 - Missing Printed Stem", "Take 5 | missing source", "Missing-source round robin remains inspectable without opening a nested page.", true},
        {"rr-06", "Round-robin detail", "Round Robin 06 - Disabled By Compact Audition Filter", "Take 6 | disabled", "Disabled alternation row keeps its scope breadcrumb and detail text.", false},
        {"rr-07", "Round-robin detail", "Round Robin 07 - Alternate Hall Decay Tail", "Take 7 | ready", "Deeper list row for keyboard scroll coverage.", true},
        {"rr-08", "Round-robin detail", "Round Robin 08 - Alternate Close Ribbon Tail", "Take 8 | ready", "Later alternation row exercises tail-end selection.", true}
    };
    fixture.missingRowIndex = 4;
    fixture.disabledRowIndex = 5;
    return fixture;
}

FixtureScopeCase makeMicPositionFixture()
{
    FixtureScopeCase fixture;
    fixture.outputStem = "density-mic-positions";
    fixture.fixtureTitle = "Mic Position Density Fixture";
    fixture.selectionPathRoot = "Project > Mic Positions";
    fixture.rows = {
        {"mic-close", "Mic-position detail", "Close Ribbon - Detailed Chamber Perspective", "Online | ready", "Close position row used for the default detail binding.", true},
        {"mic-spot", "Mic-position detail", "Spot Condenser - Articulation Focus Position", "Online | ready", "Focused mic row with long descriptive naming.", true},
        {"mic-tree", "Mic-position detail", "Decca Tree - Wide Hall Perspective with Printed Descriptor", "Online | ready", "Wide-room row ensures long names render inside the compact list.", true},
        {"mic-outrigger", "Mic-position detail", "Outrigger Pair - Phase Review Pending", "Offline | missing source", "Mic position remains selectable while describing an offline stem.", true},
        {"mic-gallery", "Mic-position detail", "Gallery Ambient Pair - Disabled in Compact Mix Audit", "Muted | disabled", "Disabled mic position row should still update the detail panel scope.", false},
        {"mic-vintage", "Mic-position detail", "Vintage Mono Crush Return", "Online | ready", "Short-name row proves the layout handles mixed label lengths.", true}
    };
    fixture.missingRowIndex = 3;
    fixture.disabledRowIndex = 4;
    return fixture;
}

std::vector<FixtureScopeCase> buildFixtureCases()
{
    return {
        makeVelocityLayerFixture(),
        makeRoundRobinFixture(),
        makeMicPositionFixture()
    };
}

std::vector<drs::engine::RuntimeProjectMacroDefinition> buildDenseMacroFixture()
{
    std::vector<drs::engine::RuntimeProjectMacroDefinition> macros;
    macros.reserve(12);

    const std::array<std::string, 5> parameterIds{
        "filter-cutoff",
        "voice-pitch",
        "zone-gain",
        "zone-pan",
        "custom-spectral-blend"
    };
    const std::array<std::string, 5> parameterPaths{
        "engine.filter.main.cutoff",
        "engine.pitch.main.semitones",
        "authoring.zone.gainDb",
        "authoring.zone.pan",
        "engine.spectral.blend.ribbonHall"
    };
    const std::array<std::string, 5> roles{
        "timbre",
        "motion",
        "mix",
        "placement",
        "space"
    };

    for (int index = 0; index < 12; ++index)
    {
        drs::engine::RuntimeProjectMacroDefinition macro;
        macro.id = "macro-density-" + std::to_string(index + 1);
        macro.name = "Macro " + std::to_string(index + 1)
            + " - Compact Density Audit with Long Narrative Label "
            + std::to_string((index % 4) + 1);
        macro.minValue = 0.0;
        macro.maxValue = 1.0;
        macro.defaultValue = std::clamp(0.08 * static_cast<double>(index + 1), 0.05, 0.95);

        if (index != 10)
        {
            drs::engine::RuntimeProjectMacroTargetDefinition target;
            target.parameterId = parameterIds[static_cast<std::size_t>(index) % parameterIds.size()];
            target.parameterPath = parameterPaths[static_cast<std::size_t>(index) % parameterPaths.size()];
            target.role = roles[static_cast<std::size_t>(index) % roles.size()];
            macro.targets.push_back(std::move(target));
        }

        macros.push_back(std::move(macro));
    }

    return macros;
}

void exerciseFixtureHarness(const fs::path& outputDirectory)
{
    DensityFixtureHarness harness;
    harness.setTopLeftPosition(0, 0);
    harness.setSize(drs::app::authoring::compactShellWidth - 40, 220);
    harness.setVisible(true);

    for (const auto& fixture : buildFixtureCases())
    {
        harness.setAdapter(FixtureScopeAdapter(fixture));
        harness.resized();

        const auto bounds = harness.getLocalBounds();
        requireComponentVisibleWithin(harness, "densityFixtureTitleLabel", bounds);
        requireComponentVisibleWithin(harness, "densityFixturePathLabel", bounds);
        requireComponentVisibleWithin(harness, "densityFixtureList", bounds);
        requireComponentVisibleWithin(harness, "densityFixtureScopeLabel", bounds);
        requireComponentVisibleWithin(harness, "densityFixtureDetailTitleLabel", bounds);
        requireComponentVisibleWithin(harness, "densityFixtureDetailStatusLabel", bounds);
        requireComponentVisibleWithin(harness, "densityFixtureDetailBodyLabel", bounds);

        auto& list = harness.getRepeatedStructureList();
        auto& listBox = list.getListBox();
        require(list.getRowCount() == static_cast<int>(fixture.rows.size()),
                fixture.fixtureTitle + " should expose every test fixture row.");
        require(listBox.getSelectedRow() == 0,
                fixture.fixtureTitle + " should select the first row by default.");

        if (fixture.missingRowIndex >= 0)
        {
            listBox.selectRow(fixture.missingRowIndex);
            require(requireLabel(harness, "densityFixtureDetailStatusLabel").getText().toStdString()
                        == fixture.rows[static_cast<std::size_t>(fixture.missingRowIndex)].statusText,
                    fixture.fixtureTitle + " should bind missing-row status text into the detail card.");
        }

        if (fixture.disabledRowIndex >= 0)
        {
            listBox.selectRow(fixture.disabledRowIndex);
            require(requireLabel(harness, "densityFixtureScopeLabel").getText().toStdString()
                        == fixture.rows[static_cast<std::size_t>(fixture.disabledRowIndex)].scopeLabel,
                    fixture.fixtureTitle + " should keep disabled rows inspectable in the compact detail card.");
        }

        for (int index = listBox.getSelectedRow() + 1; index < list.getRowCount(); ++index)
        {
            require(listBox.keyPressed(juce::KeyPress(juce::KeyPress::downKey)),
                    fixture.fixtureTitle + " should consume down-arrow navigation across dense rows.");
        }

        const auto finalIndex = list.getRowCount() - 1;
        require(listBox.getSelectedRow() == finalIndex,
                fixture.fixtureTitle + " should allow selection to reach the final dense row.");
        require(requireLabel(harness, "densityFixtureDetailTitleLabel").getText().toStdString()
                    == fixture.rows.back().title,
                fixture.fixtureTitle + " should bind the last row title into the compact detail card.");
        require(requireLabel(harness, "densityFixturePathLabel").getText().toStdString().find(fixture.rows.back().title)
                    != std::string::npos,
                fixture.fixtureTitle + " should update the breadcrumb path when late rows are selected.");

        saveComponentPng(harness, outputDirectory / (fixture.outputStem + ".png"));
    }
}

void exerciseMacroDensityInProduction(const fs::path& outputDirectory)
{
    const auto projectLoad = drs::engine::loadPhase2ReferenceProjectManifest();
    require(projectLoad.loaded, "Phase 2 reference project must load for macro density validation.");

    auto project = projectLoad.project;
    project.authoring.macros = buildDenseMacroFixture();

    drs::engine::AuthoringSession session(project);
    drs::app::AuthoringPanel panel(session,
                                   []() { return drs::app::AuthoringWaveformPreview{}; },
                                   []() { return drs::app::AuthoringPreviewStatusSnapshot{}; },
                                   []() { return drs::app::AuthoringImportResponsivenessSnapshot{}; },
                                   drs::app::AuthoringPanel::LayoutMode::compact);
    panel.setTopLeftPosition(0, 0);
    panel.setSize(drs::app::authoring::compactShellWidth, drs::app::authoring::compactShellHeight);
    panel.setVisible(true);
    panel.resized();
    panel.reloadFromSession();

    auto& toggleButton = requireButton(panel, "authoringWorkbenchToggleButton");
    if (toggleButton.getButtonText() == "Show Workbench")
        toggleButton.onClick();
    requireButton(panel, "authoringWorkbenchMacrosTab").onClick();

    const auto panelBounds = panel.getLocalBounds();
    requireComponentVisibleWithin(panel, "authoringMacroList", panelBounds);
    requireComponentVisibleWithin(panel, "authoringMacroAssignmentSelector", panelBounds);
    requireComponentVisibleWithin(panel, "authoringMacroDefaultSlider", panelBounds);
    requireComponentVisibleWithin(panel, "authoringMacroMinSlider", panelBounds);
    requireComponentVisibleWithin(panel, "authoringMacroMaxSlider", panelBounds);

    auto& macroList = requireRepeatedStructureList(panel, "authoringMacroList");
    auto& listBox = macroList.getListBox();
    require(macroList.getRowCount() == static_cast<int>(project.authoring.macros.size()),
            "Production macro workbench should expose every dense fixture macro.");

    const auto targetIndex = static_cast<int>(project.authoring.macros.size()) - 1;
    for (int index = 0; index < targetIndex; ++index)
    {
        require(listBox.keyPressed(juce::KeyPress(juce::KeyPress::downKey)),
                "Production macro workbench should support keyboard navigation across dense macro rows.");
    }

    require(listBox.getSelectedRow() == targetIndex,
            "Production macro workbench should allow selection to reach the final dense macro row.");
    require(requireLabel(panel, "authoringWorkbenchBreadcrumbLabel").getText().toStdString().find(project.authoring.macros.back().name)
                != std::string::npos,
            "Production macro workbench breadcrumb should identify the selected dense macro row.");
    require(std::abs(requireSlider(panel, "authoringMacroDefaultSlider").getValue()
                     - project.authoring.macros.back().defaultValue) < 0.001,
            "Production macro workbench detail binding should follow the selected dense macro row.");

    listBox.selectRow(10);
    require(requireLabel(panel, "authoringWorkbenchBreadcrumbLabel").getText().toStdString().find(project.authoring.macros[10].name)
                != std::string::npos,
            "Production macro workbench should keep unassigned dense macro rows selectable.");

    saveComponentPng(panel, outputDirectory / "production-macro-density-compact.png");
}

void exercisePhase3AdapterContract(const fs::path& outputDirectory)
{
    const auto fixture = makeVelocityLayerFixture();

    struct Phase3CompileTimeLayerAdapter final
    {
        FixtureScopeAdapter baseAdapter;

        std::string getPaneTitle() const { return baseAdapter.getPaneTitle(); }
        std::string getListEmptyStateText() const { return baseAdapter.getListEmptyStateText(); }
        int getSelectedIndex() const { return baseAdapter.getSelectedIndex(); }
        int getItemCount() const { return baseAdapter.getItemCount(); }
        drs::app::authoring::RepeatedStructureRowViewModel getRowViewModel(int index) const
        {
            return baseAdapter.getRowViewModel(index);
        }
        drs::app::authoring::RepeatedStructureSelectionPathViewModel getSelectionPathViewModel(int index) const
        {
            return baseAdapter.getSelectionPathViewModel(index);
        }
        drs::app::authoring::RepeatedStructureDetailViewModel getDetailViewModel(int index) const
        {
            return baseAdapter.getDetailViewModel(index);
        }
        drs::app::authoring::RepeatedStructureSelectionPathViewModel getEmptySelectionPathViewModel() const
        {
            return baseAdapter.getEmptySelectionPathViewModel();
        }
        drs::app::authoring::RepeatedStructureDetailViewModel getEmptyDetailViewModel() const
        {
            return baseAdapter.getEmptyDetailViewModel();
        }
        const drs::app::authoring::RepeatedStructureAdapterCallbacks& getCallbacks() const
        {
            return baseAdapter.getCallbacks();
        }
    };

    static_assert(std::is_same_v<
                      decltype(drs::app::authoring::buildRepeatedStructurePaneViewModel(
                          std::declval<const Phase3CompileTimeLayerAdapter&>())),
                      drs::app::authoring::RepeatedStructurePaneViewModel>,
                  "Phase 3 adapters should compile directly into repeated-structure pane view models.");

    int selectionRequests = 0;
    int editRequests = 0;
    drs::app::authoring::RepeatedStructureAdapterCallbacks callbacks;
    callbacks.onSelectionRequested = [&selectionRequests](int) { ++selectionRequests; };
    callbacks.onEditIntentRequested = [&editRequests](const drs::app::authoring::RepeatedStructureEditIntent&)
    {
        ++editRequests;
    };

    Phase3CompileTimeLayerAdapter adapter{
        FixtureScopeAdapter(fixture, 2, callbacks)
    };

    const auto pane = drs::app::authoring::buildRepeatedStructurePaneViewModel(adapter);
    require(pane.title == fixture.fixtureTitle,
            "Phase 3 compile-time adapter should populate the pane title.");
    require(pane.list.rows.size() == fixture.rows.size(),
            "Phase 3 compile-time adapter should populate the repeated list.");
    require(pane.selectionPath.breadcrumbText.find(fixture.rows[2].title) != std::string::npos,
            "Phase 3 compile-time adapter should populate the breadcrumb path.");
    require(pane.detail.title == fixture.rows[2].title,
            "Phase 3 compile-time adapter should populate the detail view title.");
    require(adapter.getCallbacks().onSelectionRequested != nullptr,
            "Phase 3 compile-time adapter should carry a selection callback seam.");
    require(adapter.getCallbacks().onEditIntentRequested != nullptr,
            "Phase 3 compile-time adapter should carry an edit-intent callback seam.");

    adapter.getCallbacks().onSelectionRequested(2);
    adapter.getCallbacks().onEditIntentRequested({"vl-03", "inspect-layer", "Inspect layer"});
    require(selectionRequests == 1,
            "Phase 3 adapter callbacks should expose a workspace selection seam.");
    require(editRequests == 1,
            "Phase 3 adapter callbacks should expose an edit-intent seam.");

    DensityFixtureHarness harness;
    harness.setTopLeftPosition(0, 0);
    harness.setSize(drs::app::authoring::compactShellWidth - 40, 220);
    harness.setVisible(true);
    harness.setAdapter(adapter);
    harness.resized();

    const auto bounds = harness.getLocalBounds();
    requireComponentVisibleWithin(harness, "densityFixturePathLabel", bounds);
    requireComponentVisibleWithin(harness, "densityFixtureList", bounds);
    requireComponentVisibleWithin(harness, "densityFixtureDetailTitleLabel", bounds);
    require(requireLabel(harness, "densityFixtureDetailTitleLabel").getText().toStdString() == fixture.rows[2].title,
            "Compile-time adapter harness should bind the selected row detail title.");
    require(requireLabel(harness, "densityFixturePathLabel").getText().toStdString().find(fixture.rows[2].title)
                != std::string::npos,
            "Compile-time adapter harness should bind the selected row breadcrumb.");

    saveComponentPng(harness, outputDirectory / "phase3-adapter-contract.png");
}
} // namespace

int main()
{
    try
    {
        juce::ScopedJuceInitialiser_GUI gui;

        const auto outputDirectory = fs::temp_directory_path() / "drs-phase2-repeated-structure-density-tests";
        fs::create_directories(outputDirectory);

        exerciseFixtureHarness(outputDirectory);
        exercisePhase3AdapterContract(outputDirectory);
        exerciseMacroDensityInProduction(outputDirectory);

        std::cout << "Phase 2 repeated structure density tests passed. Output: "
                  << outputDirectory.string() << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Phase 2 repeated structure density tests failed: "
                  << exception.what() << std::endl;
        return 1;
    }
}
