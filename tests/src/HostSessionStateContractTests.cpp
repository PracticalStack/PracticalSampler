#include "drs/engine/HostSessionState.h"
#include "drs/engine/RuntimeLoader.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

namespace
{
namespace fs = std::filesystem;

void require(const bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

std::string readTextFile(const fs::path& path)
{
    // Golden JSON is canonicalized with LF by the codec. Text mode keeps this
    // byte-for-byte assertion independent of the checkout's Windows EOL policy.
    std::ifstream input(path);
    if (!input)
        return {};

    return std::string(std::istreambuf_iterator<char>(input),
                       std::istreambuf_iterator<char>());
}

bool containsFinding(const drs::engine::HostSessionStateParseResult& result,
                     const drs::engine::HostSessionStateFindingCode code)
{
    for (const auto& finding : result.findings)
    {
        if (finding.code == code)
            return true;
    }

    return false;
}

std::string replaceFirst(std::string text,
                         const std::string& from,
                         const std::string& to)
{
    const auto position = text.find(from);
    if (position == std::string::npos)
        throw std::runtime_error("Test mutation source was not found: " + from);

    text.replace(position, from.size(), to);
    return text;
}

} // namespace

int main()
{
    try
    {
        static_assert(drs::engine::hostSessionStateMaxBytes == 16u * 1024u * 1024u);
        static_assert(drs::engine::hostSessionStateMaxProjectSnapshotBytes == 15u * 1024u * 1024u);
        static_assert(drs::engine::hostSessionStateMaxJsonDepth == 64u);
        static_assert(drs::engine::hostSessionStateMaxStringBytes == 64u * 1024u);
        static_assert(drs::engine::hostSessionStateMaxPathBytes == 32u * 1024u);
        static_assert(drs::engine::hostSessionStateMaxIdentityBytes == 512u);
        static_assert(drs::engine::hostSessionStateMaxSampleSources == 8192u);
        static_assert(drs::engine::hostSessionStateMaxZones == 65536u);
        static_assert(drs::engine::hostSessionStateMaxGroups == 2048u);
        static_assert(drs::engine::hostSessionStateMaxMacros == 128u);
        static_assert(drs::engine::hostSessionStateMaxFxSlots == 128u);
        static_assert(drs::engine::hostSessionStateMaxRoutingBuses == 128u);
        static_assert(drs::engine::hostSessionStateMaxPerformanceBanks == 256u);
        static_assert(drs::engine::hostSessionStateMaxNotesOrIssues == 4096u);
        static_assert(drs::engine::HostSessionStateParseDisposition::absent
                      != drs::engine::HostSessionStateParseDisposition::invalid);
        static_assert(drs::engine::HostSessionStateParseDisposition::legacyPreset
                      != drs::engine::HostSessionStateParseDisposition::valid);

        drs::engine::HostSessionStateParseResult absent;
        if (absent.disposition != drs::engine::HostSessionStateParseDisposition::absent
            || absent.hostState.has_value()
            || absent.legacyPreset.has_value())
        {
            throw std::runtime_error("The default parse result must represent absent input.");
        }

        const auto root = fs::path(DRS_SOURCE_ROOT);
        const auto headerPath = root / "engine_adapter/include/drs/engine/HostSessionState.h";
        const auto sourcePath = root / "engine_adapter/src/HostSessionState.cpp";
        const auto header = readTextFile(headerPath);
        const auto source = readTextFile(sourcePath);

        if (header.empty() || source.empty())
        {
            std::cerr
                << "EXPECTED RED: the drs.hostState codec does not exist. "
                << "HostSessionState.h/.cpp must define the typed envelope and strict codec."
                << std::endl;
            return 1;
        }

        const auto hasSchema = header.find("drs.hostState") != std::string::npos
            || source.find("drs.hostState") != std::string::npos;
        const auto hasParser = header.find("parseHostSessionState") != std::string::npos;
        const auto hasSerializer = header.find("serializeHostSessionState") != std::string::npos;
        const auto hasBudget = header.find("hostSessionStateMaxBytes") != std::string::npos;

        if (!hasSchema || !hasParser || !hasSerializer || !hasBudget)
        {
            std::cerr
                << "EXPECTED RED: the host-state codec contract is incomplete "
                << "(schema=" << hasSchema
                << ", parser=" << hasParser
                << ", serializer=" << hasSerializer
                << ", budget=" << hasBudget << ")."
                << std::endl;
            return 1;
        }

        const auto fixtureRoot = root / "content/runtime/phase1/host-state";
        const auto savedText = readTextFile(fixtureRoot / "reference/saved-project.hoststate.json");
        const auto dirtyText = readTextFile(fixtureRoot / "reference/dirty-project.hoststate.json");
        const auto legacyText = readTextFile(fixtureRoot / "legacy/lead-performance.preset-state.json");
        const auto unknownText = readTextFile(fixtureRoot / "negative/unknown-version.hoststate.json");
        const auto corruptText = readTextFile(fixtureRoot / "negative/corrupt.hoststate.json");
        const auto mismatchText = readTextFile(fixtureRoot / "negative/identity-mismatch.hoststate.json");
        const auto budgetDescriptor = readTextFile(fixtureRoot / "negative/over-budget.fixture.json");

        const auto saved = drs::engine::parseHostSessionState(savedText);
        require(saved.isValidHostState(), "Saved-project golden fixture must parse as valid host state.");
        require(saved.hostState->projectBinding.projectId == "drs.phase2.authoring-foundation",
                "Saved-project binding identity changed.");
        require(!saved.hostState->authoringState.dirty
                    && !saved.hostState->authoringState.projectSnapshot.has_value(),
                "Clean saved fixture must not embed a project snapshot.");
        require(saved.hostState->publishedState.has_value(),
                "Saved fixture must cover the complete published-state object.");

        const auto savedSerialized = drs::engine::serializeHostSessionState(*saved.hostState);
        require(savedSerialized.serialized, "Saved-project fixture must serialize.");
        require(savedSerialized.text == savedText,
                "Saved-project golden fixture must round-trip byte-for-byte.");
        const auto savedAgain = drs::engine::parseHostSessionState(savedSerialized.text);
        require(savedAgain.isValidHostState(), "Serialized saved state must parse again.");
        const auto savedSerializedAgain = drs::engine::serializeHostSessionState(*savedAgain.hostState);
        require(savedSerializedAgain.serialized && savedSerializedAgain.text == savedSerialized.text,
                "Host-state serialization must be deterministic.");

        const auto dirty = drs::engine::parseHostSessionState(dirtyText);
        require(dirty.isValidHostState(),
                "Dirty-project golden fixture must parse as valid host state"
                    + (dirty.findings.empty()
                           ? std::string(".")
                           : std::string(": ") + dirty.findings.front().path + " "
                               + dirty.findings.front().message));
        require(dirty.hostState->authoringState.dirty
                    && dirty.hostState->authoringState.projectSnapshot.has_value(),
                "Dirty fixture must restore its embedded project snapshot.");
        require(dirty.hostState->authoringState.projectSnapshot->projectId
                    == dirty.hostState->projectBinding.projectId,
                "Dirty snapshot and binding IDs must match.");
        const auto dirtySerialized = drs::engine::serializeHostSessionState(*dirty.hostState);
        require(dirtySerialized.serialized && dirtySerialized.text == dirtyText,
                "Dirty-project golden fixture must round-trip byte-for-byte.");

        const auto curatedDspFixtureText = readTextFile(
            fs::path(DRS_CURATED_DSP_FIXTURE_ROOT) / "valid-all-scopes.json");
        const auto curatedDspFixture = drs::engine::parseRuntimeProjectManifest(
            curatedDspFixtureText, "curated-dsp-valid-all-scopes.drsproj", false);
        require(curatedDspFixture.loaded,
                "The curated DSP all-scopes fixture must load before host-state embedding.");

        auto curatedDspHostState = *dirty.hostState;
        curatedDspHostState.projectBinding.projectId = curatedDspFixture.project.projectId;
        curatedDspHostState.projectBinding.manifestPath = "curated-dsp-valid-all-scopes.drsproj";
        curatedDspHostState.projectBinding.manifestFileName = "curated-dsp-valid-all-scopes.drsproj";
        curatedDspHostState.projectBinding.manifestDigest = drs::engine::computeHostProjectManifestDigest(
            curatedDspFixture.project, curatedDspHostState.projectBinding.manifestPath);
        curatedDspHostState.authoringState.projectSnapshot = curatedDspFixture.project;
        const auto curatedDspHostSerialized = drs::engine::serializeHostSessionState(curatedDspHostState);
        require(curatedDspHostSerialized.serialized,
                "The all-scopes curated DSP fixture must survive host-state serialization.");
        const auto curatedDspHostRestored = drs::engine::parseHostSessionState(
            curatedDspHostSerialized.text);
        require(curatedDspHostRestored.isValidHostState()
                    && curatedDspHostRestored.hostState->authoringState.projectSnapshot->authoring.fxSlots[1].unavailable,
                "Host-state restore must preserve an unavailable unknown DSP effect without rejecting the project.");

        auto overBudgetDspHostState = curatedDspHostState;
        overBudgetDspHostState.authoringState.projectSnapshot->authoring.fxSlots.front().parameters.assign(
            drs::engine::hostSessionStateMaxDspParameters + 1u, { "over-budget", 0.0 });
        const auto overBudgetDspHostSerialized = drs::engine::serializeHostSessionState(overBudgetDspHostState);
        require(!overBudgetDspHostSerialized.serialized,
                "Host-state serialization must reject DSP parameter state above its 1,024-item limit.");

        const auto legacy = drs::engine::parseHostSessionState(legacyText);
        require(legacy.isLegacyPreset(), "Raw preset-state v1 fixture must be classified as legacy.");
        require(!legacy.hostState.has_value(),
                "Legacy preset classification must not invent a project-bound host state.");

        const auto unknown = drs::engine::parseHostSessionState(unknownText);
        require(unknown.disposition == drs::engine::HostSessionStateParseDisposition::invalid
                    && containsFinding(unknown,
                                       drs::engine::HostSessionStateFindingCode::schemaVersionUnsupported),
                "Unknown host-state version must fail with a version finding.");

        const auto corrupt = drs::engine::parseHostSessionState(corruptText);
        require(corrupt.disposition == drs::engine::HostSessionStateParseDisposition::invalid
                    && containsFinding(corrupt, drs::engine::HostSessionStateFindingCode::jsonInvalid),
                "Corrupt host-state JSON must fail with a JSON finding.");

        const auto mismatch = drs::engine::parseHostSessionState(mismatchText);
        require(mismatch.disposition == drs::engine::HostSessionStateParseDisposition::invalid
                    && containsFinding(mismatch,
                                       drs::engine::HostSessionStateFindingCode::projectIdentityMismatch),
                "Binding/snapshot project ID mismatch must fail explicitly.");

        const auto missingRequired = drs::engine::parseHostSessionState(
            replaceFirst(savedText, "\"manifestFileName\"", "\"unexpectedManifestFileName\""));
        require(missingRequired.disposition == drs::engine::HostSessionStateParseDisposition::invalid
                    && containsFinding(missingRequired,
                                       drs::engine::HostSessionStateFindingCode::requiredFieldMissing)
                    && containsFinding(missingRequired,
                                       drs::engine::HostSessionStateFindingCode::unknownField),
                "Missing required and unknown fields must both be reported.");

        const auto wrongType = drs::engine::parseHostSessionState(
            replaceFirst(savedText, "\"revision\": 14", "\"revision\": \"fourteen\""));
        require(wrongType.disposition == drs::engine::HostSessionStateParseDisposition::invalid
                    && containsFinding(wrongType,
                                       drs::engine::HostSessionStateFindingCode::fieldTypeInvalid),
                "Wrong required-field type must be rejected.");

        require(budgetDescriptor.find("\"targetUtf8Bytes\": 16777217") != std::string::npos,
                "Over-budget fixture descriptor must target exactly one byte above 16 MiB.");
        const auto oversized = drs::engine::parseHostSessionState(
            std::string(drs::engine::hostSessionStateMaxBytes + 1u, 'x'));
        require(oversized.disposition == drs::engine::HostSessionStateParseDisposition::invalid
                    && containsFinding(oversized,
                                       drs::engine::HostSessionStateFindingCode::payloadTooLarge),
                "A payload one byte above the host-state limit must fail before JSON parsing.");

        std::string tooDeep;
        tooDeep.append(drs::engine::hostSessionStateMaxJsonDepth + 1u, '[');
        tooDeep += "0";
        tooDeep.append(drs::engine::hostSessionStateMaxJsonDepth + 1u, ']');
        const auto depthRejected = drs::engine::parseHostSessionState(tooDeep);
        require(containsFinding(depthRejected,
                                drs::engine::HostSessionStateFindingCode::jsonDepthExceeded),
                "JSON nesting above 64 must be rejected before DOM expansion.");

        const auto originalPath = std::string("C:/Fixtures/DecentRhapsody/phase2-authoring-foundation.drsproj");
        const auto maxPathText = replaceFirst(
            savedText, originalPath, std::string(drs::engine::hostSessionStateMaxPathBytes, 'p'));
        require(drs::engine::parseHostSessionState(maxPathText).isValidHostState(),
                "A path exactly at the byte limit must remain valid.");
        const auto longPathText = replaceFirst(
            savedText, originalPath, std::string(drs::engine::hostSessionStateMaxPathBytes + 1u, 'p'));
        const auto longPath = drs::engine::parseHostSessionState(longPathText);
        require(containsFinding(longPath, drs::engine::HostSessionStateFindingCode::stringTooLong),
                "A path one byte above the path limit must fail.");

        const auto originalProjectId = std::string("drs.phase2.authoring-foundation");
        const auto maxIdentityText = replaceFirst(
            savedText, originalProjectId,
            std::string(drs::engine::hostSessionStateMaxIdentityBytes, 'i'));
        require(drs::engine::parseHostSessionState(maxIdentityText).isValidHostState(),
                "An identity exactly at the byte limit must remain valid.");
        const auto longIdentity = drs::engine::parseHostSessionState(
            replaceFirst(savedText,
                         originalProjectId,
                         std::string(drs::engine::hostSessionStateMaxIdentityBytes + 1u, 'i')));
        require(containsFinding(longIdentity, drs::engine::HostSessionStateFindingCode::stringTooLong),
                "An identity one byte above the identity limit must fail.");

        const auto longGenericString = drs::engine::parseHostSessionState(
            replaceFirst(savedText,
                         "Host-state fixture preserving the existing strict Phase 1 preset payload.",
                         std::string(drs::engine::hostSessionStateMaxStringBytes + 1u, 's')));
        require(containsFinding(longGenericString,
                                drs::engine::HostSessionStateFindingCode::stringTooLong),
                "A generic string above 64 KiB must fail.");

        std::string excessiveMacros = "\"macros\": [";
        for (std::size_t index = 0; index < drs::engine::hostSessionStateMaxMacros + 1u; ++index)
        {
            if (index != 0)
                excessiveMacros += ",";
            excessiveMacros += "{}";
        }
        excessiveMacros += "]";
        const auto collectionRejected = drs::engine::parseHostSessionState(
            replaceFirst(dirtyText, "\"macros\": []", excessiveMacros));
        require(containsFinding(collectionRejected,
                                drs::engine::HostSessionStateFindingCode::collectionTooLarge),
                "A project collection above its declared limit must fail before project expansion.");

        std::string largeNotes = "\"notes\": [";
        for (std::size_t index = 0; index < 3850u; ++index)
        {
            if (index != 0)
                largeNotes += ",";
            largeNotes += "\"" + std::string(4100u, 'n') + "\"";
        }
        largeNotes += "]";
        const auto largeSnapshotText = replaceFirst(
            dirtyText,
            "\"notes\": [\n"
            "          \"An unsaved authoring note that must survive DAW recall.\"\n"
            "        ]",
            largeNotes);
        require(largeSnapshotText.size() < drs::engine::hostSessionStateMaxBytes,
                "Project-snapshot budget fixture must remain below the total payload limit.");
        const auto snapshotRejected = drs::engine::parseHostSessionState(largeSnapshotText);
        require(containsFinding(snapshotRejected,
                                drs::engine::HostSessionStateFindingCode::projectSnapshotTooLarge),
                "An embedded snapshot above 15 MiB must fail before project parsing.");

        auto invalidForSerialization = *saved.hostState;
        invalidForSerialization.projectBinding.manifestPath.assign(
            drs::engine::hostSessionStateMaxPathBytes + 1u, 'p');
        const auto serializationRejected = drs::engine::serializeHostSessionState(
            invalidForSerialization);
        require(!serializationRejected.serialized
                    && !serializationRejected.findings.empty(),
                "Serialization must enforce the same structural budgets as parsing.");

        const auto referenceProjectText = readTextFile(
            root / "content/runtime/phase2/authoring-foundation/reference-project/"
                   "phase2-authoring-foundation.drsproj");
        const std::string firstLocation = "C:/RelocationA/Project/phase2-authoring-foundation.drsproj";
        const std::string secondLocation = "D:/RelocationB/Project/phase2-authoring-foundation.drsproj";
        const auto firstProject = drs::engine::parseRuntimeProjectManifest(
            referenceProjectText, firstLocation, false);
        const auto secondProject = drs::engine::parseRuntimeProjectManifest(
            referenceProjectText, secondLocation, false);
        require(firstProject.loaded && secondProject.loaded,
                "Canonical digest test projects must parse without referenced-path validation.");

        const auto firstDigest = drs::engine::computeHostProjectManifestDigest(
            firstProject.project, firstLocation);
        const auto secondDigest = drs::engine::computeHostProjectManifestDigest(
            secondProject.project, secondLocation);
        require(firstDigest == "fnv1a64:cd3ad5a57f2ba52f"
                    && secondDigest == firstDigest,
                "Equivalent relocated project models must produce the checked-in canonical digest; first="
                    + firstDigest + ", second=" + secondDigest + ".");

        const auto schemaFive = drs::engine::migrateRuntimeProjectToCuratedDspSchema(
            firstProject.project);
        require(schemaFive.valid, "The legacy digest fixture must migrate to project schema 5.");
        const auto schemaSix = drs::engine::migrateRuntimeProjectToPerformanceArticulationSchema(
            schemaFive.project);
        require(schemaSix.valid, "The legacy digest fixture must migrate to project schema 6.");
        const auto beforeDamperMigration = drs::engine::computeHostProjectManifestDigest(
            schemaSix.project, firstLocation);
        const auto schemaSeven = drs::engine::migrateRuntimeProjectToContinuousDamperSchema(
            schemaSix.project);
        require(schemaSeven.valid,
                "The legacy digest fixture must migrate to project schema 7.");
        const auto afterDamperMigration = drs::engine::computeHostProjectManifestDigest(
            schemaSeven.project, firstLocation);
        require(afterDamperMigration == beforeDamperMigration,
                "Binary-default damper migration must not invalidate an existing DAW binding.");

        const auto schemaEight = drs::engine::migrateRuntimeProjectToPlaybackRegionSchema(
            schemaSeven.project);
        require(schemaEight.valid,
                "The legacy digest fixture must migrate to playback-region project schema 8.");
        const auto afterPlaybackRegionMigration = drs::engine::computeHostProjectManifestDigest(
            schemaEight.project, firstLocation);
        require(afterPlaybackRegionMigration == beforeDamperMigration,
                "A zero playback-end sentinel must not invalidate an existing DAW binding.");

        auto authoredPlaybackRegion = schemaEight.project;
        authoredPlaybackRegion.schemaVersion = drs::engine::loopCrossfadeProjectSchemaVersion;
        authoredPlaybackRegion.authoring.schemaVersion
            = drs::engine::loopCrossfadeAuthoringSchemaVersion;
        authoredPlaybackRegion.authoring.zones.front().sampleStartFrame = 8;
        authoredPlaybackRegion.authoring.zones.front().sampleEndFrame = 100;
        authoredPlaybackRegion.authoring.zones.front().loopEnabled = true;
        authoredPlaybackRegion.authoring.zones.front().loopMode
            = drs::engine::RegionLoopMode::loopSustain;
        authoredPlaybackRegion.authoring.zones.front().loopStartFrame = 24;
        authoredPlaybackRegion.authoring.zones.front().loopEndFrame = 80;
        authoredPlaybackRegion.authoring.zones.front().loopCrossfadeFrames = 12;
        require(drs::engine::computeHostProjectManifestDigest(
                    authoredPlaybackRegion, firstLocation) != afterPlaybackRegionMigration,
                "An authored playback end must participate in host binding identity.");

        auto regionHostState = *dirty.hostState;
        regionHostState.projectBinding.projectId = authoredPlaybackRegion.projectId;
        regionHostState.projectBinding.manifestPath = firstLocation;
        regionHostState.projectBinding.manifestFileName
            = fs::path(firstLocation).filename().generic_string();
        regionHostState.projectBinding.manifestDigest
            = drs::engine::computeHostProjectManifestDigest(authoredPlaybackRegion, firstLocation);
        regionHostState.authoringState.projectSnapshot = authoredPlaybackRegion;
        const auto regionHostSerialized = drs::engine::serializeHostSessionState(regionHostState);
        require(regionHostSerialized.serialized,
                "An authored SFZ playback region must survive host-state serialization.");
        const auto regionHostRestored = drs::engine::parseHostSessionState(regionHostSerialized.text);
        require(regionHostRestored.isValidHostState()
                    && regionHostRestored.hostState->authoringState.projectSnapshot.has_value(),
                "An authored SFZ playback region must survive host-state restoration.");
        const auto& restoredRegion = regionHostRestored.hostState->authoringState
            .projectSnapshot->authoring.zones.front();
        require(restoredRegion.sampleStartFrame == 8
                    && restoredRegion.sampleEndFrame == 100
                    && restoredRegion.loopEnabled
                    && restoredRegion.loopMode == drs::engine::RegionLoopMode::loopSustain
                    && restoredRegion.loopStartFrame == 24
                    && restoredRegion.loopEndFrame == 80
                    && restoredRegion.loopCrossfadeFrames == 12,
                "Host state must preserve the complete typed SFZ region contract.");

        auto authoredDamper = schemaSeven.project;
        authoredDamper.authoring.zones.front().damper.sustainControllerNumber = 90;
        require(drs::engine::computeHostProjectManifestDigest(authoredDamper, firstLocation)
                    != beforeDamperMigration,
                "Authored non-default damper metadata must participate in host binding identity.");

        const auto matchingBinding = drs::engine::verifyHostProjectBinding(
            saved.hostState->projectBinding, firstProject.project, firstLocation);
        require(matchingBinding.matched()
                    && matchingBinding.actualManifestDigest == firstDigest,
                "Canonical project ID and manifest digest must verify a matching binding.");

        auto changedContentProject = firstProject.project;
        changedContentProject.displayName += " Changed";
        const auto changedContent = drs::engine::verifyHostProjectBinding(
            saved.hostState->projectBinding, changedContentProject, firstLocation);
        require(changedContent.match
                    == drs::engine::HostProjectBindingMatch::manifestDigestMismatch
                    && changedContent.expectedProjectId == changedContent.actualProjectId,
                "Same project ID with changed authored content must be a digest mismatch.");

        auto wrongIdentityProject = firstProject.project;
        wrongIdentityProject.projectId = "drs.host-state.wrong-project";
        const auto wrongIdentity = drs::engine::verifyHostProjectBinding(
            saved.hostState->projectBinding, wrongIdentityProject, firstLocation);
        require(wrongIdentity.match
                    == drs::engine::HostProjectBindingMatch::projectIdentityMismatch,
                "Changed project identity must be distinct from a content digest mismatch.");

        std::cout << "Host-session state codec contract tests passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Host-session state contract tests failed: "
                  << exception.what() << std::endl;
        return 2;
    }
}
