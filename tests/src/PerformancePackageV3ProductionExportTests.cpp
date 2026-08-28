#include "Phase1PerformancePackageSupport.h"
#include "PerformancePackageExportSecurityTestSupport.h"
#include "plugin/PluginProcessor.h"
#include "shared/PerformancePackageExportService.h"
#include "standalone/MainComponent.h"

#include <drs/engine/PackageReaderDispatch.h>
#include <drs/engine/PackageV3FileReader.h>
#include <drs/engine/PlayableInstrumentLicense.h>
#include <drs/engine/EngineFacade.h>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>
#include <thread>

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
        juce::ScopedJuceInitialiser_GUI gui;
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

        engine::PerformancePackageV3ActivationSecurityContext activationSecurity;
        activationSecurity.compatibilityId = security->compatibilityId;
        activationSecurity.keyProvider = security->keyProvider;
        activationSecurity.trustStore = security->trustStore;
        const auto runtimeSecurity
            = std::make_shared<const engine::PerformancePackageV3ActivationSecurityContext>(
                activationSecurity);
        const auto loadedV3 = engine::loadPerformancePackageV3Metadata(
            first.packagePath, activationSecurity);
        require(loadedV3.loaded && loadedV3.package != nullptr
                    && loadedV3.contentKey != nullptr
                    && loadedV3.metadata.backgroundImage.loaded
                    && loadedV3.metadata.licenseText.loaded
                    && ! loadedV3.sampleDescriptors.empty(),
                loadedV3.issues.empty() ? loadedV3.state : loadedV3.issues.front());
        auto pagedSource = std::make_shared<engine::PackagePagedSampleDataSource>(
            loadedV3.sampleDescriptors.front(), loadedV3.package, loadedV3.contentKey);
        require(pagedSource->prepareHead(),
                "Authenticated V3 sample head must be ready before activation.");
        if (pagedSource->pageCount() != 0)
            require(pagedSource->preparePage(0),
                    "Authenticated V3 sample pages must stream on demand.");

        auto preparedV3 = engine::preparePerformancePackageV3Activation(
            loadedV3.metadata, loadedV3.package, loadedV3.contentKey,
            loadedV3.sampleDescriptors);
        require(preparedV3.prepared,
                preparedV3.issues.empty() ? preparedV3.state : preparedV3.issues.front());
        engine::EngineFacade facade;
        const auto activatedV3 = facade.activatePreparedPerformancePackageSession(
            std::move(preparedV3));
        require(activatedV3.activated
                    && facade.getPerformancePackageActivationPayload() != nullptr
                    && facade.getPerformancePackageLicenseText() != nullptr,
                "Authenticated V3 metadata, samples, controls, artwork, and license must publish atomically.");
        const auto activeRevision = facade.getStateRevision();
        const auto activePayload = facade.getPerformancePackageActivationPayload();

        auto incompatibleSecurity = activationSecurity;
        incompatibleSecurity.compatibilityId = "incompatible.product";
        const auto incompatible = engine::loadPerformancePackageV3Metadata(
            first.packagePath, incompatibleSecurity);
        require(! incompatible.loaded
                    && incompatible.failure
                        == engine::PerformancePackageV3ActivationFailure::compatibility
                    && facade.getStateRevision() == activeRevision
                    && facade.getPerformancePackageActivationPayload() == activePayload,
                "Compatibility rejection must retain last-known-good playback.");

        const auto wrongSecurity = tests::makePerformancePackageExportTestSecurityContext();
        engine::PerformancePackageV3ActivationSecurityContext wrongKeySecurity;
        wrongKeySecurity.compatibilityId = security->compatibilityId;
        wrongKeySecurity.keyProvider = wrongSecurity->keyProvider;
        wrongKeySecurity.trustStore = security->trustStore;
        const auto wrongKey = engine::loadPerformancePackageV3Metadata(
            first.packagePath, wrongKeySecurity);
        require(! wrongKey.loaded
                    && wrongKey.failure
                        == engine::PerformancePackageV3ActivationFailure::authentication
                    && facade.getStateRevision() == activeRevision
                    && facade.getPerformancePackageActivationPayload() == activePayload,
                "Envelope/AEAD rejection must retain last-known-good playback.");

        engine::PerformancePackageV3ActivationSecurityContext unknownKeySecurity
            = activationSecurity;
        unknownKeySecurity.keyProvider
            = std::make_shared<tests::StaticPerformancePackageTestKeyProvider>(
                "unrelated-key-id", std::vector<std::uint8_t>(32u, 0x2au));
        const auto unknownKey = engine::loadPerformancePackageV3Metadata(
            first.packagePath, unknownKeySecurity);
        require(! unknownKey.loaded
                    && unknownKey.failure
                        == engine::PerformancePackageV3ActivationFailure::keyUnavailable
                    && facade.getStateRevision() == activeRevision,
                "Unknown-key rejection must retain last-known-good playback.");

        auto tamperedBytes = firstBytes;
        tamperedBytes[tamperedBytes.size() / 2u] ^= 0x40u;
        const auto tamperedPath = root / "tampered-v3.drpkg";
        writeBytes(tamperedPath, tamperedBytes);
        const auto tampered = engine::loadPerformancePackageV3Metadata(
            tamperedPath.generic_string(), activationSecurity);
        require(! tampered.loaded
                    && tampered.failure
                        == engine::PerformancePackageV3ActivationFailure::signature
                    && facade.getStateRevision() == activeRevision
                    && facade.getPerformancePackageActivationPayload() == activePayload,
                "Signature rejection must retain last-known-good playback.");

        const auto truncatedPath = root / "truncated-v3.drpkg";
        writeBytes(truncatedPath, std::vector<std::uint8_t>(16u, 0u));
        const auto malformed = engine::loadPerformancePackageV3Metadata(
            truncatedPath.generic_string(), activationSecurity);
        require(! malformed.loaded
                    && malformed.failure
                        == engine::PerformancePackageV3ActivationFailure::format
                    && facade.getStateRevision() == activeRevision,
                "Format rejection must retain last-known-good playback.");
        const auto missing = engine::loadPerformancePackageV3Metadata(
            (root / "missing-v3.drpkg").generic_string(), activationSecurity);
        require(! missing.loaded
                    && missing.failure == engine::PerformancePackageV3ActivationFailure::io
                    && facade.getStateRevision() == activeRevision,
                "I/O rejection must retain last-known-good playback.");
        const auto unconfiguredLoad = engine::loadPerformancePackageV3Metadata(
            first.packagePath, {});
        require(! unconfiguredLoad.loaded
                    && unconfiguredLoad.failure
                        == engine::PerformancePackageV3ActivationFailure::configuration
                    && facade.getStateRevision() == activeRevision,
                "Configuration rejection must retain last-known-good playback.");

        engine::SecureBuffer releaseKey;
        std::string keyIssue;
        require(security->keyProvider->resolvePackageKey(
                    opened.package.packageId, opened.package.encryptionKeyId,
                    engine::PackageKeyUse::decryptExistingPackage, releaseKey, keyIssue),
                "Corruption fixture release key could not be resolved.");
        engine::PackageV3WriteRequest corruptRequest;
        corruptRequest.packageId = opened.package.packageId;
        corruptRequest.compatibilityId = opened.package.compatibilityId;
        corruptRequest.encryptionKeyId = opened.package.encryptionKeyId;
        corruptRequest.releaseKey = &releaseKey;
        corruptRequest.signingKeyId = opened.package.signingKeyId;
        corruptRequest.publisherSigner = security->publisherSigner.get();
        for (const auto& descriptor : loadedV3.package->package.records)
        {
            const auto record = engine::openPackageV3FileRecord(
                *loadedV3.package, *loadedV3.contentKey, descriptor);
            require(record.opened, "Corruption fixture record could not be opened.");
            engine::PackageV3RecordInput input;
            input.recordId = descriptor.recordId;
            input.recordKind = descriptor.recordKind;
            input.generation = descriptor.generation;
            input.pageIndex = descriptor.pageIndex;
            input.plaintext = descriptor.recordKind == "manifest"
                ? std::vector<std::uint8_t> { '{' } : record.plaintext;
            corruptRequest.records.push_back(std::move(input));
        }
        const auto corruptWritten = engine::writePackageV3(corruptRequest);
        require(corruptWritten.written, "Signed corruption fixture could not be written.");
        const auto corruptPath = root / "corrupt-metadata-v3.drpkg";
        writeBytes(corruptPath, corruptWritten.packageBytes);
        const auto corrupt = engine::loadPerformancePackageV3Metadata(
            corruptPath.generic_string(), activationSecurity);
        require(! corrupt.loaded
                    && corrupt.failure
                        == engine::PerformancePackageV3ActivationFailure::corruption
                    && facade.getStateRevision() == activeRevision,
                "Authenticated metadata corruption must retain last-known-good playback.");

        plugin::Processor pluginProcessor;
        require(pluginProcessor.setPerformancePackageActivationSecurityContext(runtimeSecurity),
                "Plugin activation must accept the provisioned V3 reader context.");
        const auto pluginLoad = pluginProcessor.loadPerformancePackageWorkspace(
            juce::File(juce::String::fromUTF8(first.packagePath.c_str())));
        require(pluginLoad.loaded
                    && ! pluginProcessor.getWorkspaceDocumentState().authoringAvailable
                    && pluginProcessor.getWorkspaceDocumentState().playable,
                pluginLoad.issues.empty() ? pluginLoad.state : pluginLoad.issues.front());
        const auto pluginPayload = pluginProcessor.getEngineFacade()
            .getPerformancePackageActivationPayload();
        const auto pluginRevision = pluginProcessor.getEngineFacade().getStateRevision();
        const auto failedReplacement = pluginProcessor.loadPerformancePackageWorkspace(
            juce::File(juce::String::fromUTF8(tamperedPath.generic_string().c_str())));
        require(! failedReplacement.loaded
                    && pluginProcessor.getEngineFacade().getStateRevision() == pluginRevision
                    && pluginProcessor.getEngineFacade().getPerformancePackageActivationPayload()
                        == pluginPayload,
                "Plugin replacement failure must leave the active V3 generation untouched.");
        require(pluginProcessor.waitForHostStatePublication(),
                "Plugin V3 package binding did not reach host-state publication.");

        standalone::MainComponent standalone(false);
        require(standalone.getProcessor().setPerformancePackageActivationSecurityContext(
                    runtimeSecurity),
                "Standalone activation must accept the provisioned V3 reader context.");
        const auto standaloneLoad = standalone.getProcessor().loadPerformancePackageWorkspace(
            juce::File(juce::String::fromUTF8(first.packagePath.c_str())));
        require(standaloneLoad.loaded
                    && standalone.getProcessor().getWorkspaceDocumentState().playable
                    && ! standalone.getProcessor().getWorkspaceDocumentState().authoringAvailable,
                standaloneLoad.issues.empty() ? standaloneLoad.state
                                              : standaloneLoad.issues.front());

        juce::MemoryBlock savedHostState;
        pluginProcessor.getStateInformation(savedHostState);
        plugin::Processor restoredProcessor;
        require(restoredProcessor.setPerformancePackageActivationSecurityContext(runtimeSecurity),
                "Host restore must receive the provisioned V3 reader context.");
        restoredProcessor.setStateInformation(savedHostState.getData(),
                                              static_cast<int>(savedHostState.getSize()));
        const auto restoreDeadline = std::chrono::steady_clock::now()
            + std::chrono::seconds(10);
        while (std::chrono::steady_clock::now() < restoreDeadline)
        {
            restoredProcessor.serviceMessageThreadWork();
            const auto snapshot = restoredProcessor.getProjectRestoreSnapshot();
            if (snapshot != nullptr
                && (snapshot->state == engine::ProjectRestoreState::active
                    || snapshot->state == engine::ProjectRestoreState::failed))
                break;
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        const auto restored = restoredProcessor.getProjectRestoreSnapshot();
        require(restored != nullptr && restored->state == engine::ProjectRestoreState::active
                    && restoredProcessor.getEngineFacade()
                        .getPerformancePackageActivationPayload() != nullptr,
                restored == nullptr ? "V3 host restore did not publish a snapshot"
                                    : restored->message);
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
