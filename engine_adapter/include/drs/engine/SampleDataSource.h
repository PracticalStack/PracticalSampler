#pragma once

#include "drs/engine/PreparedPlayback.h"
#include "drs/engine/PackageV2.h"
#include "drs/engine/PackageV3FileReader.h"

#include <array>
#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace drs::engine
{
inline constexpr std::uint32_t sampleDataSourceDescriptorVersion = 1;
// Keep enough decoded stereo PCM resident to cover background page-service jitter.
// At 48 kHz this is roughly 170 ms, including the first multi-file piano chord.
inline constexpr std::uint64_t defaultSampleHeadBytes = 64ull * 1024ull;
inline constexpr std::uint64_t defaultSamplePageBytes = 64ull * 1024ull;

enum class SampleDataSourceKind : std::uint8_t
{
    resident,
    wavFile,
    packageRecord,
    deterministicFake
};

struct SampleDataSourceDescriptor
{
    std::uint32_t version = sampleDataSourceDescriptorVersion;
    SampleDataSourceKind kind = SampleDataSourceKind::resident;
    std::string sourceId;
    std::string canonicalSourceIdentity;
    std::string provenanceIdentity;
    std::string formatName;
    std::string channelLayout;
    std::string checksumHex;
    std::uint64_t generation = 1;
    double sampleRate = 0.0;
    std::uint64_t frameCount = 0;
    std::uint32_t channelCount = 0;
    std::uint64_t bytesPerFrame = 0;
    std::uint64_t dataOffsetBytes = 0;
    std::uint64_t dataSizeBytes = 0;
    std::uint64_t headSizeBytes = defaultSampleHeadBytes;
    std::uint64_t pageSizeBytes = defaultSamplePageBytes;
};

struct SampleDataSourceDescriptorValidation
{
    bool valid = false;
    std::vector<std::string> findings;
};

SampleDataSourceDescriptorValidation validateSampleDataSourceDescriptor(
    const SampleDataSourceDescriptor& descriptor);

struct WavSampleDataSourceBuildResult
{
    bool built = false;
    SampleDataSourceDescriptor descriptor;
    std::uint16_t formatTag = 0;
    std::uint16_t bitsPerSample = 0;
    std::uint16_t blockAlign = 0;
    std::uint64_t sourceFileSizeBytes = 0;
    std::int64_t sourceModifiedTimeTicks = 0;
    bool rf64 = false;
    // Optional decoder for supported non-WAV sources. When present, the paged
    // source uses this bounded callback instead of reading PCM byte ranges.
    std::function<bool(std::uint64_t, std::uint32_t,
                       std::vector<std::vector<float>>&, std::string&)> rangeDecoder;
    std::string state;
    std::vector<std::string> findings;
};

WavSampleDataSourceBuildResult buildWavSampleDataSourceDescriptor(
    const std::string& sourceId,
    const std::string& sourcePath,
    std::uint64_t generation = 0,
    std::uint64_t headSizeBytes = defaultSampleHeadBytes,
    std::uint64_t pageSizeBytes = defaultSamplePageBytes);

WavSampleDataSourceBuildResult buildPagedSampleDataSourceDescriptor(
    const std::string& sourceId,
    const std::string& sourcePath,
    std::uint64_t generation = 0,
    std::uint64_t headSizeBytes = defaultSampleHeadBytes,
    std::uint64_t pageSizeBytes = defaultSamplePageBytes);

enum class SampleFrameViewStatus : std::uint8_t
{
    ready,
    endOfSource,
    pageMissing,
    failed
};

class SamplePageLease final
{
public:
    SamplePageLease() noexcept = default;
    explicit SamplePageLease(std::atomic<std::uint32_t>* counter,
                             std::atomic<std::uint64_t>* leasedBytes = nullptr,
                             std::uint64_t byteCount = 0) noexcept;
    SamplePageLease(const SamplePageLease& other) noexcept;
    SamplePageLease& operator=(const SamplePageLease& other) noexcept;
    SamplePageLease(SamplePageLease&& other) noexcept;
    SamplePageLease& operator=(SamplePageLease&& other) noexcept;
    ~SamplePageLease() noexcept;
    bool active() const noexcept { return leaseCounter != nullptr; }

private:
    void release() noexcept;
    std::atomic<std::uint32_t>* leaseCounter = nullptr;
    std::atomic<std::uint64_t>* leasedByteCounter = nullptr;
    std::uint64_t leasedByteCount = 0;
};

struct SampleFrameView
{
    SampleFrameViewStatus status = SampleFrameViewStatus::failed;
    std::array<const float*, 2> channels {};
    std::uint64_t firstFrame = 0;
    std::uint32_t frameCount = 0;
    std::uint32_t channelCount = 0;
    std::uint64_t generation = 0;
    SamplePageLease lease;
};

enum class SamplePageRequestPriority : std::uint8_t;

class ISampleDataSource
{
public:
    virtual ~ISampleDataSource() = default;
    virtual const SampleDataSourceDescriptor& descriptor() const noexcept = 0;
    virtual SampleFrameView acquireFrameView(std::uint64_t firstFrame,
                                             std::uint32_t requestedFrames) const noexcept = 0;
    virtual bool publishPageIntent(std::uint64_t,
                                   SamplePageRequestPriority,
                                   std::uint64_t) const noexcept
    {
        return false;
    }
};

using SampleDataSourcePtr = std::shared_ptr<const ISampleDataSource>;

class ResidentSampleDataSource final : public ISampleDataSource
{
public:
    ResidentSampleDataSource(SampleDataSourceDescriptor descriptor,
                             std::shared_ptr<const PreparedPlaybackDecodedSampleData> data);
    const SampleDataSourceDescriptor& descriptor() const noexcept override { return sourceDescriptor; }
    SampleFrameView acquireFrameView(std::uint64_t firstFrame,
                                     std::uint32_t requestedFrames) const noexcept override;

private:
    SampleDataSourceDescriptor sourceDescriptor;
    std::shared_ptr<const PreparedPlaybackDecodedSampleData> decodedData;
    mutable std::atomic<std::uint32_t> leaseCount {0};
};

class DeterministicFakePagedSampleDataSource final : public ISampleDataSource
{
public:
    DeterministicFakePagedSampleDataSource(SampleDataSourceDescriptor descriptor,
                                           std::vector<std::vector<float>> channels,
                                           std::uint64_t headFrameCount,
                                           std::uint64_t pageFrameCount,
                                           std::vector<bool> readyPages);
    const SampleDataSourceDescriptor& descriptor() const noexcept override { return sourceDescriptor; }
    SampleFrameView acquireFrameView(std::uint64_t firstFrame,
                                     std::uint32_t requestedFrames) const noexcept override;
    void setPageReady(std::size_t pageIndex, bool ready) noexcept;
    std::uint32_t pageLeaseCount(std::size_t pageIndex) const noexcept;

private:
    SampleDataSourceDescriptor sourceDescriptor;
    std::vector<std::vector<float>> sampleChannels;
    std::uint64_t headFrames = 0;
    std::uint64_t pageFrames = 1;
    std::vector<std::unique_ptr<std::atomic<bool>>> pageReady;
    mutable std::vector<std::unique_ptr<std::atomic<std::uint32_t>>> leaseCounts;
};

struct WavPagedSampleDataSourceMetrics
{
    std::uint64_t rangeReadCount = 0;
    std::uint64_t bytesRead = 0;
    std::uint64_t convertedFrameCount = 0;
    std::uint64_t residentHeadBytes = 0;
    std::uint64_t residentPageBytes = 0;
    std::uint64_t duplicateRequestCount = 0;
    std::uint64_t sourceMutationFailureCount = 0;
    std::uint64_t pageCacheBudgetBytes = 0;
    std::uint64_t allocatedPageBytes = 0;
    std::uint64_t maximumAllocatedPageBytes = 0;
    std::uint64_t evictionCount = 0;
    std::uint64_t pinnedEvictionSkipCount = 0;
    std::uint64_t cachePressureFailureCount = 0;
    std::uint64_t retiredPageCount = 0;
    std::uint64_t retiredPageBytes = 0;
    std::uint64_t headHitCount = 0;
    std::uint64_t pageHitCount = 0;
    std::uint64_t pageMissCount = 0;
    std::uint64_t leasedPageBytes = 0;
    std::uint64_t totalReadLatencyMicros = 0;
    std::uint64_t maximumReadLatencyMicros = 0;
};

enum class SamplePageRequestPriority : std::uint8_t
{
    lookAhead = 1,
    imminent = 2,
    head = 3
};

struct SamplePageRequest
{
    std::uint64_t sourceGeneration = 0;
    std::uint64_t pageIndex = 0;
    SamplePageRequestPriority priority = SamplePageRequestPriority::lookAhead;
    std::uint64_t ordinal = 0;
};

struct SamplePageIntent
{
    std::uint64_t sourceGeneration = 0;
    std::uint64_t pageIndex = 0;
    SamplePageRequestPriority priority = SamplePageRequestPriority::lookAhead;
    std::uint64_t voiceId = 0;
};

struct SamplePageIntentRingMetrics
{
    std::uint64_t publishedCount = 0;
    std::uint64_t consumedCount = 0;
    std::uint64_t droppedCount = 0;
    std::size_t maximumDepth = 0;
};

class SamplePageIntentRing final
{
public:
    static constexpr std::size_t capacity = 256;
    bool push(SamplePageIntent intent) noexcept;
    bool pop(SamplePageIntent& intent) noexcept;
    SamplePageIntentRingMetrics metrics() const noexcept;

private:
    std::array<SamplePageIntent, capacity> intents {};
    std::atomic<std::size_t> readIndex {0};
    std::atomic<std::size_t> writeIndex {0};
    std::atomic<std::uint64_t> published {0};
    std::atomic<std::uint64_t> consumed {0};
    std::atomic<std::uint64_t> dropped {0};
    std::atomic<std::size_t> maximumDepth {0};
};

struct SamplePageSchedulerMetrics
{
    std::size_t pendingDepth = 0;
    std::size_t maximumPendingDepth = 0;
    std::uint64_t acceptedCount = 0;
    std::uint64_t duplicateCount = 0;
    std::uint64_t displacedCount = 0;
    std::uint64_t rejectedCount = 0;
    std::uint64_t cancelledCount = 0;
};

class SamplePageRequestScheduler final
{
public:
    explicit SamplePageRequestScheduler(std::size_t capacity = 64);
    bool submit(SamplePageRequest request);
    bool popNext(SamplePageRequest& request);
    std::size_t cancelGeneration(std::uint64_t sourceGeneration);
    const SamplePageSchedulerMetrics& metrics() const noexcept { return schedulerMetrics; }

private:
    std::size_t maximumCapacity = 1;
    std::uint64_t nextOrdinal = 1;
    std::vector<SamplePageRequest> pending;
    SamplePageSchedulerMetrics schedulerMetrics;
};

class WavPagedSampleDataSource final : public ISampleDataSource
{
public:
    explicit WavPagedSampleDataSource(
        WavSampleDataSourceBuildResult descriptorResult,
        std::uint64_t pageCacheBudgetBytes = 64ull * 1024ull * 1024ull);
    ~WavPagedSampleDataSource() override;
    const SampleDataSourceDescriptor& descriptor() const noexcept override { return sourceDescriptor; }
    SampleFrameView acquireFrameView(std::uint64_t firstFrame,
                                     std::uint32_t requestedFrames) const noexcept override;
    bool publishPageIntent(std::uint64_t firstFrame,
                           SamplePageRequestPriority priority,
                           std::uint64_t voiceId) const noexcept override;
    std::size_t drainPageIntents(SamplePageRequestScheduler& scheduler,
                                 std::size_t maximumCount = SamplePageIntentRing::capacity);
    SamplePageIntentRingMetrics intentMetrics() const noexcept { return intentRing.metrics(); }
    bool prepareHead();
    bool preparePage(std::uint64_t pageIndex);
    std::uint64_t headFrameCount() const noexcept { return configuredHeadFrames; }
    std::uint64_t pageFrameCount() const noexcept { return configuredPageFrames; }
    std::uint64_t pageCount() const noexcept { return configuredPageCount; }
    WavPagedSampleDataSourceMetrics metrics() const noexcept;
    const std::string& lastFailure() const noexcept { return failureState; }

private:
    struct PageStorage;
    bool prepareRange(bool head, std::uint64_t pageIndex);
    bool sourceUnchanged() const;
    bool makeRoomForPage(std::uint64_t requiredBytes);
    void reclaimRetiredPages();

    SampleDataSourceDescriptor sourceDescriptor;
    std::string sourcePath;
    std::uint16_t formatTag = 0;
    std::uint16_t bitsPerSample = 0;
    std::uint16_t blockAlign = 0;
    std::function<bool(std::uint64_t, std::uint32_t,
                       std::vector<std::vector<float>>&, std::string&)> rangeDecoder;
    std::uint64_t expectedFileSize = 0;
    std::int64_t expectedModifiedTicks = 0;
    std::uint64_t configuredHeadFrames = 0;
    std::uint64_t configuredPageFrames = 0;
    std::uint64_t configuredPageCount = 0;
    std::atomic<PageStorage*> headSlot {nullptr};
    std::unique_ptr<std::atomic<PageStorage*>[]> pageSlots;
    std::vector<std::unique_ptr<PageStorage>> ownedPages;
    std::vector<std::unique_ptr<PageStorage>> retiredPages;
    std::string failureState;
    std::atomic<std::uint64_t> rangeReads {0};
    std::atomic<std::uint64_t> readBytes {0};
    std::atomic<std::uint64_t> convertedFrames {0};
    std::atomic<std::uint64_t> headResidentBytes {0};
    std::atomic<std::uint64_t> pageResidentBytes {0};
    std::atomic<std::uint64_t> duplicateRequests {0};
    std::atomic<std::uint64_t> mutationFailures {0};
    std::uint64_t maximumPageCacheBytes = 0;
    std::atomic<std::uint64_t> allocatedPagesBytes {0};
    std::atomic<std::uint64_t> peakAllocatedPagesBytes {0};
    std::atomic<std::uint64_t> evictions {0};
    std::atomic<std::uint64_t> pinnedEvictionSkips {0};
    std::atomic<std::uint64_t> cachePressureFailures {0};
    std::atomic<std::uint64_t> retiredPagesCount {0};
    std::atomic<std::uint64_t> retiredPagesBytes {0};
    mutable std::atomic<std::uint64_t> headHits {0};
    mutable std::atomic<std::uint64_t> pageHits {0};
    mutable std::atomic<std::uint64_t> pageMisses {0};
    mutable std::atomic<std::uint64_t> leasedPagesBytes {0};
    std::atomic<std::uint64_t> totalReadMicros {0};
    std::atomic<std::uint64_t> maximumReadMicros {0};
    mutable std::atomic<std::uint32_t> activeAcquisitions {0};
    mutable std::atomic<std::uint64_t> accessOrdinal {1};
    mutable SamplePageIntentRing intentRing;
};

struct PackagePagedSampleDataSourceMetrics
{
    std::uint64_t recordOpenCount = 0;
    std::uint64_t sealedBytesRead = 0;
    std::uint64_t publishedHeadBytes = 0;
    std::uint64_t publishedPageBytes = 0;
    std::uint64_t authenticationFailureCount = 0;
    std::uint64_t checksumFailureCount = 0;
    std::uint64_t cancellationCount = 0;
};

class PackagePagedSampleDataSource final : public ISampleDataSource
{
public:
    PackagePagedSampleDataSource(
        SampleDataSourceDescriptor descriptor,
        std::shared_ptr<const PackageV2OpenResult> package,
        const PackageCryptoProvider& crypto = getDeterministicPackageCryptoProvider());
    PackagePagedSampleDataSource(
        SampleDataSourceDescriptor descriptor,
        std::shared_ptr<const PackageV3FileOpenResult> package,
        std::shared_ptr<const SecureBuffer> contentKey);
    ~PackagePagedSampleDataSource() override;
    const SampleDataSourceDescriptor& descriptor() const noexcept override { return sourceDescriptor; }
    SampleFrameView acquireFrameView(std::uint64_t firstFrame,
                                     std::uint32_t requestedFrames) const noexcept override;
    bool publishPageIntent(std::uint64_t firstFrame,
                           SamplePageRequestPriority priority,
                           std::uint64_t voiceId) const noexcept override;
    std::size_t drainPageIntents(SamplePageRequestScheduler& scheduler,
                                 std::size_t maximumCount = SamplePageIntentRing::capacity);
    bool prepareHead(const std::function<bool()>& cancellationProbe = {});
    bool preparePage(std::uint64_t pageIndex,
                     const std::function<bool()>& cancellationProbe = {});
    std::uint64_t headFrameCount() const noexcept { return configuredHeadFrames; }
    std::uint64_t pageFrameCount() const noexcept { return configuredPageFrames; }
    std::uint64_t pageCount() const noexcept { return configuredPageCount; }
    PackagePagedSampleDataSourceMetrics metrics() const noexcept;
    const std::string& lastFailure() const noexcept { return failureState; }

private:
    struct PageStorage;
    bool prepareRecord(bool head, std::uint64_t pageIndex,
                       const std::function<bool()>& cancellationProbe);

    SampleDataSourceDescriptor sourceDescriptor;
    std::shared_ptr<const PackageV2OpenResult> retainedPackage;
    std::shared_ptr<const PackageV3FileOpenResult> retainedV3Package;
    std::shared_ptr<const SecureBuffer> retainedV3ContentKey;
    const PackageCryptoProvider* cryptoProvider = nullptr;
    std::uint64_t configuredHeadFrames = 0;
    std::uint64_t configuredPageFrames = 0;
    std::uint64_t configuredPageCount = 0;
    std::atomic<PageStorage*> headSlot {nullptr};
    std::unique_ptr<std::atomic<PageStorage*>[]> pageSlots;
    std::vector<std::unique_ptr<PageStorage>> ownedPages;
    std::string failureState;
    mutable SamplePageIntentRing intentRing;
    std::atomic<std::uint64_t> openedRecords {0};
    std::atomic<std::uint64_t> sealedReadBytes {0};
    std::atomic<std::uint64_t> publishedHeadBytes {0};
    std::atomic<std::uint64_t> publishedPageBytes {0};
    std::atomic<std::uint64_t> authenticationFailures {0};
    std::atomic<std::uint64_t> checksumFailures {0};
    std::atomic<std::uint64_t> cancellations {0};
};
} // namespace drs::engine
