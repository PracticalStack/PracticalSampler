#include "drs/engine/RuntimeVoice.h"
#include "drs/engine/VelocityCrossfade.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
void require(bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

drs::engine::RuntimeInstrumentModel makeSequentialRoundRobinInstrument()
{
    using namespace drs::engine;

    RuntimeInstrumentModel instrument;
    instrument.schemaName = "drs.instrument";
    instrument.schemaVersion = 1;
    instrument.instrumentId = "phase3.round-robin.contract";
    instrument.displayName = "Phase 3 Round Robin Contract";
    instrument.defaultLoadProfile = "balanced";

    RuntimeArticulationDefinition articulation;
    articulation.id = "sustain";
    articulation.name = "Sustain";
    articulation.isDefault = true;
    instrument.articulations.push_back(std::move(articulation));

    RuntimeGroupDefinition group;
    group.id = "rr-group";
    group.name = "RR Group";
    group.articulationIds = { "sustain" };
    instrument.groups.push_back(std::move(group));

    for (int slot = 1; slot <= 3; ++slot)
    {
        RuntimeZoneDefinition zone;
        zone.id = "rr-zone-" + std::to_string(slot);
        zone.groupId = "rr-group";
        zone.articulationId = "sustain";
        zone.samplePath = "Samples/rr-zone-" + std::to_string(slot) + ".flac";
        zone.streamAssetPath = "contract.drstrm";
        zone.rootKey = 60;
        zone.keyLow = 60;
        zone.keyHigh = 60;
        zone.velocityLow = 1;
        zone.velocityHigh = 127;
        zone.roundRobinLength = 3;
        zone.roundRobinPosition = slot;
        instrument.zones.push_back(std::move(zone));
    }

    return instrument;
}

drs::engine::RuntimeStreamContainerModel makeStreamContainerFor(
    const drs::engine::RuntimeInstrumentModel& instrument)
{
    using namespace drs::engine;

    RuntimeStreamContainerModel container;
    container.schemaName = "drs.stream";
    container.schemaVersion = 1;
    container.containerId = "phase3.round-robin.contract";
    container.pageSizeBytes = 16384;
    container.payloadEncoding = "float32le";

    for (std::size_t index = 0; index < instrument.zones.size(); ++index)
    {
        RuntimeStreamSampleDefinition sample;
        sample.sampleId = "sample-" + std::to_string(index + 1);
        sample.sourcePath = instrument.zones[index].samplePath;
        sample.sourceChecksumHex = "deadbeef";
        sample.formatName = "FLAC file";
        sample.role = "sustain";
        sample.channelLayout = "mono";
        sample.sampleRate = 48000.0;
        sample.frameCount = 1024;
        sample.channelCount = 1;
        sample.payloadOffsetBytes = static_cast<std::uint64_t>(index + 1) * 4096;
        sample.payloadSizeBytes = 4096;
        sample.prefetchBytes = 1024;
        container.samples.push_back(std::move(sample));
    }

    return container;
}

bool hasIssueForZone(const std::vector<drs::engine::VelocityCrossfadeTopologyFinding>& findings,
                     std::size_t zoneIndex,
                     drs::engine::VelocityCrossfadeTopologyIssue issue)
{
    for (const auto& finding : findings)
    {
        if (finding.zoneIndex == zoneIndex && finding.issue == issue)
            return true;
    }

    return false;
}

} // namespace

int main()
{
    using namespace drs::engine;

    try
    {
        const auto instrument = makeSequentialRoundRobinInstrument();
        const auto streamContainer = makeStreamContainerFor(instrument);

        require(instrument.zones.size() == 3,
                "Sprint 1 Round Robin contract fixture should expose three RR slots.");
        require(instrument.zones.front().roundRobinLength == 3
                    && instrument.zones.front().roundRobinPosition == 1
                    && instrument.zones.back().roundRobinPosition == 3,
                "Sprint 1 Round Robin slots must remain 1-based inside their declared length.");

        for (std::uint64_t voiceId = 1; voiceId <= 4; ++voiceId)
        {
            RuntimeVoiceAllocationRequest request;
            request.voiceId = voiceId;
            request.midiNote = 60;
            request.velocity = 100;
            request.articulationId = "sustain";

            const auto resolved = resolveRuntimeVoiceRoute(instrument, streamContainer, request);
            require(resolved.resolved, "Sequential RR route resolution should succeed for the canonical fixture.");

            const auto expectedZoneId = voiceId == 1 ? "rr-zone-1"
                : voiceId == 2 ? "rr-zone-2"
                : voiceId == 3 ? "rr-zone-3"
                               : "rr-zone-1";
            require(resolved.zoneId == expectedZoneId,
                    "Sequential RR route resolution changed unexpectedly for the canonical three-slot fixture.");
        }

        const std::vector<VelocityCrossfadeTopologyZoneDefinition> pairedZones {
            { 11u, 1, 60, 2, 1, { 0, 0, 25, 60, VelocityCrossfadeCurve::linear } },
            { 11u, 25, 84, 2, 1, { 25, 60, 0, 0, VelocityCrossfadeCurve::linear } },
            { 11u, 1, 60, 2, 2, { 0, 0, 25, 60, VelocityCrossfadeCurve::linear } },
            { 11u, 25, 84, 2, 2, { 25, 60, 0, 0, VelocityCrossfadeCurve::linear } }
        };
        std::vector<VelocityCrossfadeTopologyFinding> pairedFindings;
        const auto pairedTopology =
            buildFirstPassVelocityCrossfadeRuntimeTopology(pairedZones, &pairedFindings);

        require(pairedFindings.empty(),
                "Same-slot RR crossfade pairs should remain a valid supported topology.");
        require(pairedTopology[0].fadeOutNeighborZoneIndex == 1
                    && pairedTopology[1].fadeInNeighborZoneIndex == 0
                    && pairedTopology[2].fadeOutNeighborZoneIndex == 3
                    && pairedTopology[3].fadeInNeighborZoneIndex == 2,
                "RR-aware crossfade topology pairing should stay slot-local.");

        const std::vector<VelocityCrossfadeTopologyZoneDefinition> missingPartnerZones {
            { 11u, 1, 60, 2, 1, { 0, 0, 25, 60, VelocityCrossfadeCurve::linear } },
            { 11u, 25, 84, 2, 1, { 25, 60, 0, 0, VelocityCrossfadeCurve::linear } },
            { 11u, 1, 60, 2, 2, { 0, 0, 25, 60, VelocityCrossfadeCurve::linear } }
        };
        std::vector<VelocityCrossfadeTopologyFinding> missingPartnerFindings;
        const auto missingPartnerTopology =
            buildFirstPassVelocityCrossfadeRuntimeTopology(missingPartnerZones, &missingPartnerFindings);
        require(missingPartnerTopology[2].fadeOutNeighborZoneIndex < 0,
                "A missing RR-matched crossfade partner must not synthesize an unrelated neighbor.");
        require(hasIssueForZone(missingPartnerFindings, 2, VelocityCrossfadeTopologyIssue::fadeOutMissingPartner),
                "A missing RR-matched crossfade partner should remain a typed topology finding.");

        std::cout << "Phase 3 Round Robin Sprint 1 contract tests passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Phase 3 Round Robin Sprint 1 contract tests failed: "
                  << exception.what() << std::endl;
        return 1;
    }
}
