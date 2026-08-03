#include "drs/engine/AuthoringSession.h"
#include "drs/engine/RuntimeLoader.h"
#include "drs/engine/VelocityCrossfadeAuthoring.h"

#include <iostream>
#include <stdexcept>

namespace
{
void require(const bool condition, const char* message)
{
    if (!condition)
        throw std::runtime_error(message);
}

drs::engine::RuntimeProjectModel makePairProject()
{
    const auto loaded = drs::engine::loadPhase2ReferenceProjectManifest();
    require(loaded.loaded, "Crossfade authoring fixture requires the Phase 2 project.");
    const auto curated = drs::engine::migrateRuntimeProjectToCuratedDspSchema(loaded.project);
    require(curated.valid, "Crossfade authoring fixture must migrate to the curated DSP schema.");
    const auto migrated = drs::engine::migrateRuntimeProjectToPerformanceArticulationSchema(curated.project);
    require(migrated.valid && migrated.project.authoring.zones.size() >= 2,
            "Crossfade authoring fixture must migrate to the current authoring schema.");

    auto project = migrated.project;
    auto& lower = project.authoring.zones[0];
    auto& upper = project.authoring.zones[1];
    upper.articulationId = lower.articulationId;
    upper.rootKey = lower.rootKey;
    upper.keyLow = lower.keyLow;
    upper.keyHigh = lower.keyHigh;
    upper.triggerMode = lower.triggerMode;
    lower.velocityLow = 1;
    lower.velocityHigh = 64;
    upper.velocityLow = 65;
    upper.velocityHigh = 127;
    lower.velocityCrossfade = {};
    upper.velocityCrossfade = {};
    lower.roundRobin.reset();
    upper.roundRobin.reset();
    lower.roundRobinLength = 0;
    lower.roundRobinPosition = 0;
    upper.roundRobinLength = 0;
    upper.roundRobinPosition = 0;
    // Preserve the reference project's routing and audition anchors, while
    // making every unrelated zone a different crossfade identity.
    for (std::size_t index = 2; index < project.authoring.zones.size(); ++index)
        project.authoring.zones[index].rootKey += 12;
    project.authoring.selectedZoneId = lower.id;
    const auto validation = drs::engine::validateRuntimeProjectModel(project);
    if (!validation.valid)
    {
        std::string issues;
        for (const auto& issue : validation.issues)
            issues += (issues.empty() ? "" : " | ") + issue;
        throw std::runtime_error("Crossfade authoring fixture must be a valid project: " + issues);
    }
    return project;
}
} // namespace

int main()
{
    using namespace drs::engine;
    try
    {
        auto project = makePairProject();
        const auto lowerId = project.authoring.zones[0].id;
        const auto upperId = project.authoring.zones[1].id;

        const auto partner = discoverVelocityCrossfadePartner(project, lowerId,
                                                               VelocityCrossfadeDirection::fadeOut);
        require(partner.eligible() && partner.partnerZoneIds.size() == 1
                    && partner.partnerZoneIds.front() == upperId,
                "Partner discovery should find the unique compatible adjacent upper layer.");

        AuthoringSession session(project);
        const VelocityCrossfadePairRequest create { lowerId, upperId, 60, 70 };
        const auto createResult = session.createVelocityCrossfadePair(create, "Create velocity crossfade");
        require(createResult.applied && createResult.changedPaths.size() == 2
                    && session.getDocumentState().undoDepth == 1,
                "Creating a pair must use one snapshot and report both changed zones.");
        const auto& createdLower = session.getProject().authoring.zones[0];
        const auto& createdUpper = session.getProject().authoring.zones[1];
        require(createdLower.velocityHigh == 70 && createdLower.velocityCrossfade.fadeOutLowVelocity == 60
                    && createdLower.velocityCrossfade.fadeOutHighVelocity == 70
                    && createdUpper.velocityLow == 60 && createdUpper.velocityCrossfade.fadeInLowVelocity == 60
                    && createdUpper.velocityCrossfade.fadeInHighVelocity == 70,
                "Pair creation must materialize mirrored descriptors and owned outer endpoints atomically.");

        const auto audition = planVelocityCrossfadeAudition(session.getProject(), lowerId, upperId);
        require(audition.valid() && audition.steps.size() == 5
                    && audition.steps[0].velocity == 59 && audition.steps[1].velocity == 60
                    && audition.steps[2].velocity == 65 && audition.steps[3].velocity == 70
                    && audition.steps[4].velocity == 71,
                "Crossfade audition must provide below, edge, midpoint, edge, and above velocities.");
        require(audition.steps[0].lowerGain == 1.0 && audition.steps[0].upperGain == 0.0
                    && audition.steps[2].lowerGain == 0.5 && audition.steps[2].upperGain == 0.5
                    && audition.steps[4].lowerGain == 0.0 && audition.steps[4].upperGain == 1.0,
                "Crossfade audition gains must use the runtime contribution law.");

        const VelocityCrossfadePairRequest resize { lowerId, upperId, 56, 76 };
        require(session.updateVelocityCrossfadePair(resize, "Resize velocity crossfade").applied,
                "A valid pair resize should commit through the authoring session.");
        require(session.getProject().authoring.zones[0].velocityHigh == 76
                    && session.getProject().authoring.zones[1].velocityLow == 56,
                "Pair resize should synchronize both velocity endpoints.");
        require(!session.updateVelocityCrossfadePair(resize, "Repeat velocity crossfade resize").applied
                    && session.getDocumentState().undoDepth == 2,
                "A no-op pair edit must not create an additional undo snapshot.");
        require(session.undo().applied && session.getProject().authoring.zones[0].velocityHigh == 70,
                "Undo should restore the previous complete crossfade pair.");
        require(session.redo().applied && session.getProject().authoring.zones[1].velocityLow == 56,
                "Redo should restore the resized complete crossfade pair.");

        auto rangeEdit = session.getZoneSummaries();
        rangeEdit[0].velocityHigh = 82;
        rangeEdit[1].velocityLow = 50;
        require(session.updateZoneRanges({ rangeEdit[0], rangeEdit[1] }, "Move linked crossfade endpoints").applied,
                "Ordinary range edits should route a complete crossfade through linked endpoint planning.");
        require(session.getProject().authoring.zones[0].velocityCrossfade.fadeOutLowVelocity == 50
                    && session.getProject().authoring.zones[0].velocityCrossfade.fadeOutHighVelocity == 82
                    && session.getProject().authoring.zones[1].velocityCrossfade.fadeInLowVelocity == 50
                    && session.getProject().authoring.zones[1].velocityCrossfade.fadeInHighVelocity == 82,
                "Linked range edits must keep both sides of the fade interval synchronized.");

        require(session.removeVelocityCrossfadePair(lowerId, upperId, "Remove velocity crossfade").applied,
                "Removing a pair should be one atomic authoring transaction.");
        require(!hasAnyVelocityCrossfadeValue(session.getProject().authoring.zones[0].velocityCrossfade)
                    && !hasAnyVelocityCrossfadeValue(session.getProject().authoring.zones[1].velocityCrossfade),
                "Pair removal must clear both sides rather than leaving a half relationship.");

        auto boundaryProject = makePairProject();
        AuthoringSession boundarySession(boundaryProject);
        require(boundarySession.createVelocityCrossfadePair({ lowerId, upperId, 1, 2 }, "Create low-edge fade").applied,
                "The lowest two-step velocity window should be accepted.");
        boundarySession = AuthoringSession(makePairProject());
        require(boundarySession.createVelocityCrossfadePair({ lowerId, upperId, 126, 127 }, "Create high-edge fade").applied,
                "The highest two-step velocity window should be accepted.");
        require(!boundarySession.createVelocityCrossfadePair({ lowerId, upperId, 70, 70 }, "Reject inverted fade").applied,
                "An inverted or one-step overlap must be rejected before it reaches document history.");

        auto ambiguousProject = makePairProject();
        auto alternateUpper = ambiguousProject.authoring.zones[1];
        alternateUpper.id += "-alternate";
        alternateUpper.velocityLow = 80;
        ambiguousProject.authoring.zones.push_back(alternateUpper);
        const auto ambiguous = discoverVelocityCrossfadePartner(ambiguousProject, lowerId,
                                                                 VelocityCrossfadeDirection::fadeOut);
        require(ambiguous.state == VelocityCrossfadeAuthoringState::ambiguousPartner,
                "Partner discovery must reject an ambiguous compatible layer instead of guessing.");

        auto incompatibleProject = makePairProject();
        incompatibleProject.authoring.zones[1].rootKey += 1;
        const auto incompatiblePlan = planVelocityCrossfadePair(incompatibleProject, create);
        require(incompatiblePlan.state == VelocityCrossfadeAuthoringState::incompatibleMapping,
                "Pair planning must reject mismatched playback identity.");

        auto incompleteRoundRobin = makePairProject();
        for (auto& zone : incompleteRoundRobin.authoring.zones)
        {
            zone.roundRobin = RoundRobinDescriptor { "rr-pool", 2, 1, RoundRobinMode::sequential };
            zone.roundRobinLength = 2;
            zone.roundRobinPosition = 1;
        }
        const auto incomplete = discoverVelocityCrossfadePartner(incompleteRoundRobin, lowerId,
                                                                  VelocityCrossfadeDirection::fadeOut);
        require(incomplete.state == VelocityCrossfadeAuthoringState::incompleteRoundRobinPool,
                "Partner discovery must expose incomplete Round Robin slot coverage.");

        std::cout << "Velocity crossfade authoring tests passed.\n";
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Velocity crossfade authoring tests failed: " << exception.what() << '\n';
        return 1;
    }
}
