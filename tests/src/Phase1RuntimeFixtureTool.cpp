#include "drs/engine/Phase1Baseline.h"
#include "drs/engine/PackageCrypto.h"
#include "drs/engine/PackageWriter.h"
#include "drs/engine/RuntimeLoader.h"
#include "drs/engine/RuntimeStream.h"
#include "Phase1PerformancePackageSupport.h"

#include <json/json.hpp>

#include <chrono>
#include <ctime>
#include <cstring>
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
    bool writeReferencePackageCorpus = false;
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

std::string computeFnv1aChecksumHex(const std::vector<std::uint8_t>& bytes)
{
    constexpr std::uint64_t offsetBasis = 14695981039346656037ull;
    constexpr std::uint64_t prime = 1099511628211ull;

    std::uint64_t hash = offsetBasis;
    for (const auto byte : bytes)
    {
        hash ^= byte;
        hash *= prime;
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
        else if (argument == "--write-reference-package-corpus")
        {
            options.verify = false;
            options.writeReferencePackageCorpus = true;
        }
        else if (argument == "--write-all")
        {
            options.verify = false;
            options.writeReferenceFixtures = true;
            options.writeReferencePackage = true;
            options.writeReferencePackageCorpus = true;
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

void generateReferencePackageCorpus(const fs::path& outputRoot)
{
    namespace package_support = drs::tests::performance_package;

    fs::create_directories(outputRoot);
    const auto& cryptoProvider = drs::engine::getDeterministicPackageCryptoProvider();
    const auto corpusPaths = package_support::CheckedInCorpusPaths {
        outputRoot,
        outputRoot / "index.json",
        outputRoot / "valid.drpkg",
        outputRoot / "truncated.drpkg",
        outputRoot / "tampered.drpkg",
        outputRoot / "wrong-version.drpkg",
        outputRoot / "missing-payload.drpkg",
        outputRoot / "checksum-mismatch.drpkg",
    };

    const auto validPlan = package_support::buildPackagePlan(outputRoot / "staging-valid",
                                                             corpusPaths.valid);
    const auto validWrite = drs::engine::writePerformancePackage(validPlan, cryptoProvider);
    require(validWrite.written, "Valid performance package fixture should write successfully.");

    const auto validBytes = package_support::readBinaryFile(corpusPaths.valid);
    require(validBytes.size() > 64, "Valid performance package fixture should be large enough for corruption mutations.");

    auto truncatedBytes = validBytes;
    truncatedBytes.resize(truncatedBytes.size() - 32);
    package_support::writeBinaryFile(corpusPaths.truncated, truncatedBytes);

    const auto validInspection = drs::engine::inspectPerformancePackage(corpusPaths.valid.generic_string(),
                                                                        cryptoProvider,
                                                                        drs::engine::performancePackageSchemaVersion);
    require(validInspection.valid, "Generated valid package fixture should inspect successfully.");

    auto tamperedBytes = validBytes;
    const auto tocTagOffset = validInspection.header.tocOffsetBytes + cryptoProvider.nonceSizeBytes();
    require(tocTagOffset < tamperedBytes.size(), "Generated valid package TOC tag offset should remain in bounds.");
    tamperedBytes[static_cast<std::size_t>(tocTagOffset)] ^= 0x5au;
    package_support::writeBinaryFile(corpusPaths.tampered, tamperedBytes);

    const auto wrongVersionPlan = package_support::buildPackagePlan(outputRoot / "staging-wrong-version",
                                                                    corpusPaths.wrongVersion,
                                                                    drs::engine::performancePackageSchemaVersion + 1);
    const auto wrongVersionWrite = drs::engine::writePerformancePackage(wrongVersionPlan, cryptoProvider);
    require(wrongVersionWrite.written, "Wrong-version performance package fixture should write successfully.");

    auto missingPayloadPlan = drs::engine::buildPerformancePackageWritePlan(
        package_support::buildPackagePlan(outputRoot / "staging-missing-payload",
                                          corpusPaths.missingPayload));
    missingPayloadPlan.payloads.erase(
        std::remove_if(missingPayloadPlan.payloads.begin(),
                       missingPayloadPlan.payloads.end(),
                       [](const drs::engine::PerformancePackagePayloadSource& payload)
                       {
                           return payload.kind == drs::engine::PerformancePackagePayloadKind::runtimeStreamIndex;
                       }),
        missingPayloadPlan.payloads.end());
    const auto missingPayloadWrite = drs::engine::writePerformancePackage(missingPayloadPlan, cryptoProvider);
    require(missingPayloadWrite.written, "Missing-payload performance package fixture should write successfully.");

    auto checksumMismatchPlan = drs::engine::buildPerformancePackageWritePlan(
        package_support::buildPackagePlan(outputRoot / "staging-checksum-mismatch",
                                          corpusPaths.checksumMismatch));
    auto payloadIterator = std::find_if(
        checksumMismatchPlan.payloads.begin(),
        checksumMismatchPlan.payloads.end(),
        [](const drs::engine::PerformancePackagePayloadSource& payload)
        {
            return payload.kind == drs::engine::PerformancePackagePayloadKind::runtimeStreamPayload;
        });
    require(payloadIterator != checksumMismatchPlan.payloads.end(),
            "Checksum-mismatch performance package fixture should include the runtime stream payload.");
    require(!payloadIterator->plaintextBytes.empty(),
            "Checksum-mismatch runtime stream payload should include bytes to corrupt.");
    payloadIterator->plaintextBytes[0] ^= 0x7fu;
    const auto checksumMismatchWrite = drs::engine::writePerformancePackage(checksumMismatchPlan, cryptoProvider);
    require(checksumMismatchWrite.written, "Checksum-mismatch performance package fixture should write successfully.");

    ordered_json indexRoot;
    indexRoot["schemaName"] = "drs.performancePackageCorpus";
    indexRoot["schemaVersion"] = 1;
    indexRoot["packageId"] = "drs.phase1.tiny-open-instrument.package";
    indexRoot["generatedFrom"] = {
        { "referenceProjectPath", "tiny-open-instrument.drsproj" },
        { "referenceInstrumentPath", "tiny-open-instrument.drinst" },
        { "referenceStreamPath", "tiny-open-instrument.drstrm" },
        { "wrapperScriptPath", "../../../tools/package-phase1-reference-instrument.ps1" }
    };

    const auto appendFixture = [&](const std::string& id,
                                   const fs::path& path,
                                   const std::string& expectedState,
                                   const std::string& expectedFailureCategory,
                                   const std::string& expectedIssueSubstring)
    {
        ordered_json fixture;
        fixture["id"] = id;
        fixture["path"] = path.filename().generic_string();
        fixture["sizeBytes"] = fs::file_size(path);
        fixture["checksumHex"] = computeFnv1aChecksumHex(path);
        fixture["expectedState"] = expectedState;
        fixture["expectedFailureCategory"] = expectedFailureCategory;
        fixture["expectedIssueSubstring"] = expectedIssueSubstring;
        return fixture;
    };

    indexRoot["fixtures"] = ordered_json::array({
        appendFixture("valid", corpusPaths.valid, "loaded", "none", ""),
        appendFixture("truncated", corpusPaths.truncated, "read-failed", "package-format-failure", "truncated"),
        appendFixture("tampered", corpusPaths.tampered, "read-failed", "decryption-failure", "authentication failed"),
        appendFixture("wrong-version", corpusPaths.wrongVersion, "read-failed", "package-format-failure", "requires reader schema version"),
        appendFixture("missing-payload", corpusPaths.missingPayload, "load-failed", "payload-corruption", "runtimeStreamIndex payload"),
        appendFixture("checksum-mismatch", corpusPaths.checksumMismatch, "load-failed", "payload-corruption", "payload checksum mismatch"),
    });

    package_support::writeTextFile(corpusPaths.index, indexRoot.dump(2) + "\n");

    std::error_code cleanupError;
    fs::remove_all(outputRoot / "staging-valid", cleanupError);
    fs::remove_all(outputRoot / "staging-wrong-version", cleanupError);
    fs::remove_all(outputRoot / "staging-missing-payload", cleanupError);
    fs::remove_all(outputRoot / "staging-checksum-mismatch", cleanupError);
}

void verifyReferencePackageCorpus()
{
    namespace package_support = drs::tests::performance_package;

    const auto checkedInCorpus = package_support::getCheckedInCorpusPaths();
    require(fs::exists(checkedInCorpus.index), "Checked-in performance package corpus index must exist.");

    const auto generatedRoot = fs::temp_directory_path() / "drs-phase1-reference-package-corpus-verify";
    std::error_code cleanupError;
    fs::remove_all(generatedRoot, cleanupError);
    generateReferencePackageCorpus(generatedRoot);

    const auto compareTextFile = [&](const fs::path& checkedInPath, const fs::path& generatedPath, const std::string& label)
    {
        require(package_support::readTextFile(checkedInPath) == package_support::readTextFile(generatedPath),
                label + " is out of sync with the deterministic fixture generator.");
    };

    const auto compareBinaryFile = [&](const fs::path& checkedInPath, const fs::path& generatedPath, const std::string& label)
    {
        require(package_support::readBinaryFile(checkedInPath) == package_support::readBinaryFile(generatedPath),
                label + " is out of sync with the deterministic fixture generator.");
    };

    compareTextFile(checkedInCorpus.index, generatedRoot / "index.json", "Performance package corpus index");
    compareBinaryFile(checkedInCorpus.valid, generatedRoot / "valid.drpkg", "Valid performance package fixture");
    compareBinaryFile(checkedInCorpus.truncated, generatedRoot / "truncated.drpkg", "Truncated performance package fixture");
    compareBinaryFile(checkedInCorpus.tampered, generatedRoot / "tampered.drpkg", "Tampered performance package fixture");
    compareBinaryFile(checkedInCorpus.wrongVersion, generatedRoot / "wrong-version.drpkg", "Wrong-version performance package fixture");
    compareBinaryFile(checkedInCorpus.missingPayload, generatedRoot / "missing-payload.drpkg", "Missing-payload performance package fixture");
    compareBinaryFile(checkedInCorpus.checksumMismatch, generatedRoot / "checksum-mismatch.drpkg", "Checksum-mismatch performance package fixture");
}

void rewriteReferencePackageCorpus()
{
    generateReferencePackageCorpus(drs::tests::performance_package::getCheckedInCorpusPaths().root);
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
            verifyReferencePackageCorpus();
            verifyCheckedInBaseline(coldResult);
            std::cout << "Phase 1 runtime fixture tool verify passed." << std::endl;
            return 0;
        }

        if (options.writeReferenceFixtures)
            rewriteReferenceFixtures(referenceProject, coldResult);

        if (options.writeReferencePackage)
            rewriteReferencePackageManifest(referenceProject, coldResult, referenceStream);

        if (options.writeReferencePackageCorpus)
            rewriteReferencePackageCorpus();

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
