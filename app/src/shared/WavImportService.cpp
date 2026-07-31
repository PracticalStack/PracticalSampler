#include "shared/WavImportService.h"

#include <algorithm>
#include <exception>
#include <filesystem>
#include <fstream>
#include <set>
#include <stdexcept>
#include <utility>
#include <vector>

namespace drs::app
{
namespace
{
namespace fs = std::filesystem;
using Clock = std::chrono::steady_clock;

bool isActiveStage(const WavImportBatchStage stage) noexcept
{
    return stage == WavImportBatchStage::queued
        || stage == WavImportBatchStage::staging
        || stage == WavImportBatchStage::inspecting;
}

bool isTerminalStage(const WavImportBatchStage stage) noexcept
{
    return stage == WavImportBatchStage::completed
        || stage == WavImportBatchStage::canceled
        || stage == WavImportBatchStage::superseded
        || stage == WavImportBatchStage::failed
        || stage == WavImportBatchStage::consumed;
}

bool isItemTerminalStage(const WavImportItemStage stage) noexcept
{
    return stage == WavImportItemStage::ready
        || stage == WavImportItemStage::failed
        || stage == WavImportItemStage::canceled
        || stage == WavImportItemStage::skipped;
}

const WavImportItemProgress* findCurrentItem(const WavImportBatchSnapshot& snapshot) noexcept
{
    for (const auto& item : snapshot.items)
    {
        if (!isItemTerminalStage(item.stage))
            return &item;
    }

    for (auto iterator = snapshot.items.rbegin(); iterator != snapshot.items.rend(); ++iterator)
    {
        if (iterator->stage == WavImportItemStage::ready
            || iterator->stage == WavImportItemStage::failed
            || iterator->stage == WavImportItemStage::canceled)
        {
            return &*iterator;
        }
    }

    return nullptr;
}

std::size_t currentItemOrdinal(const WavImportBatchSnapshot& snapshot,
                               const WavImportItemProgress* currentItem) noexcept
{
    if (currentItem == nullptr)
        return 0;

    const auto iterator = std::find_if(snapshot.items.begin(),
                                       snapshot.items.end(),
                                       [&](const WavImportItemProgress& item)
                                       {
                                           return std::addressof(item) == currentItem;
                                       });
    if (iterator == snapshot.items.end())
        return 0;

    return static_cast<std::size_t>(std::distance(snapshot.items.begin(), iterator)) + 1;
}

juce::String describeItemStage(const WavImportItemStage stage)
{
    switch (stage)
    {
        case WavImportItemStage::pending: return "Queued";
        case WavImportItemStage::staging: return "Staging";
        case WavImportItemStage::fingerprinting: return "Fingerprinting";
        case WavImportItemStage::inspecting: return "Inspecting";
        case WavImportItemStage::ready: return "Ready";
        case WavImportItemStage::failed: return "Failed";
        case WavImportItemStage::canceled: return "Canceled";
        case WavImportItemStage::skipped: return "Skipped";
    }

    return "Unknown";
}

juce::String describeBatchState(const WavImportBatchSnapshot& snapshot)
{
    if (snapshot.stage == WavImportBatchStage::completed)
    {
        switch (snapshot.terminalDisposition)
        {
            case WavImportTerminalDisposition::completed: return "WAV import complete";
            case WavImportTerminalDisposition::partiallyCompleted: return "WAV import partially complete";
            case WavImportTerminalDisposition::canceled: return "WAV import canceled";
            case WavImportTerminalDisposition::failed: return "WAV import failed";
            case WavImportTerminalDisposition::superseded: return "WAV import superseded";
            case WavImportTerminalDisposition::consumed: return "WAV import consumed";
            case WavImportTerminalDisposition::none: break;
        }
    }

    switch (snapshot.stage)
    {
        case WavImportBatchStage::queued: return "WAV import queued";
        case WavImportBatchStage::staging: return "Staging WAV files";
        case WavImportBatchStage::inspecting: return "Inspecting WAV files";
        case WavImportBatchStage::canceled: return "WAV import canceled";
        case WavImportBatchStage::superseded: return "WAV import superseded";
        case WavImportBatchStage::failed: return "WAV import failed";
        case WavImportBatchStage::consumed: return "WAV import consumed";
        case WavImportBatchStage::completed: return "WAV import complete";
        case WavImportBatchStage::idle: break;
    }

    return "WAV import idle";
}

juce::String buildProgressStatusText(const WavImportBatchSnapshot& snapshot)
{
    juce::String status = describeBatchState(snapshot);
    const auto* currentItem = findCurrentItem(snapshot);
    if (currentItem == nullptr || !isActiveStage(snapshot.stage))
        return status;

    const auto currentItemName = juce::File(currentItem->sourcePath).getFileName();
    if (currentItemName.isEmpty())
        return status;

    return status + ": " + currentItemName + " (" + describeItemStage(currentItem->stage) + ")";
}

juce::String buildProgressDetailText(const WavImportBatchSnapshot& snapshot)
{
    juce::String detail("Items: "
                        + juce::String(static_cast<int>(snapshot.completedItemCount))
                        + "/"
                        + juce::String(static_cast<int>(snapshot.totalItemCount))
                        + " complete");
    detail += "  Ready: " + juce::String(static_cast<int>(snapshot.successfulItemCount));
    detail += "  Failed: " + juce::String(static_cast<int>(snapshot.failedItemCount));
    detail += "  Warnings: " + juce::String(static_cast<int>(snapshot.warningItemCount));
    detail += "  Bytes: "
        + juce::File::descriptionOfSizeInBytes(static_cast<int64_t>(snapshot.totalBytesProcessed))
        + " / "
        + juce::File::descriptionOfSizeInBytes(static_cast<int64_t>(snapshot.totalBytesExpected));

    const auto* currentItem = findCurrentItem(snapshot);
    if (currentItem == nullptr)
        return detail;

    const auto ordinal = currentItemOrdinal(snapshot, currentItem);
    if (ordinal == 0)
        return detail;

    return "Current: " + juce::String(static_cast<int>(ordinal))
        + "/"
        + juce::String(static_cast<int>(snapshot.totalItemCount))
        + " (" + describeItemStage(currentItem->stage) + ")"
        + "  " + detail;
}

double calculateProgressValue(const WavImportBatchSnapshot& snapshot) noexcept
{
    if (snapshot.totalBytesExpected > 0)
    {
        return juce::jlimit(0.0,
                            1.0,
                            static_cast<double>(snapshot.totalBytesProcessed)
                                / static_cast<double>(snapshot.totalBytesExpected));
    }

    if (snapshot.totalItemCount > 0)
    {
        return juce::jlimit(0.0,
                            1.0,
                            static_cast<double>(snapshot.completedItemCount)
                                / static_cast<double>(snapshot.totalItemCount));
    }

    return snapshot.stage == WavImportBatchStage::queued ? 0.0 : -1.0;
}

bool isSupersededCancellationReason(const std::string& reason)
{
    return reason.find("supersed") != std::string::npos || reason.find("Supersed") != std::string::npos;
}

std::string makeItemId(const std::uint64_t generation, const std::size_t index)
{
    return "wav-import-" + std::to_string(generation) + "-" + std::to_string(index + 1);
}

std::size_t countWarnings(const std::vector<drs::engine::AuthoringImportFinding>& findings)
{
    return static_cast<std::size_t>(std::count_if(findings.begin(),
                                                  findings.end(),
                                                  [](const auto& finding)
                                                  {
                                                      return finding.severity
                                                          == drs::engine::AuthoringImportFindingSeverity::warning;
                                                  }));
}

struct WorkItem
{
    std::string itemId;
    std::string sourcePath;
    std::string stagedPath;
    std::string finalPath;
    WavImportItemStage stage = WavImportItemStage::pending;
    std::uint64_t totalBytes = 0;
    std::uint64_t copiedBytes = 0;
    std::uint64_t fingerprintBytesProcessed = 0;
    std::uint64_t fingerprintTotalBytes = 0;
    std::uint64_t copyDurationMicros = 0;
    std::uint64_t fingerprintDurationMicros = 0;
    std::uint64_t inspectionDurationMicros = 0;
    std::uint64_t totalDurationMicros = 0;
    std::size_t warningCount = 0;
    std::size_t issueCount = 0;
    std::string status = "Queued";
    drs::engine::SampleSourceFingerprintResult fingerprint;
    drs::engine::SampleInspectionResult inspection;
    std::vector<drs::engine::SampleFilenameToken> filenameTokens;
    std::vector<drs::engine::AuthoringImportFinding> findings;
    drs::engine::AuthoringImportZoneSuggestion suggestedZone;
};

fs::path samplesDirectoryFor(const WavImportRequestIdentity& identity)
{
    return fs::path(identity.contentRootPath) / "Samples";
}

fs::path stageDirectoryFor(const WavImportRequestIdentity& identity)
{
    return samplesDirectoryFor(identity) / ".staging"
        / ("wav-import-" + std::to_string(identity.ownerId) + "-" + std::to_string(identity.generation));
}

void removePathIfExists(const fs::path& path)
{
    if (path.empty())
        return;

    std::error_code error;
    if (!fs::exists(path, error))
        return;

    if (fs::is_directory(path, error))
        fs::remove_all(path, error);
    else
        fs::remove(path, error);
}

void removeStageDirectoryIfEmpty(const WavImportRequestIdentity& identity)
{
    const auto stageDirectory = stageDirectoryFor(identity);
    std::error_code error;
    if (!fs::exists(stageDirectory, error) || !fs::is_directory(stageDirectory, error))
        return;
    if (fs::is_empty(stageDirectory, error))
        fs::remove(stageDirectory, error);
}

void cleanupRequestArtifacts(const WavImportRequestIdentity& identity)
{
    if (identity.contentRootPath.empty() || identity.generation == 0)
        return;
    removePathIfExists(stageDirectoryFor(identity));
}

void cleanupDiscardedArtifacts(const WavImportRequestIdentity& identity,
                               const std::vector<WorkItem>& workItems)
{
    if (identity.contentRootPath.empty() || identity.generation == 0)
        return;

    for (const auto& workItem : workItems)
    {
        if (workItem.stage == WavImportItemStage::ready)
            continue;

        removePathIfExists(workItem.stagedPath);
        removePathIfExists(workItem.finalPath);
    }

    removeStageDirectoryIfEmpty(identity);
}

std::uint64_t fileSizeOrThrow(const fs::path& path)
{
    std::error_code error;
    const auto size = fs::file_size(path, error);
    if (error)
        throw std::runtime_error("Could not size WAV import source file: " + path.generic_string());
    return size;
}

fs::path chooseUniquePath(const fs::path& directory,
                          const fs::path& originalFilename,
                          std::set<std::string>& reservedPaths)
{
    const auto stem = originalFilename.stem().generic_string();
    const auto extension = originalFilename.extension().generic_string();
    auto candidate = directory / (stem + extension);

    for (std::size_t suffix = 2;
         fs::exists(candidate) || reservedPaths.count(candidate.generic_string()) > 0;
         ++suffix)
    {
        candidate = directory / (stem + "-" + std::to_string(suffix) + extension);
    }

    reservedPaths.insert(candidate.generic_string());
    return candidate;
}

std::vector<WorkItem> buildWorkItems(const WavImportRequestIdentity& identity,
                                     const WavImportRequest& request)
{
    const auto samplesDirectory = samplesDirectoryFor(identity);
    const auto stageDirectory = stageDirectoryFor(identity);
    std::set<std::string> reservedFinalPaths;
    std::set<std::string> reservedStagePaths;
    std::vector<WorkItem> items;
    items.reserve(request.sourcePaths.size());

    for (std::size_t index = 0; index < request.sourcePaths.size(); ++index)
    {
        const fs::path sourcePath(request.sourcePaths[index]);
        if (!fs::exists(sourcePath) || !fs::is_regular_file(sourcePath))
            throw std::runtime_error("Missing WAV import source file: " + sourcePath.generic_string());
        if (sourcePath.filename().empty())
            throw std::runtime_error("WAV import source file must have a filename: " + sourcePath.generic_string());

        WorkItem item;
        item.itemId = makeItemId(identity.generation, index);
        item.sourcePath = sourcePath.generic_string();
        item.stagedPath = chooseUniquePath(stageDirectory, sourcePath.filename(), reservedStagePaths).generic_string();
        item.finalPath = chooseUniquePath(samplesDirectory, sourcePath.filename(), reservedFinalPaths).generic_string();
        item.totalBytes = fileSizeOrThrow(sourcePath);
        item.fingerprintTotalBytes = item.totalBytes;
        items.push_back(std::move(item));
    }

    return items;
}

std::vector<WavImportItemProgress> buildQueuedItems(const WavImportRequestIdentity& identity,
                                                    const WavImportRequest& request)
{
    std::vector<WavImportItemProgress> items;
    items.reserve(request.sourcePaths.size());
    for (std::size_t index = 0; index < request.sourcePaths.size(); ++index)
    {
        WavImportItemProgress item;
        item.itemId = makeItemId(identity.generation, index);
        item.sourcePath = request.sourcePaths[index];
        item.stage = WavImportItemStage::pending;
        item.status = "Queued";
        items.push_back(std::move(item));
    }
    return items;
}

std::vector<WavImportItemProgress> buildProgressItems(const std::vector<WorkItem>& workItems)
{
    std::vector<WavImportItemProgress> items;
    items.reserve(workItems.size());
    for (const auto& workItem : workItems)
    {
        WavImportItemProgress item;
        item.itemId = workItem.itemId;
        item.sourcePath = workItem.sourcePath;
        item.stagedPath = workItem.stagedPath;
        item.stage = workItem.stage;
        item.bytesProcessed = workItem.copiedBytes;
        item.totalBytes = workItem.totalBytes;
        item.fingerprintBytesProcessed = workItem.fingerprintBytesProcessed;
        item.fingerprintTotalBytes = workItem.fingerprintTotalBytes;
        item.copyDurationMicros = workItem.copyDurationMicros;
        item.fingerprintDurationMicros = workItem.fingerprintDurationMicros;
        item.inspectionDurationMicros = workItem.inspectionDurationMicros;
        item.totalDurationMicros = workItem.totalDurationMicros;
        item.warningCount = workItem.warningCount;
        item.issueCount = workItem.issueCount;
        item.status = workItem.status;
        items.push_back(std::move(item));
    }
    return items;
}

std::shared_ptr<WavImportCompletionPayload> buildCompletionPayload(
    const WavImportRequestIdentity& identity,
    const std::vector<WorkItem>& workItems)
{
    auto payload = std::make_shared<WavImportCompletionPayload>();
    payload->identity = identity;
    payload->items.reserve(workItems.size());

    for (const auto& workItem : workItems)
    {
        WavImportCompletionItem item;
        item.itemId = workItem.itemId;
        item.sourcePath = workItem.sourcePath;
        item.stagedPath = workItem.stagedPath;
        item.finalPath = workItem.finalPath;
        item.stage = workItem.stage;
        item.sourceBytes = workItem.totalBytes;
        item.copiedBytes = workItem.copiedBytes;
        item.copyDurationMicros = workItem.copyDurationMicros;
        item.fingerprintDurationMicros = workItem.fingerprintDurationMicros;
        item.inspectionDurationMicros = workItem.inspectionDurationMicros;
        item.totalDurationMicros = workItem.totalDurationMicros;
        item.fingerprint = workItem.fingerprint;
        item.inspection = workItem.inspection;
        item.filenameTokens = workItem.filenameTokens;
        item.findings = workItem.findings;
        item.suggestedZone = workItem.suggestedZone;
        payload->items.push_back(std::move(item));

        ++payload->totalItemCount;
        if (workItem.stage == WavImportItemStage::ready)
            ++payload->successfulItemCount;
        else if (workItem.stage == WavImportItemStage::failed)
            ++payload->failedItemCount;
        else if (workItem.stage == WavImportItemStage::canceled)
            ++payload->canceledItemCount;
        if (workItem.warningCount > 0)
            ++payload->warningItemCount;
        payload->totalBytesProcessed += workItem.copiedBytes;
        payload->totalBytesExpected += workItem.totalBytes;
        payload->copyDurationMicros += workItem.copyDurationMicros;
        payload->fingerprintDurationMicros += workItem.fingerprintDurationMicros;
        payload->inspectionDurationMicros += workItem.inspectionDurationMicros;
        payload->totalDurationMicros += workItem.totalDurationMicros;
    }

    if (payload->successfulItemCount == payload->totalItemCount && payload->failedItemCount == 0
        && payload->canceledItemCount == 0)
    {
        payload->disposition = WavImportTerminalDisposition::completed;
        payload->status = payload->warningItemCount > 0
            ? "WAV import completed with findings"
            : "WAV import completed";
    }
    else if (payload->successfulItemCount > 0)
    {
        payload->disposition = WavImportTerminalDisposition::partiallyCompleted;
        payload->status = "WAV import completed with some failures";
    }
    else if (payload->canceledItemCount == payload->totalItemCount && payload->totalItemCount > 0)
    {
        payload->disposition = WavImportTerminalDisposition::canceled;
        payload->status = "WAV import canceled";
    }
    else
    {
        payload->disposition = WavImportTerminalDisposition::failed;
        payload->status = "WAV import failed";
    }

    return payload;
}

void copyFileChunked(WorkItem& workItem,
                     const std::size_t chunkBytes,
                     const std::shared_ptr<std::atomic<bool>>& cancellation,
                     const std::function<void()>& progressCallback)
{
    std::ifstream input(workItem.sourcePath, std::ios::binary);
    if (!input)
        throw std::runtime_error("Could not open WAV import source file: " + workItem.sourcePath);

    std::ofstream output(workItem.stagedPath, std::ios::binary | std::ios::trunc);
    if (!output)
        throw std::runtime_error("Could not create WAV import staging file: " + workItem.stagedPath);

    std::vector<char> buffer(std::max<std::size_t>(chunkBytes, 1));
    while (!cancellation->load(std::memory_order_acquire))
    {
        input.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
        const auto bytesRead = input.gcount();
        if (bytesRead <= 0)
            break;

        output.write(buffer.data(), bytesRead);
        if (!output)
            throw std::runtime_error("Could not write WAV import staging file: " + workItem.stagedPath);

        workItem.copiedBytes += static_cast<std::uint64_t>(bytesRead);
        progressCallback();
    }

    if (input.bad())
        throw std::runtime_error("Could not read WAV import source file: " + workItem.sourcePath);
    if (workItem.copiedBytes != workItem.totalBytes && !cancellation->load(std::memory_order_acquire))
        throw std::runtime_error("WAV import staging copy ended before the source file was fully copied: "
                                 + workItem.sourcePath);
}

void addInspectionWarningFindings(WorkItem& workItem)
{
    for (const auto& warning : workItem.inspection.warnings)
    {
        drs::engine::AuthoringImportFinding finding;
        finding.severity = drs::engine::AuthoringImportFindingSeverity::warning;
        finding.code = "import.policy_warning";
        finding.summary = "Source sample triggered a portability warning";
        finding.detail = warning;
        workItem.findings.push_back(std::move(finding));
    }
}

void summarizeItems(const std::vector<WavImportItemProgress>& items,
                    std::size_t& completedItemCount,
                    std::size_t& successfulItemCount,
                    std::size_t& failedItemCount,
                    std::size_t& canceledItemCount,
                    std::size_t& warningItemCount)
{
    completedItemCount = 0;
    successfulItemCount = 0;
    failedItemCount = 0;
    canceledItemCount = 0;
    warningItemCount = 0;

    for (const auto& item : items)
    {
        if (isItemTerminalStage(item.stage))
            ++completedItemCount;
        if (item.stage == WavImportItemStage::ready)
            ++successfulItemCount;
        else if (item.stage == WavImportItemStage::failed)
            ++failedItemCount;
        else if (item.stage == WavImportItemStage::canceled)
            ++canceledItemCount;
        if (item.warningCount > 0)
            ++warningItemCount;
    }
}
} // namespace

WavImportProgressComponent::WavImportProgressComponent(CancelCallback callback)
    : cancelCallback(std::move(callback))
{
    setComponentID("wavImportProgress");
    statusLabel.setComponentID("wavImportProgressLabel");
    statusLabel.setText("WAV import queued", juce::dontSendNotification);
    addAndMakeVisible(statusLabel);

    detailLabel.setComponentID("wavImportProgressDetailLabel");
    detailLabel.setText("Items: 0/0 complete", juce::dontSendNotification);
    addAndMakeVisible(detailLabel);

    progressBar.setComponentID("wavImportProgressBar");
    addAndMakeVisible(progressBar);

    cancelButton.setComponentID("wavImportProgressCancelButton");
    cancelButton.onClick = [this]
    {
        if (cancelCallback)
            cancelCallback();
    };
    addAndMakeVisible(cancelButton);
}

void WavImportProgressComponent::setCancelCallback(CancelCallback callback)
{
    cancelCallback = std::move(callback);
}

void WavImportProgressComponent::update(const WavImportBatchSnapshot& snapshot)
{
    progressValue = calculateProgressValue(snapshot);
    statusLabel.setText(buildProgressStatusText(snapshot), juce::dontSendNotification);
    detailLabel.setText(buildProgressDetailText(snapshot), juce::dontSendNotification);
    cancelButton.setEnabled(isActiveStage(snapshot.stage));
    setVisible(snapshot.stage != WavImportBatchStage::idle
               && snapshot.stage != WavImportBatchStage::consumed);
}

void WavImportProgressComponent::resized()
{
    auto area = getLocalBounds().reduced(8);
    auto topRow = area.removeFromTop(22);
    cancelButton.setBounds(topRow.removeFromRight(80));
    topRow.removeFromRight(8);
    statusLabel.setBounds(topRow);
    detailLabel.setBounds(area.removeFromTop(22));
    progressBar.setBounds(area.removeFromTop(18).reduced(0, 4));
}

bool isWavImportBatchStageTransitionAllowed(const WavImportBatchStage from,
                                            const WavImportBatchStage to) noexcept
{
    using Stage = WavImportBatchStage;
    switch (from)
    {
        case Stage::idle: return to == Stage::queued;
        case Stage::queued: return to == Stage::staging || to == Stage::canceled || to == Stage::failed;
        case Stage::staging: return to == Stage::inspecting || to == Stage::canceled
                || to == Stage::superseded || to == Stage::failed;
        case Stage::inspecting: return to == Stage::completed || to == Stage::canceled
                || to == Stage::superseded || to == Stage::failed;
        case Stage::completed: return to == Stage::consumed || to == Stage::queued;
        case Stage::canceled:
        case Stage::superseded:
        case Stage::failed:
        case Stage::consumed: return to == Stage::idle || to == Stage::queued;
    }
    return false;
}

const char* toString(const WavImportBatchStage stage) noexcept
{
    using Stage = WavImportBatchStage;
    switch (stage)
    {
        case Stage::idle: return "idle";
        case Stage::queued: return "queued";
        case Stage::staging: return "staging";
        case Stage::inspecting: return "inspecting";
        case Stage::completed: return "completed";
        case Stage::canceled: return "canceled";
        case Stage::superseded: return "superseded";
        case Stage::failed: return "failed";
        case Stage::consumed: return "consumed";
    }
    return "failed";
}

WavImportService::Client::Client(WavImportService* serviceIn, const std::uint64_t ownerId) noexcept
    : service(serviceIn), owner(ownerId)
{
}

WavImportService::Client::~Client()
{
    reset();
}

WavImportService::Client::Client(Client&& other) noexcept
    : service(other.service), owner(other.owner), activeGeneration(other.activeGeneration)
{
    other.service = nullptr;
    other.owner = 0;
    other.activeGeneration = 0;
}

WavImportService::Client& WavImportService::Client::operator=(Client&& other) noexcept
{
    if (this != &other)
    {
        reset();
        service = other.service;
        owner = other.owner;
        activeGeneration = other.activeGeneration;
        other.service = nullptr;
        other.owner = 0;
        other.activeGeneration = 0;
    }
    return *this;
}

void WavImportService::Client::reset() noexcept
{
    if (service != nullptr && activeGeneration != 0)
    {
        service->cancel(owner, activeGeneration, "WAV import owner closed");
        service->waitForTerminal(owner, activeGeneration, std::chrono::milliseconds::max());
    }
    service = nullptr;
    owner = 0;
    activeGeneration = 0;
}

WavImportSubmitResult WavImportService::Client::submit(WavImportRequest request)
{
    if (service == nullptr)
        return {};

    const auto result = service->submit(owner, std::move(request));
    if (result.disposition == WavImportSubmitDisposition::accepted)
        activeGeneration = result.identity.generation;
    return result;
}

bool WavImportService::Client::cancel(std::string reason)
{
    return service != nullptr && activeGeneration != 0
        && service->cancel(owner, activeGeneration, std::move(reason));
}

bool WavImportService::Client::waitForTerminal(const std::chrono::milliseconds timeout) const
{
    return service != nullptr && activeGeneration != 0
        && service->waitForTerminal(owner, activeGeneration, timeout);
}

bool WavImportService::Client::waitForTerminal() const
{
    return waitForTerminal(std::chrono::milliseconds::max());
}

std::shared_ptr<const WavImportBatchSnapshot> WavImportService::Client::getSnapshot() const
{
    return service == nullptr ? nullptr : service->getSnapshot(owner, activeGeneration);
}

bool WavImportService::Client::consume()
{
    return service != nullptr && activeGeneration != 0
        && service->consume(owner, activeGeneration);
}

WavImportService::WavImportService(WavImportServiceOptions optionsIn)
    : options(std::move(optionsIn))
{
    auto initial = std::make_shared<const WavImportBatchSnapshot>();
    std::atomic_store_explicit(&snapshot, std::move(initial), std::memory_order_release);
    metrics.liveWorkerCount = 1;
    worker = std::thread([this] { runWorker(); });
}

WavImportService::~WavImportService()
{
    shutdown();
}

WavImportService::Client WavImportService::openClient()
{
    return Client(this, nextOwnerId.fetch_add(1, std::memory_order_relaxed));
}

WavImportSubmitResult WavImportService::submit(const std::uint64_t ownerId, WavImportRequest request)
{
    WavImportSubmitResult result;
    if (request.sourcePaths.empty() || request.projectId.empty() || request.contentRootPath.empty())
    {
        result.disposition = WavImportSubmitDisposition::invalid;
        return result;
    }

    {
        std::lock_guard<std::mutex> lock(mutex);
        if (shutdownRequested)
        {
            result.disposition = WavImportSubmitDisposition::shuttingDown;
            return result;
        }

        const auto currentSnapshot = std::atomic_load_explicit(&snapshot, std::memory_order_acquire);
        if (pending.has_value() || active.has_value()
            || (currentSnapshot && currentSnapshot->stage == WavImportBatchStage::completed))
        {
            ++metrics.rejectedBusyCount;
            result.disposition = WavImportSubmitDisposition::busy;
            return result;
        }

        result.disposition = WavImportSubmitDisposition::accepted;
        result.identity.ownerId = ownerId;
        result.identity.generation = ++nextGeneration;
        result.identity.projectId = request.projectId;
        result.identity.baseRevision = request.baseRevision;
        result.identity.contentRootPath = request.contentRootPath;
        result.identity.selectedGroupId = request.selectedGroupId;

        pending = PendingRequest { result.identity,
                                   std::move(request),
                                   std::make_shared<std::atomic<bool>>(false),
                                   {} };
        ++metrics.requestedCount;
        metrics.maximumPendingCount = std::max<std::size_t>(metrics.maximumPendingCount, 1);
        auto queued = std::make_shared<WavImportBatchSnapshot>();
        queued->identity = result.identity;
        queued->stage = WavImportBatchStage::queued;
        queued->status = "WAV import queued";
        queued->totalItemCount = pending->request.sourcePaths.size();
        queued->items = buildQueuedItems(result.identity, pending->request);
        std::atomic_store_explicit(&snapshot,
                                   std::shared_ptr<const WavImportBatchSnapshot>(std::move(queued)),
                                   std::memory_order_release);
    }

    condition.notify_one();
    return result;
}

WavImportServiceMetrics WavImportService::getMetrics() const
{
    std::lock_guard<std::mutex> lock(mutex);
    auto copy = metrics;
    copy.shutdownWaitDuration = std::chrono::microseconds(copy.maximumShutdownWaitMicros);
    copy.shutdownWaitMilliseconds = static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(copy.shutdownWaitDuration).count());
    return copy;
}

std::shared_ptr<const WavImportBatchSnapshot> WavImportService::getSnapshot() const
{
    return std::atomic_load_explicit(&snapshot, std::memory_order_acquire);
}

std::shared_ptr<const WavImportBatchSnapshot> WavImportService::getSnapshot(
    const std::uint64_t ownerId,
    const std::uint64_t generation) const
{
    const auto current = std::atomic_load_explicit(&snapshot, std::memory_order_acquire);
    if (!current || current->identity.ownerId != ownerId
        || (generation != 0 && current->identity.generation != generation))
        return nullptr;
    return current;
}

bool WavImportService::cancel(const std::uint64_t ownerId,
                              const std::uint64_t generation,
                              std::string reason)
{
    std::lock_guard<std::mutex> lock(mutex);
    const auto matches = [&](const auto& request)
    {
        return request.has_value() && request->identity.ownerId == ownerId
            && request->identity.generation == generation;
    };

    if (matches(pending))
    {
        pending->cancellationReason = std::move(reason);
        pending->cancellation->store(true, std::memory_order_release);
        return true;
    }

    if (matches(active))
    {
        active->cancellationReason = std::move(reason);
        active->cancellation->store(true, std::memory_order_release);
        return true;
    }

    const auto current = std::atomic_load_explicit(&snapshot, std::memory_order_acquire);
    return current && current->identity.ownerId == ownerId
        && current->identity.generation == generation
        && !isTerminalStage(current->stage);
}

bool WavImportService::waitForTerminal(const std::uint64_t ownerId,
                                       const std::uint64_t generation,
                                       const std::chrono::milliseconds timeout) const
{
    std::unique_lock<std::mutex> lock(mutex);
    return terminalCondition.wait_for(lock, timeout, [&]
    {
        const auto current = std::atomic_load_explicit(&snapshot, std::memory_order_acquire);
        if (shutdownRequested)
            return true;

        if (current && current->identity.ownerId == ownerId
            && current->identity.generation == generation)
        {
            return isTerminalStage(current->stage);
        }

        const auto matches = [&](const auto& request)
        {
            return request.has_value() && request->identity.ownerId == ownerId
                && request->identity.generation == generation;
        };

        return !matches(pending) && !matches(active);
    });
}

bool WavImportService::consume(const std::uint64_t ownerId, const std::uint64_t generation)
{
    std::shared_ptr<const WavImportBatchSnapshot> current;
    {
        std::lock_guard<std::mutex> lock(mutex);
        const auto currentSnapshot = std::atomic_load_explicit(&snapshot, std::memory_order_acquire);
        if (!currentSnapshot || currentSnapshot->identity.ownerId != ownerId
            || currentSnapshot->identity.generation != generation
            || currentSnapshot->stage != WavImportBatchStage::completed)
            return false;
        current = currentSnapshot;
    }

    publish(current->identity,
            WavImportBatchStage::consumed,
            WavImportTerminalDisposition::consumed,
            "WAV import result consumed",
            current->items,
            current->completion);
    cleanupRequestArtifacts(current->identity);
    return true;
}

void WavImportService::shutdown() noexcept
{
    std::lock_guard<std::mutex> shutdownLock(shutdownMutex);
    const auto started = std::chrono::steady_clock::now();
    {
        std::lock_guard<std::mutex> lock(mutex);
        shutdownRequested = true;
        if (pending)
            pending->cancellation->store(true, std::memory_order_release);
        if (active)
            active->cancellation->store(true, std::memory_order_release);
    }

    condition.notify_all();
    terminalCondition.notify_all();
    if (worker.joinable() && worker.get_id() != std::this_thread::get_id())
        worker.join();

    std::lock_guard<std::mutex> lock(mutex);
    const auto currentSnapshot = std::atomic_load_explicit(&snapshot, std::memory_order_acquire);
    if (currentSnapshot)
        cleanupRequestArtifacts(currentSnapshot->identity);
    metrics.liveWorkerCount = 0;
    const auto elapsed = std::chrono::steady_clock::now() - started;
    const auto micros = static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count());
    metrics.maximumShutdownWaitMicros = std::max(metrics.maximumShutdownWaitMicros, micros);
}

bool WavImportService::isTerminal(const WavImportBatchStage stage) const noexcept
{
    return isTerminalStage(stage);
}

std::string WavImportService::cancellationReason(const WavImportRequestIdentity& identity) const
{
    std::lock_guard<std::mutex> lock(mutex);
    const auto matches = [&identity](const auto& request)
    {
        return request.has_value() && request->identity.ownerId == identity.ownerId
            && request->identity.generation == identity.generation;
    };

    if (matches(active))
        return active->cancellationReason;
    if (matches(pending))
        return pending->cancellationReason;
    return {};
}

void WavImportService::publish(WavImportRequestIdentity identity,
                               const WavImportBatchStage stage,
                               const WavImportTerminalDisposition terminalDisposition,
                               std::string status,
                               std::vector<WavImportItemProgress> items,
                               std::shared_ptr<const WavImportCompletionPayload> completion)
{
    WavImportServiceOptions localOptions;
    WavImportBatchStage publishedStage = WavImportBatchStage::idle;
    {
        std::lock_guard<std::mutex> lock(mutex);
        const auto current = std::atomic_load_explicit(&snapshot, std::memory_order_acquire);
        const auto previous = current ? current->stage : WavImportBatchStage::idle;
        if (current && current->identity.generation == identity.generation
            && previous != stage
            && !isWavImportBatchStageTransitionAllowed(previous, stage))
            return;

        auto next = std::make_shared<WavImportBatchSnapshot>();
        next->identity = std::move(identity);
        next->stage = stage;
        next->terminalDisposition = terminalDisposition;
        next->status = std::move(status);
        next->items = std::move(items);
        next->completion = std::move(completion);
        next->totalItemCount = next->items.size();
        next->totalBytesProcessed = 0;
        next->totalBytesExpected = 0;
        for (const auto& item : next->items)
        {
            next->totalBytesProcessed += item.bytesProcessed;
            next->totalBytesExpected += item.totalBytes;
            next->copyDurationMicros += item.copyDurationMicros;
            next->fingerprintDurationMicros += item.fingerprintDurationMicros;
            next->inspectionDurationMicros += item.inspectionDurationMicros;
            next->totalDurationMicros += item.totalDurationMicros;
        }
        summarizeItems(next->items,
                       next->completedItemCount,
                       next->successfulItemCount,
                       next->failedItemCount,
                       next->canceledItemCount,
                       next->warningItemCount);
        if (next->completion)
        {
            next->totalItemCount = std::max(next->totalItemCount, next->completion->totalItemCount);
            next->successfulItemCount = std::max(next->successfulItemCount,
                                                 next->completion->successfulItemCount);
            next->failedItemCount = std::max(next->failedItemCount,
                                             next->completion->failedItemCount);
            next->canceledItemCount = std::max(next->canceledItemCount,
                                               next->completion->canceledItemCount);
            next->warningItemCount = std::max(next->warningItemCount,
                                              next->completion->warningItemCount);
            next->totalBytesProcessed = std::max(next->totalBytesProcessed,
                                                 next->completion->totalBytesProcessed);
            next->totalBytesExpected = std::max(next->totalBytesExpected,
                                                next->completion->totalBytesExpected);
            next->copyDurationMicros = std::max(next->copyDurationMicros,
                                                next->completion->copyDurationMicros);
            next->fingerprintDurationMicros = std::max(next->fingerprintDurationMicros,
                                                       next->completion->fingerprintDurationMicros);
            next->inspectionDurationMicros = std::max(next->inspectionDurationMicros,
                                                      next->completion->inspectionDurationMicros);
            next->totalDurationMicros = std::max(next->totalDurationMicros,
                                                 next->completion->totalDurationMicros);
            next->completedItemCount = std::max(next->completedItemCount,
                                                next->completion->successfulItemCount
                                                    + next->completion->failedItemCount
                                                    + next->completion->canceledItemCount);
        }

        publishedStage = next->stage;
        const auto generation = next->identity.generation;
        const auto copyDurationMicros = next->copyDurationMicros;
        const auto fingerprintDurationMicros = next->fingerprintDurationMicros;
        const auto inspectionDurationMicros = next->inspectionDurationMicros;
        const auto totalDurationMicros = next->totalDurationMicros;
        std::atomic_store_explicit(&snapshot,
                                   std::shared_ptr<const WavImportBatchSnapshot>(std::move(next)),
                                   std::memory_order_release);

        if (stage == WavImportBatchStage::completed)
            ++metrics.completedCount;
        else if (stage == WavImportBatchStage::canceled)
            ++metrics.canceledCount;
        else if (stage == WavImportBatchStage::failed)
            ++metrics.failedCount;
        else if (stage == WavImportBatchStage::consumed)
            ++metrics.consumedCount;

        if (isTerminalStage(stage))
        {
            metrics.lastTerminalGeneration = generation;
            metrics.lastCopyDurationMicros = copyDurationMicros;
            metrics.lastFingerprintDurationMicros = fingerprintDurationMicros;
            metrics.lastInspectionDurationMicros = inspectionDurationMicros;
            metrics.lastBatchDurationMicros = totalDurationMicros;
            metrics.maxBatchDurationMicros = std::max(metrics.maxBatchDurationMicros,
                                                      totalDurationMicros);
            const auto completedBatches = metrics.completedCount + metrics.failedCount + metrics.canceledCount;
            metrics.averageBatchDurationMicros = completedBatches > 0
                ? static_cast<std::uint64_t>(
                    ((metrics.averageBatchDurationMicros * (completedBatches - 1))
                        + totalDurationMicros) / completedBatches)
                : totalDurationMicros;
        }

        localOptions = options;
    }

    terminalCondition.notify_all();
    try
    {
        if (localOptions.stageObserver)
            localOptions.stageObserver(publishedStage);
    }
    catch (...)
    {
    }

    try
    {
        if (localOptions.checkpointObserver)
            localOptions.checkpointObserver(publishedStage);
    }
    catch (...)
    {
    }
}

void WavImportService::runWorker()
{
    {
        std::lock_guard<std::mutex> lock(mutex);
        metrics.liveWorkerCount = 1;
    }

    while (true)
    {
        std::optional<PendingRequest> request;
        {
            std::unique_lock<std::mutex> lock(mutex);
            condition.wait(lock, [&] { return shutdownRequested || pending.has_value(); });
            if (shutdownRequested && !pending.has_value())
                break;
            request = std::move(pending);
            pending.reset();
            active = request;
            metrics.maximumInFlightCount = std::max<std::size_t>(metrics.maximumInFlightCount, 1);
        }

        if (request)
        {
            try
            {
                process(std::move(*request));
            }
            catch (const std::exception& exception)
            {
                cleanupRequestArtifacts(request->identity);
                publish(request->identity,
                        request->cancellation->load(std::memory_order_acquire)
                            ? (isSupersededCancellationReason(request->cancellationReason)
                                  ? WavImportBatchStage::superseded
                                  : WavImportBatchStage::canceled)
                            : WavImportBatchStage::failed,
                        request->cancellation->load(std::memory_order_acquire)
                            ? (isSupersededCancellationReason(request->cancellationReason)
                                  ? WavImportTerminalDisposition::superseded
                                  : WavImportTerminalDisposition::canceled)
                            : WavImportTerminalDisposition::failed,
                        exception.what());
            }
            catch (...)
            {
                cleanupRequestArtifacts(request->identity);
                publish(request->identity,
                        WavImportBatchStage::failed,
                        WavImportTerminalDisposition::failed,
                        "Unexpected WAV import worker failure");
            }
        }

        {
            std::lock_guard<std::mutex> lock(mutex);
            active.reset();
        }
    }

    std::lock_guard<std::mutex> lock(mutex);
    metrics.liveWorkerCount = 0;
}

void WavImportService::process(PendingRequest pendingRequest)
{
    const auto identity = pendingRequest.identity;
    const auto requestStart = Clock::now();
    const auto canceled = [&pendingRequest]
    {
        return pendingRequest.cancellation->load(std::memory_order_acquire);
    };
    std::vector<WorkItem> workItems;

    const auto publishCurrent = [this, &identity, &workItems](const WavImportBatchStage stage,
                                                               const WavImportTerminalDisposition disposition,
                                                               const std::string& status)
    {
        publish(identity, stage, disposition, status, buildProgressItems(workItems));
    };

    const auto markNonTerminalItems = [&workItems](const WavImportItemStage stage, const std::string& status)
    {
        for (auto& workItem : workItems)
        {
            if (isItemTerminalStage(workItem.stage))
                continue;
            workItem.stage = stage;
            workItem.status = status;
        }
    };

    const auto publishCanceled = [&, this]
    {
        const auto reason = cancellationReason(identity);
        const auto superseded = isSupersededCancellationReason(reason);
        markNonTerminalItems(WavImportItemStage::canceled, reason.empty() ? "Canceled" : reason);
        for (auto& workItem : workItems)
            workItem.totalDurationMicros = workItem.copyDurationMicros + workItem.fingerprintDurationMicros
                + workItem.inspectionDurationMicros;
        cleanupRequestArtifacts(identity);
        publishCurrent(identity.ownerId == 0 ? WavImportBatchStage::failed
                                             : (superseded ? WavImportBatchStage::superseded
                                                           : WavImportBatchStage::canceled),
                       superseded ? WavImportTerminalDisposition::superseded
                                  : WavImportTerminalDisposition::canceled,
                       reason.empty()
                           ? (superseded ? "WAV import superseded" : "WAV import canceled")
                           : reason);
    };

    try
    {
        workItems = buildWorkItems(identity, pendingRequest.request);
        fs::create_directories(samplesDirectoryFor(identity));
        fs::create_directories(stageDirectoryFor(identity));
    }
    catch (const std::exception& exception)
    {
        cleanupRequestArtifacts(identity);
        publish(identity,
                WavImportBatchStage::failed,
                WavImportTerminalDisposition::failed,
                exception.what(),
                buildProgressItems(workItems));
        return;
    }

    if (canceled())
    {
        publishCanceled();
        return;
    }

    for (auto& workItem : workItems)
    {
        workItem.stage = WavImportItemStage::staging;
        workItem.status = "Staging";
    }
    publishCurrent(WavImportBatchStage::staging, WavImportTerminalDisposition::none, "Staging WAV import sources");

    for (auto& workItem : workItems)
    {
        if (canceled())
        {
            publishCanceled();
            return;
        }

        if (isItemTerminalStage(workItem.stage))
            continue;

        try
        {
            const auto copyStart = Clock::now();
            copyFileChunked(workItem,
                            options.copyChunkBytes,
                            pendingRequest.cancellation,
                            [this, &identity, &workItems]
                            {
                                publish(identity,
                                        WavImportBatchStage::staging,
                                        WavImportTerminalDisposition::none,
                                        "Staging WAV import sources",
                                        buildProgressItems(workItems));
                            });
            workItem.copyDurationMicros = static_cast<std::uint64_t>(
                std::chrono::duration_cast<std::chrono::microseconds>(Clock::now() - copyStart).count());
            workItem.totalDurationMicros = workItem.copyDurationMicros + workItem.fingerprintDurationMicros
                + workItem.inspectionDurationMicros;
        }
        catch (const std::exception& exception)
        {
            workItem.stage = WavImportItemStage::failed;
            workItem.status = exception.what();
            workItem.issueCount = 1;
            workItem.totalDurationMicros = workItem.copyDurationMicros + workItem.fingerprintDurationMicros
                + workItem.inspectionDurationMicros;
        }
    }

    if (canceled())
    {
        publishCanceled();
        return;
    }

    for (auto& workItem : workItems)
    {
        if (!isItemTerminalStage(workItem.stage))
        {
            workItem.stage = WavImportItemStage::fingerprinting;
            workItem.status = "Fingerprinting staged sample";
        }
    }
    publishCurrent(WavImportBatchStage::inspecting,
                   WavImportTerminalDisposition::none,
                   "Inspecting staged WAV sources");

    for (auto& workItem : workItems)
    {
        if (canceled())
        {
            publishCanceled();
            return;
        }

        if (workItem.stage == WavImportItemStage::failed)
            continue;

        class FingerprintCallbacks final : public drs::engine::SampleFingerprintCallbacks
        {
        public:
            FingerprintCallbacks(WorkItem& nextItem,
                                 std::shared_ptr<std::atomic<bool>> nextCancellation,
                                 std::function<void()> nextPublishProgress)
                : item(nextItem),
                  cancellation(std::move(nextCancellation)),
                  publishProgress(std::move(nextPublishProgress))
            {
            }

            bool isCancellationRequested() const override
            {
                return cancellation->load(std::memory_order_acquire);
            }

            void onProgress(const drs::engine::SampleFingerprintProgress& progress) const override
            {
                item.fingerprintBytesProcessed = progress.bytesProcessed;
                item.fingerprintTotalBytes = item.totalBytes;
                item.stage = WavImportItemStage::fingerprinting;
                item.status = "Fingerprinting staged sample";
                publishProgress();
            }

        private:
            WorkItem& item;
            std::shared_ptr<std::atomic<bool>> cancellation;
            std::function<void()> publishProgress;
        };

        FingerprintCallbacks callbacks(workItem,
                                       pendingRequest.cancellation,
                                       [this, &identity, &workItems]
                                       {
                                           publish(identity,
                                                   WavImportBatchStage::inspecting,
                                                   WavImportTerminalDisposition::none,
                                                   "Inspecting staged WAV sources",
                                                   buildProgressItems(workItems));
                                       });

        workItem.status = "Fingerprinting staged sample";
        const auto fingerprintStart = Clock::now();
        workItem.fingerprint = drs::engine::fingerprintSampleSourceFile(
            workItem.stagedPath,
            drs::engine::SampleFingerprintOptions {
                static_cast<std::uint64_t>(std::max<std::size_t>(1, options.fingerprintChunkBytes)),
                &callbacks,
            });
        workItem.fingerprintDurationMicros = static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::microseconds>(Clock::now() - fingerprintStart).count());
        workItem.totalDurationMicros = workItem.copyDurationMicros + workItem.fingerprintDurationMicros
            + workItem.inspectionDurationMicros;

        if (workItem.fingerprint.canceled || canceled())
        {
            workItem.stage = WavImportItemStage::canceled;
            workItem.status = "Canceled";
            publishCanceled();
            return;
        }

        if (!workItem.fingerprint.fingerprinted)
        {
            workItem.stage = WavImportItemStage::failed;
            workItem.status = workItem.fingerprint.state;
            workItem.issueCount = workItem.fingerprint.issues.size();
            publishCurrent(WavImportBatchStage::inspecting,
                           WavImportTerminalDisposition::none,
                           "Inspecting staged WAV sources");
            continue;
        }

        workItem.fingerprintBytesProcessed = workItem.totalBytes;
        workItem.fingerprintTotalBytes = workItem.totalBytes;
        workItem.stage = WavImportItemStage::inspecting;
        workItem.status = "Inspecting staged sample";
        publishCurrent(WavImportBatchStage::inspecting,
                       WavImportTerminalDisposition::none,
                       "Inspecting staged WAV sources");

        const auto inspectionStart = Clock::now();
        workItem.inspection = drs::engine::inspectSampleFile(workItem.stagedPath,
                                                             workItem.fingerprint.fingerprintHex);
        addInspectionWarningFindings(workItem);
        workItem.warningCount = countWarnings(workItem.findings);
        workItem.issueCount = workItem.inspection.issues.size();
        workItem.inspectionDurationMicros = static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::microseconds>(Clock::now() - inspectionStart).count());
        workItem.totalDurationMicros = workItem.copyDurationMicros + workItem.fingerprintDurationMicros
            + workItem.inspectionDurationMicros;

        if (!workItem.inspection.inspected || !workItem.inspection.accepted)
        {
            workItem.stage = WavImportItemStage::failed;
            workItem.status = workItem.inspection.state;
            publishCurrent(WavImportBatchStage::inspecting,
                           WavImportTerminalDisposition::none,
                           "Inspecting staged WAV sources");
            continue;
        }

        const auto heuristics = drs::engine::parseSampleFilenameHeuristics(workItem.sourcePath,
                                                                           &workItem.inspection.metadata);
        workItem.filenameTokens = heuristics.tokens;
        workItem.findings.insert(workItem.findings.end(),
                                 heuristics.findings.begin(),
                                 heuristics.findings.end());
        workItem.suggestedZone = heuristics.suggestedZone;
        workItem.warningCount = countWarnings(workItem.findings);
        workItem.issueCount = workItem.inspection.issues.size();
        workItem.stage = WavImportItemStage::ready;
        workItem.status = workItem.warningCount > 0
            ? "Authoring import completed with findings"
            : "Authoring import inferred draft zone";
        workItem.totalDurationMicros = workItem.copyDurationMicros + workItem.fingerprintDurationMicros
            + workItem.inspectionDurationMicros;
        publishCurrent(WavImportBatchStage::inspecting,
                       WavImportTerminalDisposition::none,
                       "Inspecting staged WAV sources");
    }

    if (canceled())
    {
        publishCanceled();
        return;
    }

    auto completion = buildCompletionPayload(identity, workItems);
    completion->totalDurationMicros = static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(Clock::now() - requestStart).count());
    const auto finalStage = completion->successfulItemCount == 0 && completion->failedItemCount > 0
        ? WavImportBatchStage::failed
        : WavImportBatchStage::completed;
    if (completion->disposition == WavImportTerminalDisposition::failed)
        cleanupRequestArtifacts(identity);
    else if (completion->disposition == WavImportTerminalDisposition::partiallyCompleted)
        cleanupDiscardedArtifacts(identity, workItems);
    publish(identity,
            finalStage,
            completion->disposition,
            completion->status,
            buildProgressItems(workItems),
            completion);
}
} // namespace drs::app
