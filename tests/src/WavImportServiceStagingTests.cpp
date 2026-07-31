#include "shared/WavImportService.h"
#include "../support/WavImportTestSupport.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>

namespace
{
namespace fs = std::filesystem;
using namespace std::chrono_literals;

void require(const bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

struct StagingFixture
{
    fs::path root;
    fs::path sourceLargePath;
    fs::path sourceSmallPath;
    fs::path contentRoot;
    drs::app::WavImportRequest request;
};

StagingFixture makeFixture()
{
    const auto unique = std::to_string(
        static_cast<std::uint64_t>(std::chrono::steady_clock::now().time_since_epoch().count()));
    StagingFixture fixture;
    fixture.root = fs::temp_directory_path() / ("drs-wav-import-staging-" + unique);
    const auto sourceDirectory = fixture.root / "source";
    fixture.contentRoot = fixture.root / "project";
    const auto samplesDirectory = fixture.contentRoot / "Samples";
    fs::create_directories(samplesDirectory);
    const auto corpus = drs::tests::createGeneratedWavImportBatchCorpus(sourceDirectory);
    fixture.sourceLargePath = corpus.cleanPath;
    fixture.sourceSmallPath = corpus.policyWarningPath;
    require(fs::copy_file(corpus.cleanSiblingOnePath,
                          samplesDirectory / corpus.cleanPath.filename(),
                          fs::copy_options::overwrite_existing),
            "Could not create occupied final sample fixture.");

    fixture.request.sourcePaths = {
        fixture.sourceLargePath.generic_string(),
        fixture.sourceSmallPath.generic_string(),
    };
    fixture.request.projectId = "wav-staging-project";
    fixture.request.baseRevision = 11;
    fixture.request.contentRootPath = fixture.contentRoot.generic_string();
    fixture.request.selectedGroupId = "kit";
    return fixture;
}
} // namespace

int main()
{
    using namespace drs::app;

    try
    {
        const auto fixture = makeFixture();
        WavImportServiceOptions options;
        options.copyChunkBytes = 1;
        WavImportService service(std::move(options));
        auto client = service.openClient();

        const auto accepted = client.submit(fixture.request);
        require(accepted.disposition == WavImportSubmitDisposition::accepted,
                "The staging request must be accepted.");

        auto sawIntermediateProgress = false;
        const auto deadline = std::chrono::steady_clock::now() + 10s;
        while (std::chrono::steady_clock::now() < deadline)
        {
            const auto snapshot = client.getSnapshot();
            if (snapshot != nullptr && snapshot->stage == WavImportBatchStage::staging)
            {
                for (const auto& item : snapshot->items)
                {
                    if (item.totalBytes > 0 && item.bytesProcessed > 0 && item.bytesProcessed < item.totalBytes)
                    {
                        sawIntermediateProgress = true;
                        break;
                    }
                }
            }

            if (snapshot != nullptr && snapshot->stage == WavImportBatchStage::completed)
                break;

            std::this_thread::sleep_for(1ms);
        }

        require(client.waitForTerminal(5s),
                "The staging request must reach a terminal state.");
        const auto completed = client.getSnapshot();
        const auto completedStatus = completed == nullptr
            ? "no snapshot"
            : std::string(toString(completed->stage)) + " / " + completed->status;
        require(completed != nullptr
                    && completed->stage == WavImportBatchStage::completed
                    && completed->completion != nullptr
                    && completed->completion->items.size() == 2,
                "The staging request must publish a completed immutable payload. Current snapshot: "
                    + completedStatus);
        require(sawIntermediateProgress,
                "Chunked staging copies must publish visible intermediate byte progress.");

        const auto occupiedFinalPath = fixture.contentRoot / "Samples" / fixture.sourceLargePath.filename();
        for (const auto& item : completed->completion->items)
        {
            const fs::path stagedPath(item.stagedPath);
            const fs::path finalPath(item.finalPath);
            require(fs::exists(stagedPath),
                    "Each staged WAV import item must leave a private staged file behind for later commit.");
            require(stagedPath.extension() == fs::path(item.sourcePath).extension(),
                    "Staging must preserve the original source extension.");
            require(stagedPath.string().find(".staging") != std::string::npos,
                    "Staged WAV import files must live under the private staging directory.");
            require(finalPath.parent_path() == fixture.contentRoot / "Samples",
                    "Final WAV import targets must stay inside the project Samples directory.");
            require(!fs::exists(finalPath),
                    "Staging must not create committed final sample files yet.");
            require(finalPath.extension() == fs::path(item.sourcePath).extension(),
                    "Reserved final sample paths must preserve the original source extension.");
            require(item.copiedBytes == item.sourceBytes,
                    "Each staged WAV import item must copy its full source byte count.");
        }

        require(fs::path(completed->completion->items.front().finalPath) != occupiedFinalPath,
                "Reserved final sample paths must never overwrite an existing project sample file.");
        require(fs::file_size(fs::path(completed->completion->items.front().stagedPath))
                    == fs::file_size(fixture.sourceLargePath),
                "The staged large WAV file must match the source file size.");
        require(fs::file_size(fs::path(completed->completion->items.back().stagedPath))
                    == fs::file_size(fixture.sourceSmallPath),
                "The staged secondary sample must match the source file size.");

        std::cout << "WAV import staging tests passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "WAV import staging tests failed: "
                  << exception.what() << std::endl;
        return 1;
    }
}
