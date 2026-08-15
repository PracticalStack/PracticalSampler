#include "Sprint4OfflineRenderHarness.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace drs::tests
{
namespace
{
std::string escapeJson(const std::string& value)
{
    std::string escaped;
    escaped.reserve(value.size());
    for (const auto character : value)
    {
        switch (character)
        {
            case '\\': escaped += "\\\\"; break;
            case '"': escaped += "\\\""; break;
            case '\n': escaped += "\\n"; break;
            case '\r': escaped += "\\r"; break;
            case '\t': escaped += "\\t"; break;
            default: escaped += character; break;
        }
    }
    return escaped;
}

void hashByte(std::uint64_t& hash, std::uint8_t value) noexcept
{
    hash ^= value;
    hash *= 1099511628211ull;
}

std::string checksum(const std::array<std::vector<float>, 2>& channels)
{
    auto hash = 1469598103934665603ull;
    for (std::size_t channel = 0; channel < channels.size(); ++channel)
    {
        hashByte(hash, static_cast<std::uint8_t>(channel));
        for (const auto sample : channels[channel])
        {
            const auto quantized = static_cast<std::int64_t>(
                std::llround(static_cast<double>(sample) / offlineChecksumQuantum));
            const auto bits = static_cast<std::uint64_t>(quantized);
            for (std::size_t byte = 0; byte < sizeof(bits); ++byte)
                hashByte(hash, static_cast<std::uint8_t>((bits >> (byte * 8u)) & 0xffu));
        }
    }

    std::ostringstream text;
    text << std::hex << std::setfill('0') << std::setw(16) << hash;
    return text.str();
}

OfflineRenderSummary summarize(const std::array<std::vector<float>, 2>& channels,
                               const engine::SamplerPlaybackContextSnapshot& context)
{
    OfflineRenderSummary summary;
    summary.frameCount = channels[0].size();
    summary.quantizedChecksum = checksum(channels);
    long double squareSum = 0.0;
    std::uint64_t sampleCount = 0;
    for (std::size_t frame = 0; frame < channels[0].size(); ++frame)
    {
        auto frameNonZero = false;
        for (const auto& channel : channels)
        {
            const auto value = static_cast<double>(channel[frame]);
            summary.peak = std::max(summary.peak, std::abs(value));
            squareSum += static_cast<long double>(value) * static_cast<long double>(value);
            ++sampleCount;
            frameNonZero = frameNonZero || std::abs(value) > offlineSampleTolerance;
        }
        if (frameNonZero)
        {
            if (summary.firstNonZeroFrame < 0)
                summary.firstNonZeroFrame = static_cast<std::int64_t>(frame);
            summary.lastNonZeroFrame = static_cast<std::int64_t>(frame);
        }
    }
    summary.rms = sampleCount > 0
        ? std::sqrt(static_cast<double>(squareSum / static_cast<long double>(sampleCount)))
        : 0.0;
    summary.counters = context.counters;
    summary.activeVoiceCount = context.activeVoiceCount;
    summary.releasingVoiceCount = context.releasingVoiceCount;
    summary.finishedVoiceCount = context.finishedVoiceCount;
    return summary;
}

std::string safeFileName(std::string value)
{
    for (auto& character : value)
    {
        const auto valid = (character >= 'a' && character <= 'z')
            || (character >= 'A' && character <= 'Z')
            || (character >= '0' && character <= '9')
            || character == '-' || character == '_';
        if (!valid)
            character = '_';
    }
    return value;
}
} // namespace

OfflineRenderArtifact renderOffline(const OfflineRenderRequest& request)
{
    if (request.scenarioId.empty() || request.model == nullptr
        || !std::isfinite(request.sampleRate) || request.sampleRate <= 0.0
        || request.frameCount == 0 || request.partitionSize == 0)
    {
        throw std::invalid_argument("Offline render request is incomplete.");
    }

    // Deterministic package renders cannot depend on worker scheduling. Materialize
    // package pages before entering the callback loop, using the same authenticated
    // off-audio preparation API as the runtime page service.
    for (const auto& sample : request.model->getSamples())
    {
        const auto constPackageSource
            = std::dynamic_pointer_cast<const engine::PackagePagedSampleDataSource>(sample.dataSource);
        if (constPackageSource == nullptr)
            continue;
        const auto packageSource
            = std::const_pointer_cast<engine::PackagePagedSampleDataSource>(constPackageSource);
        if (!packageSource->prepareHead())
            throw std::runtime_error("Offline package source head could not be prepared.");
        for (std::uint64_t page = 0; page < packageSource->pageCount(); ++page)
        {
            if (!packageSource->preparePage(page))
                throw std::runtime_error("Offline package source page could not be prepared.");
        }
    }

    auto events = request.events;
    std::stable_sort(events.begin(), events.end(), [](const auto& left, const auto& right)
    {
        return left.frame < right.frame;
    });
    if (!events.empty() && events.back().frame >= request.frameCount)
        throw std::invalid_argument("Offline timeline event lies outside the requested render length.");

    engine::SamplerPlaybackContext context(request.model->getLane());
    std::shared_ptr<engine::DspRenderGeneration> dspGeneration;
    if (request.dspGraphPlan.has_value())
    {
        std::string generationFailure;
        dspGeneration = engine::createDspRenderGeneration(
            request.model, *request.dspGraphPlan, request.partitionSize, &generationFailure);
        if (dspGeneration == nullptr)
        {
            throw std::runtime_error("Offline render DSP generation could not be created: "
                                     + generationFailure);
        }
    }
    if (!context.prepare(request.sampleRate)
        || !(dspGeneration != nullptr
                 ? context.stageActivation(request.model, dspGeneration)
                 : context.stageActivation(request.model))
        || !context.activatePendingForPreparation())
    {
        throw std::runtime_error("Offline render context could not activate its immutable model.");
    }

    OfflineRenderArtifact artifact;
    artifact.scenarioId = request.scenarioId;
    artifact.partitionSize = request.partitionSize;
    artifact.channels[0].assign(static_cast<std::size_t>(request.frameCount), 0.0f);
    artifact.channels[1].assign(static_cast<std::size_t>(request.frameCount), 0.0f);

    std::size_t eventIndex = 0;
    for (std::uint64_t blockStart = 0; blockStart < request.frameCount;)
    {
        const auto blockFrames = static_cast<std::uint32_t>(std::min<std::uint64_t>(
            request.partitionSize, request.frameCount - blockStart));
        std::vector<float> left(blockFrames, 0.0f);
        std::vector<float> right(blockFrames, 0.0f);
        float* outputChannels[] { left.data(), right.data() };
        engine::SamplerEventBlock blockEvents;
        const auto blockEnd = blockStart + blockFrames;
        while (eventIndex < events.size() && events[eventIndex].frame < blockEnd)
        {
            const auto& event = events[eventIndex++];
            engine::SamplerRenderEvent renderEvent;
            renderEvent.type = event.type;
            renderEvent.sampleOffset = static_cast<std::uint32_t>(event.frame - blockStart);
            renderEvent.midiNote = event.midiNote;
            renderEvent.velocity = event.velocity;
            renderEvent.midiChannel = event.midiChannel;
            renderEvent.noteOffVelocity = event.noteOffVelocity;
            renderEvent.inputSequence = event.inputSequence;
            renderEvent.controllerNumber = event.controllerNumber;
            renderEvent.controllerValue = event.controllerValue;
            if (!blockEvents.push(renderEvent))
            {
                throw std::runtime_error("Offline event block exceeded the core event capacity.");
            }
        }

        const auto result = context.renderBlock(
            { outputChannels, 2, blockFrames }, blockEvents.view());
        if (!result.accepted)
            throw std::runtime_error("Offline core render rejected a valid block.");

        std::copy(left.begin(), left.end(), artifact.channels[0].begin() + static_cast<std::ptrdiff_t>(blockStart));
        std::copy(right.begin(), right.end(), artifact.channels[1].begin() + static_cast<std::ptrdiff_t>(blockStart));
        blockStart = blockEnd;
    }

    artifact.summary = summarize(artifact.channels, context.getSnapshot());
    return artifact;
}

OfflineArtifactComparison compareOfflineArtifacts(const OfflineRenderArtifact& expected,
                                                   const OfflineRenderArtifact& actual,
                                                   double sampleTolerance)
{
    OfflineArtifactComparison comparison;
    if (expected.channels[0].size() != actual.channels[0].size()
        || expected.channels[1].size() != actual.channels[1].size())
    {
        comparison.message = "Rendered channel lengths differ.";
        return comparison;
    }

    for (std::size_t channel = 0; channel < expected.channels.size(); ++channel)
    {
        for (std::size_t frame = 0; frame < expected.channels[channel].size(); ++frame)
        {
            const auto expectedValue = static_cast<double>(expected.channels[channel][frame]);
            const auto actualValue = static_cast<double>(actual.channels[channel][frame]);
            if (!std::isfinite(expectedValue) || !std::isfinite(actualValue)
                || std::abs(expectedValue - actualValue) > sampleTolerance)
            {
                comparison.message = "Rendered samples differ beyond tolerance.";
                comparison.channel = channel;
                comparison.frame = frame;
                comparison.expected = expectedValue;
                comparison.actual = actualValue;
                return comparison;
            }
        }
    }

    const auto& expectedCounters = expected.summary.counters;
    const auto& actualCounters = actual.summary.counters;
    if (expectedCounters.startedVoiceCount != actualCounters.startedVoiceCount
        || expectedCounters.releasedVoiceCount != actualCounters.releasedVoiceCount
        || expectedCounters.completedVoiceCount != actualCounters.completedVoiceCount
        || expectedCounters.stolenVoiceCount != actualCounters.stolenVoiceCount
        || expectedCounters.droppedEventCount != actualCounters.droppedEventCount
        || expectedCounters.resetVoiceCount != actualCounters.resetVoiceCount
        || expectedCounters.appliedActivationCount != actualCounters.appliedActivationCount
        || expectedCounters.enqueuedRetirementCount != actualCounters.enqueuedRetirementCount
        || expectedCounters.reclaimedActivationCount != actualCounters.reclaimedActivationCount
        || expectedCounters.dynamicReleaseUpdateCount != actualCounters.dynamicReleaseUpdateCount
        || expectedCounters.repedalCatchCount != actualCounters.repedalCatchCount
        || expected.summary.activeVoiceCount != actual.summary.activeVoiceCount
        || expected.summary.releasingVoiceCount != actual.summary.releasingVoiceCount
        || expected.summary.finishedVoiceCount != actual.summary.finishedVoiceCount)
    {
        comparison.message = "Lifecycle counters differ.";
        return comparison;
    }

    comparison.equivalent = true;
    comparison.message = "Artifacts are equivalent.";
    return comparison;
}

std::string serializeOfflineArtifactJson(const OfflineRenderArtifact& artifact)
{
    const auto& summary = artifact.summary;
    std::ostringstream output;
    output << std::setprecision(10)
           << "{\n  \"schema\": \"drs.sprint4.offline-render-artifact\",\n"
           << "  \"version\": 1,\n"
           << "  \"scenario\": \"" << escapeJson(artifact.scenarioId) << "\",\n"
           << "  \"partitionSize\": " << artifact.partitionSize << ",\n"
           << "  \"sampleTolerance\": " << offlineSampleTolerance << ",\n"
           << "  \"checksumQuantum\": " << offlineChecksumQuantum << ",\n"
           << "  \"summary\": {\n"
           << "    \"frames\": " << summary.frameCount << ",\n"
           << "    \"checksum\": \"" << summary.quantizedChecksum << "\",\n"
           << "    \"peak\": " << summary.peak << ",\n"
           << "    \"rms\": " << summary.rms << ",\n"
           << "    \"firstNonZeroFrame\": " << summary.firstNonZeroFrame << ",\n"
           << "    \"lastNonZeroFrame\": " << summary.lastNonZeroFrame << ",\n"
           << "    \"startedVoices\": " << summary.counters.startedVoiceCount << ",\n"
           << "    \"releasedVoices\": " << summary.counters.releasedVoiceCount << ",\n"
           << "    \"completedVoices\": " << summary.counters.completedVoiceCount << ",\n"
           << "    \"stolenVoices\": " << summary.counters.stolenVoiceCount << ",\n"
           << "    \"droppedEvents\": " << summary.counters.droppedEventCount << ",\n"
           << "    \"resetVoices\": " << summary.counters.resetVoiceCount << "\n"
           << "  },\n  \"channels\": [\n";
    for (std::size_t channel = 0; channel < artifact.channels.size(); ++channel)
    {
        output << "    [";
        for (std::size_t frame = 0; frame < artifact.channels[channel].size(); ++frame)
        {
            if (frame != 0)
                output << ',';
            output << artifact.channels[channel][frame];
        }
        output << ']' << (channel + 1 == artifact.channels.size() ? "\n" : ",\n");
    }
    output << "  ]\n}\n";
    return output.str();
}

void writeOfflineMismatchArtifacts(const std::filesystem::path& outputDirectory,
                                   const OfflineRenderArtifact& expected,
                                   const OfflineRenderArtifact& actual,
                                   const OfflineArtifactComparison& comparison)
{
    std::filesystem::create_directories(outputDirectory);
    const auto stem = safeFileName(actual.scenarioId) + "-p" + std::to_string(actual.partitionSize);
    {
        std::ofstream expectedFile(outputDirectory / (stem + "-expected.json"), std::ios::binary);
        expectedFile << serializeOfflineArtifactJson(expected);
    }
    {
        std::ofstream actualFile(outputDirectory / (stem + "-actual.json"), std::ios::binary);
        actualFile << serializeOfflineArtifactJson(actual);
    }
    {
        std::ofstream reportFile(outputDirectory / (stem + "-mismatch.txt"), std::ios::binary);
        reportFile << comparison.message << '\n'
                   << "channel=" << comparison.channel << '\n'
                   << "frame=" << comparison.frame << '\n'
                   << std::setprecision(10)
                   << "expected=" << comparison.expected << '\n'
                   << "actual=" << comparison.actual << '\n'
                   << "tolerance=" << offlineSampleTolerance << '\n';
    }
}
} // namespace drs::tests
