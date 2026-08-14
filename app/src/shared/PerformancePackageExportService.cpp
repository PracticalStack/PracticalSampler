#include "shared/PerformancePackageExportService.h"
#include "shared/PerformancePackageProjection.h"

#include "drs/engine/PackageReader.h"
#include "drs/engine/PackageReaderDispatch.h"
#include "drs/engine/PackageV2StreamingExport.h"
#include "drs/engine/PlayableInstrumentLicense.h"

#include <algorithm>
#include <cmath>
#include <exception>
#include <filesystem>
#include <memory>
#include <optional>
#include <sstream>
#include <unordered_set>
#include <utility>

namespace drs::app
{
namespace
{
namespace fs = std::filesystem;

constexpr double validationProgress = 0.05;
constexpr double compileStartProgress = 0.10;
constexpr double compileEndProgress = 0.25;
constexpr double streamStartProgress = 0.25;
constexpr double streamEndProgress = 0.75;
constexpr double packageStartProgress = 0.75;
constexpr double packageEndProgress = 0.95;
constexpr double verifyStartProgress = 0.95;
constexpr double verifyEndProgress = 1.0;

bool isTerminalStage(const PerformancePackageExportStage stage) noexcept
{
    return stage == PerformancePackageExportStage::completed
        || stage == PerformancePackageExportStage::canceled
        || stage == PerformancePackageExportStage::failed
        || stage == PerformancePackageExportStage::consumed;
}

double clampProgress(const double value) noexcept
{
    return std::clamp(value, 0.0, 1.0);
}

double mapPhaseProgress(const double start, const double end, const double localProgress) noexcept
{
    return start + (end - start) * clampProgress(localProgress);
}

double ratio(std::uint64_t numerator, std::uint64_t denominator) noexcept
{
    if (denominator == 0)
        return 0.0;

    return clampProgress(static_cast<double>(numerator) / static_cast<double>(denominator));
}

std::string summarizeIssues(const std::vector<std::string>& issues)
{
    if (issues.empty())
        return {};

    if (issues.size() == 1)
        return issues.front();

    return issues.front() + " (+" + std::to_string(issues.size() - 1) + " more)";
}

std::string resolveProjectSampleSourcePath(const drs::engine::RuntimeProjectModel& project,
                                           const drs::engine::RuntimeProjectSampleSource& sampleSource)
{
    const fs::path sourcePath(sampleSource.path);
    if (sourcePath.is_absolute() || project.contentRootPath.empty())
        return sourcePath.lexically_normal().generic_string();

    return (fs::path(project.contentRootPath) / sourcePath).lexically_normal().generic_string();
}

std::vector<std::uint8_t> readValidJpegBytes(const juce::File& imageFile, std::string& issue)
{
    issue.clear();
    if (imageFile == juce::File() || !imageFile.existsAsFile())
        return {};

    constexpr std::int64_t maximumBackgroundImageBytes = 16ll * 1024ll * 1024ll;
    if (imageFile.getSize() > maximumBackgroundImageBytes)
    {
        issue = "Performance background image exceeds the 16 MiB package limit.";
        return {};
    }

    if (!imageFile.hasFileExtension("jpg;jpeg"))
    {
        issue = "Performance background image must use a .jpg extension.";
        return {};
    }

    std::unique_ptr<juce::InputStream> input(imageFile.createInputStream());
    if (input == nullptr)
    {
        issue = "Performance background image could not be opened for export.";
        return {};
    }

    auto* format = juce::ImageFileFormat::findImageFormatForStream(*input);
    if (format == nullptr || dynamic_cast<juce::JPEGImageFormat*>(format) == nullptr)
    {
        issue = "Performance background image must be a valid JPG file.";
        return {};
    }

    input->setPosition(0);
    const auto image = format->decodeImage(*input);
    if (!image.isValid())
    {
        issue = "Performance background image must decode as a valid JPG file.";
        return {};
    }

    juce::MemoryBlock bytes;
    if (!imageFile.loadFileAsData(bytes))
    {
        issue = "Performance background image could not be read for export.";
        return {};
    }

    const auto* data = static_cast<const std::uint8_t*>(bytes.getData());
    return std::vector<std::uint8_t>(data, data + bytes.getSize());
}

std::optional<drs::engine::PerformancePackagePayloadSource> resolveProjectBackgroundImagePayload(
    const drs::engine::RuntimeProjectModel& project,
    std::vector<std::string>& issues)
{
    if (project.contentRootPath.empty())
        return std::nullopt;

    const auto imageFile = juce::File(juce::String::fromUTF8(project.contentRootPath.c_str()))
        .getChildFile("Images")
        .getChildFile("background.jpg");
    std::string imageIssue;
    const auto jpgBytes = readValidJpegBytes(imageFile, imageIssue);
    if (!imageIssue.empty())
    {
        issues.push_back(imageIssue);
        return std::nullopt;
    }

    if (jpgBytes.empty())
        return std::nullopt;

    drs::engine::PerformancePackagePayloadSource payload;
    payload.payloadId = "background-image";
    payload.kind = drs::engine::PerformancePackagePayloadKind::backgroundImage;
    payload.logicalPath = "images/background.jpg";
    payload.mediaType = "image/jpeg";
    payload.plaintextBytes = jpgBytes;
    return payload;
}

std::optional<drs::engine::PerformancePackagePayloadSource> resolveProjectLicensePayload(
    const drs::engine::RuntimeProjectModel& project,
    std::vector<std::string>& issues)
{
    if (project.contentRootPath.empty())
        return std::nullopt;

    const auto licenseFile = juce::File(juce::String::fromUTF8(project.contentRootPath.c_str()))
        .getChildFile(drs::engine::playableInstrumentLicenseFileName);
    if (!licenseFile.existsAsFile())
        return std::nullopt;

    const auto licenseFileBytes = licenseFile.getSize();
    if (licenseFileBytes < 0
        || static_cast<std::uint64_t>(licenseFileBytes)
            > drs::engine::maximumPlayableInstrumentLicenseBytes)
    {
        issues.push_back("Performance license text exceeds the 1 MiB package limit.");
        return std::nullopt;
    }

    juce::MemoryBlock data;
    if (!licenseFile.loadFileAsData(data))
    {
        issues.push_back("Performance license text could not be read for export.");
        return std::nullopt;
    }

    const auto* begin = static_cast<const std::uint8_t*>(data.getData());
    std::vector<std::uint8_t> bytes;
    if (data.getSize() > 0)
        bytes.assign(begin, begin + data.getSize());
    const auto validation = drs::engine::validatePlayableInstrumentLicenseBytes(bytes);
    if (!validation.valid)
    {
        issues.push_back("Performance license text is invalid. " + validation.issue);
        return std::nullopt;
    }

    drs::engine::PerformancePackagePayloadSource payload;
    payload.payloadId = drs::engine::playableInstrumentLicensePayloadId;
    payload.kind = drs::engine::PerformancePackagePayloadKind::licenseText;
    payload.logicalPath = drs::engine::playableInstrumentLicenseLogicalPath;
    payload.mediaType = drs::engine::playableInstrumentLicenseMediaType;
    payload.plaintextBytes = std::move(bytes);
    return payload;
}

struct PerformancePackageExportPreparationResult
{
    bool ready = false;
    std::string state;
    std::vector<std::string> issues;
    drs::engine::RuntimeCompilePlan compilePlan;
    drs::engine::PerformancePackageManifest manifest;
    std::vector<drs::engine::PerformancePackagePayloadSource> additionalPayloads;
};

PerformancePackageExportPreparationResult preparePerformancePackageExport(
    const drs::engine::RuntimeProjectModel& project,
    const drs::engine::RuntimeSessionStateSnapshot& sessionState,
    const juce::File& targetPackageFile,
    const juce::File& stagingDirectory)
{
    PerformancePackageExportPreparationResult result;
    result.state = "Playable package export validation failed";

    if (targetPackageFile == juce::File())
    {
        result.issues.push_back("Select a valid .drpkg export destination.");
        return result;
    }

    PerformancePackageProjectionContext projectionContext;
    projectionContext.fallbackPackageName
        = targetPackageFile.getFileNameWithoutExtension().toStdString();
    projectionContext.outputProjectPath = stagingDirectory
        .getChildFile("export-runtime-project.drsproj").getFullPathName().toStdString();
    projectionContext.outputInstrumentPath = stagingDirectory
        .getChildFile("export-runtime-instrument.drinst").getFullPathName().toStdString();
    projectionContext.outputStreamPath = stagingDirectory
        .getChildFile("export-runtime-stream.drstrm").getFullPathName().toStdString();

    std::unordered_set<std::string> inspectedSampleSourceIds;
    inspectedSampleSourceIds.reserve(project.sampleSources.size());
    for (const auto& sampleSource : project.sampleSources)
    {
        if (sampleSource.id.empty() || sampleSource.path.empty()
            || !inspectedSampleSourceIds.insert(sampleSource.id).second)
            continue;

        const auto resolvedPath = resolveProjectSampleSourcePath(project, sampleSource);
        const auto inspection = drs::engine::inspectSampleFile(resolvedPath);
        if (!inspection.accepted)
        {
            auto issue = "Sample source '" + sampleSource.id + "' could not be prepared for export from '"
                + resolvedPath + "'.";
            if (!inspection.state.empty())
                issue += " " + inspection.state + ".";
            if (!inspection.issues.empty())
                issue += " " + summarizeIssues(inspection.issues);
            result.issues.push_back(std::move(issue));
            continue;
        }

        drs::engine::RuntimeCompileSourceDefinition compileSource;
        compileSource.id = sampleSource.id;
        compileSource.sourcePath = resolvedPath;
        compileSource.role = sampleSource.role;
        compileSource.metadata = inspection.metadata;
        projectionContext.sampleSources.push_back(std::move(compileSource));
    }

    auto projection = projectPerformancePackage(project, sessionState, std::move(projectionContext));
    result.issues.insert(result.issues.end(), projection.issues.begin(), projection.issues.end());
    result.compilePlan = std::move(projection.compilePlan);
    result.manifest = std::move(projection.manifest);

    if (const auto backgroundImagePayload = resolveProjectBackgroundImagePayload(project, result.issues);
        backgroundImagePayload.has_value())
    {
        result.manifest.backgroundImage.payloadId = backgroundImagePayload->payloadId;
        result.additionalPayloads.push_back(*backgroundImagePayload);
    }

    if (const auto licensePayload = resolveProjectLicensePayload(project, result.issues);
        licensePayload.has_value())
    {
        result.manifest.license.payloadId = licensePayload->payloadId;
        result.additionalPayloads.push_back(*licensePayload);
    }

    if (!result.issues.empty())
        return result;

    result.ready = true;
    result.state = "Playable package export ready";
    return result;
}

bool isCancellationRequested(const PerformancePackageExportExecutionOptions& options)
{
    return options.cancellationProbe && options.cancellationProbe();
}

void publishProgress(const PerformancePackageExportExecutionOptions& options,
                     const PerformancePackageExportStage stage,
                     const double progress01,
                     std::string status,
                     std::string detail = {},
                     const std::uint64_t bytesProcessed = 0,
                     const std::uint64_t totalBytes = 0,
                     std::string itemId = {})
{
    if (!options.progressSink)
        return;

    options.progressSink(PerformancePackageExportProgress { stage,
                                                            clampProgress(progress01),
                                                            std::move(status),
                                                            std::move(detail),
                                                            bytesProcessed,
                                                            totalBytes,
                                                            std::move(itemId) });
}

juce::String describeStage(const PerformancePackageExportStage stage)
{
    switch (stage)
    {
        case PerformancePackageExportStage::queued: return "Playable package export queued";
        case PerformancePackageExportStage::validating: return "Validating playable package export";
        case PerformancePackageExportStage::compiling: return "Compiling playable instrument";
        case PerformancePackageExportStage::writingStream: return "Writing compiled stream assets";
        case PerformancePackageExportStage::sealingPackage: return "Sealing playable package";
        case PerformancePackageExportStage::verifying: return "Verifying playable package";
        case PerformancePackageExportStage::completed: return "Playable package export complete";
        case PerformancePackageExportStage::canceled: return "Playable package export canceled";
        case PerformancePackageExportStage::failed: return "Playable package export failed";
        case PerformancePackageExportStage::consumed: return "Playable package export consumed";
        case PerformancePackageExportStage::idle: break;
    }

    return "Playable package export idle";
}

juce::String buildProgressDetailText(const PerformancePackageExportSnapshot& snapshot)
{
    juce::String detail = juce::String(snapshot.detail);
    if (snapshot.result != nullptr && snapshot.result->exported)
    {
        if (!detail.isEmpty())
            detail += "  ";
        detail += "Package size: "
            + juce::File::descriptionOfSizeInBytes(static_cast<int64_t>(snapshot.result->packageBytes));
        detail += "  Payloads: " + juce::String(static_cast<int>(snapshot.result->payloadCount));
    }

    return detail;
}
} // namespace

bool isPerformancePackageExportStageTransitionAllowed(const PerformancePackageExportStage from,
                                                      const PerformancePackageExportStage to) noexcept
{
    using Stage = PerformancePackageExportStage;
    switch (from)
    {
        case Stage::idle: return to == Stage::queued;
        case Stage::queued:
            return to == Stage::validating || to == Stage::canceled || to == Stage::failed;
        case Stage::validating:
            return to == Stage::compiling || to == Stage::canceled || to == Stage::failed;
        case Stage::compiling:
            return to == Stage::writingStream || to == Stage::canceled || to == Stage::failed;
        case Stage::writingStream:
            return to == Stage::sealingPackage || to == Stage::canceled || to == Stage::failed;
        case Stage::sealingPackage:
            return to == Stage::verifying || to == Stage::canceled || to == Stage::failed;
        case Stage::verifying:
            return to == Stage::completed || to == Stage::canceled || to == Stage::failed;
        case Stage::completed:
        case Stage::canceled:
        case Stage::failed:
            return to == Stage::consumed || to == Stage::queued;
        case Stage::consumed:
            return to == Stage::idle || to == Stage::queued;
    }

    return false;
}

const char* toString(const PerformancePackageExportStage stage) noexcept
{
    using Stage = PerformancePackageExportStage;
    switch (stage)
    {
        case Stage::idle: return "idle";
        case Stage::queued: return "queued";
        case Stage::validating: return "validating";
        case Stage::compiling: return "compiling";
        case Stage::writingStream: return "writingStream";
        case Stage::sealingPackage: return "sealingPackage";
        case Stage::verifying: return "verifying";
        case Stage::completed: return "completed";
        case Stage::canceled: return "canceled";
        case Stage::failed: return "failed";
        case Stage::consumed: return "consumed";
    }

    return "failed";
}

PerformancePackageExportOperationResult executePerformancePackageExport(
    const PerformancePackageExportRequest& request,
    const PerformancePackageExportExecutionOptions& options)
{
    PerformancePackageExportOperationResult result;
    result.packagePath = request.packagePath;
    result.state = "Playable package export failed";

    const auto targetPackageFile = request.packagePath.empty()
        ? juce::File {}
        : juce::File(request.packagePath)
              .withFileExtension(juce::String::fromUTF8(drs::engine::performancePackageFileExtension));
    if (targetPackageFile == juce::File())
    {
        result.issues.push_back("Select a valid .drpkg export destination.");
        return result;
    }

    publishProgress(options,
                    PerformancePackageExportStage::validating,
                    validationProgress,
                    "Validating playable package export",
                    targetPackageFile.getFileName().toStdString());

    const auto targetDirectory = targetPackageFile.getParentDirectory();
    if ((!targetDirectory.exists() && !targetDirectory.createDirectory()) || !targetDirectory.isDirectory())
    {
        result.issues.push_back("The playable package destination folder could not be created.");
        return result;
    }

    if (isCancellationRequested(options))
    {
        result.canceled = true;
        result.state = "Playable package export canceled";
        return result;
    }

    const auto stagingDirectory = juce::File::getSpecialLocation(juce::File::tempDirectory)
        .getChildFile("drs-package-export-" + juce::Uuid().toDashedString());
    const auto cleanupStagingDirectory = [&stagingDirectory]()
    {
        std::error_code cleanupError;
        fs::remove_all(fs::path(stagingDirectory.getFullPathName().toStdString()), cleanupError);
    };
    if ((!stagingDirectory.exists() && !stagingDirectory.createDirectory()) || !stagingDirectory.isDirectory())
    {
        result.issues.push_back("A temporary export staging directory could not be created.");
        return result;
    }

    const auto cleanupAndReturn = [&](PerformancePackageExportOperationResult terminalResult)
    {
        cleanupStagingDirectory();
        return terminalResult;
    };

    const auto preparation = preparePerformancePackageExport(request.project,
                                                             request.sessionState,
                                                             targetPackageFile,
                                                             stagingDirectory);
    if (!preparation.ready)
    {
        result.state = preparation.state;
        result.issues = preparation.issues;
        if (result.issues.empty())
            result.issues.push_back("The playable package export validation did not succeed.");
        return cleanupAndReturn(std::move(result));
    }

    if (isCancellationRequested(options))
    {
        result.canceled = true;
        result.state = "Playable package export canceled";
        return cleanupAndReturn(std::move(result));
    }

    publishProgress(options,
                    PerformancePackageExportStage::compiling,
                    compileStartProgress,
                    "Compiling playable instrument",
                    "Building runtime manifest and stream plan");

    auto compileResult = drs::engine::compileRuntimeInstrument(preparation.compilePlan);
    if (!compileResult.compiled)
    {
        result.state = compileResult.state;
        result.issues = compileResult.issues;
        if (result.issues.empty())
            result.issues.push_back("The project could not be compiled for playable package export.");
        return cleanupAndReturn(std::move(result));
    }

    publishProgress(options,
                    PerformancePackageExportStage::compiling,
                    compileEndProgress,
                    "Compiling playable instrument",
                    "Runtime compile completed");

    if (isCancellationRequested(options))
    {
        result.canceled = true;
        result.state = "Playable package export canceled";
        return cleanupAndReturn(std::move(result));
    }

    publishProgress(options,
                    PerformancePackageExportStage::writingStream,
                    streamStartProgress,
                    "Writing compiled stream assets",
                    "Decoding source samples into the package stream");

    const auto streamWrite = drs::engine::writeCompiledStreamAssets(
        compileResult,
        drs::engine::RuntimeStreamWriteOptions {
            [options](const drs::engine::RuntimeStreamWriteProgress& progress)
            {
                publishProgress(options,
                                PerformancePackageExportStage::writingStream,
                                mapPhaseProgress(streamStartProgress,
                                                 streamEndProgress,
                                                 ratio(progress.bytesProcessed,
                                                       progress.totalBytes)),
                                "Writing compiled stream assets",
                                progress.status,
                                progress.bytesProcessed,
                                progress.totalBytes,
                                progress.sampleId);
            },
            options.cancellationProbe
        });
    if (!streamWrite.written)
    {
        result.state = streamWrite.state;
        result.issues = streamWrite.issues;
        if (isCancellationRequested(options))
        {
            result.canceled = true;
            result.state = "Playable package export canceled";
        }
        else if (result.issues.empty())
        {
            result.issues.push_back("The compiled sample stream could not be written.");
        }
        return cleanupAndReturn(std::move(result));
    }

    publishProgress(options,
                    PerformancePackageExportStage::writingStream,
                    streamEndProgress,
                    "Writing compiled stream assets",
                    "Building bounded package record plan",
                    streamWrite.alignedPayloadBytes,
                    streamWrite.alignedPayloadBytes);

    auto packagePlan = drs::engine::buildPerformancePackageV2StreamingExportPlan(
        preparation.manifest,
        compileResult,
        targetPackageFile.getFullPathName().toStdString(),
        preparation.additionalPayloads);
    if (!packagePlan.built)
    {
        result.state = packagePlan.state;
        result.issues = packagePlan.issues;
        if (result.issues.empty())
            result.issues.push_back("The bounded package v2 record plan could not be built.");
        return cleanupAndReturn(std::move(result));
    }

    if (isCancellationRequested(options))
    {
        result.canceled = true;
        result.state = "Playable package export canceled";
        return cleanupAndReturn(std::move(result));
    }

    const auto packageWrite = drs::engine::writePackageV2Streaming(
        packagePlan.plan,
        drs::engine::getDeterministicPackageCryptoProvider(),
        drs::engine::PackageV2StreamingWriteOptions {
            [options](const drs::engine::PackageV2StreamingWriteProgress& progress)
            {
                publishProgress(options,
                                PerformancePackageExportStage::sealingPackage,
                                mapPhaseProgress(packageStartProgress,
                                                 packageEndProgress,
                                                 ratio(progress.completedPlaintextBytes,
                                                       progress.totalPlaintextBytes)),
                                "Sealing playable package",
                                progress.status,
                                progress.completedPlaintextBytes,
                                progress.totalPlaintextBytes,
                                progress.identity.sourceId);
            },
            options.cancellationProbe
        });
    if (!packageWrite.written)
    {
        result.state = packageWrite.state;
        result.issues = packageWrite.issues;
        if (result.issues.empty())
            result.issues.push_back("The playable package file could not be written.");
        if (packageWrite.failure == drs::engine::PackageV2Failure::cancelled)
            result.canceled = true;
        return cleanupAndReturn(std::move(result));
    }

    if (isCancellationRequested(options))
    {
        result.canceled = true;
        result.state = "Playable package export canceled";
        return cleanupAndReturn(std::move(result));
    }

    const auto packagePath = targetPackageFile.getFullPathName().toStdString();
    publishProgress(options,
                    PerformancePackageExportStage::verifying,
                    verifyStartProgress,
                    "Verifying playable package",
                    targetPackageFile.getFileName().toStdString());

    const auto verification = drs::engine::loadPerformancePackageV2Metadata(
        packagePath, drs::engine::getDeterministicPackageCryptoProvider());
    if (!verification.loaded)
    {
        result.state = verification.state.empty()
            ? std::string("Playable package export verification failed")
            : verification.state;
        result.issues = verification.issues;
        if (result.issues.empty())
        {
            result.issues.push_back(
                "The exported playable package was written, but the current reader could not reopen it.");
        }
        return cleanupAndReturn(std::move(result));
    }

    result.exported = true;
    result.state = "Playable package exported";
    result.packagePath = packagePath;
    result.packageBytes = packageWrite.packageBytes;
    result.payloadCount = static_cast<std::uint32_t>(packageWrite.completedRecordCount);
    result.peakPlaintextBufferBytes = packageWrite.peakPlaintextBufferBytes;
    result.peakSealedBufferBytes = packageWrite.peakSealedBufferBytes;
    result.verificationBytesRead = packageWrite.verificationBytesRead;
    result.totalDurationMicros = packageWrite.totalDurationMicros;
    result.plaintextThroughputBytesPerSecond = packageWrite.plaintextThroughputBytesPerSecond;
    publishProgress(options,
                    PerformancePackageExportStage::verifying,
                    verifyEndProgress,
                    result.state,
                    "Playable package verification completed");
    return cleanupAndReturn(std::move(result));
}

PerformancePackageExportProgressComponent::PerformancePackageExportProgressComponent(
    CancelCallback callback)
    : cancelCallback(std::move(callback))
{
    setComponentID("performancePackageExportProgress");
    statusLabel.setComponentID("performancePackageExportProgressLabel");
    addAndMakeVisible(statusLabel);
    progressBar.setComponentID("performancePackageExportProgressBar");
    addAndMakeVisible(progressBar);
    detailLabel.setComponentID("performancePackageExportProgressDetailLabel");
    detailLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(detailLabel);
    cancelButton.setComponentID("performancePackageExportProgressCancelButton");
    cancelButton.onClick = [this]
    {
        if (cancelCallback)
            cancelCallback();
    };
    addAndMakeVisible(cancelButton);
    setVisible(false);
}

void PerformancePackageExportProgressComponent::setCancelCallback(CancelCallback callback)
{
    cancelCallback = std::move(callback);
}

void PerformancePackageExportProgressComponent::update(const PerformancePackageExportSnapshot& snapshot)
{
    progressValue = snapshot.stage == PerformancePackageExportStage::queued
        ? 0.0
        : clampProgress(snapshot.progress01);
    statusLabel.setText(describeStage(snapshot.stage), juce::dontSendNotification);
    detailLabel.setText(buildProgressDetailText(snapshot), juce::dontSendNotification);
    cancelButton.setEnabled(snapshot.stage == PerformancePackageExportStage::queued
                            || snapshot.stage == PerformancePackageExportStage::validating
                            || snapshot.stage == PerformancePackageExportStage::compiling
                            || snapshot.stage == PerformancePackageExportStage::writingStream
                            || snapshot.stage == PerformancePackageExportStage::sealingPackage
                            || snapshot.stage == PerformancePackageExportStage::verifying);
    setVisible(snapshot.stage != PerformancePackageExportStage::idle
               && snapshot.stage != PerformancePackageExportStage::consumed);
}

void PerformancePackageExportProgressComponent::resized()
{
    auto area = getLocalBounds().reduced(8, 6);
    statusLabel.setBounds(area.removeFromTop(22));
    cancelButton.setBounds(area.removeFromRight(80).removeFromTop(24));
    area.removeFromRight(8);
    progressBar.setBounds(area.removeFromTop(18));
    area.removeFromTop(6);
    detailLabel.setBounds(area.removeFromTop(20));
}

PerformancePackageExportService::Client::Client(PerformancePackageExportService* serviceIn,
                                                const std::uint64_t ownerId) noexcept
    : service(serviceIn), owner(ownerId)
{
}

PerformancePackageExportService::Client::~Client()
{
    reset();
}

PerformancePackageExportService::Client::Client(Client&& other) noexcept
    : service(other.service), owner(other.owner), activeGeneration(other.activeGeneration)
{
    other.service = nullptr;
    other.owner = 0;
    other.activeGeneration = 0;
}

PerformancePackageExportService::Client& PerformancePackageExportService::Client::operator=(
    Client&& other) noexcept
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

void PerformancePackageExportService::Client::reset() noexcept
{
    if (service != nullptr && activeGeneration != 0)
    {
        service->cancel(owner, activeGeneration, "Playable package export owner closed");
        service->waitForTerminal(owner, activeGeneration, std::chrono::milliseconds::max());
    }
    service = nullptr;
    owner = 0;
    activeGeneration = 0;
}

PerformancePackageExportSubmitResult PerformancePackageExportService::Client::submit(
    PerformancePackageExportRequest request)
{
    if (service == nullptr)
        return {};

    const auto result = service->submit(owner, std::move(request));
    if (result.disposition == PerformancePackageExportSubmitDisposition::accepted)
        activeGeneration = result.identity.generation;
    return result;
}

bool PerformancePackageExportService::Client::cancel(std::string reason)
{
    return service != nullptr && activeGeneration != 0
        && service->cancel(owner, activeGeneration, std::move(reason));
}

bool PerformancePackageExportService::Client::waitForTerminal(
    const std::chrono::milliseconds timeout) const
{
    return service != nullptr && activeGeneration != 0
        && service->waitForTerminal(owner, activeGeneration, timeout);
}

bool PerformancePackageExportService::Client::waitForTerminal() const
{
    return waitForTerminal(std::chrono::milliseconds::max());
}

std::shared_ptr<const PerformancePackageExportSnapshot>
PerformancePackageExportService::Client::getSnapshot() const
{
    return service == nullptr ? nullptr : service->getSnapshot(owner, activeGeneration);
}

bool PerformancePackageExportService::Client::consume()
{
    return service != nullptr && activeGeneration != 0
        && service->consume(owner, activeGeneration);
}

PerformancePackageExportService::PerformancePackageExportService(
    PerformancePackageExportServiceOptions optionsIn)
    : options(std::move(optionsIn))
{
    auto initial = std::make_shared<const PerformancePackageExportSnapshot>();
    std::atomic_store_explicit(&snapshot, std::move(initial), std::memory_order_release);
    metrics.liveWorkerCount = 1;
    worker = std::thread([this] { runWorker(); });
}

PerformancePackageExportService::~PerformancePackageExportService()
{
    shutdown();
}

PerformancePackageExportService::Client PerformancePackageExportService::openClient()
{
    return Client(this, nextOwnerId.fetch_add(1, std::memory_order_relaxed));
}

PerformancePackageExportSubmitResult PerformancePackageExportService::submit(
    const std::uint64_t ownerId,
    PerformancePackageExportRequest request)
{
    PerformancePackageExportSubmitResult result;
    if (request.projectId.empty() || request.packagePath.empty())
    {
        result.disposition = PerformancePackageExportSubmitDisposition::invalid;
        return result;
    }

    {
        std::lock_guard<std::mutex> lock(mutex);
        if (shutdownRequested)
        {
            result.disposition = PerformancePackageExportSubmitDisposition::shuttingDown;
            return result;
        }
        if (pending.has_value() || active.has_value())
        {
            ++metrics.rejectedBusyCount;
            result.disposition = PerformancePackageExportSubmitDisposition::busy;
            return result;
        }

        result.disposition = PerformancePackageExportSubmitDisposition::accepted;
        result.identity.ownerId = ownerId;
        result.identity.generation = ++nextGeneration;
        result.identity.projectId = request.projectId;
        result.identity.baseRevision = request.baseRevision;
        result.identity.packagePath = request.packagePath;
        pending = PendingRequest { result.identity,
                                   std::move(request),
                                   std::make_shared<std::atomic<bool>>(false),
                                   {} };
        ++metrics.requestedCount;
        metrics.maximumPendingCount = std::max<std::size_t>(metrics.maximumPendingCount, 1);
        const auto queued = std::make_shared<PerformancePackageExportSnapshot>(
            PerformancePackageExportSnapshot { result.identity,
                                              PerformancePackageExportStage::queued,
                                              0.0,
                                              "Playable package export queued",
                                              juce::File(result.identity.packagePath)
                                                  .getFileName()
                                                  .toStdString(),
                                              {} });
        std::atomic_store_explicit(&snapshot,
                                   std::shared_ptr<const PerformancePackageExportSnapshot>(queued),
                                   std::memory_order_release);
    }
    condition.notify_one();
    return result;
}

PerformancePackageExportServiceMetrics PerformancePackageExportService::getMetrics() const
{
    std::lock_guard<std::mutex> lock(mutex);
    auto copy = metrics;
    copy.shutdownWaitDuration = std::chrono::microseconds(copy.maximumShutdownWaitMicros);
    copy.shutdownWaitMilliseconds = static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(copy.shutdownWaitDuration).count());
    return copy;
}

std::shared_ptr<const PerformancePackageExportSnapshot>
PerformancePackageExportService::getSnapshot() const
{
    return std::atomic_load_explicit(&snapshot, std::memory_order_acquire);
}

std::shared_ptr<const PerformancePackageExportSnapshot> PerformancePackageExportService::getSnapshot(
    const std::uint64_t ownerId,
    const std::uint64_t generation) const
{
    const auto current = std::atomic_load_explicit(&snapshot, std::memory_order_acquire);
    if (!current || current->identity.ownerId != ownerId
        || (generation != 0 && current->identity.generation != generation))
    {
        return nullptr;
    }
    return current;
}

bool PerformancePackageExportService::cancel(const std::uint64_t ownerId,
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

bool PerformancePackageExportService::waitForTerminal(const std::uint64_t ownerId,
                                                      const std::uint64_t generation,
                                                      const std::chrono::milliseconds timeout) const
{
    std::unique_lock<std::mutex> lock(mutex);
    return terminalCondition.wait_for(lock, timeout, [&]
    {
        const auto current = std::atomic_load_explicit(&snapshot, std::memory_order_acquire);
        return shutdownRequested || (current && current->identity.ownerId == ownerId
            && current->identity.generation == generation && isTerminalStage(current->stage));
    });
}

bool PerformancePackageExportService::consume(const std::uint64_t ownerId, const std::uint64_t generation)
{
    std::shared_ptr<const PerformancePackageExportSnapshot> current;
    {
        std::lock_guard<std::mutex> lock(mutex);
        const auto currentSnapshot = std::atomic_load_explicit(&snapshot, std::memory_order_acquire);
        if (!currentSnapshot || currentSnapshot->identity.ownerId != ownerId
            || currentSnapshot->identity.generation != generation
            || !isTerminalStage(currentSnapshot->stage)
            || currentSnapshot->stage == PerformancePackageExportStage::consumed)
        {
            return false;
        }
        current = currentSnapshot;
    }
    publish(current->identity,
            PerformancePackageExportStage::consumed,
            current->progress01,
            "Playable package export consumed",
            current->detail,
            current->result);
    return true;
}

void PerformancePackageExportService::shutdown() noexcept
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
    metrics.liveWorkerCount = 0;
    const auto elapsed = std::chrono::steady_clock::now() - started;
    const auto micros = static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count());
    metrics.maximumShutdownWaitMicros = std::max(metrics.maximumShutdownWaitMicros, micros);
}

bool PerformancePackageExportService::isTerminal(const PerformancePackageExportStage stage) const noexcept
{
    return isTerminalStage(stage);
}

std::string PerformancePackageExportService::cancellationReason(
    const PerformancePackageExportRequestIdentity& identity) const
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

void PerformancePackageExportService::publish(
    PerformancePackageExportRequestIdentity identity,
    const PerformancePackageExportStage stage,
    const double progress01,
    std::string status,
    std::string detail,
    std::shared_ptr<const PerformancePackageExportOperationResult> result)
{
    PerformancePackageExportServiceOptions localOptions;
    PerformancePackageExportStage publishedStage = PerformancePackageExportStage::idle;
    {
        std::lock_guard<std::mutex> lock(mutex);
        const auto current = std::atomic_load_explicit(&snapshot, std::memory_order_acquire);
        const auto previous = current ? current->stage : PerformancePackageExportStage::idle;
        if (current && current->identity.generation == identity.generation && previous != stage
            && !isPerformancePackageExportStageTransitionAllowed(previous, stage))
        {
            return;
        }

        auto next = std::make_shared<PerformancePackageExportSnapshot>();
        next->identity = std::move(identity);
        next->stage = stage;
        next->progress01 = clampProgress(progress01);
        next->status = std::move(status);
        next->detail = std::move(detail);
        next->result = std::move(result);
        publishedStage = next->stage;
        std::atomic_store_explicit(&snapshot,
                                   std::shared_ptr<const PerformancePackageExportSnapshot>(std::move(next)),
                                   std::memory_order_release);

        if (stage == PerformancePackageExportStage::completed)
            ++metrics.completedCount;
        else if (stage == PerformancePackageExportStage::canceled)
            ++metrics.canceledCount;
        else if (stage == PerformancePackageExportStage::failed)
            ++metrics.failedCount;
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

void PerformancePackageExportService::runWorker()
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
                auto result = std::make_shared<PerformancePackageExportOperationResult>();
                result->state = exception.what();
                result->issues.push_back(exception.what());
                publish(request->identity,
                        request->cancellation->load(std::memory_order_acquire)
                            ? PerformancePackageExportStage::canceled
                            : PerformancePackageExportStage::failed,
                        1.0,
                        request->cancellation->load(std::memory_order_acquire)
                            ? "Playable package export canceled"
                            : "Playable package export failed",
                        exception.what(),
                        result);
            }
            catch (...)
            {
                auto result = std::make_shared<PerformancePackageExportOperationResult>();
                result->state = "Unexpected playable package export worker failure";
                result->issues.push_back(result->state);
                publish(request->identity,
                        PerformancePackageExportStage::failed,
                        1.0,
                        "Playable package export failed",
                        result->state,
                        result);
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

void PerformancePackageExportService::process(PendingRequest pendingRequest)
{
    const auto identity = pendingRequest.identity;
    const auto progressCallback = [this, identity](const PerformancePackageExportProgress& progress)
    {
        publish(identity,
                progress.stage,
                progress.progress01,
                progress.status,
                progress.detail);
    };

    const auto result = std::make_shared<PerformancePackageExportOperationResult>(
        executePerformancePackageExport(
            pendingRequest.request,
            PerformancePackageExportExecutionOptions {
                progressCallback,
                [flag = pendingRequest.cancellation]
                {
                    return flag->load(std::memory_order_acquire);
                } }));

    if (result->exported)
    {
        publish(identity,
                PerformancePackageExportStage::completed,
                1.0,
                result->state,
                juce::File(result->packagePath).getFileName().toStdString(),
                result);
        return;
    }

    if (result->canceled)
    {
        const auto reason = cancellationReason(identity);
        publish(identity,
                PerformancePackageExportStage::canceled,
                1.0,
                "Playable package export canceled",
                reason.empty() ? result->state : reason,
                result);
        return;
    }

    publish(identity,
            PerformancePackageExportStage::failed,
            1.0,
            "Playable package export failed",
            !result->issues.empty() ? result->issues.front() : result->state,
            result);
}
} // namespace drs::app
