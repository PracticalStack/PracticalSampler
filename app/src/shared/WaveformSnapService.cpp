#include "shared/WaveformSnapService.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <sstream>
#include <utility>
#include <vector>

namespace drs::app
{
WaveformSnapService::WaveformSnapService(WaveformSnapServiceOptions optionsIn)
    : options(std::move(optionsIn))
{
    auto initial = std::make_shared<const WaveformSnapServiceSnapshot>();
    std::atomic_store_explicit(&snapshot, std::move(initial), std::memory_order_release);
    worker = std::thread([this] { runWorker(); });
}

WaveformSnapService::~WaveformSnapService()
{
    shutdown();
}

WaveformSnapSubmitResult WaveformSnapService::submit(WaveformSnapRequest request)
{
    WaveformSnapSubmitResult result;
    if (request.sourceIdentity.empty() || request.sourcePath.empty())
        return result;

    std::unique_lock<std::mutex> lock(mutex);
    if (shutdownRequested)
        return result;
    request.searchRadiusFrames = std::min(request.searchRadiusFrames,
                                          options.maximumSearchRadiusFrames);
    result.accepted = true;
    result.identity = { ++nextGeneration, std::move(request.sourceIdentity),
                        std::move(request.sourcePath), request.candidateFrame,
                        request.searchRadiusFrames };

    const auto key = cacheKey(result.identity);
    if (const auto found = cache.find(key); found != cache.end())
    {
        if (active.has_value())
        {
            active->cancellationStage = WaveformSnapServiceStage::superseded;
            active->cancellationReason = "Zero-crossing search superseded by cached result";
            active->cancellation->store(true, std::memory_order_release);
        }
        pending.reset();
        WaveformSnapServiceSnapshot completed;
        completed.identity = result.identity;
        completed.stage = WaveformSnapServiceStage::completed;
        completed.decision = found->second;
        completed.cacheHit = true;
        completed.cacheEntryCount = cache.size();
        completed.status = completed.decision.applied
            ? "Snapped to cached zero crossing" : "No zero crossing in search window";
        auto published = std::make_shared<const WaveformSnapServiceSnapshot>(std::move(completed));
        std::atomic_store_explicit(&snapshot, std::move(published), std::memory_order_release);
        lock.unlock();
        terminalCondition.notify_all();
        return result;
    }

    if (pending.has_value())
        pending->cancellation->store(true, std::memory_order_release);
    pending = PendingRequest { result.identity, std::make_shared<std::atomic<bool>>(false),
                               WaveformSnapServiceStage::canceled, {} };
    if (active.has_value())
    {
        active->cancellationStage = WaveformSnapServiceStage::superseded;
        active->cancellationReason = "Zero-crossing search superseded";
        active->cancellation->store(true, std::memory_order_release);
    }
    lock.unlock();

    WaveformSnapServiceSnapshot queued;
    queued.identity = result.identity;
    queued.stage = WaveformSnapServiceStage::queued;
    queued.status = "Zero-crossing search queued";
    publish(std::move(queued));
    condition.notify_one();
    return result;
}

bool WaveformSnapService::cancel(std::string reason)
{
    std::lock_guard<std::mutex> lock(mutex);
    auto cancelRequest = [&](PendingRequest& request)
    {
        request.cancellationReason = std::move(reason);
        request.cancellationStage = WaveformSnapServiceStage::canceled;
        request.cancellation->store(true, std::memory_order_release);
        return true;
    };
    if (active.has_value())
        return cancelRequest(*active);
    if (pending.has_value())
        return cancelRequest(*pending);
    return false;
}

std::shared_ptr<const WaveformSnapServiceSnapshot> WaveformSnapService::getSnapshot() const
{
    return std::atomic_load_explicit(&snapshot, std::memory_order_acquire);
}

bool WaveformSnapService::waitForTerminal(const std::chrono::milliseconds timeout) const
{
    std::unique_lock<std::mutex> lock(mutex);
    return terminalCondition.wait_for(lock, timeout, [&]
    {
        const auto current = std::atomic_load_explicit(&snapshot, std::memory_order_acquire);
        return current != nullptr && terminal(current->stage)
            && !pending.has_value() && !active.has_value();
    });
}

void WaveformSnapService::shutdown() noexcept
{
    {
        std::lock_guard<std::mutex> lock(mutex);
        if (shutdownRequested)
            return;
        shutdownRequested = true;
        if (pending.has_value())
            pending->cancellation->store(true, std::memory_order_release);
        if (active.has_value())
            active->cancellation->store(true, std::memory_order_release);
    }
    condition.notify_all();
    terminalCondition.notify_all();
    if (worker.joinable())
        worker.join();
}

void WaveformSnapService::runWorker()
{
    for (;;)
    {
        std::optional<PendingRequest> request;
        {
            std::unique_lock<std::mutex> lock(mutex);
            condition.wait(lock, [&] { return shutdownRequested || pending.has_value(); });
            if (shutdownRequested && !pending.has_value())
                return;
            active = std::move(pending);
            pending.reset();
            request = active;
        }
        if (request.has_value())
            process(std::move(*request));
        {
            std::lock_guard<std::mutex> lock(mutex);
            active.reset();
        }
        terminalCondition.notify_all();
    }
}

void WaveformSnapService::process(PendingRequest request)
{
    WaveformSnapServiceSnapshot searching;
    searching.identity = request.identity;
    searching.stage = WaveformSnapServiceStage::searching;
    searching.status = "Searching for zero crossing";
    publish(searching);

    auto reader = createReader(request.identity.sourcePath);
    if (reader == nullptr || reader->lengthInSamples <= 0 || reader->numChannels == 0)
    {
        searching.stage = WaveformSnapServiceStage::failed;
        searching.status = "Audio source could not be opened for zero-crossing search";
        publish(std::move(searching));
        return;
    }

    const auto frameCount = static_cast<std::uint64_t>(reader->lengthInSamples);
    const auto center = std::min(request.identity.candidateFrame, frameCount);
    const auto radius = request.identity.searchRadiusFrames;
    const auto start = center > radius + 1 ? center - radius - 1 : 0;
    const auto maximumEnd = center > std::numeric_limits<std::uint64_t>::max() - radius - 2
        ? std::numeric_limits<std::uint64_t>::max() : center + radius + 2;
    const auto end = std::min(frameCount, maximumEnd);
    const auto sampleCount = static_cast<int>(std::min<std::uint64_t>(
        end - start, static_cast<std::uint64_t>(std::numeric_limits<int>::max())));
    const auto channelCount = static_cast<int>(std::min<unsigned int>(reader->numChannels, 8));
    juce::AudioBuffer<float> samples(channelCount, sampleCount);
    if (sampleCount <= 1
        || !reader->read(&samples, 0, sampleCount, static_cast<juce::int64>(start), true, true))
    {
        searching.stage = WaveformSnapServiceStage::failed;
        searching.status = "Audio frames could not be read for zero-crossing search";
        publish(std::move(searching));
        return;
    }
    if (request.cancellation->load(std::memory_order_acquire))
    {
        searching.stage = request.cancellationStage;
        searching.status = request.cancellationReason.empty()
            ? "Zero-crossing search canceled" : request.cancellationReason;
        publish(std::move(searching));
        return;
    }

    std::vector<std::uint64_t> candidates;
    candidates.reserve(static_cast<std::size_t>(sampleCount / 8 + 1));
    auto monoAt = [&](const int frame)
    {
        double sum = 0.0;
        for (int channel = 0; channel < channelCount; ++channel)
            sum += samples.getSample(channel, frame);
        return sum / static_cast<double>(channelCount);
    };
    auto previous = monoAt(0);
    for (int frame = 1; frame < sampleCount; ++frame)
    {
        const auto current = monoAt(frame);
        if ((previous <= 0.0 && current >= 0.0)
            || (previous >= 0.0 && current <= 0.0))
            candidates.push_back(start + static_cast<std::uint64_t>(frame));
        previous = current;
    }

    searching.decision = drs::engine::chooseWaveformSnapCandidate(
        center, candidates, { 0, frameCount }, radius);
    searching.stage = WaveformSnapServiceStage::completed;
    searching.status = searching.decision.applied
        ? "Snapped to zero crossing" : "No zero crossing in search window";
    {
        std::lock_guard<std::mutex> lock(mutex);
        if (request.identity.generation == nextGeneration && options.maximumCacheEntries > 0)
        {
            if (cache.size() >= options.maximumCacheEntries)
                cache.erase(cache.begin());
            cache[cacheKey(request.identity)] = searching.decision;
        }
        searching.cacheEntryCount = cache.size();
    }
    publish(std::move(searching));
}

void WaveformSnapService::publish(WaveformSnapServiceSnapshot next)
{
    {
        std::lock_guard<std::mutex> lock(mutex);
        if (next.identity.generation != 0 && next.identity.generation < nextGeneration)
            return;
        next.cacheEntryCount = cache.size();
    }
    auto published = std::make_shared<const WaveformSnapServiceSnapshot>(std::move(next));
    std::atomic_store_explicit(&snapshot, std::move(published), std::memory_order_release);
    if (terminal(std::atomic_load_explicit(&snapshot, std::memory_order_acquire)->stage))
        terminalCondition.notify_all();
}

std::unique_ptr<juce::AudioFormatReader> WaveformSnapService::createReader(
    const std::string& path) const
{
    if (options.readerFactory)
        return options.readerFactory(path);
    juce::AudioFormatManager manager;
    manager.registerBasicFormats();
    return std::unique_ptr<juce::AudioFormatReader>(
        manager.createReaderFor(juce::File(juce::String::fromUTF8(path.c_str()))));
}

std::string WaveformSnapService::cacheKey(const WaveformSnapRequestIdentity& identity)
{
    std::ostringstream key;
    key << identity.sourceIdentity << '\n' << identity.sourcePath << '\n'
        << identity.candidateFrame << ':' << identity.searchRadiusFrames;
    return key.str();
}

bool WaveformSnapService::terminal(const WaveformSnapServiceStage stage) noexcept
{
    return stage == WaveformSnapServiceStage::completed
        || stage == WaveformSnapServiceStage::canceled
        || stage == WaveformSnapServiceStage::superseded
        || stage == WaveformSnapServiceStage::failed;
}
} // namespace drs::app
