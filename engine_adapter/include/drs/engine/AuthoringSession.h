#pragma once

#include "drs/engine/ProjectDocument.h"
#include "drs/engine/VelocityCrossfadeAuthoring.h"

#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace drs::engine
{
struct AuthoringZoneSummary
{
    std::string id;
    std::string displayName;
    std::string sampleSourceId;
    std::string articulationId;
    int rootKey = 60;
    int keyLow = 0;
    int keyHigh = 127;
    int velocityLow = 1;
    int velocityHigh = 127;
    VelocityCrossfadeDescriptor velocityCrossfade;
    double gainDb = 0.0;
    double pan = 0.0;
    bool loopEnabled = false;
    std::optional<RoundRobinDescriptor> roundRobin;
    int roundRobinLength = 0;
    int roundRobinPosition = 0;
    ZoneTriggerMode triggerMode = ZoneTriggerMode::gated;
    bool selected = false;
    bool additionallySelected = false;
};

struct AuthoringZonePreviewRequest
{
    bool available = false;
    int midiNote = 60;
    int velocity = 96;
    std::string zoneId;
    std::string articulationId;
    std::string state;
};

struct AuthoringGroupPreviewRequest
{
    bool available = false;
    int midiNote = 60;
    int velocity = 96;
    std::string groupId;
    std::string anchorZoneId;
    std::string state;
};

struct AuthoringGroupRoundRobinStatus
{
    bool enabled = false;
    bool eligible = false;
    RoundRobinMode mode = RoundRobinMode::sequential;
    std::vector<std::string> incompatibleZoneIds;
    std::string state;
};

struct AuthoringDspSelection
{
    std::string fxSlotId;
    std::string routingBusId;
};

struct AuthoringMixerTaperUpgradePreview
{
    std::vector<std::string> affectedMacroIds;
    std::vector<std::string> affectedTargetPaths;
};

class AuthoringSession
{
public:
    using DspParameterGesturePreviewListener = std::function<void(const std::string&, const std::string&, double)>;
    explicit AuthoringSession(RuntimeProjectModel project);

    const RuntimeProjectModel& getProject() const;
    const RuntimeProjectDocumentState& getDocumentState() const;
    RuntimeProjectDocumentCheckpoint exportCheckpoint() const;
    RuntimeProjectDocumentActionResult restoreCheckpoint(
        RuntimeProjectDocumentCheckpoint checkpoint,
        RuntimeProjectDocumentCheckpointConstraints constraints = {});
    void replaceProject(RuntimeProjectModel project);

    std::vector<AuthoringZoneSummary> getZoneSummaries() const;
    std::optional<RuntimeProjectZoneDefinition> getSelectedZone() const;
    std::vector<RuntimeProjectArticulationDefinition> getArticulations() const;
    std::optional<RuntimeProjectGroupDefinition> getSelectedGroup() const;
    std::optional<RuntimeProjectMacroDefinition> getSelectedMacro() const;
    std::optional<std::size_t> getSelectedMacroIndex() const;
    AuthoringGroupRoundRobinStatus getSelectedGroupRoundRobinStatus() const;
    AuthoringDspSelection getDspSelection() const;
    std::optional<RuntimeProjectPerformanceBankDefinition> getSelectedPerformanceBank() const;
    AuthoringZonePreviewRequest buildSelectedZonePreviewRequest() const;
    AuthoringGroupPreviewRequest buildSelectedGroupPreviewRequest() const;

    RuntimeProjectDocumentActionResult selectZone(const std::string& zoneId);
    RuntimeProjectDocumentActionResult selectGroup(const std::string& groupId);
    RuntimeProjectDocumentActionResult selectMacro(const std::string& macroId);
    RuntimeProjectDocumentActionResult selectPerformanceBank(const std::string& performanceBankId);
    RuntimeProjectDocumentActionResult selectDspSlot(const std::string& fxSlotId);
    RuntimeProjectDocumentActionResult updateSelectedZone(const RuntimeProjectZoneDefinition& zone,
                                                          const std::string& label);
    RuntimeProjectDocumentActionResult updateZoneRanges(
        const std::vector<AuthoringZoneSummary>& zones,
        const std::string& label);
    RuntimeProjectDocumentActionResult createVelocityCrossfadePair(
        const VelocityCrossfadePairRequest& request,
        const std::string& label);
    RuntimeProjectDocumentActionResult updateVelocityCrossfadePair(
        const VelocityCrossfadePairRequest& request,
        const std::string& label);
    RuntimeProjectDocumentActionResult removeVelocityCrossfadePair(
        const std::string& lowerZoneId,
        const std::string& upperZoneId,
        const std::string& label);
    RuntimeProjectDocumentActionResult createVelocityCrossfadeStack(
        const VelocityCrossfadeStackRequest& request,
        const std::string& label);
    RuntimeProjectDocumentActionResult removeVelocityCrossfadeStack(
        const std::vector<std::string>& zoneIds,
        const std::string& label);
    RuntimeProjectDocumentActionResult createArticulation(
        const RuntimeProjectArticulationDefinition& articulation,
        const std::string& label);
    RuntimeProjectDocumentActionResult updateArticulation(
        std::size_t articulationIndex,
        const RuntimeProjectArticulationDefinition& articulation,
        const std::string& label);
    RuntimeProjectDocumentActionResult moveArticulation(std::size_t articulationIndex,
                                                        int direction,
                                                        const std::string& label);
    RuntimeProjectDocumentActionResult setDefaultArticulation(const std::string& articulationId,
                                                              const std::string& label);
    RuntimeProjectDocumentActionResult deleteArticulation(const std::string& articulationId,
                                                          const std::string& reassignmentArticulationId,
                                                          const std::string& label);
    RuntimeProjectDocumentActionResult reassignZonesToArticulation(
        const std::vector<std::string>& zoneIds,
        const std::string& articulationId,
        const std::string& label);
    RuntimeProjectDocumentActionResult updateRoundRobinResetRules(
        std::vector<RuntimeProjectRoundRobinResetRuleDefinition> rules,
        const std::string& label);
    RuntimeProjectDocumentActionResult createGroup(const RuntimeProjectGroupDefinition& group,
                                                   const std::string& label);
    RuntimeProjectDocumentActionResult updateGroup(std::size_t groupIndex,
                                                   const RuntimeProjectGroupDefinition& group,
                                                   const std::string& label);
    RuntimeProjectDocumentActionResult moveGroup(std::size_t groupIndex,
                                                 int direction,
                                                 const std::string& label);
    RuntimeProjectDocumentActionResult deleteGroup(const std::string& groupId,
                                                   const std::string& label);
    RuntimeProjectDocumentActionResult reassignZoneToGroup(const std::string& zoneId,
                                                           const std::string& groupId,
                                                           const std::string& label);
    RuntimeProjectDocumentActionResult reassignZonesToGroup(const std::vector<std::string>& zoneIds,
                                                            const std::string& groupId,
                                                            const std::string& label);
    RuntimeProjectDocumentActionResult createRoundRobinPoolForSelectedZone(const std::string& label);
    RuntimeProjectDocumentActionResult addCompatibleZonesToSelectedRoundRobinPool(const std::string& label);
    RuntimeProjectDocumentActionResult normalizeSelectedRoundRobinPool(const std::string& label);
    RuntimeProjectDocumentActionResult removeSelectedZoneFromRoundRobinPool(const std::string& label);
    // Compatibility aliases retained for existing callers; group operations now apply to every member.
    RuntimeProjectDocumentActionResult createRoundRobinPoolForSelectedGroup(const std::string& label);
    RuntimeProjectDocumentActionResult addCompatibleZonesToSelectedGroupRoundRobinPool(const std::string& label);
    RuntimeProjectDocumentActionResult normalizeSelectedGroupRoundRobinPool(const std::string& label);
    RuntimeProjectDocumentActionResult removeSelectedGroupAnchorFromRoundRobinPool(const std::string& label);
    RuntimeProjectDocumentActionResult setSelectedGroupRoundRobinEnabled(bool enabled,
                                                                         RoundRobinMode mode,
                                                                         const std::string& label);
    RuntimeProjectDocumentActionResult setSelectedGroupRoundRobinMode(RoundRobinMode mode,
                                                                      const std::string& label);
    RuntimeProjectDocumentActionResult deleteZones(const std::vector<std::string>& zoneIds,
                                                   const std::string& label);
    RuntimeProjectDocumentActionResult deleteSelectedSample();
    RuntimeProjectDocumentActionResult appendImportedContent(std::vector<RuntimeProjectSampleSource> sampleSources,
                                                            std::vector<RuntimeProjectZoneDefinition> zones,
                                                            const std::string& label);
    RuntimeProjectDocumentActionResult appendImportedContent(std::vector<RuntimeProjectSampleSource> sampleSources,
                                                            std::vector<RuntimeProjectZoneDefinition> zones,
                                                            std::vector<std::string> projectNotes,
                                                            std::vector<std::string> authoringNotes,
                                                            const std::string& label,
                                                            bool reconcileInferredRoundRobin = true);
    RuntimeProjectDocumentActionResult createMacro(const RuntimeProjectMacroDefinition& macro,
                                                   const std::string& label);
    RuntimeProjectDocumentActionResult duplicateMacro(const std::string& macroId,
                                                      const std::string& label);
    RuntimeProjectDocumentActionResult deleteMacro(const std::string& macroId,
                                                   const std::string& label);
    RuntimeProjectDocumentActionResult updateMacro(std::size_t macroIndex,
                                                   const RuntimeProjectMacroDefinition& macro,
                                                   const std::string& label);
    RuntimeProjectDocumentActionResult moveMacro(std::size_t macroIndex,
                                                 int direction,
                                                 const std::string& label);
    AuthoringMixerTaperUpgradePreview previewMixerTaperUpgrade() const;
    RuntimeProjectDocumentActionResult upgradeMixerTaper(const std::string& label);
    RuntimeProjectDocumentActionResult createFxSlot(const RuntimeProjectFxSlotDefinition& fxSlot,
                                                    const std::string& ownerBusId,
                                                    const std::string& label);
    RuntimeProjectDocumentActionResult duplicateFxSlot(const std::string& fxSlotId,
                                                       const std::string& duplicateId,
                                                       const std::string& label);
    RuntimeProjectDocumentActionResult deleteFxSlot(const std::string& fxSlotId,
                                                    const std::string& label);
    RuntimeProjectDocumentActionResult moveFxSlot(const std::string& fxSlotId,
                                                  int direction,
                                                  const std::string& label);
    RuntimeProjectDocumentActionResult moveFxSlotToBus(const std::string& fxSlotId,
                                                        const std::string& destinationBusId,
                                                        const std::string& label);
    RuntimeProjectDocumentActionResult createRoutingBus(const RuntimeProjectRoutingBusDefinition& routingBus,
                                                         const std::string& label);
    RuntimeProjectDocumentActionResult deleteRoutingBus(const std::string& busId,
                                                         const std::string& label);
    RuntimeProjectDocumentActionResult setRoutingBusChainBypassed(const std::string& busId,
                                                                   bool bypassed,
                                                                   const std::string& label);
    RuntimeProjectDocumentActionResult setFxSlotParameter(const std::string& fxSlotId,
                                                           const std::string& parameterId,
                                                           double value,
                                                           const std::string& label);
    RuntimeProjectDocumentActionResult resetFxSlotParameterToDefault(const std::string& fxSlotId,
                                                                      const std::string& parameterId,
                                                                      const std::string& label);
    RuntimeProjectDocumentActionResult beginFxSlotParameterGesture(const std::string& fxSlotId,
                                                                    const std::string& parameterId);
    RuntimeProjectDocumentActionResult updateFxSlotParameterGesture(double value);
    void setDspParameterGesturePreviewListener(DspParameterGesturePreviewListener listener)
    {
        dspParameterGesturePreviewListener = std::move(listener);
    }
    RuntimeProjectDocumentActionResult commitFxSlotParameterGesture(const std::string& label);
    RuntimeProjectDocumentActionResult cancelFxSlotParameterGesture();
    RuntimeProjectDocumentActionResult updateFxSlot(std::size_t fxSlotIndex,
                                                    const RuntimeProjectFxSlotDefinition& fxSlot,
                                                    const std::string& label);
    RuntimeProjectDocumentActionResult updateRoutingBus(std::size_t routingBusIndex,
                                                        const RuntimeProjectRoutingBusDefinition& routingBus,
                                                        const std::string& label);
    RuntimeProjectDocumentActionResult updatePerformanceBank(std::size_t performanceBankIndex,
                                                             const RuntimeProjectPerformanceBankDefinition& performanceBank,
                                                             const std::string& label);
    RuntimeProjectDocumentActionResult undo();
    RuntimeProjectDocumentActionResult redo();
    RuntimeProjectDocumentActionResult applyProjectMigration(RuntimeProjectModel migratedProject);
    void markSaved();

private:
    struct PendingDspParameterGesture
    {
        std::string fxSlotId;
        std::string parameterId;
        double value = 0.0;
    };

    RuntimeProjectDocumentController documentController;
    std::optional<PendingDspParameterGesture> pendingDspParameterGesture;
    DspParameterGesturePreviewListener dspParameterGesturePreviewListener;
    AuthoringDspSelection dspSelection;
    std::string selectedMacroId;

    void recoverDspSelection();
    void recoverMacroSelection();
};
} // namespace drs::engine
