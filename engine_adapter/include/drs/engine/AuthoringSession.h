#pragma once

#include "drs/engine/ProjectDocument.h"

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

class AuthoringSession
{
public:
    explicit AuthoringSession(RuntimeProjectModel project);

    const RuntimeProjectModel& getProject() const;
    const RuntimeProjectDocumentState& getDocumentState() const;
    void replaceProject(RuntimeProjectModel project);

    std::vector<AuthoringZoneSummary> getZoneSummaries() const;
    std::optional<RuntimeProjectZoneDefinition> getSelectedZone() const;
    std::optional<RuntimeProjectPerformanceBankDefinition> getSelectedPerformanceBank() const;
    AuthoringZonePreviewRequest buildSelectedZonePreviewRequest() const;

    RuntimeProjectDocumentActionResult selectZone(const std::string& zoneId);
    RuntimeProjectDocumentActionResult selectPerformanceBank(const std::string& performanceBankId);
    RuntimeProjectDocumentActionResult updateSelectedZone(const RuntimeProjectZoneDefinition& zone,
                                                          const std::string& label);
    RuntimeProjectDocumentActionResult createRoundRobinPoolForSelectedZone(const std::string& label);
    RuntimeProjectDocumentActionResult addCompatibleZonesToSelectedRoundRobinPool(const std::string& label);
    RuntimeProjectDocumentActionResult normalizeSelectedRoundRobinPool(const std::string& label);
    RuntimeProjectDocumentActionResult removeSelectedZoneFromRoundRobinPool(const std::string& label);
    RuntimeProjectDocumentActionResult deleteSelectedSample();
    RuntimeProjectDocumentActionResult appendImportedContent(std::vector<RuntimeProjectSampleSource> sampleSources,
                                                            std::vector<RuntimeProjectZoneDefinition> zones,
                                                            const std::string& label);
    RuntimeProjectDocumentActionResult appendImportedContent(std::vector<RuntimeProjectSampleSource> sampleSources,
                                                            std::vector<RuntimeProjectZoneDefinition> zones,
                                                            std::vector<std::string> projectNotes,
                                                            std::vector<std::string> authoringNotes,
                                                            const std::string& label);
    RuntimeProjectDocumentActionResult updateMacro(std::size_t macroIndex,
                                                   const RuntimeProjectMacroDefinition& macro,
                                                   const std::string& label);
    RuntimeProjectDocumentActionResult moveMacro(std::size_t macroIndex,
                                                 int direction,
                                                 const std::string& label);
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
    void markSaved();

private:
    RuntimeProjectDocumentController documentController;
};
} // namespace drs::engine
