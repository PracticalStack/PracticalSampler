#pragma once

#include <chrono>
#include <cstdint>
#include <vector>

namespace drs::app
{
enum class MessageThreadSpanKind : std::uint8_t
{
    performanceKeyboardCallback = 0,
    editorTimerWork,
    performanceRefresh,
    zoneSelection,
    authoringRefresh,
    hostStateSerialization,
    previewDispatch,
    publishDispatch,
    count
};

struct MessageThreadSpanStatistics
{
    MessageThreadSpanKind kind = MessageThreadSpanKind::performanceKeyboardCallback;
    const char* name = "";
    std::uint64_t observationCount = 0;
    std::uint64_t slowCount = 0;
    double thresholdMilliseconds = 1.0;
    double lastSlowMilliseconds = 0.0;
    double maximumMilliseconds = 0.0;
};

class MessageThreadMetrics final
{
public:
    static constexpr double slowSpanThresholdMilliseconds = 1.0;

    static void record(MessageThreadSpanKind kind,
                       std::chrono::nanoseconds elapsed) noexcept;
    static std::vector<MessageThreadSpanStatistics> getStatistics();
    static std::vector<MessageThreadSpanStatistics> getSlowSpanStatistics();
    static void resetForTests() noexcept;
    static const char* name(MessageThreadSpanKind kind) noexcept;
};

class ScopedMessageThreadSpan final
{
public:
    explicit ScopedMessageThreadSpan(MessageThreadSpanKind kindToMeasure) noexcept
        : kind(kindToMeasure), startedAt(std::chrono::steady_clock::now()) {}

    ~ScopedMessageThreadSpan() noexcept
    {
        MessageThreadMetrics::record(
            kind, std::chrono::steady_clock::now() - startedAt);
    }

    ScopedMessageThreadSpan(const ScopedMessageThreadSpan&) = delete;
    ScopedMessageThreadSpan& operator=(const ScopedMessageThreadSpan&) = delete;

private:
    MessageThreadSpanKind kind;
    std::chrono::steady_clock::time_point startedAt;
};
} // namespace drs::app
