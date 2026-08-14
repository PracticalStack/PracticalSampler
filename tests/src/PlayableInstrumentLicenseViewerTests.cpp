#include "shared/PlayableInstrumentLicenseViewer.h"

#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

namespace
{
void require(const bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

juce::Component* findDescendantById(juce::Component& root, const juce::String& componentId)
{
    for (auto index = 0; index < root.getNumChildComponents(); ++index)
    {
        auto* child = root.getChildComponent(index);
        if (child->getComponentID() == componentId)
            return child;
        if (auto* match = findDescendantById(*child, componentId))
            return match;
    }
    return nullptr;
}
} // namespace

int main()
{
    try
    {
        juce::ScopedJuceInitialiser_GUI gui;
        const auto sourceText = std::make_shared<const std::string>(
            "Example license\n\nCopyright 2026\nPermission is granted.\n");
        drs::app::PlayableInstrumentLicenseViewer viewer(sourceText);

        require(viewer.getComponentID() == "playableInstrumentLicenseViewer"
                    && viewer.getTitle().isNotEmpty()
                    && viewer.getDescription().isNotEmpty(),
                "The license viewer must expose stable identity and accessibility metadata.");

        auto* editor = dynamic_cast<juce::TextEditor*>(
            findDescendantById(viewer, "playableInstrumentLicenseText"));
        require(editor != nullptr
                    && editor->isReadOnly()
                    && editor->isMultiLine()
                    && editor->getText().toStdString() == *sourceText,
                "The viewer must present exact selectable text in a read-only multiline editor.");
        editor->selectAll();
        require(editor->getHighlightedText() == editor->getText(),
                "Read-only license text must remain selectable.");

        auto* closeButton = dynamic_cast<juce::TextButton*>(
            findDescendantById(viewer, "playableInstrumentLicenseCloseButton"));
        require(closeButton != nullptr
                    && closeButton->getButtonText() == "Close"
                    && closeButton->getDescription().isNotEmpty(),
                "The viewer must provide an accessible Close action.");

        viewer.setSize(240, 220);
        viewer.resized();
        require(editor->getWidth() > 0 && editor->getHeight() > 0
                    && closeButton->getWidth() > 0 && closeButton->getHeight() > 0
                    && viewer.getLocalBounds().contains(editor->getBounds())
                    && viewer.getLocalBounds().contains(closeButton->getBounds()),
                "The license viewer must retain usable controls in a small host window.");

        std::cout << "Playable instrument license LI-04 viewer tests passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Playable instrument license LI-04 viewer tests failed: "
                  << exception.what() << std::endl;
        return 1;
    }
}
