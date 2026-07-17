#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

namespace drs::app::authoring
{
class CompactInspectorCommitSlider final : public juce::Slider,
                                           private juce::Label::Listener
{
public:
    enum class CommitSource
    {
        pointerDrag,
        textEntry
    };

    CompactInspectorCommitSlider();
    ~CompactInspectorCommitSlider() override;

    void lookAndFeelChanged() override;
    void resized() override;

    void setOnCommitFinished(std::function<void(CommitSource)> nextCallback);

private:
    void labelTextChanged(juce::Label* labelThatHasChanged) override;
    void editorShown(juce::Label* label, juce::TextEditor& textEditor) override;
    void editorHidden(juce::Label* label, juce::TextEditor& textEditor) override;

    void attachTextBoxListener();
    void detachTextBoxListener();

    bool textEditActive = false;
    bool pointerGestureActive = false;
    double textEditStartValue = 0.0;
    juce::Label* textBoxLabel = nullptr;
    std::function<void(CommitSource)> onCommitFinished;
};

class CompactInspectorSection final : public juce::Component
{
public:
    CompactInspectorSection(const juce::String& titleText,
                            const juce::String& sectionComponentId,
                            bool startsExpanded);

    void resized() override;

    void setContent(juce::Component* nextContent);
    void setExpanded(bool shouldExpand);
    bool isExpanded() const { return expanded; }
    int getPreferredHeight() const;
    void setOnExpandedChanged(std::function<void(bool)> nextCallback);

private:
    bool expanded = true;
    int expandedContentHeight = 0;
    std::function<void(bool)> onExpandedChanged;
    juce::Label titleLabel;
    juce::TextButton disclosureButton;
    juce::Component* content = nullptr;
};

class CompactInspectorSliderRow final : public juce::Component
{
public:
    CompactInspectorSliderRow(const juce::String& labelText,
                              const juce::String& rowComponentId,
                              double minValue,
                              double maxValue,
                              double interval);

    void resized() override;

    CompactInspectorCommitSlider& getSlider() { return slider; }
    const CompactInspectorCommitSlider& getSlider() const { return slider; }
    juce::Label& getLabel() { return label; }
    const juce::Label& getLabel() const { return label; }

private:
    juce::Label label;
    CompactInspectorCommitSlider slider;
};

class CompactInspectorRangeRow final : public juce::Component
{
public:
    CompactInspectorRangeRow(const juce::String& titleText,
                             const juce::String& rowComponentId,
                             const juce::String& lowLabelText,
                             const juce::String& highLabelText,
                             double minValue,
                             double maxValue,
                             double interval);

    void resized() override;

    CompactInspectorCommitSlider& getLowSlider() { return lowSlider; }
    const CompactInspectorCommitSlider& getLowSlider() const { return lowSlider; }
    CompactInspectorCommitSlider& getHighSlider() { return highSlider; }
    const CompactInspectorCommitSlider& getHighSlider() const { return highSlider; }
    juce::Label& getTitleLabel() { return titleLabel; }
    const juce::Label& getTitleLabel() const { return titleLabel; }

private:
    juce::Label titleLabel;
    juce::Label lowLabel;
    juce::Label highLabel;
    CompactInspectorCommitSlider lowSlider;
    CompactInspectorCommitSlider highSlider;
};

class CompactInspectorToggleRow final : public juce::Component
{
public:
    CompactInspectorToggleRow(const juce::String& titleText,
                              const juce::String& rowComponentId,
                              const juce::String& toggleText);

    void resized() override;

    juce::ToggleButton& getToggle() { return toggle; }
    const juce::ToggleButton& getToggle() const { return toggle; }
    juce::Label& getLabel() { return label; }
    const juce::Label& getLabel() const { return label; }

private:
    juce::Label label;
    juce::ToggleButton toggle;
};

class CompactInspectorActionRow final : public juce::Component
{
public:
    CompactInspectorActionRow(const juce::String& titleText,
                              const juce::String& rowComponentId,
                              const juce::String& buttonText);

    void resized() override;

    juce::TextButton& getButton() { return button; }
    const juce::TextButton& getButton() const { return button; }
    juce::Label& getLabel() { return label; }
    const juce::Label& getLabel() const { return label; }

private:
    juce::Label label;
    juce::TextButton button;
};

class CompactInspectorMessage final : public juce::Component
{
public:
    CompactInspectorMessage(const juce::String& messageComponentId,
                            juce::Justification justification);

    void resized() override;
    void setText(const juce::String& text);

    juce::Label& getLabel() { return label; }
    const juce::Label& getLabel() const { return label; }

private:
    juce::Label label;
};
} // namespace drs::app::authoring
