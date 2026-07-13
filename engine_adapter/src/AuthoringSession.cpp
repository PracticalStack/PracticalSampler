#include "drs/engine/AuthoringSession.h"

#include <algorithm>

namespace drs::engine
{
namespace
{
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
        summary.gainDb = zone.gainDb;
        summary.pan = zone.pan;
        summary.loopEnabled = zone.loopEnabled;
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
    {
        RuntimeProjectDocumentActionResult result;
        result.state = "Zone selection rejected";
        result.issues.push_back("Zone '" + zoneId + "' does not exist in the current authoring project.");
        result.documentState = getDocumentState();
        return result;
    }

    project.authoring.selectedZoneId = zoneId;
    return documentController.commitSnapshot(project, "Select zone", {"authoring.selectedZoneId"});
}

RuntimeProjectDocumentActionResult AuthoringSession::updateSelectedZone(const RuntimeProjectZoneDefinition& zone,
                                                                        const std::string& label)
{
    const auto selectedZoneIndex = findSelectedZoneIndex(getProject());
    if (!selectedZoneIndex.has_value())
    {
        RuntimeProjectDocumentActionResult result;
        result.state = "Zone edit rejected";
        result.issues.push_back("No zone is currently selected for editing.");
        result.documentState = getDocumentState();
        return result;
    }

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
