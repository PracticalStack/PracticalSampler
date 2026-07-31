#include "shared/ProjectSourceValidationService.h"
#include "WavImportTestSupport.h"

#include <juce_audio_formats/juce_audio_formats.h>

#include <chrono>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
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

struct TempDirectoryGuard
{
    explicit TempDirectoryGuard(fs::path pathIn)
        : path(std::move(pathIn))
    {
        fs::create_directories(path);
    }

    ~TempDirectoryGuard()
    {
        std::error_code error;
        fs::remove_all(path, error);
    }

    fs::path path;
};

drs::app::ProjectSourceValidationRequest makeRequest(const std::string& projectId,
                                                     const std::vector<fs::path>& paths)
{
    drs::app::ProjectSourceValidationRequest request;
    request.projectId = projectId;
    request.baseRevision = 12;
    request.contentRootPath = paths.empty() ? std::string {} : paths.front().parent_path().generic_string();
    request.sampleSources.reserve(paths.size());
    for (std::size_t index = 0; index < paths.size(); ++index)
    {
        drs::engine::RuntimeProjectSampleSource sampleSource;
        sampleSource.id = "source-" + std::to_string(index + 1);
        sampleSource.path = paths[index].generic_string();
        sampleSource.role = "sample";
        request.sampleSources.push_back(std::move(sampleSource));
    }
    return request;
}
} // namespace

int main()
{
    try
    {
        const auto tempRoot = fs::temp_directory_path()
            / ("drs-project-source-validation-" + juce::Uuid().toString().toLowerCase().toStdString());
        TempDirectoryGuard tempDirectory(tempRoot);
        const auto corpus = drs::tests::createGeneratedWavImportBatchCorpus(tempDirectory.path);

        {
            drs::app::ProjectSourceValidationService service;
            auto request = makeRequest("validation-complete",
                                       {
                                           corpus.cleanSiblingOnePath,
                                           corpus.unsupportedPath,
                                           corpus.cleanSiblingThreePath
                                       });
            require(service.submit(std::move(request)).accepted,
                    "Explicit project source validation request should be accepted.");
            require(service.waitForTerminal(5s),
                    "Project source validation should reach a terminal state for the mixed corpus.");

            const auto snapshot = service.getSnapshot();
            require(snapshot != nullptr, "Project source validation should publish a terminal snapshot.");
            require(snapshot->stage == drs::app::ProjectSourceValidationStage::failed,
                    "Mixed corpus validation should surface a failed terminal state when one source is unsupported.");
            require(snapshot->totalItemCount == 3,
                    "Mixed corpus validation should preserve the requested source count.");
            require(snapshot->completedItemCount == 3,
                    "Mixed corpus validation should finish every requested source.");
            require(snapshot->failedItemCount == 1,
                    "Mixed corpus validation should flag the unsupported fixture as a failure.");
            require(snapshot->successfulItemCount == 2,
                    "Mixed corpus validation should retain successful inspection for valid fixtures.");
        }

        {
            drs::tests::DeterministicSampleImportHooks hooks;
            hooks.fingerprintGate().arm();

            drs::app::ProjectSourceValidationServiceOptions options;
            options.sampleImportHooks = &hooks;
            drs::app::ProjectSourceValidationService service(options);
            auto request = makeRequest("validation-cancel",
                                       {
                                           corpus.cleanPath,
                                           corpus.cleanSiblingThreePath
                                       });
            require(service.submit(std::move(request)).accepted,
                    "Cancelable validation request should be accepted.");
            require(hooks.fingerprintGate().waitUntilBlocked(5s),
                    "Validation cancel coverage requires the fingerprint hook to block deterministically.");
            require(service.cancel("User canceled validation"),
                    "Active project source validation should be cancelable.");
            hooks.fingerprintGate().release();
            require(service.waitForTerminal(5s),
                    "Canceled project source validation should reach a terminal state.");

            const auto snapshot = service.getSnapshot();
            require(snapshot != nullptr, "Canceled validation should publish a terminal snapshot.");
            require(snapshot->stage == drs::app::ProjectSourceValidationStage::canceled,
                    "Canceled validation should report a canceled terminal state.");
            require(snapshot->canceledItemCount >= 1,
                    "Canceled validation should record at least one canceled source.");
            require(!snapshot->status.empty() && snapshot->status.find("canceled") != std::string::npos,
                    "Canceled validation should preserve an explicit cancellation status.");
        }

        std::cout << "Project source validation service tests passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Project source validation service tests failed: "
                  << exception.what() << std::endl;
        return 1;
    }
}
