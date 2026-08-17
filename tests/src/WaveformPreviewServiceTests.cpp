#include "shared/WaveformPreviewService.h"
#include "WavImportTestSupport.h"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
using namespace std::chrono_literals;

void require(const bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

drs::app::WaveformPreviewRequest makeRequest(const std::string& projectId,
                                             const std::string& sampleSourceId,
                                             const std::string& sourcePath,
                                             const std::string& requestStamp)
{
    drs::app::WaveformPreviewRequest request;
    request.projectId = projectId;
    request.baseRevision = 5;
    request.contentRootPath = "E:/scratch";
    request.sampleSourceId = sampleSourceId;
    request.sourcePath = sourcePath;
    request.displayPointCount = 64;
    request.chunkFrameCount = 256;
    request.requestStamp = requestStamp;
    return request;
}
} // namespace

int main()
{
    try
    {
        drs::tests::DeterministicSampleImportHooks hooks;
        const auto sourceA = std::string("synthetic-preview-a.wav");
        const auto sourceB = std::string("synthetic-preview-b.wav");
        hooks.addReaderFixture({ sourceA, "WAV file", 48000.0, 8192, 2, 32, true, {}, false });
        hooks.addReaderFixture({ sourceB, "WAV file", 48000.0, 4096, 1, 32, true, {}, false });
        hooks.setFingerprintBytes(sourceA, std::string(16384, 'a'));
        hooks.setFingerprintBytes(sourceB, std::string(8192, 'b'));

        {
            drs::app::WaveformPreviewServiceOptions options;
            options.sampleImportHooks = &hooks;

            hooks.readGate().arm();
            drs::app::WaveformPreviewService service(options);
            require(service.submit(makeRequest("project-a", "source-a", sourceA, "stamp-a")).accepted,
                    "Initial waveform preview request should be accepted.");
            require(hooks.readGate().waitUntilBlocked(5s),
                    "Waveform preview supersession coverage requires the first request to block in chunked reads.");
            require(service.submit(makeRequest("project-a", "source-b", sourceB, "stamp-b")).accepted,
                    "Superseding waveform preview request should be accepted.");
            hooks.readGate().release();
            require(service.waitForTerminal(5s),
                    "Superseded waveform preview service should eventually publish a terminal snapshot.");

            const auto snapshot = service.getSnapshot();
            require(snapshot != nullptr, "Waveform preview supersession should publish a terminal snapshot.");
            require(snapshot->stage == drs::app::WaveformPreviewServiceStage::completed,
                    "Latest waveform preview request should complete after superseding the earlier request.");
            require(snapshot->identity.sampleSourceId == "source-b"
                        && snapshot->identity.requestStamp == "stamp-b",
                    "Latest waveform preview terminal snapshot should belong to the newest request.");
            require(snapshot->result != nullptr && snapshot->result->built
                        && snapshot->result->metadata.frameCount == 4096,
                    "Latest waveform preview request should publish built peak data for the newest source.");
        }

        {
            hooks.readGate().arm();
            drs::app::WaveformPreviewServiceOptions options;
            options.sampleImportHooks = &hooks;
            drs::app::WaveformPreviewService service(options);
            require(service.submit(makeRequest("project-b", "source-a", sourceA, "stamp-c")).accepted,
                    "Cancelable waveform preview request should be accepted.");
            require(hooks.readGate().waitUntilBlocked(5s),
                    "Waveform preview cancellation coverage requires the request to block in chunked reads.");
            require(service.cancel("User canceled waveform preview"),
                    "Active waveform preview request should accept cancellation.");
            hooks.readGate().release();
            require(service.waitForTerminal(5s),
                    "Canceled waveform preview request should publish a terminal snapshot.");

            const auto snapshot = service.getSnapshot();
            require(snapshot != nullptr
                        && snapshot->stage == drs::app::WaveformPreviewServiceStage::canceled,
                    "Canceled waveform preview request should terminate in the canceled state.");
        }

        {
            drs::app::WaveformPreviewServiceOptions options;
            options.sampleImportHooks = &hooks;
            options.maximumCacheEntries = 2;
            options.maximumCacheBytes = 4096;
            drs::app::WaveformPreviewService service(options);

            auto ranged = makeRequest("project-c", "source-a", sourceA, "range-a");
            ranged.rangeStartFrame = 1024;
            ranged.rangeFrameCount = 2048;
            drs::engine::resetSampleImportIoCounters();
            require(service.submit(ranged).accepted && service.waitForTerminal(5s),
                    "Visible-range waveform request should complete.");
            auto snapshot = service.getSnapshot();
            require(snapshot != nullptr && snapshot->result != nullptr
                        && snapshot->result->rangeStartFrame == 1024
                        && snapshot->result->rangeEndFrameExclusive == 3072,
                    "Visible-range service result should retain exact source-frame coverage.");
            const auto readsAfterBuild = drs::engine::getSampleImportIoCounters().peakChunkReadCount;

            ranged.requestStamp = "range-a-again";
            require(service.submit(ranged).accepted && service.waitForTerminal(5s),
                    "Repeated visible-range request should complete from cache.");
            snapshot = service.getSnapshot();
            require(snapshot != nullptr && snapshot->cacheHit
                        && drs::engine::getSampleImportIoCounters().peakChunkReadCount == readsAfterBuild,
                    "An exact tile cache hit must not perform additional sample reads.");

            for (int index = 0; index < 3; ++index)
            {
                auto tile = makeRequest("project-c", "source-a", sourceA,
                                        "eviction-" + std::to_string(index));
                tile.rangeStartFrame = static_cast<std::uint64_t>(index) * 1024;
                tile.rangeFrameCount = 512;
                require(service.submit(std::move(tile)).accepted && service.waitForTerminal(5s),
                        "Cache eviction coverage tile should complete.");
            }
            snapshot = service.getSnapshot();
            require(snapshot != nullptr && snapshot->cacheEntryCount <= 2
                        && snapshot->cacheBytes <= options.maximumCacheBytes
                        && snapshot->cacheEvictionCount > 0,
                    "Waveform tile cache must remain bounded and report evictions.");
        }

        std::cout << "Waveform preview service tests passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Waveform preview service tests failed: "
                  << exception.what() << std::endl;
        return 1;
    }
}
