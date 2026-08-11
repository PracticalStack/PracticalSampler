#include "drs/engine/EngineFacade.h"
#include "shared/MessageThreadMetrics.h"
#include "shared/PerformancePanel.h"

#include <juce_gui_extra/juce_gui_extra.h>

#include <array>
#include <iostream>
#include <memory>
#include <stdexcept>

namespace
{
void require(const bool condition, const char* message)
{
    if (!condition)
        throw std::runtime_error(message);
}

const drs::app::MessageThreadSpanStatistics& statisticsFor(
    const std::vector<drs::app::MessageThreadSpanStatistics>& statistics,
    const drs::app::MessageThreadSpanKind kind)
{
    const auto index = static_cast<std::size_t>(kind);
    require(index < statistics.size(), "Message-thread statistics kind is out of range.");
    return statistics[index];
}
} // namespace

int main()
{
    try
    {
        juce::ScopedJuceInitialiser_GUI gui;
        drs::engine::EngineFacade facade;
        facade.resetSessionStateToDefault();

        auto presentation = std::make_shared<drs::engine::PerformancePublishPresentationSnapshot>();
        std::array<int, 128> queuedNotes {};
        std::size_t queuedCount = 0;
        drs::app::PerformancePanel panel(
            facade, {},
            [&](const int note, const float)
            {
                if (queuedCount < queuedNotes.size()) queuedNotes[queuedCount++] = note;
            },
            [&](const int note)
            {
                if (queuedCount < queuedNotes.size()) queuedNotes[queuedCount++] = -note;
            }, {},
            [&]() -> std::shared_ptr<const drs::engine::PerformancePublishPresentationSnapshot>
            {
                return presentation;
            });
        panel.setSize(1280, 900);

        panel.refreshNow();
        drs::app::MessageThreadMetrics::resetForTests();
        panel.refreshNow();
        panel.refreshNow();
        auto metrics = drs::app::MessageThreadMetrics::getStatistics();
        require(statisticsFor(metrics, drs::app::MessageThreadSpanKind::performanceRefresh)
                    .observationCount == 0,
                "Unchanged shell ticks must not rebuild the Performance surface.");

        for (int note = 48; note < 72; ++note)
        {
            panel.getKeyboardState().noteOn(1, note, 0.8f);
            panel.getKeyboardState().noteOff(1, note, 0.8f);
        }
        metrics = drs::app::MessageThreadMetrics::getStatistics();
        const auto keyboard = statisticsFor(
            metrics, drs::app::MessageThreadSpanKind::performanceKeyboardCallback);
        require(queuedCount == 48 && keyboard.observationCount == 48,
                "Every Performance note callback must only enqueue its note event.");
        require(keyboard.slowCount == 0,
                "Performance note callbacks must remain below the 1 ms gate.");
        require(statisticsFor(metrics, drs::app::MessageThreadSpanKind::performanceRefresh)
                    .observationCount == 0,
                "Performance note callbacks must not refresh the surface.");

        presentation = std::make_shared<drs::engine::PerformancePublishPresentationSnapshot>(
            *presentation);
        presentation->publicationSequence = 1;
        panel.refreshNow();
        metrics = drs::app::MessageThreadMetrics::getStatistics();
        require(statisticsFor(metrics, drs::app::MessageThreadSpanKind::performanceRefresh)
                    .observationCount == 1,
                "A changed publication sequence must refresh the Performance surface once.");
        panel.refreshNow();
        metrics = drs::app::MessageThreadMetrics::getStatistics();
        require(statisticsFor(metrics, drs::app::MessageThreadSpanKind::performanceRefresh)
                    .observationCount == 1,
                "A consumed publication sequence must not trigger another refresh.");

        std::cout << "Performance responsiveness tests passed; callbacks="
                  << keyboard.observationCount << " slowCallbacks=" << keyboard.slowCount
                  << std::endl;
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "Performance responsiveness tests failed: " << error.what() << std::endl;
        return 1;
    }
}
