#include "drs/engine/RuntimeLoader.h"
#include "drs/engine/RuntimeStream.h"

#include <json/json.hpp>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

namespace
{
namespace fs = std::filesystem;
using json = nlohmann::json;

void require(bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

std::string readTextFile(const fs::path& path)
{
    std::ifstream input(path, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
}

void writeTextFile(const fs::path& path, const std::string& text)
{
    fs::create_directories(path.parent_path());
    std::ofstream output(path, std::ios::binary);
    require(output.good(), "Could not open stream-reader test fixture for writing: " + path.generic_string());
    output << text;
    require(output.good(), "Could not finish writing stream-reader test fixture: " + path.generic_string());
}

bool containsIssue(const drs::engine::RuntimeStreamLoadResult& result, const std::string& needle)
{
    for (const auto& issue : result.issues)
    {
        if (issue.find(needle) != std::string::npos)
            return true;
    }

    return false;
}
} // namespace

int main()
{
    try
    {
        const auto streamResult = drs::engine::loadPhase1ReferenceStreamContainer();
        require(streamResult.loaded, "Reference stream-container should load cleanly.");
        require(streamResult.container.containerId == "drs.phase1.tiny-open-instrument",
                "Reference stream-container id changed unexpectedly.");
        require(streamResult.metrics.sampleCount == 2, "Reference stream-container sample count changed unexpectedly.");
        require(streamResult.metrics.pageCount == 12, "Reference stream-container page count changed unexpectedly.");
        require(streamResult.metrics.checksumValidatedCount == 2,
                "Reference stream-container checksum-validation count changed unexpectedly.");

        const auto instrumentResult = drs::engine::loadPhase1ReferenceInstrumentManifest();
        require(instrumentResult.loaded, "Reference instrument manifest must load before stream tests run.");
        require(instrumentResult.instrument.zones.size() == 4, "Reference instrument zone count changed unexpectedly.");

        for (const auto& zone : instrumentResult.instrument.zones)
        {
            const auto sampleIterator = std::find_if(streamResult.container.samples.begin(),
                                                     streamResult.container.samples.end(),
                                                     [&](const drs::engine::RuntimeStreamSampleDefinition& sample)
                                                     {
                                                         return sample.sourcePath == zone.samplePath;
                                                     });
            require(sampleIterator != streamResult.container.samples.end(),
                    "Every runtime zone should map to a stream-container sample entry.");
            require(sampleIterator->payloadOffsetBytes == zone.streamOffsetBytes,
                    "Zone streamOffsetBytes must match the stream-container payload offset.");
            require(sampleIterator->prefetchBytes == zone.prefetchBytes,
                    "Zone prefetchBytes must match the stream-container sample prefetch head.");
        }

        const auto firstHeadRead = drs::engine::resolveRuntimeStreamRead(streamResult.container, "sine-a3", 0);
        require(firstHeadRead.resolved && firstHeadRead.inPrefetchHead,
                "Stream read at offset 0 should resolve inside the prefetch head.");
        require(firstHeadRead.absoluteOffsetBytes == 0,
                "First sample prefetch-head absolute offset changed unexpectedly.");
        require(firstHeadRead.readableBytes == 16384,
                "First sample prefetch-head readable span changed unexpectedly.");

        const auto firstPageRead = drs::engine::resolveRuntimeStreamRead(streamResult.container, "sine-a3", 16384);
        require(firstPageRead.resolved && firstPageRead.inPageTable,
                "Stream read at the prefetch boundary should resolve through the page table.");
        require(firstPageRead.pageIndex == 0, "First page-table page index changed unexpectedly.");
        require(firstPageRead.absoluteOffsetBytes == 16384,
                "First page-table absolute offset changed unexpectedly.");
        require(firstPageRead.readableBytes == 65536,
                "First page-table readable span changed unexpectedly.");

        const auto secondHeadRead = drs::engine::resolveRuntimeStreamRead(streamResult.container, "triangle-a4", 0);
        require(secondHeadRead.resolved && secondHeadRead.inPrefetchHead,
                "Second sample offset 0 should resolve inside the prefetch head.");
        require(secondHeadRead.absoluteOffsetBytes == 393216,
                "Second sample prefetch-head absolute offset changed unexpectedly.");

        const auto lastByteRead = drs::engine::resolveRuntimeStreamRead(streamResult.container, "triangle-a4", 352799);
        require(lastByteRead.resolved && lastByteRead.inPageTable,
                "Last byte of the second payload should resolve through the final page-table entry.");
        require(lastByteRead.pageIndex == 5, "Final page-table page index changed unexpectedly.");
        require(lastByteRead.readableBytes == 1, "Final page-table readable span changed unexpectedly.");

        const auto outOfRangeRead = drs::engine::resolveRuntimeStreamRead(streamResult.container, "triangle-a4", 352800);
        require(!outOfRangeRead.resolved, "Out-of-range stream reads should fail cleanly.");

        const auto scratchDirectory = fs::temp_directory_path() / "drs-phase1-stream-reader-tests";
        const auto checksumCorruptPath = scratchDirectory / "checksum-corrupt.drstrm";
        const auto offsetCorruptPath = scratchDirectory / "offset-corrupt.drstrm";

        auto checksumCorruptJson = json::parse(readTextFile(fs::path(drs::engine::getPhase1ReferenceStreamContainerPath())));
        checksumCorruptJson["samples"][0]["sourcePath"] = streamResult.container.samples[0].sourcePath;
        checksumCorruptJson["samples"][1]["sourcePath"] = streamResult.container.samples[1].sourcePath;
        checksumCorruptJson["samples"][0]["sourceChecksumHex"] = "deadbeefdeadbeef";
        writeTextFile(checksumCorruptPath, checksumCorruptJson.dump(2) + "\n");

        const auto checksumCorruptResult = drs::engine::loadRuntimeStreamContainer(checksumCorruptPath.generic_string());
        require(!checksumCorruptResult.loaded, "Checksum-corrupt stream-container should fail validation.");
        require(containsIssue(checksumCorruptResult, "checksum mismatch"),
                "Checksum-corrupt stream-container must report checksum mismatch.");

        auto offsetCorruptJson = json::parse(readTextFile(fs::path(drs::engine::getPhase1ReferenceStreamContainerPath())));
        offsetCorruptJson["samples"][0]["sourcePath"] = streamResult.container.samples[0].sourcePath;
        offsetCorruptJson["samples"][1]["sourcePath"] = streamResult.container.samples[1].sourcePath;
        offsetCorruptJson["samples"][0]["pages"][0]["offsetBytes"] = 16385;
        writeTextFile(offsetCorruptPath, offsetCorruptJson.dump(2) + "\n");

        const auto offsetCorruptResult = drs::engine::loadRuntimeStreamContainer(offsetCorruptPath.generic_string());
        require(!offsetCorruptResult.loaded, "Offset-corrupt stream-container should fail validation.");
        require(containsIssue(offsetCorruptResult, "offsetBytes did not match the expected stream offset"),
                "Offset-corrupt stream-container must report a page-offset corruption issue.");

        std::cout << "Phase 1 stream reader tests passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Phase 1 stream reader tests failed: " << exception.what() << std::endl;
        return 1;
    }
}
