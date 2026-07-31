#include "WavImportTestSupport.h"

#include <chrono>
#include <filesystem>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <thread>
#include <vector>

namespace
{
namespace fs = std::filesystem;
using namespace std::chrono_literals;

void require(const bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

fs::path getScratchDirectory()
{
    auto path = fs::temp_directory_path() / "drs-wav-import-fixture-support-tests";
    fs::create_directories(path);
    return path;
}

class RecordingFingerprintCallbacks final : public drs::engine::SampleFingerprintCallbacks
{
public:
    explicit RecordingFingerprintCallbacks(const std::uint64_t cancelAfterBytes = 0)
        : cancelAfterBytesThreshold(cancelAfterBytes)
    {
    }

    bool isCancellationRequested() const override
    {
        return cancelRequested;
    }

    void onProgress(const drs::engine::SampleFingerprintProgress& progress) const override
    {
        observedBytesProcessed.push_back(progress.bytesProcessed);
        if (cancelAfterBytesThreshold > 0 && progress.bytesProcessed >= cancelAfterBytesThreshold)
            cancelRequested = true;
    }

    mutable std::vector<std::uint64_t> observedBytesProcessed;

private:
    std::uint64_t cancelAfterBytesThreshold = 0;
    mutable bool cancelRequested = false;
};
} // namespace

int main()
{
    try
    {
        const auto scratchDirectory = getScratchDirectory();
        const auto corpus = drs::tests::createGeneratedWavImportBatchCorpus(scratchDirectory / "corpus");
        require(fs::exists(corpus.cleanPath)
                    && fs::exists(corpus.policyWarningPath)
                    && fs::exists(corpus.unsupportedPath)
                    && !fs::exists(corpus.missingPath),
                "The generated mixed WAV-import corpus must materialize deterministic present and missing cases.");

        drs::tests::DeterministicSampleImportHooks hooks;
        drs::tests::SyntheticSampleReaderFixture longFixture;
        longFixture.samplePath = (scratchDirectory / "synthetic-long.wav").generic_string();
        longFixture.frameCount = 1'000'000;
        longFixture.channelCount = 2;
        longFixture.metadata.set("MidiUnityNote", "72");
        hooks.addReaderFixture(longFixture);
        hooks.setFingerprintBytes(longFixture.samplePath, std::string(16 * 1024, 'x'));

        std::optional<drs::engine::SampleImportResult> syntheticResult;
        hooks.readGate().arm();
        drs::engine::resetSampleImportIoCounters();
        std::thread readThread([&]
        {
            drs::engine::ScopedSampleImportHooksOverride scope(hooks);
            syntheticResult = drs::engine::importSampleFile(longFixture.samplePath);
        });
        require(hooks.readGate().waitUntilBlocked(2s),
                "The deterministic reader fixture must pause at the synthetic full-frame read stage.");
        hooks.readGate().release();
        readThread.join();

        require(syntheticResult.has_value() && syntheticResult->imported,
                "The synthetic long-sample fixture must import successfully after the read gate releases.");
        require(syntheticResult->sample.metadata.frameCount == static_cast<std::uint64_t>(longFixture.frameCount),
                "The synthetic long-sample fixture must report its configured frame count.");
        const auto syntheticCounters = drs::engine::getSampleImportIoCounters();
        require(syntheticCounters.readerOpenCount == 1
                    && syntheticCounters.fingerprintOpenCount == 1
                    && syntheticCounters.fullFrameReadCount == 1
                    && syntheticCounters.bytesReadCount >= 16 * 1024,
                "The synthetic long-sample fixture must exercise the shared import counters deterministically.");

        const auto pausedCopyDestination = scratchDirectory / "paused-copy.wav";
        fs::remove(pausedCopyDestination);
        bool pausedCopyCompleted = false;
        hooks.copyGate().arm();
        drs::engine::resetSampleImportIoCounters();
        std::thread copyThread([&]
        {
            drs::engine::ScopedSampleImportHooksOverride scope(hooks);
            pausedCopyCompleted = drs::engine::copySampleFileForImport(corpus.cleanPath.generic_string(),
                                                                      pausedCopyDestination.generic_string());
        });
        require(hooks.copyGate().waitUntilBlocked(2s),
                "The deterministic copy fixture must pause at the copy stage.");
        hooks.copyGate().release();
        copyThread.join();
        require(pausedCopyCompleted && fs::exists(pausedCopyDestination),
                "The deterministic copy fixture must resume to a successful copied file.");
        require(drs::engine::getSampleImportIoCounters().copyCount == 1,
                "The deterministic copy fixture must record exactly one counted copy attempt.");

        drs::tests::DeterministicSampleImportHooks failingCopyHooks;
        failingCopyHooks.failCopyFor(corpus.cleanPath.generic_string());
        drs::engine::resetSampleImportIoCounters();
        {
            drs::engine::ScopedSampleImportHooksOverride scope(failingCopyHooks);
            require(!drs::engine::copySampleFileForImport(corpus.cleanPath.generic_string(),
                                                          (scratchDirectory / "failing-copy.wav").generic_string()),
                    "The deterministic copy fixture must force a copy failure without depending on the host filesystem.");
        }
        require(drs::engine::getSampleImportIoCounters().copyCount == 1,
                "The deterministic copy-failure fixture must still record its attempted copy.");

        const auto expectedFingerprintBytes = static_cast<std::uint64_t>(16 * 1024);
        drs::engine::SampleSourceFingerprintResult baselineFingerprint;
        drs::engine::SampleSourceFingerprintResult chunkedFingerprint;
        RecordingFingerprintCallbacks progressCallbacks;
        drs::engine::resetSampleImportIoCounters();
        {
            drs::engine::ScopedSampleImportHooksOverride scope(hooks);
            baselineFingerprint = drs::engine::fingerprintSampleSourceFile(longFixture.samplePath);
            chunkedFingerprint = drs::engine::fingerprintSampleSourceFile(
                longFixture.samplePath,
                drs::engine::SampleFingerprintOptions { 1024, &progressCallbacks });
        }
        require(baselineFingerprint.fingerprinted && chunkedFingerprint.fingerprinted,
                "Synthetic fingerprint fixtures must support repeated stable hash generation.");
        require(!baselineFingerprint.canceled && !chunkedFingerprint.canceled,
                "Stable fingerprint checks should finish without cancellation.");
        require(baselineFingerprint.fingerprintHex == chunkedFingerprint.fingerprintHex,
                "Fingerprint hashes must remain stable across chunk sizes.");
        require(!progressCallbacks.observedBytesProcessed.empty(),
                "Fingerprint progress callbacks must observe chunk progress.");
        require(progressCallbacks.observedBytesProcessed.back() == expectedFingerprintBytes,
                "Fingerprint progress callbacks must report total bytes processed.");

        RecordingFingerprintCallbacks cancelCallbacks(4096);
        drs::engine::resetSampleImportIoCounters();
        drs::engine::SampleSourceFingerprintResult canceledFingerprint;
        {
            drs::engine::ScopedSampleImportHooksOverride scope(hooks);
            canceledFingerprint = drs::engine::fingerprintSampleSourceFile(
                longFixture.samplePath,
                drs::engine::SampleFingerprintOptions { 4096, &cancelCallbacks });
        }
        require(canceledFingerprint.canceled,
                "Fingerprint requests should report cancellation when the callback requests stop.");
        require(!canceledFingerprint.fingerprinted,
                "Canceled fingerprint requests must not report a finished hash.");
        require(canceledFingerprint.state == "Source fingerprint canceled",
                "Canceled fingerprint requests should expose a stable canceled disposition.");
        require(cancelCallbacks.observedBytesProcessed.size() == 1
                    && cancelCallbacks.observedBytesProcessed.front() == 4096,
                "Fingerprint cancellation should be observed within one configured chunk.");
        const auto canceledFingerprintCounters = drs::engine::getSampleImportIoCounters();
        require(canceledFingerprintCounters.fingerprintOpenCount == 1,
                "Canceled fingerprint requests should still record the opened fingerprint stream.");
        require(canceledFingerprintCounters.bytesReadCount == 4096,
                "Canceled fingerprint requests should stop after one configured chunk.");

        std::cout << "WAV import fixture support tests passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "WAV import fixture support tests failed: "
                  << exception.what() << std::endl;
        return 1;
    }
}
