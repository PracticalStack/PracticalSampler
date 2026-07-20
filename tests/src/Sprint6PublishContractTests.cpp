#include "drs/engine/PerformancePublishContract.h"

#include <iostream>
#include <stdexcept>
#include <string>

namespace
{
void require(bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}
} // namespace

int main()
{
    using namespace drs::engine;
    using Preparation = PerformancePublishPreparationState;
    using VoiceEvent = PerformancePublishVoicePolicyEvent;
    using VoiceAction = PerformancePublishVoicePolicyAction;
    using MacroCase = PerformancePublishMacroMigrationCase;
    using MacroAction = PerformancePublishMacroMigrationAction;

    try
    {
        const PerformancePublishCommand command;
        require(command.type == PerformancePublishCommandType::publishCurrentDraft,
                "Sprint 6 must expose one intentional typed Publish command.");

        require(isPerformancePublishPreparationTransitionAllowed(Preparation::idle, Preparation::queued)
                    && isPerformancePublishPreparationTransitionAllowed(Preparation::queued, Preparation::preparing)
                    && isPerformancePublishPreparationTransitionAllowed(Preparation::preparing, Preparation::ready),
                "The ordinary Publish preparation lifecycle must remain executable.");
        require(isPerformancePublishPreparationTransitionAllowed(Preparation::queued, Preparation::canceled)
                    && isPerformancePublishPreparationTransitionAllowed(Preparation::queued, Preparation::superseded)
                    && isPerformancePublishPreparationTransitionAllowed(Preparation::preparing, Preparation::failed),
                "Queued/running Publish work must have explicit terminal outcomes.");
        require(!isPerformancePublishPreparationTransitionAllowed(Preparation::idle, Preparation::ready)
                    && !isPerformancePublishPreparationTransitionAllowed(Preparation::failed, Preparation::ready)
                    && !isPerformancePublishPreparationTransitionAllowed(Preparation::superseded, Preparation::preparing),
                "Publish work must not skip ownership or revive a terminal result.");

        const PerformancePublishRequestIdentity identity {
            21, 4, 3, 108, "authored-digest-108", "macro-schema-a"
        };
        auto differentRequest = identity;
        ++differentRequest.requestId;
        auto differentCancellation = identity;
        ++differentCancellation.cancellationGeneration;
        auto differentProject = identity;
        ++differentProject.projectGeneration;
        auto differentRevision = identity;
        ++differentRevision.draftRevision;
        auto differentContent = identity;
        differentContent.authoredContentDigest = "authored-digest-109";
        auto differentMacros = identity;
        differentMacros.macroSchemaDigest = "macro-schema-b";
        require(identity != differentRequest && identity != differentCancellation
                    && identity != differentProject && identity != differentRevision
                    && identity != differentContent && identity != differentMacros,
                "Publish identity must capture request, cancellation, project, revision, content, and macro schema.");

        PerformancePublishResult eligible;
        eligible.identity = identity;
        eligible.completeProject = true;
        eligible.activationEligible = true;
        eligible.preparedBuildId = 44;
        eligible.preparedContentDigest = "prepared-digest-108";
        eligible.routeDigest = "route-digest-108";
        eligible.sourceProvenanceDigest = "source-digest-108";
        eligible.preparedMacroSchemaDigest = identity.macroSchemaDigest;
        require(performancePublishResultIsEligible(identity, eligible)
                    && performancePublishCompletionDisposition(identity, eligible)
                        == PerformancePublishCompletionDisposition::stageEligiblePayload,
                "Only a complete exact-identity result may stage a Performance payload.");

        auto older = eligible;
        older.identity.requestId -= 1;
        auto partial = eligible;
        partial.completeProject = false;
        auto failed = eligible;
        failed.activationEligible = false;
        auto unprepared = eligible;
        unprepared.preparedBuildId = 0;
        auto macroMismatch = eligible;
        macroMismatch.preparedMacroSchemaDigest = "macro-schema-other";
        require(performancePublishCompletionDisposition(identity, older)
                        == PerformancePublishCompletionDisposition::preserveLastKnownGood
                    && performancePublishCompletionDisposition(identity, partial)
                        == PerformancePublishCompletionDisposition::preserveLastKnownGood
                    && performancePublishCompletionDisposition(identity, failed)
                        == PerformancePublishCompletionDisposition::preserveLastKnownGood
                    && performancePublishCompletionDisposition(identity, unprepared)
                        == PerformancePublishCompletionDisposition::preserveLastKnownGood
                    && performancePublishCompletionDisposition(identity, macroMismatch)
                        == PerformancePublishCompletionDisposition::preserveLastKnownGood,
                "Every stale, partial, failed, unprepared, or schema-mismatched result must preserve last-known-good.");

        require(performancePublishVoicePolicyFor(VoiceEvent::activationReplacement)
                        == VoiceAction::retainOriginalGeneration
                    && performancePublishVoicePolicyFor(VoiceEvent::existingVoiceNoteOff)
                        == VoiceAction::routeToOwningGeneration
                    && performancePublishVoicePolicyFor(VoiceEvent::retriggerAfterActivation)
                        == VoiceAction::useActiveGeneration
                    && performancePublishVoicePolicyFor(VoiceEvent::performanceStopAll)
                        == VoiceAction::releaseAllPerformanceGenerations,
                "The default voice-generation cutover policy must remain explicit.");

        require(performancePublishMacroMigrationFor(MacroCase::compatibleStableId)
                        == MacroAction::preserveAndClampByStableId
                    && performancePublishMacroMigrationFor(MacroCase::changedRange)
                        == MacroAction::preserveAndClampByStableId
                    && performancePublishMacroMigrationFor(MacroCase::addedStableId)
                        == MacroAction::useAuthoredDefault
                    && performancePublishMacroMigrationFor(MacroCase::removedStableId)
                        == MacroAction::retireBinding
                    && performancePublishMacroMigrationFor(MacroCase::reorderedStableId)
                        == MacroAction::preserveByStableId,
                "Published macro migration must use stable identity rather than list position.");

        std::cout << "Mini Sprint 6.1 Publish contract matrix passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Mini Sprint 6.1 Publish contract matrix failed: "
                  << exception.what() << std::endl;
        return 1;
    }
}
