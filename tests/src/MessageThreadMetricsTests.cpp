#include "shared/MessageThreadMetrics.h"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <stdexcept>

namespace
{
void require(const bool condition, const char* message)
{
    if (!condition)
        throw std::runtime_error(message);
}
} // namespace

int main()
{
    try
    {
        using drs::app::MessageThreadMetrics;
        using drs::app::MessageThreadSpanKind;

        MessageThreadMetrics::resetForTests();
        MessageThreadMetrics::record(MessageThreadSpanKind::performanceRefresh,
                                     std::chrono::microseconds(999));
        require(MessageThreadMetrics::getSlowSpanStatistics().empty(),
                "Sub-threshold spans must not be reported.");

        for (std::size_t index = 0;
             index < static_cast<std::size_t>(MessageThreadSpanKind::count);
             ++index)
        {
            MessageThreadMetrics::record(static_cast<MessageThreadSpanKind>(index),
                                         std::chrono::microseconds(1250 + index));
        }

        const auto snapshot = MessageThreadMetrics::getSlowSpanStatistics();
        require(snapshot.size() == static_cast<std::size_t>(MessageThreadSpanKind::count),
                "Every required message-thread span must be reportable.");
        require(std::all_of(snapshot.begin(), snapshot.end(), [](const auto& span)
        {
            return span.name != nullptr && span.name[0] != '\0'
                && span.slowCount == 1
                && span.maximumMilliseconds >= MessageThreadMetrics::slowSpanThresholdMilliseconds;
        }), "Slow-span statistics must include names, counts, and measured maxima.");

        const auto performanceRefresh = std::find_if(snapshot.begin(), snapshot.end(), [](const auto& span)
        {
            return span.kind == MessageThreadSpanKind::performanceRefresh;
        });
        require(performanceRefresh != snapshot.end()
                    && performanceRefresh->observationCount == 2
                    && performanceRefresh->slowCount == 1,
                "All observations must be counted while only slow observations are reported.");

        std::cout << "Message-thread metrics tests passed; coveredSpans=" << snapshot.size()
                  << " thresholdMs=" << MessageThreadMetrics::slowSpanThresholdMilliseconds
                  << std::endl;
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "Message-thread metrics tests failed: " << error.what() << std::endl;
        return 1;
    }
}
