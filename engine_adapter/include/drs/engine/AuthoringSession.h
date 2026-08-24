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
    std::string groupId;
    int rootKey = 60;
    int keyLow = 0;
    int keyHigh = 127;
    int velocityLow = 1;
    int velocityHigh = 127;
    VelocityCrossfadeDescriptor velocityCrossfade;
    double gainDb = 0.0;
    double pan = 0.0;
    bool loopEnabled = false;
    RegionLoopMode loopMode = RegionLoopMode::noLoop;
    std::uint64_t sampleEndFrame = 0;
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

enum class AuthoringStructureEntityKind
{
    layer,
    group,
    zone
};

// Optional fields form an all-or-nothing patch for a same-kind stable-ID
// selection. The session validates every target before committing one
// document snapshot, so mixed edits cannot partially apply.
struct AuthoringStructureBatchPatch
{
    std::optional<std::string> displayName;
    std::optional<std::string> layerId;
    std::optional<std::string> groupId;
    std::optional<std::string> routingBusId;
    std::optional<std::string> auditionAnchorId;
    std::optional<std::string> articulationId;
    std::optional<bool> workspaceVisible;
    std::optional<double> gainDb;
    std::optional<double> pan;
    std::optional<double> gainDelta;
    std::optional<double> panDelta;
    std::optional<double> releaseSeconds;
    std::optional<int> rootKey;
    std::optional<int> keyLow;
    std::optional<int> keyHigh;
    std::optional<int> velocityLow;
    std::optional<int> velocityHigh;
};

class AuthoringSession
{
public:
    using DspParameterGesturePreviewListener = std::function<void(const std::string&, const std::string&, double)>;
    explicit AuthoringSession(RuntimeProjectModel project);

    const RuntimeProjectModel& getProject() const;
    const RuntimeProjectDocumentState& getDocumentState() const;
    std::size_t getWorkspaceSelectionRevision() const { return workspaceSelectionRevision; }
    RuntimeProjectDocumentCheckpoint exportCheckpoint() const;
    RuntimeProjectDocumentActionResult restoreCheckpoint(
        RuntimeProjectDocumentCheckpoint checkpoint,
        RuntimeProjectDocumentCheckpointConstraints constraints = {});
    void replaceProject(RuntimeProjectModel project);

    std::vector<AuthoringZoneSummary> getZoneSummaries() const;
    std::optional<RuntimeProjectZoneDefinition> getSelectedZone() const;
    std::vector<RuntimeProjectArticulationDefinition> getArticulations() const;
    std::optional<RuntimeProjectGroupDefinition> getSelectedGroup() const;
    std::optional<RuntimeProjectLayerDefinition> getSelectedLayer() const;
    std::optional<RuntimeProjectMacroDefinition> getSelectedMacro() const;
    std::optional<std::size_t> getSelectedMacroIndex() const;
    AuthoringGroupRoundRobinStatus getSelectedGroupRoundRobinStatus() const;
    AuthoringDspSelection getDspSelection() const;
    std::optional<RuntimeProjectPerformanceBankDefinition> getSelectedPerformanceBank() const;
    const std::vector<RuntimeProjectInstrumentControlDefinition>& getInstrumentControls() const;
    const std::vector<RuntimeProjectInstrumentControlTargetDefinition>& getInstrumentControlTargets() const;
    const std::vector<RuntimeProjectMidiControlBindingDefinition>& getMidiControlBindings() const;
    AuthoringZonePreviewRequest buildSelectedZonePreviewRequest() const;
    AuthoringGroupPreviewRequest buildSelectedGroupPreviewRequest() const;

    RuntimeProjectDocumentActionResult selectZone(const std::string& zoneId);
    RuntimeProjectDocumentActionResult selectGroup(const std::string& groupId);
    RuntimeProjectDocumentActionResult selectLayer(const std::string& layerId);
    RuntimeProjectDocumentActionResult selectMacro(const std::string& macroId);
    RuntimeProjectDocumentActionResult selectPerformanceBank(const std::string& performanceBankId);
    RuntimeProjectDocumentActionResult selectDspSlot(const std::string& fxSlotId);
    RuntimeProjectDocumentActionResult updateSelectedZone(const RuntimeProjectZoneDefinition& zone,
                                                          const std::string& label);
    RuntimeProjectDocumentActionResult updateZoneRanges(
        const std::vector<AuthoringZoneSummary>& zones,
        const std::string& label);
    RuntimeProjectDocumentActionResult updateZoneReleaseSeconds(
        const std::vector<std::string>& zoneIds,
        double releaseSeconds,
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
    RuntimeProjectDocumentActionResult updateMasterGain(double masterGainDb,
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
    RuntimeProjectDocumentActionResult createLayer(const RuntimeProjectLayerDefinition& layer,
                                                   const std::string& label);
    RuntimeProjectDocumentActionResult updateLayer(std::size_t layerIndex,
                                                   const RuntimeProjectLayerDefinition& layer,
                                                   const std::string& label);
    RuntimeProjectDocumentActionResult moveLayer(std::size_t layerIndex,
                                                 int direction,
                                                 const std::string& label);
    RuntimeProjectDocumentActionResult reassignGroupsToLayer(const std::vector<std::string>& groupIds,
                                                             const std::string& layerId,
                                                             const std::string& label);
    RuntimeProjectDocumentActionResult applyStructureBatchPatch(
        AuthoringStructureEntityKind kind,
        const std::vector<std::string>& entityIds,
        const AuthoringStructureBatchPatch& patch,
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
    RuntimeProjectDocumentActionResult appendImportedContent(std::vector<RuntimeProjectSampleSource> sampleSources,
                                                            std::vector<RuntimeProjectZoneDefinition> zones,
                                                            double importedMasterGainDb,
                                                            std::vector<RuntimeProjectGroupDefinition> groups,
                                                            std::vector<std::string> projectNotes,
                                                            std::vector<std::string> authoringNotes,
                                                            const std::string& label,
                                                            bool reconcileInferredRoundRobin = true,
                                                            std::vector<RuntimeControllerDefault> controllerDefaults = {},
                                                            std::vector<RuntimeProjectInstrumentControlDefinition> instrumentControls = {},
                                                            std::vector<RuntimeProjectInstrumentControlTargetDefinition> instrumentControlTargets = {},
                                                            std::vector<RuntimeProjectMidiControlBindingDefinition> midiControlBindings = {});
    RuntimeProjectDocumentActionResult createMacro(const RuntimeProjectMacroDefinition& macro,
                                                   const std::string& label);
    RuntimeProjectDocumentActionResult createInstrumentControl(
        const RuntimeProjectInstrumentControlDefinition& control,
        const std::string& label);
    RuntimeProjectDocumentActionResult updateInstrumentControl(
        const std::string& controlId,
        const RuntimeProjectInstrumentControlDefinition& control,
        const std::string& label);
    RuntimeProjectDocumentActionResult deleteInstrumentControl(const std::string& controlId,
                                                               const std::string& label);
    RuntimeProjectDocumentActionResult upsertMidiControlBinding(
        const RuntimeProjectMidiControlBindingDefinition& binding,
        const std::string& label);
    RuntimeProjectDocumentActionResult deleteMidiControlBinding(const std::string& bindingId,
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
    std::string selectedZoneId;
    std::string selectedGroupId;
    std::string selectedLayerId;
    std::string selectedMacroId;
    std::size_t workspaceSelectionRevision = 0;

    void recoverDspSelection();
    void recoverMacroSelection();
    void recoverWorkspaceSelection(bool initializeFromProject = false);
};
} // namespace drs::engine
