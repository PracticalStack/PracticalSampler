#include "Phase1PerformancePackageSupport.h"
#include "PerformancePackageExportSecurityTestSupport.h"
#include "shared/PerformancePackageExportService.h"

#include <drs/engine/PackageReaderDispatch.h>
#include <drs/engine/PackageV3FileReader.h>
#include <drs/engine/PlayableInstrumentLicense.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
namespace fs = std::filesystem;

void require(const bool condition, const std::string& message)
{
    if (! condition) throw std::runtime_error(message);
}

void writeBytes(const fs::path& path, const std::vector<std::uint8_t>& bytes)
{
    fs::create_directories(path.parent_path());
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    output.write(reinterpret_cast<const char*>(bytes.data()),
                 static_cast<std::streamsize>(bytes.size()));
    output.close();
    require(output.good(), "V3 production export fixture write failed.");
}

std::vector<std::uint8_t> readBytes(const fs::path& path)
{
    std::ifstream input(path, std::ios::binary);
    return { std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>() };
}

bool contains(const std::vector<std::uint8_t>& bytes, const std::string& needle)
{
    return std::search(bytes.begin(), bytes.end(), needle.begin(), needle.end()) != bytes.end();
}

drs::app::PerformancePackageExportRequest makeRequest(
    const fs::path& root,
    const fs::path& output,
    std::shared_ptr<const drs::app::PerformancePackageExportSecurityContext> security)
{
    auto project = drs::tests::performance_package::buildAuthoringProjectFixture();
    const auto sourceRoot = fs::path(project.contentRootPath);
    const auto contentRoot = root / "author-content";
    fs::create_directories(contentRoot);
    for (const auto& source : project.sampleSources)
    {
        require(fs::copy_file(sourceRoot / source.path, contentRoot / source.path,
                              fs::copy_options::overwrite_existing),
                "V3 production export fixture sample copy failed.");
    }
    writeBytes(contentRoot / "Images" / "background.jpg",
               drs::tests::performance_package::buildBackgroundImageJpegFixture());
    writeBytes(contentRoot / drs::engine::playableInstrumentLicenseFileName,
               drs::tests::performance_package::buildLicenseTextFixture());
    project.contentRootPath = contentRoot.generic_string();
    project.authoring.zones.front().displayName = "Protected UI Canary Label";
    project.notes.push_back("Protected project content canary");

    drs::app::PerformancePackageExportRequest request;
    request.project = std::move(project);
    request.projectId = request.project.projectId;
    request.baseRevision = 9;
    request.packagePath = output.generic_string();
    request.securityContext = std::move(security);
    return request;
}
} // namespace

int main()
{
    try
    {
        using namespace drs;
        const auto root = fs::temp_directory_path() / "drs-production-v3-export-tests";
        std::error_code error;
        fs::remove_all(root, error);
        fs::create_directories(root);
        const auto security = tests::makePerformancePackageExportTestSecurityContext();

        auto firstRequest = makeRequest(root, root / "first.drpkg", security);
        auto secondRequest = firstRequest;
        secondRequest.packagePath = (root / "second.drpkg").generic_string();
        const auto first = app::executePerformancePackageExport(firstRequest);
        const auto second = app::executePerformancePackageExport(secondRequest);
        require(first.exported, first.issues.empty() ? first.state : first.issues.front());
        require(second.exported, second.issues.empty() ? second.state : second.issues.front());
        require(first.semanticDigest == second.semanticDigest && ! first.semanticDigest.empty(),
                "Production V3 exports must retain deterministic semantic identity.");
        const auto firstBytes = readBytes(first.packagePath);
        const auto secondBytes = readBytes(second.packagePath);
        require(firstBytes != secondBytes,
                "Production V3 exports must use fresh content keys and record nonces.");
        require(engine::dispatchPerformancePackageReader(first.packagePath).format
                    == engine::PerformancePackageDiskFormat::version3,
                "Normal playable-instrument export must emit V3 only.");
        require(! contains(firstBytes, "Protected UI Canary Label")
                    && ! contains(firstBytes, "Protected project content canary")
                    && ! contains(firstBytes, "DRS_Sine_A3.wav")
                    && ! contains(firstBytes, "DRS_TriangleLead_A4.wav")
                    && ! contains(firstBytes, "RIFF") && ! contains(firstBytes, "WAVE"),
                "V3 package bytes must not expose protected settings, paths, filenames, or WAV data.");

        const auto opened = engine::openPackageV3File(first.packagePath, *security->trustStore);
        require(opened.opened && opened.package.signatureVerified,
                opened.issues.empty() ? "V3 production export did not verify"
                                      : opened.issues.front());
        std::set<std::string> kinds;
        for (const auto& record : opened.package.records) kinds.insert(record.recordKind);
        require(kinds.count("manifest") == 1 && kinds.count("runtime-instrument") == 1
                    && kinds.count("stream-index") == 1 && kinds.count("background-image") == 1
                    && kinds.count("license-text") == 1 && kinds.count("sample-head") == 1
                    && kinds.count("sample-page") == 1,
                "Production V3 export must map every protected asset class into records.");
        require(first.peakPlaintextBufferBytes <= 65536u
                    && first.peakSealedBufferBytes <= 65536u,
                "Production V3 export must preserve bounded per-record buffers.");
        const auto hasStagingFile = [&](const std::string& packagePath)
        {
            const auto prefix = fs::path(packagePath).filename().string() + ".v3-stage-";
            for (const auto& entry : fs::directory_iterator(fs::path(packagePath).parent_path()))
                if (entry.path().filename().string().find(prefix) == 0u) return true;
            return false;
        };
        require(! hasStagingFile(first.packagePath) && ! hasStagingFile(second.packagePath),
                "Production V3 export must remove staging files.");

        auto unconfigured = makeRequest(root, root / "unconfigured.drpkg", nullptr);
        const auto rejected = app::executePerformancePackageExport(unconfigured);
        require(! rejected.exported && ! fs::exists(unconfigured.packagePath)
                    && rejected.state.find("security is unavailable") != std::string::npos,
                "Unprovisioned production export must fail closed without V1/V2 fallback.");

        app::PerformancePackageExportService service;
        require(service.setSecurityContext(security),
                "Production export service must accept an injected V3 security context.");
        auto client = service.openClient();
        auto asynchronous = makeRequest(root, root / "async.drpkg", nullptr);
        const auto submitted = client.submit(std::move(asynchronous));
        require(submitted.wasAccepted() && client.waitForTerminal(),
                "Normal asynchronous export workflow did not reach a terminal state.");
        const auto snapshot = client.getSnapshot();
        require(snapshot && snapshot->stage == app::PerformancePackageExportStage::completed
                    && snapshot->result && snapshot->result->exported,
                "Normal asynchronous export workflow must publish a signed V3 package.");
        service.shutdown();

        fs::remove_all(root, error);
        std::cout << "Performance package V3 production export tests passed\n";
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Performance package V3 production export tests failed: "
                  << exception.what() << '\n';
        return 1;
    }
}
