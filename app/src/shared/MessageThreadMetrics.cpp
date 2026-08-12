#include "shared/MessageThreadMetrics.h"

#include <algorithm>
#include <array>
#include <atomic>

namespace drs::app
{
namespace
{
constexpr auto spanCount = static_cast<std::size_t>(MessageThreadSpanKind::count);

struct AtomicSpanStatistics
{
    std::atomic<std::uint64_t> observationCount { 0 };
    std::atomic<std::uint64_t> slowCount { 0 };
    std::atomic<std::uint64_t> lastSlowNanoseconds { 0 };
    std::atomic<std::uint64_t> maximumNanoseconds { 0 };
};

std::array<AtomicSpanStatistics, spanCount> statistics;

constexpr std::uint64_t slowThresholdNanoseconds = 1'000'000;

std::size_t toIndex(const MessageThreadSpanKind kind) noexcept
{
    return static_cast<std::size_t>(kind);
}
} // namespace

void MessageThreadMetrics::record(const MessageThreadSpanKind kind,
                                  const std::chrono::nanoseconds elapsed) noexcept
{
    const auto index = toIndex(kind);
    if (index >= statistics.size())
        return;

    auto& span = statistics[index];
    span.observationCount.fetch_add(1, std::memory_order_relaxed);

    const auto elapsedCount = elapsed.count() > 0
        ? static_cast<std::uint64_t>(elapsed.count()) : std::uint64_t { 0 };
    if (elapsedCount < slowThresholdNanoseconds)
        return;

    span.slowCount.fetch_add(1, std::memory_order_relaxed);
    span.lastSlowNanoseconds.store(elapsedCount, std::memory_order_relaxed);

    auto maximum = span.maximumNanoseconds.load(std::memory_order_relaxed);
    while (maximum < elapsedCount
           && !span.maximumNanoseconds.compare_exchange_weak(
               maximum, elapsedCount, std::memory_order_relaxed))
    {
    }
}

std::vector<MessageThreadSpanStatistics> MessageThreadMetrics::getStatistics()
{
    std::vector<MessageThreadSpanStatistics> snapshot;
    snapshot.reserve(statistics.size());

    for (std::size_t index = 0; index < statistics.size(); ++index)
    {
        const auto slowCount = statistics[index].slowCount.load(std::memory_order_relaxed);
        const auto toMilliseconds = [](const std::uint64_t nanoseconds)
        {
            return static_cast<double>(nanoseconds) / 1'000'000.0;
        };

        snapshot.push_back({
            static_cast<MessageThreadSpanKind>(index),
            name(static_cast<MessageThreadSpanKind>(index)),
            statistics[index].observationCount.load(std::memory_order_relaxed),
            slowCount,
            slowSpanThresholdMilliseconds,
            toMilliseconds(statistics[index].lastSlowNanoseconds.load(std::memory_order_relaxed)),
            toMilliseconds(statistics[index].maximumNanoseconds.load(std::memory_order_relaxed))
        });
    }

    return snapshot;
}

std::vector<MessageThreadSpanStatistics> MessageThreadMetrics::getSlowSpanStatistics()
{
    auto snapshot = getStatistics();
    snapshot.erase(std::remove_if(snapshot.begin(), snapshot.end(), [](const auto& span)
    {
        return span.slowCount == 0;
    }), snapshot.end());
    return snapshot;
}

void MessageThreadMetrics::resetForTests() noexcept
{
    for (auto& span : statistics)
    {
        span.observationCount.store(0, std::memory_order_relaxed);
        span.slowCount.store(0, std::memory_order_relaxed);
        span.lastSlowNanoseconds.store(0, std::memory_order_relaxed);
        span.maximumNanoseconds.store(0, std::memory_order_relaxed);
    }
}

const char* MessageThreadMetrics::name(const MessageThreadSpanKind kind) noexcept
{
    switch (kind)
    {
        case MessageThreadSpanKind::performanceKeyboardCallback: return "keyboard callbacks";
        case MessageThreadSpanKind::editorTimerWork: return "editor timer";
        case MessageThreadSpanKind::editorServiceWork: return "4 Hz engine service";
        case MessageThreadSpanKind::editorRestoreWork: return "4 Hz restore presentation";
        case MessageThreadSpanKind::editorPerformanceWork: return "4 Hz Performance presentation";
        case MessageThreadSpanKind::editorAuthoringWork: return "4 Hz Authoring presentation";
        case MessageThreadSpanKind::editorPackageOpenWork: return "4 Hz package-open poll";
        case MessageThreadSpanKind::editorStatusWork: return "4 Hz shell status";
        case MessageThreadSpanKind::editorExportWork: return "4 Hz export poll";
        case MessageThreadSpanKind::editorWavImportWork: return "4 Hz WAV-import poll";
        case MessageThreadSpanKind::editorSfzImportWork: return "4 Hz SFZ-import poll";
        case MessageThreadSpanKind::performanceRefresh: return "Performance refresh";
        case MessageThreadSpanKind::zoneSelection: return "zone selection";
        case MessageThreadSpanKind::authoringRefresh: return "Authoring refresh";
        case MessageThreadSpanKind::hostStateSerialization: return "host-state serialization";
        case MessageThreadSpanKind::previewDispatch: return "preview dispatch";
        case MessageThreadSpanKind::publishDispatch: return "publish dispatch";
        case MessageThreadSpanKind::count: return "unknown";
        default: return "unknown";
    }
}
} // namespace drs::app
