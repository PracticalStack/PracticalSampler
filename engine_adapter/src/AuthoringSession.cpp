#include "drs/engine/AuthoringSession.h"

#include <algorithm>

namespace drs::engine
{
namespace
{
RuntimeProjectDocumentActionResult makeRejectedResult(const RuntimeProjectDocumentState& documentState,
                                                      const std::string& state,
                                                      const std::string& issue)
{
    RuntimeProjectDocumentActionResult result;
    result.state = state;
    result.issues.push_back(issue);
    result.documentState = documentState;
    return result;
}

std::vector<AuthoringZoneSummary> buildZoneSummaries(const RuntimeProjectModel& project)
{
    std::vector<AuthoringZoneSummary> summaries;
    summaries.reserve(project.authoring.zones.size());

    for (const auto& zone : project.authoring.zones)
    {
        AuthoringZoneSummary summary;
        summary.id = zone.id;
        summary.displayName = zone.displayName;
        summary.sampleSourceId = zone.sampleSourceId;
        summary.articulationId = zone.articulationId;
        summary.rootKey = zone.rootKey;
        summary.keyLow = zone.keyLow;
        summary.keyHigh = zone.keyHigh;
        summary.velocityLow = zone.velocityLow;
        summary.velocityHigh = zone.velocityHigh;
        summary.velocityCrossfade = zone.velocityCrossfade;
        summary.gainDb = zone.gainDb;
        summary.pan = zone.pan;
        summary.loopEnabled = zone.loopEnabled;
        summary.triggerMode = zone.triggerMode;
        summary.selected = zone.id == project.authoring.selectedZoneId;
        summaries.push_back(std::move(summary));
    }

    return summaries;
}

std::optional<std::size_t> findSelectedZoneIndex(const RuntimeProjectModel& project)
{
    const auto iterator = std::find_if(project.authoring.zones.begin(),
                                       project.authoring.zones.end(),
                                       [&](const RuntimeProjectZoneDefinition& zone)
                                       {
                                           return zone.id == project.authoring.selectedZoneId;
                                       });
    if (iterator == project.authoring.zones.end())
        return std::nullopt;

    return static_cast<std::size_t>(std::distance(project.authoring.zones.begin(), iterator));
}

std::optional<std::size_t> findSelectedPerformanceBankIndex(const RuntimeProjectModel& project)
{
    const auto iterator = std::find_if(project.authoring.performanceBanks.begin(),
                                       project.authoring.performanceBanks.end(),
                                       [&](const RuntimeProjectPerformanceBankDefinition& performanceBank)
                                       {
                                           return performanceBank.id == project.authoring.selectedPerformanceBankId;
                                       });
    if (iterator == project.authoring.performanceBanks.end())
        return std::nullopt;

    return static_cast<std::size_t>(std::distance(project.authoring.performanceBanks.begin(), iterator));
}
} // namespace

AuthoringSession::AuthoringSession(RuntimeProjectModel project)
    : documentController(std::move(project))
{
}

const RuntimeProjectModel& AuthoringSession::getProject() const
{
    return documentController.getProject();
}

const RuntimeProjectDocumentState& AuthoringSession::getDocumentState() const
{
    return documentController.getDocumentState();
}

void AuthoringSession::replaceProject(RuntimeProjectModel project)
{
    documentController = RuntimeProjectDocumentController(std::move(project));
}

std::vector<AuthoringZoneSummary> AuthoringSession::getZoneSummaries() const
{
    return buildZoneSummaries(getProject());
}

std::optional<RuntimeProjectZoneDefinition> AuthoringSession::getSelectedZone() const
{
    const auto selectedZoneIndex = findSelectedZoneIndex(getProject());
    if (!selectedZoneIndex.has_value())
        return std::nullopt;

    return getProject().authoring.zones[*selectedZoneIndex];
}

std::optional<RuntimeProjectPerformanceBankDefinition> AuthoringSession::getSelectedPerformanceBank() const
{
    const auto selectedPerformanceBankIndex = findSelectedPerformanceBankIndex(getProject());
    if (!selectedPerformanceBankIndex.has_value())
        return std::nullopt;

    return getProject().authoring.performanceBanks[*selectedPerformanceBankIndex];
}

AuthoringZonePreviewRequest AuthoringSession::buildSelectedZonePreviewRequest() const
{
    AuthoringZonePreviewRequest request;
    const auto zone = getSelectedZone();
    if (!zone.has_value())
    {
        request.state = "No zone selected";
        return request;
    }

    request.available = true;
    request.midiNote = std::clamp(zone->rootKey, zone->keyLow, zone->keyHigh);
    request.velocity = std::clamp((zone->velocityLow + zone->velocityHigh) / 2, 1, 127);
    request.zoneId = zone->id;
    request.articulationId = zone->articulationId;
    request.state = "Zone preview ready";
    return request;
}

RuntimeProjectDocumentActionResult AuthoringSession::selectZone(const std::string& zoneId)
{
    auto project = getProject();
    const auto iterator = std::find_if(project.authoring.zones.begin(),
                                       project.authoring.zones.end(),
                                       [&](const RuntimeProjectZoneDefinition& zone)
                                       {
                                           return zone.id == zoneId;
                                       });
    if (iterator == project.authoring.zones.end())
        return makeRejectedResult(getDocumentState(),
                                  "Zone selection rejected",
                                  "Zone '" + zoneId + "' does not exist in the current authoring project.");

    project.authoring.selectedZoneId = zoneId;
    return documentController.commitSnapshot(project, "Select zone", {"authoring.selectedZoneId"});
}

RuntimeProjectDocumentActionResult AuthoringSession::selectPerformanceBank(const std::string& performanceBankId)
{
    auto project = getProject();
    const auto iterator = std::find_if(project.authoring.performanceBanks.begin(),
                                       project.authoring.performanceBanks.end(),
                                       [&](const RuntimeProjectPerformanceBankDefinition& performanceBank)
                                       {
                                           return performanceBank.id == performanceBankId;
                                       });
    if (iterator == project.authoring.performanceBanks.end())
        return makeRejectedResult(getDocumentState(),
                                  "Performance-bank selection rejected",
                                  "Performance bank '" + performanceBankId + "' does not exist in the current authoring project.");

    project.authoring.selectedPerformanceBankId = performanceBankId;
    return documentController.commitSnapshot(project,
                                             "Select performance bank",
                                             {"authoring.selectedPerformanceBankId"});
}

RuntimeProjectDocumentActionResult AuthoringSession::updateSelectedZone(const RuntimeProjectZoneDefinition& zone,
                                                                        const std::string& label)
{
    const auto selectedZoneIndex = findSelectedZoneIndex(getProject());
    if (!selectedZoneIndex.has_value())
        return makeRejectedResult(getDocumentState(),
                                  "Zone edit rejected",
                                  "No zone is currently selected for editing.");

    auto project = getProject();
    project.authoring.zones[*selectedZoneIndex] = zone;
    project.authoring.selectedZoneId = zone.id;

    return documentController.commitSnapshot(project,
                                             label,
                                             {
                                                 "authoring.zones[" + std::to_string(*selectedZoneIndex) + "]",
                                                 "authoring.selectedZoneId"
                                             });
}

RuntimeProjectDocumentActionResult AuthoringSession::appendImportedContent(
    std::vector<RuntimeProjectSampleSource> sampleSources,
    std::vector<RuntimeProjectZoneDefinition> zones,
    const std::string& label)
{
    return appendImportedContent(std::move(sampleSources), std::move(zones), {}, {}, label);
}

RuntimeProjectDocumentActionResult AuthoringSession::appendImportedContent(
    std::vector<RuntimeProjectSampleSource> sampleSources,
    std::vector<RuntimeProjectZoneDefinition> zones,
    std::vector<std::string> projectNotes,
    std::vector<std::string> authoringNotes,
    const std::string& label)
{
    if (zones.empty())
        return makeRejectedResult(getDocumentState(),
                                  "Authoring import rejected",
                                  "Imported content must include at least one zone.");

    auto project = getProject();
    const auto originalSampleSourceCount = project.sampleSources.size();
    const auto originalZoneCount = project.authoring.zones.size();

    project.sampleSources.insert(project.sampleSources.end(),
                                 std::make_move_iterator(sampleSources.begin()),
                                 std::make_move_iterator(sampleSources.end()));
    project.authoring.zones.insert(project.authoring.zones.end(),
                                   std::make_move_iterator(zones.begin()),
                                   std::make_move_iterator(zones.end()));
    project.notes.insert(project.notes.end(),
                         std::make_move_iterator(projectNotes.begin()),
                         std::make_move_iterator(projectNotes.end()));
    project.authoring.notes.insert(project.authoring.notes.end(),
                                   std::make_move_iterator(authoringNotes.begin()),
                                   std::make_move_iterator(authoringNotes.end()));
    project.authoring.selectedZoneId = project.authoring.zones[originalZoneCount].id;

    std::vector<std::string> changedPaths {
        "sampleSources[" + std::to_string(originalSampleSourceCount) + "]",
        "authoring.zones[" + std::to_string(originalZoneCount) + "]",
        "authoring.selectedZoneId"
    };
    if (!projectNotes.empty())
        changedPaths.push_back("notes");
    if (!authoringNotes.empty())
        changedPaths.push_back("authoring.notes");

    return documentController.commitSnapshot(project, label, changedPaths);
}

RuntimeProjectDocumentActionResult AuthoringSession::updateMacro(std::size_t macroIndex,
                                                                 const RuntimeProjectMacroDefinition& macro,
                                                                 const std::string& label)
{
    if (macroIndex >= getProject().authoring.macros.size())
        return makeRejectedResult(getDocumentState(),
                                  "Macro edit rejected",
                                  "Macro index " + std::to_string(macroIndex) + " is out of range.");

    auto project = getProject();
    project.authoring.macros[macroIndex] = macro;
    return documentController.commitSnapshot(project,
                                             label,
                                             {"authoring.macros[" + std::to_string(macroIndex) + "]"});
}

RuntimeProjectDocumentActionResult AuthoringSession::moveMacro(std::size_t macroIndex,
                                                               int direction,
                                                               const std::string& label)
{
    if (macroIndex >= getProject().authoring.macros.size())
        return makeRejectedResult(getDocumentState(),
                                  "Macro reorder rejected",
                                  "Macro index " + std::to_string(macroIndex) + " is out of range.");

    if (direction != -1 && direction != 1)
        return makeRejectedResult(getDocumentState(),
                                  "Macro reorder rejected",
                                  "Macro reordering only supports directions of -1 or 1.");

    const auto targetIndexSigned = static_cast<int>(macroIndex) + direction;
    if (targetIndexSigned < 0
        || targetIndexSigned >= static_cast<int>(getProject().authoring.macros.size()))
    {
        return makeRejectedResult(getDocumentState(),
                                  "Macro reorder rejected",
                                  "Macro cannot move beyond the current authoring surface bounds.");
    }

    auto project = getProject();
    const auto targetIndex = static_cast<std::size_t>(targetIndexSigned);
    std::swap(project.authoring.macros[macroIndex], project.authoring.macros[targetIndex]);

    return documentController.commitSnapshot(project,
                                             label,
                                             {
                                                 "authoring.macros[" + std::to_string(macroIndex) + "]",
                                                 "authoring.macros[" + std::to_string(targetIndex) + "]"
                                             });
}

RuntimeProjectDocumentActionResult AuthoringSession::updateFxSlot(std::size_t fxSlotIndex,
                                                                  const RuntimeProjectFxSlotDefinition& fxSlot,
                                                                  const std::string& label)
{
    if (fxSlotIndex >= getProject().authoring.fxSlots.size())
        return makeRejectedResult(getDocumentState(),
                                  "FX slot edit rejected",
                                  "FX slot index " + std::to_string(fxSlotIndex) + " is out of range.");

    auto project = getProject();
    project.authoring.fxSlots[fxSlotIndex] = fxSlot;
    return documentController.commitSnapshot(project,
                                             label,
                                             {"authoring.fxSlots[" + std::to_string(fxSlotIndex) + "]"});
}

RuntimeProjectDocumentActionResult AuthoringSession::updateRoutingBus(std::size_t routingBusIndex,
                                                                      const RuntimeProjectRoutingBusDefinition& routingBus,
                                                                      const std::string& label)
{
    if (routingBusIndex >= getProject().authoring.routingBuses.size())
        return makeRejectedResult(getDocumentState(),
                                  "Routing edit rejected",
                                  "Routing bus index " + std::to_string(routingBusIndex) + " is out of range.");

    auto project = getProject();
    project.authoring.routingBuses[routingBusIndex] = routingBus;
    return documentController.commitSnapshot(project,
                                             label,
                                             {"authoring.routingBuses[" + std::to_string(routingBusIndex) + "]"});
}

RuntimeProjectDocumentActionResult AuthoringSession::updatePerformanceBank(
    std::size_t performanceBankIndex,
    const RuntimeProjectPerformanceBankDefinition& performanceBank,
    const std::string& label)
{
    if (performanceBankIndex >= getProject().authoring.performanceBanks.size())
        return makeRejectedResult(getDocumentState(),
                                  "Performance-bank edit rejected",
                                  "Performance bank index " + std::to_string(performanceBankIndex) + " is out of range.");

    auto project = getProject();
    project.authoring.performanceBanks[performanceBankIndex] = performanceBank;
    project.authoring.selectedPerformanceBankId = performanceBank.id;
    return documentController.commitSnapshot(project,
                                             label,
                                             {
                                                 "authoring.performanceBanks[" + std::to_string(performanceBankIndex) + "]",
                                                 "authoring.selectedPerformanceBankId"
                                             });
}

RuntimeProjectDocumentActionResult AuthoringSession::undo()
{
    return documentController.undo();
}

RuntimeProjectDocumentActionResult AuthoringSession::redo()
{
    return documentController.redo();
}

void AuthoringSession::markSaved()
{
    documentController.markSaved();
}
} // namespace drs::engine
