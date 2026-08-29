#pragma once

#include "drs/engine/PackageWriter.h"
#include "drs/engine/PackageKeys.h"
#include "drs/engine/PackagePublisherSigning.h"
#include "drs/engine/PackagePublisherTrustStore.h"
#include "drs/engine/RuntimeCompiler.h"
#include "drs/engine/RuntimeModel.h"
#include "drs/engine/RuntimePresetState.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

namespace drs::app
{
enum class PerformancePackageExportStage
{
    idle,
    queued,
    validating,
    compiling,
    writingStream,
    sealingPackage,
    verifying,
    completed,
    canceled,
    failed,
    consumed
};

bool isPerformancePackageExportStageTransitionAllowed(PerformancePackageExportStage from,
                                                      PerformancePackageExportStage to) noexcept;
const char* toString(PerformancePackageExportStage stage) noexcept;

struct PerformancePackageExportRequestIdentity
{
    std::uint64_t ownerId = 0;
    std::uint64_t generation = 0;
    std::string projectId;
    std::size_t baseRevision = 0;
    std::string packagePath;
};

inline constexpr const char* performancePackageV3CompatibilityId
    = "practical-sampler.performance-package.v3";

struct PerformancePackageExportSecurityContext
{
    std::string compatibilityId = performancePackageV3CompatibilityId;
    std::string encryptionKeyId;
    std::string signingKeyId;
    // The field and interface names remain compatible with the original V3
    // contract. In the portable offline profile these objects recognize and
    // integrity-check a package; they do not establish author identity.
    std::shared_ptr<const drs::engine::PackageKeyProvider> keyProvider;
    std::shared_ptr<const drs::engine::PackagePublisherSigningClient> publisherSigner;
    std::shared_ptr<const drs::engine::PackagePublisherTrustStore> trustStore;

    bool valid() const noexcept
    {
        return ! compatibilityId.empty() && ! encryptionKeyId.empty()
            && ! signingKeyId.empty() && keyProvider != nullptr
            && publisherSigner != nullptr && trustStore != nullptr
            && trustStore->valid();
    }
};

struct PerformancePackageExportRequest
{
    drs::engine::RuntimeProjectModel project;
    drs::engine::RuntimeSessionStateSnapshot sessionState;
    std::string projectId;
    std::size_t baseRevision = 0;
    std::string packagePath;
    std::shared_ptr<const PerformancePackageExportSecurityContext> securityContext;
};

struct PerformancePackageExportOperationResult
{
    bool exported = false;
    bool canceled = false;
    std::string state;
    std::vector<std::string> issues;
    std::string packagePath;
    std::uint64_t packageBytes = 0;
    std::uint32_t payloadCount = 0;
    std::uint64_t peakPlaintextBufferBytes = 0;
    std::uint64_t peakSealedBufferBytes = 0;
    std::uint64_t verificationBytesRead = 0;
    std::uint64_t totalDurationMicros = 0;
    double plaintextThroughputBytesPerSecond = 0.0;
    std::vector<std::uint8_t> semanticDigest;
    std::string signingAuditId;
};

struct PerformancePackageExportProgress
{
    PerformancePackageExportStage stage = PerformancePackageExportStage::idle;
    double progress01 = 0.0;
    std::string status;
    std::string detail;
    std::uint64_t bytesProcessed = 0;
    std::uint64_t totalBytes = 0;
    std::string itemId;
};

struct PerformancePackageExportSnapshot
{
    PerformancePackageExportRequestIdentity identity;
    PerformancePackageExportStage stage = PerformancePackageExportStage::idle;
    double progress01 = 0.0;
    std::string status;
    std::string detail;
    std::shared_ptr<const PerformancePackageExportOperationResult> result;
};

enum class PerformancePackageExportSubmitDisposition
{
    accepted,
    busy,
    shuttingDown,
    invalid
};

struct PerformancePackageExportSubmitResult
{
    PerformancePackageExportSubmitDisposition disposition
        = PerformancePackageExportSubmitDisposition::invalid;
    PerformancePackageExportRequestIdentity identity;

    bool wasAccepted() const noexcept
    {
        return disposition == PerformancePackageExportSubmitDisposition::accepted;
    }
};

struct PerformancePackageExportServiceMetrics
{
    std::size_t requestedCount = 0;
    std::size_t completedCount = 0;
    std::size_t canceledCount = 0;
    std::size_t failedCount = 0;
    std::size_t rejectedBusyCount = 0;
    std::size_t maximumPendingCount = 0;
    std::size_t maximumInFlightCount = 0;
    std::size_t liveWorkerCount = 0;
    std::uint64_t maximumShutdownWaitMicros = 0;
    std::uint64_t shutdownWaitMilliseconds = 0;
    std::chrono::nanoseconds shutdownWaitDuration { 0 };
};

struct PerformancePackageExportExecutionOptions
{
    std::function<void(const PerformancePackageExportProgress&)> progressSink;
    std::function<bool()> cancellationProbe;
};

struct PerformancePackageExportServiceOptions
{
    std::function<void(PerformancePackageExportStage)> stageObserver;
    std::function<void(PerformancePackageExportStage)> checkpointObserver;
    std::shared_ptr<const PerformancePackageExportSecurityContext> securityContext;
};

PerformancePackageExportOperationResult executePerformancePackageExport(
    const PerformancePackageExportRequest& request,
    const PerformancePackageExportExecutionOptions& options = {});

class PerformancePackageExportProgressComponent final : public juce::Component
{
public:
    using CancelCallback = std::function<void()>;
    explicit PerformancePackageExportProgressComponent(CancelCallback callback = {});
    void update(const PerformancePackageExportSnapshot& snapshot);
    void resized() override;
    void setCancelCallback(CancelCallback callback);

private:
    juce::Label statusLabel;
    juce::Label detailLabel;
    double progressValue = 0.0;
    juce::ProgressBar progressBar { progressValue };
    juce::TextButton cancelButton { "Cancel" };
    CancelCallback cancelCallback;
};

class PerformancePackageExportService
{
public:
    class Client
    {
    public:
        Client() = default;
        ~Client();

        Client(const Client&) = delete;
        Client& operator=(const Client&) = delete;
        Client(Client&& other) noexcept;
        Client& operator=(Client&& other) noexcept;

        PerformancePackageExportSubmitResult submit(PerformancePackageExportRequest request);
        bool cancel(std::string reason = "Playable package export canceled");
        bool waitForTerminal(std::chrono::milliseconds timeout) const;
        bool waitForTerminal() const;
        std::shared_ptr<const PerformancePackageExportSnapshot> getSnapshot() const;
        bool consume();
        std::uint64_t ownerId() const noexcept { return owner; }
        std::uint64_t generation() const noexcept { return activeGeneration; }

    private:
        friend class PerformancePackageExportService;
        Client(PerformancePackageExportService* service, std::uint64_t ownerId) noexcept;
        void reset() noexcept;

        PerformancePackageExportService* service = nullptr;
        std::uint64_t owner = 0;
        std::uint64_t activeGeneration = 0;
    };

    explicit PerformancePackageExportService(PerformancePackageExportServiceOptions options = {});
    ~PerformancePackageExportService();

    PerformancePackageExportService(const PerformancePackageExportService&) = delete;
    PerformancePackageExportService& operator=(const PerformancePackageExportService&) = delete;

    Client openClient();
    PerformancePackageExportServiceMetrics getMetrics() const;
    bool setSecurityContext(
        std::shared_ptr<const PerformancePackageExportSecurityContext> securityContext);
    std::shared_ptr<const PerformancePackageExportSecurityContext> getSecurityContext() const;
    std::shared_ptr<const PerformancePackageExportSnapshot> getSnapshot() const;
    std::shared_ptr<const PerformancePackageExportSnapshot> getSnapshot(std::uint64_t ownerId,
                                                                        std::uint64_t generation) const;
    bool cancel(std::uint64_t ownerId, std::uint64_t generation, std::string reason);
    bool waitForTerminal(std::uint64_t ownerId,
                         std::uint64_t generation,
                         std::chrono::milliseconds timeout) const;
    bool consume(std::uint64_t ownerId, std::uint64_t generation);
    void shutdown() noexcept;

private:
    struct PendingRequest
    {
        PerformancePackageExportRequestIdentity identity;
        PerformancePackageExportRequest request;
        std::shared_ptr<std::atomic<bool>> cancellation;
        std::string cancellationReason;
    };

    void runWorker();
    void process(PendingRequest pending);
    PerformancePackageExportSubmitResult submit(std::uint64_t ownerId,
                                                PerformancePackageExportRequest request);
    bool isTerminal(PerformancePackageExportStage stage) const noexcept;
    std::string cancellationReason(const PerformancePackageExportRequestIdentity& identity) const;
    void publish(PerformancePackageExportRequestIdentity identity,
                 PerformancePackageExportStage stage,
                 double progress01,
                 std::string status,
                 std::string detail,
                 std::shared_ptr<const PerformancePackageExportOperationResult> result = {});

    PerformancePackageExportServiceOptions options;
    std::atomic<std::uint64_t> nextOwnerId { 1 };
    std::uint64_t nextGeneration = 0;
    mutable std::mutex mutex;
    mutable std::condition_variable condition;
    mutable std::condition_variable terminalCondition;
    std::optional<PendingRequest> pending;
    std::optional<PendingRequest> active;
    std::shared_ptr<const PerformancePackageExportSnapshot> snapshot;
    PerformancePackageExportServiceMetrics metrics;
    bool shutdownRequested = false;
    mutable std::mutex shutdownMutex;
    std::thread worker;
};
} // namespace drs::app
