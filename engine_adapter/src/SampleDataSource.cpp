#include "drs/engine/SampleDataSource.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <limits>

namespace drs::engine
{
namespace
{
std::uint16_t readLe16(const unsigned char* bytes) noexcept
{
    return static_cast<std::uint16_t>(bytes[0])
        | (static_cast<std::uint16_t>(bytes[1]) << 8u);
}

std::uint32_t readLe32(const unsigned char* bytes) noexcept
{
    return static_cast<std::uint32_t>(bytes[0])
        | (static_cast<std::uint32_t>(bytes[1]) << 8u)
        | (static_cast<std::uint32_t>(bytes[2]) << 16u)
        | (static_cast<std::uint32_t>(bytes[3]) << 24u);
}

std::uint64_t readLe64(const unsigned char* bytes) noexcept
{
    return static_cast<std::uint64_t>(readLe32(bytes))
        | (static_cast<std::uint64_t>(readLe32(bytes + 4)) << 32u);
}

std::uint64_t stableGeneration(const std::string& value) noexcept
{
    std::uint64_t hash = 14695981039346656037ull;
    for (const auto character : value)
    {
        hash ^= static_cast<unsigned char>(character);
        hash *= 1099511628211ull;
    }
    return hash == 0 ? 1 : hash;
}
} // namespace

SamplePageLease::SamplePageLease(std::atomic<std::uint32_t>* counter,
                                 std::atomic<std::uint64_t>* leasedBytes,
                                 const std::uint64_t byteCount) noexcept
    : leaseCounter(counter), leasedByteCounter(leasedBytes), leasedByteCount(byteCount)
{
    if (leaseCounter != nullptr)
        leaseCounter->fetch_add(1, std::memory_order_acq_rel);
    if (leasedByteCounter != nullptr)
        leasedByteCounter->fetch_add(leasedByteCount, std::memory_order_relaxed);
}

SamplePageLease::SamplePageLease(const SamplePageLease& other) noexcept
    : SamplePageLease(other.leaseCounter, other.leasedByteCounter, other.leasedByteCount)
{
}

SamplePageLease& SamplePageLease::operator=(const SamplePageLease& other) noexcept
{
    if (this != &other)
    {
        release();
        leaseCounter = other.leaseCounter;
        leasedByteCounter = other.leasedByteCounter;
        leasedByteCount = other.leasedByteCount;
        if (leaseCounter != nullptr)
            leaseCounter->fetch_add(1, std::memory_order_acq_rel);
        if (leasedByteCounter != nullptr)
            leasedByteCounter->fetch_add(leasedByteCount, std::memory_order_relaxed);
    }
    return *this;
}

SamplePageLease::SamplePageLease(SamplePageLease&& other) noexcept
    : leaseCounter(other.leaseCounter),
      leasedByteCounter(other.leasedByteCounter),
      leasedByteCount(other.leasedByteCount)
{
    other.leaseCounter = nullptr;
    other.leasedByteCounter = nullptr;
    other.leasedByteCount = 0;
}

SamplePageLease& SamplePageLease::operator=(SamplePageLease&& other) noexcept
{
    if (this != &other)
    {
        release();
        leaseCounter = other.leaseCounter;
        leasedByteCounter = other.leasedByteCounter;
        leasedByteCount = other.leasedByteCount;
        other.leaseCounter = nullptr;
        other.leasedByteCounter = nullptr;
        other.leasedByteCount = 0;
    }
    return *this;
}

SamplePageLease::~SamplePageLease() noexcept
{
    release();
}

void SamplePageLease::release() noexcept
{
    if (leaseCounter != nullptr)
        leaseCounter->fetch_sub(1, std::memory_order_acq_rel);
    if (leasedByteCounter != nullptr)
        leasedByteCounter->fetch_sub(leasedByteCount, std::memory_order_relaxed);
    leaseCounter = nullptr;
    leasedByteCounter = nullptr;
    leasedByteCount = 0;
}

SampleDataSourceDescriptorValidation validateSampleDataSourceDescriptor(
    const SampleDataSourceDescriptor& descriptor)
{
    SampleDataSourceDescriptorValidation result;
    const auto add = [&](std::string finding) { result.findings.push_back(std::move(finding)); };
    if (descriptor.version != sampleDataSourceDescriptorVersion)
        add("Unsupported sample-data-source descriptor version.");
    if (descriptor.sourceId.empty() || descriptor.canonicalSourceIdentity.empty()
        || descriptor.provenanceIdentity.empty())
        add("Source, canonical, and provenance identities are required.");
    if (descriptor.generation == 0)
        add("Source generation must be non-zero.");
    if (!std::isfinite(descriptor.sampleRate) || descriptor.sampleRate <= 0.0)
        add("Sample rate must be finite and positive.");
    if (descriptor.frameCount == 0 || descriptor.channelCount == 0 || descriptor.channelCount > 2)
        add("Frame count and supported mono/stereo channel count are required.");
    if (descriptor.bytesPerFrame == 0
        || descriptor.frameCount > std::numeric_limits<std::uint64_t>::max() / descriptor.bytesPerFrame)
        add("Frame byte mapping is zero or overflows 64-bit range accounting.");
    else if (descriptor.dataSizeBytes != 0
             && descriptor.frameCount * descriptor.bytesPerFrame > descriptor.dataSizeBytes)
        add("Data range cannot contain the declared frame count.");
    if (descriptor.dataOffsetBytes > std::numeric_limits<std::uint64_t>::max() - descriptor.dataSizeBytes)
        add("Data offset and size overflow 64-bit range accounting.");
    if (descriptor.headSizeBytes == 0 || descriptor.pageSizeBytes == 0)
        add("Head and page sizes must be non-zero.");
    result.valid = result.findings.empty();
    return result;
}

WavSampleDataSourceBuildResult buildWavSampleDataSourceDescriptor(
    const std::string& sourceId,
    const std::string& sourcePath,
    const std::uint64_t generation,
    const std::uint64_t headSizeBytes,
    const std::uint64_t pageSizeBytes)
{
    namespace fs = std::filesystem;
    WavSampleDataSourceBuildResult result;
    result.state = "WAV descriptor unavailable";
    const fs::path path(sourcePath);
    std::error_code error;
    result.sourceFileSizeBytes = fs::file_size(path, error);
    if (error)
    {
        result.findings.push_back("WAV source file is missing or unreadable.");
        return result;
    }
    const auto modified = fs::last_write_time(path, error);
    if (!error)
        result.sourceModifiedTimeTicks = modified.time_since_epoch().count();

    std::ifstream input(path, std::ios::binary);
    std::array<unsigned char, 12> header {};
    if (!input.read(reinterpret_cast<char*>(header.data()), header.size()))
    {
        result.findings.push_back("WAV RIFF header is truncated.");
        return result;
    }
    const std::string container(reinterpret_cast<const char*>(header.data()), 4);
    const std::string wave(reinterpret_cast<const char*>(header.data() + 8), 4);
    result.rf64 = container == "RF64";
    if ((!result.rf64 && container != "RIFF") || wave != "WAVE")
    {
        result.findings.push_back("Source is not a RIFF/RF64 WAVE file.");
        return result;
    }

    bool fmtFound = false;
    bool dataFound = false;
    std::uint16_t channelCount = 0;
    std::uint32_t sampleRate = 0;
    std::uint64_t rf64DataSize = 0;
    while (input && static_cast<std::uint64_t>(input.tellg()) + 8 <= result.sourceFileSizeBytes)
    {
        std::array<unsigned char, 8> chunkHeader {};
        if (!input.read(reinterpret_cast<char*>(chunkHeader.data()), chunkHeader.size()))
            break;
        const std::string chunkId(reinterpret_cast<const char*>(chunkHeader.data()), 4);
        const auto chunkSize32 = readLe32(chunkHeader.data() + 4);
        const auto chunkDataOffset = static_cast<std::uint64_t>(input.tellg());
        std::uint64_t chunkSize = chunkSize32;

        if (chunkId == "ds64")
        {
            if (chunkSize < 28)
            {
                result.findings.push_back("RF64 ds64 chunk is truncated.");
                return result;
            }
            std::array<unsigned char, 28> ds64 {};
            if (!input.read(reinterpret_cast<char*>(ds64.data()), ds64.size()))
            {
                result.findings.push_back("RF64 ds64 values are truncated.");
                return result;
            }
            rf64DataSize = readLe64(ds64.data() + 8);
        }
        else if (chunkId == "fmt ")
        {
            if (chunkSize < 16)
            {
                result.findings.push_back("WAV fmt chunk is shorter than 16 bytes.");
                return result;
            }
            std::array<unsigned char, 16> format {};
            if (!input.read(reinterpret_cast<char*>(format.data()), format.size()))
            {
                result.findings.push_back("WAV fmt payload is truncated.");
                return result;
            }
            result.formatTag = readLe16(format.data());
            channelCount = readLe16(format.data() + 2);
            sampleRate = readLe32(format.data() + 4);
            result.blockAlign = readLe16(format.data() + 12);
            result.bitsPerSample = readLe16(format.data() + 14);
            if (result.formatTag == 0xfffeu)
            {
                if (chunkSize < 40)
                {
                    result.findings.push_back("WAVE_FORMAT_EXTENSIBLE fmt metadata is truncated.");
                    return result;
                }
                std::array<unsigned char, 24> extension {};
                if (!input.read(reinterpret_cast<char*>(extension.data()), extension.size()))
                {
                    result.findings.push_back("WAVE_FORMAT_EXTENSIBLE payload is truncated.");
                    return result;
                }
                static constexpr std::array<unsigned char, 14> canonicalSubformatTail {
                    0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x80,
                    0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71
                };
                const auto extensionSize = readLe16(extension.data());
                const auto validBitsPerSample = readLe16(extension.data() + 2);
                const auto subformatTag = readLe16(extension.data() + 8);
                const auto canonicalTail = std::equal(canonicalSubformatTail.begin(),
                                                      canonicalSubformatTail.end(),
                                                      extension.data() + 10);
                if (extensionSize < 22 || !canonicalTail
                    || (subformatTag != 1 && subformatTag != 3)
                    || validBitsPerSample == 0
                    || validBitsPerSample > result.bitsPerSample)
                {
                    result.findings.push_back("WAVE_FORMAT_EXTENSIBLE subformat is unsupported.");
                    return result;
                }
                result.formatTag = subformatTag;
            }
            fmtFound = true;
        }
        else if (chunkId == "data")
        {
            if (!fmtFound)
            {
                result.findings.push_back("WAV data chunk appears before required format metadata.");
                return result;
            }
            chunkSize = result.rf64 && chunkSize32 == 0xffffffffu ? rf64DataSize : chunkSize32;
            result.descriptor.dataOffsetBytes = chunkDataOffset;
            result.descriptor.dataSizeBytes = chunkSize;
            dataFound = true;
            break;
        }

        if (chunkDataOffset > result.sourceFileSizeBytes
            || chunkSize > result.sourceFileSizeBytes - chunkDataOffset)
        {
            result.findings.push_back("WAV chunk range is truncated or overflows the source file.");
            return result;
        }
        const auto paddedSize = chunkSize + (chunkSize & 1u);
        if (paddedSize < chunkSize
            || chunkDataOffset > std::numeric_limits<std::uint64_t>::max() - paddedSize)
        {
            result.findings.push_back("WAV chunk padding overflows 64-bit range accounting.");
            return result;
        }
        input.seekg(static_cast<std::streamoff>(chunkDataOffset + paddedSize));
    }

    if (!fmtFound || !dataFound)
    {
        result.findings.push_back("WAV requires both fmt and data chunks.");
        return result;
    }
    if (result.descriptor.dataOffsetBytes > result.sourceFileSizeBytes
        || result.descriptor.dataSizeBytes
            > result.sourceFileSizeBytes - result.descriptor.dataOffsetBytes)
    {
        result.findings.push_back("WAV data range is truncated.");
        return result;
    }
    const auto supportedPcm = result.formatTag == 1
        && (result.bitsPerSample == 16 || result.bitsPerSample == 24
            || result.bitsPerSample == 32);
    const auto supportedFloat = result.formatTag == 3 && result.bitsPerSample == 32;
    if ((!supportedPcm && !supportedFloat) || channelCount < 1 || channelCount > 2
        || sampleRate == 0 || result.blockAlign == 0
        || result.blockAlign != channelCount * (result.bitsPerSample / 8u))
    {
        result.findings.push_back("WAV encoding is unsupported; expected mono/stereo PCM16/24/32 or float32.");
        return result;
    }
    if (result.descriptor.dataSizeBytes == 0
        || result.descriptor.dataSizeBytes % result.blockAlign != 0)
    {
        result.findings.push_back("WAV data size is empty or not frame-aligned.");
        return result;
    }

    const auto canonical = fs::weakly_canonical(path, error).generic_string();
    const auto canonicalPath = error ? path.lexically_normal().generic_string() : canonical;
    const auto provenance = canonicalPath + "|size="
        + std::to_string(result.sourceFileSizeBytes) + "|mtime="
        + std::to_string(result.sourceModifiedTimeTicks);
    result.descriptor.version = sampleDataSourceDescriptorVersion;
    result.descriptor.kind = SampleDataSourceKind::wavFile;
    result.descriptor.sourceId = sourceId;
    result.descriptor.canonicalSourceIdentity = canonicalPath;
    result.descriptor.provenanceIdentity = provenance;
    result.descriptor.formatName = supportedFloat ? "WAV float32" : "WAV PCM";
    result.descriptor.channelLayout = channelCount == 1 ? "mono" : "stereo";
    result.descriptor.generation = generation == 0 ? stableGeneration(provenance) : generation;
    result.descriptor.sampleRate = sampleRate;
    result.descriptor.frameCount = result.descriptor.dataSizeBytes / result.blockAlign;
    result.descriptor.channelCount = channelCount;
    result.descriptor.bytesPerFrame = result.blockAlign;
    result.descriptor.headSizeBytes = headSizeBytes;
    result.descriptor.pageSizeBytes = pageSizeBytes;
    const auto validation = validateSampleDataSourceDescriptor(result.descriptor);
    result.findings.insert(result.findings.end(), validation.findings.begin(), validation.findings.end());
    result.built = validation.valid;
    result.state = result.built ? "WAV descriptor ready" : "WAV descriptor invalid";
    return result;
}

ResidentSampleDataSource::ResidentSampleDataSource(
    SampleDataSourceDescriptor descriptor,
    std::shared_ptr<const PreparedPlaybackDecodedSampleData> data)
    : sourceDescriptor(std::move(descriptor)), decodedData(std::move(data))
{
}

SampleFrameView ResidentSampleDataSource::acquireFrameView(
    const std::uint64_t firstFrame, const std::uint32_t requestedFrames) const noexcept
{
    SampleFrameView view;
    view.firstFrame = firstFrame;
    view.generation = sourceDescriptor.generation;
    if (firstFrame >= sourceDescriptor.frameCount)
    {
        view.status = SampleFrameViewStatus::endOfSource;
        return view;
    }
    if (decodedData == nullptr || decodedData->normalizedChannels.empty()
        || decodedData->normalizedChannels.size() < sourceDescriptor.channelCount)
        return view;
    const auto available = std::min<std::uint64_t>(
        requestedFrames, sourceDescriptor.frameCount - firstFrame);
    if (available == 0 || available > std::numeric_limits<std::uint32_t>::max())
        return view;
    for (std::size_t channel = 0; channel < sourceDescriptor.channelCount; ++channel)
    {
        if (decodedData->normalizedChannels[channel].size() < firstFrame + available)
            return view;
        view.channels[channel] = decodedData->normalizedChannels[channel].data() + firstFrame;
    }
    view.status = SampleFrameViewStatus::ready;
    view.frameCount = static_cast<std::uint32_t>(available);
    view.channelCount = sourceDescriptor.channelCount;
    view.lease = SamplePageLease(&leaseCount);
    return view;
}

DeterministicFakePagedSampleDataSource::DeterministicFakePagedSampleDataSource(
    SampleDataSourceDescriptor descriptor,
    std::vector<std::vector<float>> channels,
    const std::uint64_t headFrameCount,
    const std::uint64_t pageFrameCount,
    std::vector<bool> readyPages)
    : sourceDescriptor(std::move(descriptor)),
      sampleChannels(std::move(channels)),
      headFrames(std::min(headFrameCount, sourceDescriptor.frameCount)),
      pageFrames(std::max<std::uint64_t>(1, pageFrameCount))
{
    pageReady.reserve(readyPages.size());
    for (const auto ready : readyPages)
        pageReady.push_back(std::make_unique<std::atomic<bool>>(ready));
    leaseCounts.reserve(pageReady.size() + 1);
    for (std::size_t index = 0; index <= pageReady.size(); ++index)
        leaseCounts.push_back(std::make_unique<std::atomic<std::uint32_t>>(0));
}

SampleFrameView DeterministicFakePagedSampleDataSource::acquireFrameView(
    const std::uint64_t firstFrame, const std::uint32_t requestedFrames) const noexcept
{
    SampleFrameView view;
    view.firstFrame = firstFrame;
    view.generation = sourceDescriptor.generation;
    if (firstFrame >= sourceDescriptor.frameCount)
    {
        view.status = SampleFrameViewStatus::endOfSource;
        return view;
    }
    if (sampleChannels.size() < sourceDescriptor.channelCount || requestedFrames == 0)
        return view;

    std::uint64_t boundary = headFrames;
    std::size_t leaseIndex = 0;
    if (firstFrame >= headFrames)
    {
        const auto pageIndex = (firstFrame - headFrames) / pageFrames;
        if (pageIndex >= pageReady.size()
            || !pageReady[static_cast<std::size_t>(pageIndex)]->load(std::memory_order_acquire))
        {
            view.status = SampleFrameViewStatus::pageMissing;
            return view;
        }
        boundary = headFrames + (pageIndex + 1) * pageFrames;
        leaseIndex = static_cast<std::size_t>(pageIndex + 1);
    }
    const auto available = std::min<std::uint64_t>(
        { requestedFrames, sourceDescriptor.frameCount - firstFrame, boundary - firstFrame });
    if (available == 0)
        return view;
    for (std::size_t channel = 0; channel < sourceDescriptor.channelCount; ++channel)
    {
        if (sampleChannels[channel].size() < firstFrame + available)
            return view;
        view.channels[channel] = sampleChannels[channel].data() + firstFrame;
    }
    view.status = SampleFrameViewStatus::ready;
    view.frameCount = static_cast<std::uint32_t>(available);
    view.channelCount = sourceDescriptor.channelCount;
    view.lease = SamplePageLease(leaseCounts[leaseIndex].get());
    return view;
}

void DeterministicFakePagedSampleDataSource::setPageReady(
    const std::size_t pageIndex, const bool ready) noexcept
{
    if (pageIndex < pageReady.size())
        pageReady[pageIndex]->store(ready, std::memory_order_release);
}

std::uint32_t DeterministicFakePagedSampleDataSource::pageLeaseCount(
    const std::size_t pageIndex) const noexcept
{
    return pageIndex < leaseCounts.size()
        ? leaseCounts[pageIndex]->load(std::memory_order_acquire) : 0;
}

bool SamplePageIntentRing::push(SamplePageIntent intent) noexcept
{
    const auto write = writeIndex.load(std::memory_order_relaxed);
    const auto read = readIndex.load(std::memory_order_acquire);
    if (write - read >= capacity)
    {
        dropped.fetch_add(1, std::memory_order_relaxed);
        return false;
    }
    intents[write % capacity] = intent;
    writeIndex.store(write + 1, std::memory_order_release);
    published.fetch_add(1, std::memory_order_relaxed);
    const auto depth = write + 1 - read;
    auto observed = maximumDepth.load(std::memory_order_relaxed);
    while (observed < depth
           && !maximumDepth.compare_exchange_weak(observed, depth,
                                                  std::memory_order_relaxed))
    {
    }
    return true;
}

bool SamplePageIntentRing::pop(SamplePageIntent& intent) noexcept
{
    const auto read = readIndex.load(std::memory_order_relaxed);
    const auto write = writeIndex.load(std::memory_order_acquire);
    if (read == write)
        return false;
    intent = intents[read % capacity];
    readIndex.store(read + 1, std::memory_order_release);
    consumed.fetch_add(1, std::memory_order_relaxed);
    return true;
}

SamplePageIntentRingMetrics SamplePageIntentRing::metrics() const noexcept
{
    return { published.load(std::memory_order_relaxed),
             consumed.load(std::memory_order_relaxed),
             dropped.load(std::memory_order_relaxed),
             maximumDepth.load(std::memory_order_relaxed) };
}

struct WavPagedSampleDataSource::PageStorage
{
    std::uint64_t firstFrame = 0;
    std::uint64_t pageIndex = std::numeric_limits<std::uint64_t>::max();
    std::uint64_t residentBytes = 0;
    std::uint32_t frameCount = 0;
    std::vector<std::vector<float>> channels;
    std::atomic<std::uint32_t> leaseCount {0};
    std::atomic<std::uint64_t> lastUseOrdinal {0};
};

SamplePageRequestScheduler::SamplePageRequestScheduler(const std::size_t capacity)
    : maximumCapacity(std::max<std::size_t>(1, capacity))
{
    pending.reserve(maximumCapacity);
}

bool SamplePageRequestScheduler::submit(SamplePageRequest request)
{
    if (request.sourceGeneration == 0)
    {
        ++schedulerMetrics.rejectedCount;
        return false;
    }
    const auto duplicate = std::find_if(pending.begin(), pending.end(), [&](const auto& item)
    {
        return item.sourceGeneration == request.sourceGeneration
            && item.pageIndex == request.pageIndex;
    });
    if (duplicate != pending.end())
    {
        if (static_cast<int>(request.priority) > static_cast<int>(duplicate->priority))
            duplicate->priority = request.priority;
        ++schedulerMetrics.duplicateCount;
        return true;
    }
    if (pending.size() >= maximumCapacity)
    {
        const auto lowest = std::min_element(pending.begin(), pending.end(), [](const auto& left, const auto& right)
        {
            if (left.priority != right.priority)
                return static_cast<int>(left.priority) < static_cast<int>(right.priority);
            return left.ordinal > right.ordinal;
        });
        if (lowest == pending.end()
            || static_cast<int>(request.priority) <= static_cast<int>(lowest->priority))
        {
            ++schedulerMetrics.rejectedCount;
            return false;
        }
        pending.erase(lowest);
        ++schedulerMetrics.displacedCount;
    }
    request.ordinal = nextOrdinal++;
    pending.push_back(request);
    ++schedulerMetrics.acceptedCount;
    schedulerMetrics.pendingDepth = pending.size();
    schedulerMetrics.maximumPendingDepth = std::max(schedulerMetrics.maximumPendingDepth,
                                                     pending.size());
    return true;
}

bool SamplePageRequestScheduler::popNext(SamplePageRequest& request)
{
    if (pending.empty())
        return false;
    const auto selected = std::max_element(pending.begin(), pending.end(), [](const auto& left, const auto& right)
    {
        if (left.priority != right.priority)
            return static_cast<int>(left.priority) < static_cast<int>(right.priority);
        return left.ordinal > right.ordinal;
    });
    request = *selected;
    pending.erase(selected);
    schedulerMetrics.pendingDepth = pending.size();
    return true;
}

std::size_t SamplePageRequestScheduler::cancelGeneration(
    const std::uint64_t sourceGeneration)
{
    const auto previousSize = pending.size();
    pending.erase(std::remove_if(pending.begin(), pending.end(), [&](const auto& request)
    {
        return request.sourceGeneration == sourceGeneration;
    }), pending.end());
    const auto cancelled = previousSize - pending.size();
    schedulerMetrics.cancelledCount += cancelled;
    schedulerMetrics.pendingDepth = pending.size();
    return cancelled;
}

WavPagedSampleDataSource::WavPagedSampleDataSource(
    WavSampleDataSourceBuildResult descriptorResult,
    const std::uint64_t pageCacheBudgetBytes)
    : sourceDescriptor(std::move(descriptorResult.descriptor)),
      sourcePath(sourceDescriptor.canonicalSourceIdentity),
      formatTag(descriptorResult.formatTag),
      bitsPerSample(descriptorResult.bitsPerSample),
      blockAlign(descriptorResult.blockAlign),
      expectedFileSize(descriptorResult.sourceFileSizeBytes),
      expectedModifiedTicks(descriptorResult.sourceModifiedTimeTicks),
      maximumPageCacheBytes(pageCacheBudgetBytes)
{
    if (!descriptorResult.built || !validateSampleDataSourceDescriptor(sourceDescriptor).valid)
    {
        failureState = "WAV paged source requires a valid descriptor";
        return;
    }
    const auto decodedBytesPerFrame = static_cast<std::uint64_t>(
        sourceDescriptor.channelCount) * sizeof(float);
    configuredHeadFrames = std::min<std::uint64_t>(
        sourceDescriptor.frameCount,
        std::max<std::uint64_t>(1, sourceDescriptor.headSizeBytes / decodedBytesPerFrame));
    configuredPageFrames = std::max<std::uint64_t>(
        1, sourceDescriptor.pageSizeBytes / decodedBytesPerFrame);
    const auto remaining = sourceDescriptor.frameCount - configuredHeadFrames;
    configuredPageCount = remaining == 0 ? 0 : 1 + (remaining - 1) / configuredPageFrames;
    if (configuredPageCount != 0)
    {
        pageSlots = std::make_unique<std::atomic<PageStorage*>[]>(configuredPageCount);
        for (std::uint64_t index = 0; index < configuredPageCount; ++index)
            pageSlots[index].store(nullptr, std::memory_order_relaxed);
    }
}

WavPagedSampleDataSource::~WavPagedSampleDataSource() = default;

bool WavPagedSampleDataSource::sourceUnchanged() const
{
    namespace fs = std::filesystem;
    std::error_code error;
    const auto size = fs::file_size(fs::path(sourcePath), error);
    if (error || size != expectedFileSize)
        return false;
    const auto modified = fs::last_write_time(fs::path(sourcePath), error);
    return !error && modified.time_since_epoch().count() == expectedModifiedTicks;
}

bool WavPagedSampleDataSource::prepareHead()
{
    return prepareRange(true, 0);
}

bool WavPagedSampleDataSource::preparePage(const std::uint64_t pageIndex)
{
    return prepareRange(false, pageIndex);
}

void WavPagedSampleDataSource::reclaimRetiredPages()
{
    if (activeAcquisitions.load(std::memory_order_acquire) != 0)
        return;
    for (auto iterator = retiredPages.begin(); iterator != retiredPages.end();)
    {
        if ((*iterator)->leaseCount.load(std::memory_order_acquire) != 0)
        {
            ++iterator;
            continue;
        }
        allocatedPagesBytes.fetch_sub((*iterator)->residentBytes, std::memory_order_relaxed);
        retiredPagesBytes.fetch_sub((*iterator)->residentBytes, std::memory_order_relaxed);
        iterator = retiredPages.erase(iterator);
        retiredPagesCount.fetch_sub(1, std::memory_order_relaxed);
    }
}

bool WavPagedSampleDataSource::makeRoomForPage(const std::uint64_t requiredBytes)
{
    if (requiredBytes == 0 || requiredBytes > maximumPageCacheBytes)
        return false;
    reclaimRetiredPages();
    while (allocatedPagesBytes.load(std::memory_order_relaxed)
               > maximumPageCacheBytes - requiredBytes)
    {
        auto selected = ownedPages.end();
        auto oldestUse = std::numeric_limits<std::uint64_t>::max();
        for (auto iterator = ownedPages.begin(); iterator != ownedPages.end(); ++iterator)
        {
            const auto& page = **iterator;
            if (page.pageIndex == std::numeric_limits<std::uint64_t>::max())
                continue;
            if (page.leaseCount.load(std::memory_order_acquire) != 0)
            {
                pinnedEvictionSkips.fetch_add(1, std::memory_order_relaxed);
                continue;
            }
            const auto use = page.lastUseOrdinal.load(std::memory_order_relaxed);
            if (use < oldestUse)
            {
                oldestUse = use;
                selected = iterator;
            }
        }
        if (selected == ownedPages.end())
            return false;

        auto retired = std::move(*selected);
        pageSlots[retired->pageIndex].store(nullptr, std::memory_order_release);
        pageResidentBytes.fetch_sub(retired->residentBytes, std::memory_order_relaxed);
        ownedPages.erase(selected);
        retiredPages.push_back(std::move(retired));
        retiredPagesCount.fetch_add(1, std::memory_order_relaxed);
        retiredPagesBytes.fetch_add(retiredPages.back()->residentBytes, std::memory_order_relaxed);
        evictions.fetch_add(1, std::memory_order_relaxed);
        const auto before = allocatedPagesBytes.load(std::memory_order_relaxed);
        reclaimRetiredPages();
        if (allocatedPagesBytes.load(std::memory_order_relaxed) == before)
            return false;
    }
    return true;
}

bool WavPagedSampleDataSource::prepareRange(const bool head, const std::uint64_t pageIndex)
{
    const auto preparationStarted = std::chrono::steady_clock::now();
    if (configuredHeadFrames == 0 || (!head && pageIndex >= configuredPageCount))
    {
        failureState = "WAV range request is outside the descriptor";
        return false;
    }
    auto* existing = head ? headSlot.load(std::memory_order_acquire)
                          : pageSlots[pageIndex].load(std::memory_order_acquire);
    if (existing != nullptr)
    {
        duplicateRequests.fetch_add(1, std::memory_order_relaxed);
        return true;
    }
    if (!sourceUnchanged())
    {
        mutationFailures.fetch_add(1, std::memory_order_relaxed);
        failureState = "WAV source changed after descriptor construction";
        return false;
    }

    const auto firstFrame = head
        ? 0 : configuredHeadFrames + pageIndex * configuredPageFrames;
    const auto frameCount64 = std::min<std::uint64_t>(
        head ? configuredHeadFrames : configuredPageFrames,
        sourceDescriptor.frameCount - firstFrame);
    if (frameCount64 == 0 || frameCount64 > std::numeric_limits<std::uint32_t>::max())
        return false;
    const auto byteCount = frameCount64 * blockAlign;
    const auto byteOffset = sourceDescriptor.dataOffsetBytes + firstFrame * blockAlign;
    if (byteOffset > expectedFileSize || byteCount > expectedFileSize - byteOffset
        || byteCount > std::numeric_limits<std::size_t>::max())
    {
        failureState = "WAV range overflows or is truncated";
        return false;
    }

    std::ifstream input(std::filesystem::path(sourcePath), std::ios::binary);
    input.seekg(static_cast<std::streamoff>(byteOffset));
    std::vector<unsigned char> raw(static_cast<std::size_t>(byteCount));
    if (!input.read(reinterpret_cast<char*>(raw.data()),
                    static_cast<std::streamsize>(raw.size())))
    {
        failureState = "WAV range read failed";
        return false;
    }

    const auto residentBytes = frameCount64 * sourceDescriptor.channelCount * sizeof(float);
    if (!head && !makeRoomForPage(residentBytes))
    {
        cachePressureFailures.fetch_add(1, std::memory_order_relaxed);
        failureState = "WAV page cache budget is exhausted by pinned or retired pages";
        return false;
    }

    auto page = std::make_unique<PageStorage>();
    page->firstFrame = firstFrame;
    page->pageIndex = head ? std::numeric_limits<std::uint64_t>::max() : pageIndex;
    page->residentBytes = residentBytes;
    page->frameCount = static_cast<std::uint32_t>(frameCount64);
    page->channels.resize(sourceDescriptor.channelCount);
    for (auto& channel : page->channels)
        channel.resize(page->frameCount);
    const auto bytesPerSample = bitsPerSample / 8u;
    for (std::uint32_t frame = 0; frame < page->frameCount; ++frame)
    {
        for (std::uint32_t channel = 0; channel < sourceDescriptor.channelCount; ++channel)
        {
            const auto offset = static_cast<std::size_t>(frame) * blockAlign
                + static_cast<std::size_t>(channel) * bytesPerSample;
            float value = 0.0f;
            if (formatTag == 3)
            {
                std::uint32_t bits = readLe32(raw.data() + offset);
                std::memcpy(&value, &bits, sizeof(value));
            }
            else if (bitsPerSample == 16)
            {
                const auto sample = static_cast<std::int16_t>(readLe16(raw.data() + offset));
                value = static_cast<float>(sample) / 32768.0f;
            }
            else if (bitsPerSample == 24)
            {
                std::int32_t sample = static_cast<std::int32_t>(raw[offset])
                    | (static_cast<std::int32_t>(raw[offset + 1]) << 8)
                    | (static_cast<std::int32_t>(raw[offset + 2]) << 16);
                if ((sample & 0x00800000) != 0)
                    sample |= static_cast<std::int32_t>(0xff000000);
                value = static_cast<float>(sample) / 8388608.0f;
            }
            else
            {
                const auto sample = static_cast<std::int32_t>(readLe32(raw.data() + offset));
                value = static_cast<float>(static_cast<double>(sample) / 2147483648.0);
            }
            page->channels[channel][frame] = value;
        }
    }

    auto* published = page.get();
    ownedPages.push_back(std::move(page));
    if (head)
        headSlot.store(published, std::memory_order_release);
    else
        pageSlots[pageIndex].store(published, std::memory_order_release);
    rangeReads.fetch_add(1, std::memory_order_relaxed);
    readBytes.fetch_add(byteCount, std::memory_order_relaxed);
    convertedFrames.fetch_add(frameCount64, std::memory_order_relaxed);
    (head ? headResidentBytes : pageResidentBytes).fetch_add(residentBytes,
                                                             std::memory_order_relaxed);
    if (!head)
    {
        const auto allocated = allocatedPagesBytes.fetch_add(
            residentBytes, std::memory_order_relaxed) + residentBytes;
        auto peak = peakAllocatedPagesBytes.load(std::memory_order_relaxed);
        while (peak < allocated
               && !peakAllocatedPagesBytes.compare_exchange_weak(
                   peak, allocated, std::memory_order_relaxed))
        {
        }
    }
    const auto latencyMicros = static_cast<std::uint64_t>(std::chrono::duration_cast<
        std::chrono::microseconds>(std::chrono::steady_clock::now() - preparationStarted).count());
    totalReadMicros.fetch_add(latencyMicros, std::memory_order_relaxed);
    auto maximumLatency = maximumReadMicros.load(std::memory_order_relaxed);
    while (maximumLatency < latencyMicros
           && !maximumReadMicros.compare_exchange_weak(
               maximumLatency, latencyMicros, std::memory_order_relaxed))
    {
    }
    return true;
}

SampleFrameView WavPagedSampleDataSource::acquireFrameView(
    const std::uint64_t firstFrame, const std::uint32_t requestedFrames) const noexcept
{
    SampleFrameView view;
    view.firstFrame = firstFrame;
    view.generation = sourceDescriptor.generation;
    if (firstFrame >= sourceDescriptor.frameCount)
    {
        view.status = SampleFrameViewStatus::endOfSource;
        return view;
    }
    const std::atomic<PageStorage*>* slot = nullptr;
    if (firstFrame < configuredHeadFrames)
        slot = &headSlot;
    else
    {
        const auto pageIndex = (firstFrame - configuredHeadFrames) / configuredPageFrames;
        if (pageIndex < configuredPageCount)
            slot = &pageSlots[pageIndex];
    }
    activeAcquisitions.fetch_add(1, std::memory_order_acq_rel);
    auto* page = slot == nullptr ? nullptr : slot->load(std::memory_order_acquire);
    if (page == nullptr)
    {
        activeAcquisitions.fetch_sub(1, std::memory_order_acq_rel);
        pageMisses.fetch_add(1, std::memory_order_relaxed);
        view.status = SampleFrameViewStatus::pageMissing;
        return view;
    }
    view.lease = SamplePageLease(&page->leaseCount, &leasedPagesBytes, page->residentBytes);
    if (slot->load(std::memory_order_acquire) != page)
    {
        view.lease = {};
        activeAcquisitions.fetch_sub(1, std::memory_order_acq_rel);
        view.status = SampleFrameViewStatus::pageMissing;
        return view;
    }
    page->lastUseOrdinal.store(accessOrdinal.fetch_add(1, std::memory_order_relaxed),
                               std::memory_order_relaxed);
    (firstFrame < configuredHeadFrames ? headHits : pageHits).fetch_add(
        1, std::memory_order_relaxed);
    activeAcquisitions.fetch_sub(1, std::memory_order_acq_rel);
    const auto localFrame = firstFrame - page->firstFrame;
    const auto available = std::min<std::uint64_t>(
        requestedFrames, page->frameCount - localFrame);
    for (std::size_t channel = 0; channel < sourceDescriptor.channelCount; ++channel)
        view.channels[channel] = page->channels[channel].data() + localFrame;
    view.status = SampleFrameViewStatus::ready;
    view.frameCount = static_cast<std::uint32_t>(available);
    view.channelCount = sourceDescriptor.channelCount;
    return view;
}

bool WavPagedSampleDataSource::publishPageIntent(
    const std::uint64_t firstFrame,
    const SamplePageRequestPriority priority,
    const std::uint64_t voiceId) const noexcept
{
    if (firstFrame < configuredHeadFrames || firstFrame >= sourceDescriptor.frameCount)
        return false;
    const auto pageIndex = (firstFrame - configuredHeadFrames) / configuredPageFrames;
    return pageIndex < configuredPageCount
        && intentRing.push({ sourceDescriptor.generation, pageIndex, priority, voiceId });
}

std::size_t WavPagedSampleDataSource::drainPageIntents(
    SamplePageRequestScheduler& scheduler, const std::size_t maximumCount)
{
    std::size_t drained = 0;
    SamplePageIntent intent;
    while (drained < maximumCount && intentRing.pop(intent))
    {
        scheduler.submit({ intent.sourceGeneration, intent.pageIndex, intent.priority });
        ++drained;
    }
    return drained;
}

WavPagedSampleDataSourceMetrics WavPagedSampleDataSource::metrics() const noexcept
{
    return { rangeReads.load(std::memory_order_relaxed),
             readBytes.load(std::memory_order_relaxed),
             convertedFrames.load(std::memory_order_relaxed),
             headResidentBytes.load(std::memory_order_relaxed),
             pageResidentBytes.load(std::memory_order_relaxed),
             duplicateRequests.load(std::memory_order_relaxed),
             mutationFailures.load(std::memory_order_relaxed),
             maximumPageCacheBytes,
             allocatedPagesBytes.load(std::memory_order_relaxed),
             peakAllocatedPagesBytes.load(std::memory_order_relaxed),
             evictions.load(std::memory_order_relaxed),
             pinnedEvictionSkips.load(std::memory_order_relaxed),
             cachePressureFailures.load(std::memory_order_relaxed),
             retiredPagesCount.load(std::memory_order_relaxed),
             retiredPagesBytes.load(std::memory_order_relaxed),
             headHits.load(std::memory_order_relaxed),
             pageHits.load(std::memory_order_relaxed),
             pageMisses.load(std::memory_order_relaxed),
             leasedPagesBytes.load(std::memory_order_relaxed),
             totalReadMicros.load(std::memory_order_relaxed),
             maximumReadMicros.load(std::memory_order_relaxed) };
}

struct PackagePagedSampleDataSource::PageStorage
{
    std::uint64_t firstFrame = 0;
    std::uint32_t frameCount = 0;
    std::vector<std::vector<float>> channels;
    std::atomic<std::uint32_t> leaseCount {0};
};

PackagePagedSampleDataSource::PackagePagedSampleDataSource(
    SampleDataSourceDescriptor descriptor,
    std::shared_ptr<const PackageV2OpenResult> package,
    const PackageCryptoProvider& crypto)
    : sourceDescriptor(std::move(descriptor)),
      retainedPackage(std::move(package)),
      cryptoProvider(&crypto)
{
    if (sourceDescriptor.kind != SampleDataSourceKind::packageRecord
        || !validateSampleDataSourceDescriptor(sourceDescriptor).valid
        || retainedPackage == nullptr || !retainedPackage->opened)
    {
        failureState = "Package paged source requires a valid descriptor and open package TOC";
        return;
    }
    const auto decodedBytesPerFrame = static_cast<std::uint64_t>(
        sourceDescriptor.channelCount) * sizeof(float);
    configuredHeadFrames = std::min<std::uint64_t>(
        sourceDescriptor.frameCount,
        std::max<std::uint64_t>(1, sourceDescriptor.headSizeBytes / decodedBytesPerFrame));
    configuredPageFrames = std::max<std::uint64_t>(
        1, sourceDescriptor.pageSizeBytes / decodedBytesPerFrame);
    const auto remaining = sourceDescriptor.frameCount - configuredHeadFrames;
    configuredPageCount = remaining == 0 ? 0 : 1 + (remaining - 1) / configuredPageFrames;
    if (configuredPageCount != 0)
    {
        pageSlots = std::make_unique<std::atomic<PageStorage*>[]>(configuredPageCount);
        for (std::uint64_t index = 0; index < configuredPageCount; ++index)
            pageSlots[index].store(nullptr, std::memory_order_relaxed);
    }
}

PackagePagedSampleDataSource::~PackagePagedSampleDataSource() = default;

bool PackagePagedSampleDataSource::prepareHead(
    const std::function<bool()>& cancellationProbe)
{
    return prepareRecord(true, 0, cancellationProbe);
}

bool PackagePagedSampleDataSource::preparePage(
    const std::uint64_t pageIndex, const std::function<bool()>& cancellationProbe)
{
    return prepareRecord(false, pageIndex, cancellationProbe);
}

bool PackagePagedSampleDataSource::prepareRecord(
    const bool head, const std::uint64_t pageIndex,
    const std::function<bool()>& cancellationProbe)
{
    if (configuredHeadFrames == 0 || (!head && pageIndex >= configuredPageCount)
        || retainedPackage == nullptr || cryptoProvider == nullptr)
    {
        failureState = "Package page request is outside the source descriptor";
        return false;
    }
    auto* existing = head ? headSlot.load(std::memory_order_acquire)
                          : pageSlots[pageIndex].load(std::memory_order_acquire);
    if (existing != nullptr)
        return true;

    const PackageV2RecordIdentity identity {
        sourceDescriptor.sourceId,
        head ? PackageV2RecordKind::sampleHead : PackageV2RecordKind::samplePage,
        head ? 0 : pageIndex,
        sourceDescriptor.generation
    };
    auto opened = openPackageV2Record(*retainedPackage, identity,
                                      *cryptoProvider, cancellationProbe);
    sealedReadBytes.fetch_add(opened.metrics.bytesRead, std::memory_order_relaxed);
    if (!opened.opened)
    {
        authenticationFailures.fetch_add(opened.metrics.authenticationFailures,
                                         std::memory_order_relaxed);
        checksumFailures.fetch_add(opened.metrics.checksumFailures, std::memory_order_relaxed);
        cancellations.fetch_add(opened.metrics.cancellationCount, std::memory_order_relaxed);
        failureState = opened.state;
        if (!opened.issues.empty())
            failureState += ": " + opened.issues.front();
        return false;
    }
    openedRecords.fetch_add(1, std::memory_order_relaxed);
    const auto firstFrame = head ? 0 : configuredHeadFrames + pageIndex * configuredPageFrames;
    const auto frameCount = std::min<std::uint64_t>(
        head ? configuredHeadFrames : configuredPageFrames,
        sourceDescriptor.frameCount - firstFrame);
    const auto expectedBytes = frameCount * sourceDescriptor.channelCount * sizeof(float);
    if (opened.plaintextBytes.size() != expectedBytes
        || frameCount > std::numeric_limits<std::uint32_t>::max())
    {
        failureState = "Package sample record size does not match its frame mapping";
        return false;
    }

    auto storage = std::make_unique<PageStorage>();
    storage->firstFrame = firstFrame;
    storage->frameCount = static_cast<std::uint32_t>(frameCount);
    storage->channels.resize(sourceDescriptor.channelCount);
    for (auto& channel : storage->channels)
        channel.resize(storage->frameCount);
    for (std::uint32_t frame = 0; frame < storage->frameCount; ++frame)
    for (std::uint32_t channel = 0; channel < sourceDescriptor.channelCount; ++channel)
    {
        float sample = 0.0f;
        const auto offset = (static_cast<std::size_t>(frame) * sourceDescriptor.channelCount
                             + channel) * sizeof(float);
        std::memcpy(&sample, opened.plaintextBytes.data() + offset, sizeof(float));
        storage->channels[channel][frame] = sample;
    }
    auto* published = storage.get();
    ownedPages.push_back(std::move(storage));
    if (head)
        headSlot.store(published, std::memory_order_release);
    else
        pageSlots[pageIndex].store(published, std::memory_order_release);
    (head ? publishedHeadBytes : publishedPageBytes).fetch_add(expectedBytes,
                                                               std::memory_order_relaxed);
    return true;
}

SampleFrameView PackagePagedSampleDataSource::acquireFrameView(
    const std::uint64_t firstFrame, const std::uint32_t requestedFrames) const noexcept
{
    SampleFrameView view;
    view.firstFrame = firstFrame;
    view.generation = sourceDescriptor.generation;
    if (firstFrame >= sourceDescriptor.frameCount)
    {
        view.status = SampleFrameViewStatus::endOfSource;
        return view;
    }
    PageStorage* page = nullptr;
    if (firstFrame < configuredHeadFrames)
        page = headSlot.load(std::memory_order_acquire);
    else
    {
        const auto pageIndex = (firstFrame - configuredHeadFrames) / configuredPageFrames;
        if (pageIndex < configuredPageCount)
            page = pageSlots[pageIndex].load(std::memory_order_acquire);
    }
    if (page == nullptr)
    {
        view.status = SampleFrameViewStatus::pageMissing;
        return view;
    }
    const auto localFrame = firstFrame - page->firstFrame;
    const auto available = std::min<std::uint64_t>(requestedFrames,
                                                   page->frameCount - localFrame);
    for (std::uint32_t channel = 0; channel < sourceDescriptor.channelCount; ++channel)
        view.channels[channel] = page->channels[channel].data() + localFrame;
    view.status = SampleFrameViewStatus::ready;
    view.frameCount = static_cast<std::uint32_t>(available);
    view.channelCount = sourceDescriptor.channelCount;
    view.lease = SamplePageLease(&page->leaseCount);
    return view;
}

bool PackagePagedSampleDataSource::publishPageIntent(
    const std::uint64_t firstFrame, const SamplePageRequestPriority priority,
    const std::uint64_t voiceId) const noexcept
{
    if (firstFrame < configuredHeadFrames || firstFrame >= sourceDescriptor.frameCount)
        return false;
    const auto pageIndex = (firstFrame - configuredHeadFrames) / configuredPageFrames;
    return pageIndex < configuredPageCount
        && intentRing.push({ sourceDescriptor.generation, pageIndex, priority, voiceId });
}

std::size_t PackagePagedSampleDataSource::drainPageIntents(
    SamplePageRequestScheduler& scheduler, const std::size_t maximumCount)
{
    std::size_t drained = 0;
    SamplePageIntent intent;
    while (drained < maximumCount && intentRing.pop(intent))
    {
        scheduler.submit({ intent.sourceGeneration, intent.pageIndex, intent.priority });
        ++drained;
    }
    return drained;
}

PackagePagedSampleDataSourceMetrics PackagePagedSampleDataSource::metrics() const noexcept
{
    return { openedRecords.load(std::memory_order_relaxed),
             sealedReadBytes.load(std::memory_order_relaxed),
             publishedHeadBytes.load(std::memory_order_relaxed),
             publishedPageBytes.load(std::memory_order_relaxed),
             authenticationFailures.load(std::memory_order_relaxed),
             checksumFailures.load(std::memory_order_relaxed),
             cancellations.load(std::memory_order_relaxed) };
}
} // namespace drs::engine
