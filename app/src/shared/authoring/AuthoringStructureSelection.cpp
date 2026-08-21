#include "shared/authoring/AuthoringStructureSelection.h"

#include <algorithm>

namespace drs::app::authoring
{
bool AuthoringStructureSelection::contains(const std::string& id) const noexcept
{
    return std::find(ids.begin(), ids.end(), id) != ids.end();
}

void AuthoringStructureSelection::clear() noexcept
{
    kind = StructureSelectionKind::none;
    ids.clear();
    primaryId.clear();
}

void AuthoringStructureSelection::replace(const StructureSelectionKind nextKind,
                                          std::vector<std::string> nextIds,
                                          std::string nextPrimaryId)
{
    if (nextKind == StructureSelectionKind::none)
    {
        clear();
        return;
    }

    kind = nextKind;
    ids = normalize(std::move(nextIds));
    primaryId = std::move(nextPrimaryId);
    ensurePrimaryIsSelected();
}

void AuthoringStructureSelection::setPrimary(const std::string& id) noexcept
{
    if (contains(id))
        primaryId = id;
}

void AuthoringStructureSelection::toggle(const std::string& id,
                                         const StructureSelectionKind itemKind)
{
    if (itemKind == StructureSelectionKind::none)
        return;

    if (kind != itemKind)
    {
        replace(itemKind, { id }, id);
        return;
    }

    const auto iterator = std::find(ids.begin(), ids.end(), id);
    if (iterator == ids.end())
    {
        ids.push_back(id);
        primaryId = id;
        return;
    }

    const auto removedPrimary = *iterator == primaryId;
    ids.erase(iterator);
    if (ids.empty())
    {
        clear();
        return;
    }

    if (removedPrimary || !contains(primaryId))
        primaryId = ids.front();
}

void AuthoringStructureSelection::selectRange(const std::vector<std::string>& orderedIds,
                                              const std::string& targetId,
                                              const StructureSelectionKind itemKind,
                                              const bool additive)
{
    if (itemKind == StructureSelectionKind::none)
        return;

    const auto target = std::find(orderedIds.begin(), orderedIds.end(), targetId);
    if (target == orderedIds.end())
    {
        replace(itemKind, { targetId }, targetId);
        return;
    }

    const auto anchor = (!additive && kind == itemKind && !primaryId.empty())
        ? std::find(orderedIds.begin(), orderedIds.end(), primaryId)
        : target;
    const auto first = anchor == orderedIds.end() ? target : std::min(anchor, target);
    const auto last = anchor == orderedIds.end() ? target : std::max(anchor, target);

    std::vector<std::string> range(first, last + 1);
    if (additive && kind == itemKind)
    {
        range.insert(range.end(), ids.begin(), ids.end());
    }
    replace(itemKind, std::move(range), targetId);
}

void AuthoringStructureSelection::reconcile(const std::vector<std::string>& validIds)
{
    if (kind == StructureSelectionKind::none)
        return;

    std::vector<std::string> retained;
    retained.reserve(ids.size());
    for (const auto& validId : validIds)
    {
        if (contains(validId))
            retained.push_back(validId);
    }

    ids = normalize(std::move(retained));
    if (ids.empty())
    {
        clear();
        return;
    }

    if (!contains(primaryId))
        primaryId = ids.front();
}

std::vector<std::string> AuthoringStructureSelection::normalize(std::vector<std::string> values)
{
    values.erase(std::remove_if(values.begin(), values.end(), [](const auto& value)
                                {
                                    return value.empty();
                                }),
                 values.end());
    std::sort(values.begin(), values.end());
    values.erase(std::unique(values.begin(), values.end()), values.end());
    return values;
}

void AuthoringStructureSelection::ensurePrimaryIsSelected() noexcept
{
    if (ids.empty())
    {
        kind = StructureSelectionKind::none;
        primaryId.clear();
        return;
    }

    if (primaryId.empty() || !contains(primaryId))
        primaryId = ids.front();
}
} // namespace drs::app::authoring
