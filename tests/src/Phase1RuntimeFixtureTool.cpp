#include "drs/engine/Phase1Baseline.h"
#include "drs/engine/RuntimeLoader.h"
#include "drs/engine/RuntimeStream.h"

#include <json/json.hpp>

#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>

namespace
{
namespace fs = std::filesystem;
using json = nlohmann::json;
using ordered_json = nlohmann::ordered_json;

struct Options
{
    bool verify = true;
    bool writeReferenceFixtures = false;
    bool writeReferencePackage = false;
    bool writeBaseline = false;
    std::string capturedOnIsoDate;
};

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
    std::ofstream output(path, std::ios::binary);
    require(output.good(), "Could not open file for writing: " + path.generic_string());
    output << text;
    require(output.good(), "Could not finish writing file: " + path.generic_string());
}

std::string computeFnv1aChecksumHex(const fs::path& path)
{
    constexpr std::uint64_t offsetBasis = 14695981039346656037ull;
    constexpr std::uint64_t prime = 1099511628211ull;

    std::ifstream input(path, std::ios::binary);
    require(input.good(), "Could not open file for checksum: " + path.generic_string());

    std::uint64_t hash = offsetBasis;
    char buffer[4096];

    while (input.read(buffer, sizeof(buffer)) || input.gcount() > 0)
    {
        for (std::streamsize index = 0; index < input.gcount(); ++index)
        {
            hash ^= static_cast<unsigned char>(buffer[index]);
            hash *= prime;
        }
    }

    std::ostringstream stream;
    stream << std::hex << std::setfill('0') << std::setw(16) << hash;
    return stream.str();
}

std::string toPackageRelativePath(const fs::path& packageDirectory, const std::string& rawPath)
{
    const fs::path path(rawPath);
    const auto normalizedPath = path.lexically_normal();

    if (!normalizedPath.is_absolute())
        return normalizedPath.generic_string();

    const auto relativePath = normalizedPath.lexically_relative(packageDirectory);
    if (!relativePath.empty())
        return relativePath.generic_string();

    return normalizedPath.generic_string();
}

std::string getCurrentIsoDate()
{
    const auto now = std::chrono::system_clock::now();
    const auto time = std::chrono::system_clock::to_time_t(now);

    std::tm localTime {};
    localtime_s(&localTime, &time);

    char buffer[11] {};
    const auto written = std::strftime(buffer, sizeof(buffer), "%Y-%m-%d", &localTime);
    require(written == 10, "Could not format current date as YYYY-MM-DD.");
    return std::string(buffer);
}

Options parseOptions(int argc, char* argv[])
{
    Options options;

    for (int index = 1; index < argc; ++index)
    {
        const std::string argument = argv[index];

        if (argument == "--verify")
        {
            options.verify = true;
        }
        else if (argument == "--write-reference-fixtures")
        {
            options.verify = false;
            options.writeReferenceFixtures = true;
        }
        else if (argument == "--write-baseline")
        {
            options.verify = false;
            options.writeBaseline = true;
        }
        else if (argument == "--write-reference-package")
        {
            options.verify = false;
            options.writeReferencePackage = true;
        }
        else if (argument == "--write-all")
        {
            options.verify = false;
            options.writeReferenceFixtures = true;
            options.writeReferencePackage = true;
            options.writeBaseline = true;
        }
        else if (argument == "--captured-on")
        {
            require(index + 1 < argc, "--captured-on requires a YYYY-MM-DD value.");
            options.capturedOnIsoDate = argv[++index];
        }
        else
        {
            throw std::runtime_error("Unknown argument: " + argument);
        }
    }

    return options;
}

std::string buildReferencePackageManifest(const drs::engine::RuntimeProjectLoadResult& referenceProject,
                                          const drs::engine::RuntimeManifestLoadResult& referenceInstrument,
                                          const drs::engine::RuntimeStreamLoadResult& referenceStream)
{
    const auto packageDirectory = fs::path(drs::engine::getPhase1ReferencePackageManifestPath()).parent_path();
    std::unordered_map<std::string, const drs::engine::RuntimeStreamSampleDefinition*> streamSamplesById;
    for (const auto& sample : referenceStream.container.samples)
        streamSamplesById.emplace(sample.sampleId, &sample);

    ordered_json packageManifest;
    packageManifest["schemaName"] = "drs.referencePackage";
    packageManifest["schemaVersion"] = 1;
    packageManifest["packageId"] = "drs.phase1.tiny-open-instrument.package";
    packageManifest["displayName"] = "DRS Tiny Open Instrument Package";
    packageManifest["instrumentId"] = referenceInstrument.instrument.instrumentId;

    ordered_json generatedFrom;
    generatedFrom["referenceCorpusIndexPath"] = "../index.json";
    generatedFrom["projectManifestPath"] = fs::path(referenceProject.manifestPath).filename().generic_string();
    generatedFrom["instrumentManifestPath"] = fs::path(referenceInstrument.manifestPath).filename().generic_string();
    generatedFrom["streamAssetPath"] = fs::path(referenceStream.containerPath).filename().generic_string();
    generatedFrom["contentRootPath"] = toPackageRelativePath(packageDirectory, referenceProject.project.contentRootPath);
    packageManifest["generatedFrom"] = std::move(generatedFrom);

    ordered_json packageFiles = ordered_json::array();
    const auto appendPackageFile = [&packageFiles](const char* role, const fs::path& path)
    {
        ordered_json fileObject;
        fileObject["role"] = role;
        fileObject["path"] = path.filename().generic_string();
        fileObject["sizeBytes"] = fs::file_size(path);
        fileObject["checksumHex"] = computeFnv1aChecksumHex(path);
        packageFiles.push_back(std::move(fileObject));
    };

    appendPackageFile("projectManifest", referenceProject.manifestPath);
    appendPackageFile("instrumentManifest", referenceInstrument.manifestPath);
    appendPackageFile("streamContainer", referenceStream.containerPath);
    if (!referenceStream.container.payloadAssetPath.empty())
        appendPackageFile("streamPayload", referenceStream.container.payloadAssetPath);
    packageManifest["packageFiles"] = std::move(packageFiles);

    ordered_json sourceSamples = ordered_json::array();
    for (const auto& sampleSource : referenceProject.project.sampleSources)
    {
        const auto iterator = streamSamplesById.find(sampleSource.id);
        require(iterator != streamSamplesById.end(),
                "Reference package manifest generation could not find compiled stream metadata for source sample '" + sampleSource.id + "'.");

        const auto* streamSample = iterator->second;
        ordered_json sampleObject;
        sampleObject["id"] = sampleSource.id;
        sampleObject["role"] = sampleSource.role;
        sampleObject["sourcePath"] = toPackageRelativePath(packageDirectory, sampleSource.path);
        sampleObject["sourceChecksumHex"] = streamSample->sourceChecksumHex;
        sampleObject["sampleRate"] = streamSample->sampleRate;
        sampleObject["frameCount"] = streamSample->frameCount;
        sampleObject["channelCount"] = streamSample->channelCount;
        sampleObject["payloadOffsetBytes"] = streamSample->payloadOffsetBytes;
        sampleObject["payloadSizeBytes"] = streamSample->payloadSizeBytes;
        sampleObject["prefetchBytes"] = streamSample->prefetchBytes;
        sourceSamples.push_back(std::move(sampleObject));
    }

    packageManifest["sourceSamples"] = std::move(sourceSamples);

    ordered_json compiledRuntime;
    compiledRuntime["defaultLoadProfile"] = referenceInstrument.instrument.defaultLoadProfile;
    compiledRuntime["macroCount"] = referenceInstrument.metrics.macroCount;
    compiledRuntime["articulationCount"] = referenceInstrument.metrics.articulationCount;
    compiledRuntime["groupCount"] = referenceInstrument.metrics.groupCount;
    compiledRuntime["zoneCount"] = referenceInstrument.metrics.zoneCount;
    compiledRuntime["referencedSampleCount"] = referenceInstrument.metrics.referencedSampleCount;
    compiledRuntime["totalPrefetchBytes"] = referenceInstrument.metrics.totalPrefetchBytes;
    compiledRuntime["streamPageSizeBytes"] = referenceStream.container.pageSizeBytes;
    compiledRuntime["streamSampleCount"] = referenceStream.metrics.sampleCount;
    compiledRuntime["streamPageCount"] = referenceStream.metrics.pageCount;
    compiledRuntime["totalPayloadBytes"] = referenceStream.container.totalPayloadBytes;
    compiledRuntime["payloadFileBytes"] = referenceStream.container.payloadFileBytes;
    packageManifest["compiledRuntime"] = std::move(compiledRuntime);

    ordered_json validation = ordered_json::array();
    validation.push_back("Build and run drs_phase1_runtime_fixture_tool --verify to confirm the checked-in manifests and package metadata still match the canonical runtime serializers.");
    validation.push_back("Run tools/package-phase1-reference-instrument.ps1 -Mode Verify to exercise the contributor-facing wrapper around that same package check.");
    validation.push_back("Open the standalone shell, load the default or lead demo from the Phase 1 performance surface, and confirm playback without hidden debug commands.");
    packageManifest["validation"] = std::move(validation);

    return packageManifest.dump(2) + "\n";
}

void verifyReferenceFixtures(const drs::engine::RuntimeProjectLoadResult& referenceProject,
                             const drs::engine::RuntimeManifestLoadResult& referenceInstrument)
{
    const auto referenceProjectPath = fs::path(drs::engine::getPhase1ReferenceProjectManifestPath());
    const auto serializedProject = drs::engine::serializeRuntimeProjectManifest(referenceProject.project,
                                                                               referenceProjectPath.generic_string());
    require(serializedProject == readTextFile(referenceProjectPath),
            "Reference project fixture is out of sync with the canonical serializer.");

    const auto referenceManifestPath = fs::path(drs::engine::getPhase1ReferenceInstrumentManifestPath());
    const auto serializedInstrument = drs::engine::serializeRuntimeInstrumentManifest(referenceInstrument.instrument,
                                                                                      referenceManifestPath.generic_string());
    require(serializedInstrument == readTextFile(referenceManifestPath),
            "Reference instrument fixture is out of sync with the canonical serializer.");
}

void verifyReferencePackageManifest(const drs::engine::RuntimeProjectLoadResult& referenceProject,
                                    const drs::engine::RuntimeManifestLoadResult& referenceInstrument,
                                    const drs::engine::RuntimeStreamLoadResult& referenceStream)
{
    const auto packageManifestPath = fs::path(drs::engine::getPhase1ReferencePackageManifestPath());
    require(fs::exists(packageManifestPath), "Reference package manifest must exist.");

    const auto serializedPackageManifest = buildReferencePackageManifest(referenceProject,
                                                                        referenceInstrument,
                                                                        referenceStream);
    require(serializedPackageManifest == readTextFile(packageManifestPath),
            "Reference package manifest is out of sync with the canonical fixture package description.");
}

void verifyCheckedInBaseline(const drs::engine::RuntimeManifestLoadResult& coldResult)
{
    const auto baselinePath = fs::path(drs::engine::getPhase1ReferenceBaselinePath());
    require(fs::exists(baselinePath), "Checked-in baseline snapshot must exist.");

    const auto baselineJson = json::parse(readTextFile(baselinePath));
    require(baselineJson.at("schemaName").get<std::string>() == "drs.runtimeBaseline",
            "Checked-in baseline schemaName changed unexpectedly.");
    require(baselineJson.at("schemaVersion").get<int>() == 1,
            "Checked-in baseline schemaVersion changed unexpectedly.");
    require(baselineJson.at("baselineId").get<std::string>() == coldResult.instrument.instrumentId,
            "Checked-in baseline baselineId changed unexpectedly.");
    require(baselineJson.at("referenceManifestPath").get<std::string>()
                == "content/runtime/phase1/reference-corpus/tiny-open-instrument/tiny-open-instrument.drinst",
            "Checked-in baseline referenceManifestPath changed unexpectedly.");
    require(baselineJson.at("timingUnits").get<std::string>() == "microseconds",
            "Checked-in baseline timingUnits changed unexpectedly.");

    const auto& staticExpectations = baselineJson.at("staticExpectations");
    require(staticExpectations.at("manifestBytes").get<std::uint64_t>() == coldResult.metrics.manifestSizeBytes,
            "Checked-in baseline manifestBytes no longer matches the live reference fixture.");
    require(staticExpectations.at("sourceProjectResolved").get<bool>() == coldResult.metrics.sourceProjectResolved,
            "Checked-in baseline sourceProjectResolved no longer matches the live reference fixture.");
    require(staticExpectations.at("compiledStreamAssetResolved").get<bool>() == coldResult.metrics.compiledStreamAssetResolved,
            "Checked-in baseline compiledStreamAssetResolved no longer matches the live reference fixture.");
    require(staticExpectations.at("macroCount").get<std::size_t>() == coldResult.metrics.macroCount,
            "Checked-in baseline macroCount no longer matches the live reference fixture.");
    require(staticExpectations.at("articulationCount").get<std::size_t>() == coldResult.metrics.articulationCount,
            "Checked-in baseline articulationCount no longer matches the live reference fixture.");
    require(staticExpectations.at("groupCount").get<std::size_t>() == coldResult.metrics.groupCount,
            "Checked-in baseline groupCount no longer matches the live reference fixture.");
    require(staticExpectations.at("zoneCount").get<std::size_t>() == coldResult.metrics.zoneCount,
            "Checked-in baseline zoneCount no longer matches the live reference fixture.");
    require(staticExpectations.at("referencedSampleCount").get<std::size_t>() == coldResult.metrics.referencedSampleCount,
            "Checked-in baseline referencedSampleCount no longer matches the live reference fixture.");
    require(staticExpectations.at("totalPrefetchBytes").get<std::uint64_t>() == coldResult.metrics.totalPrefetchBytes,
            "Checked-in baseline totalPrefetchBytes no longer matches the live reference fixture.");

    const auto& latestObserved = baselineJson.at("latestObserved");
    require(latestObserved.at("capturedOn").get<std::string>().size() == 10,
            "Checked-in baseline capturedOn should use YYYY-MM-DD formatting.");
    require(latestObserved.at("coldLoadMicros").get<std::uint64_t>() > 0,
            "Checked-in baseline coldLoadMicros must be positive.");
    require(latestObserved.at("warmLoadMicros").get<std::uint64_t>() > 0,
            "Checked-in baseline warmLoadMicros must be positive.");

    const auto& driftPolicy = baselineJson.at("driftPolicy");
    require(driftPolicy.at("allowedPositiveDriftMicros").get<std::uint64_t>() > 0,
            "Checked-in baseline allowedPositiveDriftMicros must be positive.");
    require(driftPolicy.at("allowedNegativeDriftMicros").get<std::uint64_t>() > 0,
            "Checked-in baseline allowedNegativeDriftMicros must be positive so faster timing drift is tolerated within the reviewed window.");
}

void rewriteReferenceFixtures(const drs::engine::RuntimeProjectLoadResult& referenceProject,
                              const drs::engine::RuntimeManifestLoadResult& referenceInstrument)
{
    const auto referenceProjectPath = fs::path(drs::engine::getPhase1ReferenceProjectManifestPath());
    const auto serializedProject = drs::engine::serializeRuntimeProjectManifest(referenceProject.project,
                                                                               referenceProjectPath.generic_string());
    writeTextFile(referenceProjectPath, serializedProject);

    const auto referenceManifestPath = fs::path(drs::engine::getPhase1ReferenceInstrumentManifestPath());
    const auto serializedInstrument = drs::engine::serializeRuntimeInstrumentManifest(referenceInstrument.instrument,
                                                                                      referenceManifestPath.generic_string());
    writeTextFile(referenceManifestPath, serializedInstrument);
}

void rewriteReferencePackageManifest(const drs::engine::RuntimeProjectLoadResult& referenceProject,
                                     const drs::engine::RuntimeManifestLoadResult& referenceInstrument,
                                     const drs::engine::RuntimeStreamLoadResult& referenceStream)
{
    const auto packageManifestPath = fs::path(drs::engine::getPhase1ReferencePackageManifestPath());
    const auto packageManifestText = buildReferencePackageManifest(referenceProject,
                                                                  referenceInstrument,
                                                                  referenceStream);
    writeTextFile(packageManifestPath, packageManifestText);
}

void rewriteBaselineSnapshot(const drs::engine::RuntimeManifestLoadResult& coldResult,
                             const drs::engine::RuntimeManifestLoadResult& warmResult,
                             const std::string& capturedOnIsoDate)
{
    const auto baselinePath = fs::path(drs::engine::getPhase1ReferenceBaselinePath());
    const auto baselineText = drs::engine::buildPhase1CheckedInBaselineSnapshotJson(coldResult,
                                                                                    warmResult,
                                                                                    capturedOnIsoDate);
    writeTextFile(baselinePath, baselineText);
}
} // namespace

int main(int argc, char* argv[])
{
    try
    {
        const auto options = parseOptions(argc, argv);

        const auto referenceProject = drs::engine::loadPhase1ReferenceProjectManifest();
        require(referenceProject.loaded, "Reference project must load cleanly before fixture maintenance can run.");

        const auto coldResult = drs::engine::loadPhase1ReferenceInstrumentManifest();
        require(coldResult.loaded, "Reference instrument must load cleanly before fixture maintenance can run.");

        const auto warmResult = drs::engine::loadPhase1ReferenceInstrumentManifest();
        require(warmResult.loaded, "Reference instrument warm-load must succeed before fixture maintenance can run.");

        const auto referenceStream = drs::engine::loadPhase1ReferenceStreamContainer();
        require(referenceStream.loaded, "Reference stream container must load cleanly before fixture maintenance can run.");

        if (options.verify)
        {
            verifyReferenceFixtures(referenceProject, coldResult);
            verifyReferencePackageManifest(referenceProject, coldResult, referenceStream);
            verifyCheckedInBaseline(coldResult);
            std::cout << "Phase 1 runtime fixture tool verify passed." << std::endl;
            return 0;
        }

        if (options.writeReferenceFixtures)
            rewriteReferenceFixtures(referenceProject, coldResult);

        if (options.writeReferencePackage)
            rewriteReferencePackageManifest(referenceProject, coldResult, referenceStream);

        if (options.writeBaseline)
        {
            const auto capturedOnIsoDate = options.capturedOnIsoDate.empty()
                ? getCurrentIsoDate()
                : options.capturedOnIsoDate;
            rewriteBaselineSnapshot(coldResult, warmResult, capturedOnIsoDate);
        }

        std::cout << "Phase 1 runtime fixture tool write completed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Phase 1 runtime fixture tool failed: " << exception.what() << std::endl;
        return 1;
    }
}
