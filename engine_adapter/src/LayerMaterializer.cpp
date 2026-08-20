#include "drs/engine/LayerMaterializer.h"

#include <algorithm>

namespace drs::engine
{
namespace
{
RuntimeProjectGroupDefinition makeDefaultGroup(const std::string& id,
                                                const std::string& anchorZoneId,
                                                const int displayOrder,
                                                const std::string& layerId)
{
    RuntimeProjectGroupDefinition group;
    group.id = id;
    group.layerId = layerId;
    group.displayName = "Default Group";
    group.displayOrder = displayOrder;
    group.workspaceVisible = true;
    group.auditionAnchorZoneId = anchorZoneId;
    return group;
}

RuntimeProjectLayerDefinition makeDefaultLayer(const std::string& id,
                                               const std::string& anchorGroupId,
                                               const int displayOrder)
{
    RuntimeProjectLayerDefinition layer;
    layer.id = id;
    layer.displayName = "Default Layer";
    layer.displayOrder = displayOrder;
    layer.workspaceVisible = true;
    layer.auditionAnchorGroupId = anchorGroupId;
    return layer;
}
} // namespace

LayerMaterializationResult materializeProjectLayerHierarchy(RuntimeProjectModel& project,
                                                             const bool addNotes)
{
    LayerMaterializationResult result;
    if (project.schemaVersion < layerContractProjectSchemaVersion
        || project.authoring.schemaVersion < layerContractAuthoringSchemaVersion)
    {
        return result;
    }

    auto& authoring = project.authoring;
    const auto findGroup = [&](const std::string& id)
    {
        return std::find_if(authoring.groups.begin(), authoring.groups.end(),
                            [&](const auto& group) { return group.id == id; });
    };
    const auto findLayer = [&](const std::string& id)
    {
        return std::find_if(authoring.layers.begin(), authoring.layers.end(),
                            [&](const auto& layer) { return layer.id == id; });
    };

    const std::string defaultGroupId = "default-group";
    const std::string defaultLayerId = "default-layer";

    const auto ensureDefaultLayer = [&]
    {
        if (findLayer(defaultLayerId) != authoring.layers.end())
            return;

        authoring.layers.push_back(makeDefaultLayer(defaultLayerId,
                                                    authoring.groups.empty() ? std::string {} : authoring.groups.front().id,
                                                    static_cast<int>(authoring.layers.size())));
        result.synthesizedDefaultLayer = true;
        result.changed = true;
    };

    bool needsDefaultGroup = false;
    for (const auto& zone : authoring.zones)
        needsDefaultGroup = needsDefaultGroup || zone.groupId.empty();

    if (needsDefaultGroup && findGroup(defaultGroupId) == authoring.groups.end())
    {
        ensureDefaultLayer();
        if (auto layer = findLayer(defaultLayerId); layer != authoring.layers.end()
            && layer->auditionAnchorGroupId.empty())
            layer->auditionAnchorGroupId = defaultGroupId;

        authoring.groups.push_back(makeDefaultGroup(defaultGroupId,
                                                    {},
                                                    static_cast<int>(authoring.groups.size()),
                                                    defaultLayerId));
        result.synthesizedDefaultGroup = true;
        result.changed = true;
    }

    for (auto& zone : authoring.zones)
    {
        if (zone.groupId.empty())
        {
            zone.groupId = defaultGroupId;
            result.changed = true;
        }

        if (findGroup(zone.groupId) == authoring.groups.end())
        {
            ensureDefaultLayer();
            authoring.groups.push_back(makeDefaultGroup(zone.groupId,
                                                        zone.id,
                                                        static_cast<int>(authoring.groups.size()),
                                                        defaultLayerId));
            result.changed = true;
        }
    }

    bool hasUnassignedGroup = std::any_of(authoring.groups.begin(), authoring.groups.end(),
                                          [](const auto& group) { return group.layerId.empty(); });
    if (hasUnassignedGroup && findLayer(defaultLayerId) == authoring.layers.end())
        ensureDefaultLayer();

    for (auto& group : authoring.groups)
    {
        if (group.layerId.empty())
        {
            group.layerId = defaultLayerId;
            result.changed = true;
        }

        if (group.auditionAnchorZoneId.empty())
        {
            const auto zone = std::find_if(authoring.zones.begin(), authoring.zones.end(),
                                           [&](const auto& candidate) { return candidate.groupId == group.id; });
            if (zone != authoring.zones.end())
            {
                group.auditionAnchorZoneId = zone->id;
                result.changed = true;
            }
        }
    }

    for (auto& layer : authoring.layers)
    {
        if (layer.auditionAnchorGroupId.empty())
        {
            const auto group = std::find_if(authoring.groups.begin(), authoring.groups.end(),
                                            [&](const auto& candidate) { return candidate.layerId == layer.id; });
            if (group != authoring.groups.end())
            {
                layer.auditionAnchorGroupId = group->id;
                result.changed = true;
            }
        }
    }

    if (!authoring.selectedGroupId.empty())
    {
        const auto group = findGroup(authoring.selectedGroupId);
        if (group != authoring.groups.end() && !group->layerId.empty())
            authoring.selectedLayerId = group->layerId;
    }
    if (authoring.selectedLayerId.empty() && !authoring.layers.empty())
    {
        authoring.selectedLayerId = authoring.layers.front().id;
        result.changed = true;
    }

    if (result.changed && addNotes)
    {
        if (result.synthesizedDefaultGroup)
            result.notes.push_back("Synthesized default-group for imported zones without group membership.");
        if (result.synthesizedDefaultLayer)
            result.notes.push_back("Synthesized default-layer for imported or migrated groups without layer membership.");
        if (!result.notes.empty())
            authoring.notes.insert(authoring.notes.end(), result.notes.begin(), result.notes.end());
    }

    return result;
}
} // namespace drs::engine
