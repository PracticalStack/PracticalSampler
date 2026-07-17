#pragma once

#include "shared/authoring/AuthoringViewModels.h"

#include <algorithm>

namespace drs::app::authoring
{
/*
    Phase 3 bridges real layer, round-robin, and mic entities into the compact workspace by
    implementing this adapter shape rather than changing layout code.

    Required adapter methods:
      std::string getPaneTitle() const;
      std::string getListEmptyStateText() const;
      int getSelectedIndex() const;
      int getItemCount() const;
      RepeatedStructureRowViewModel getRowViewModel(int index) const;
      RepeatedStructureSelectionPathViewModel getSelectionPathViewModel(int index) const;
      RepeatedStructureDetailViewModel getDetailViewModel(int index) const;
      RepeatedStructureSelectionPathViewModel getEmptySelectionPathViewModel() const;
      RepeatedStructureDetailViewModel getEmptyDetailViewModel() const;

    Phase 3 edit surfaces keep workspace layout code unchanged by routing real commit actions
    through RepeatedStructureAdapterCallbacks / RepeatedStructureEditIntentHandler while adapters
    keep returning only real project entities.
*/
template <typename Adapter>
RepeatedStructurePaneViewModel buildRepeatedStructurePaneViewModel(const Adapter& adapter,
                                                                  int selectedIndexOverride = -1)
{
    RepeatedStructurePaneViewModel pane;
    pane.title = adapter.getPaneTitle();
    pane.list.emptyStateText = adapter.getListEmptyStateText();

    const auto itemCount = std::max(0, adapter.getItemCount());
    pane.list.rows.reserve(static_cast<std::size_t>(itemCount));
    for (int index = 0; index < itemCount; ++index)
        pane.list.rows.push_back(adapter.getRowViewModel(index));

    pane.list.selectedIndex = selectedIndexOverride >= 0 ? selectedIndexOverride
                                                         : adapter.getSelectedIndex();

    if (itemCount == 0)
        pane.list.selectedIndex = -1;
    else
        pane.list.selectedIndex = std::clamp(pane.list.selectedIndex, 0, itemCount - 1);

    const auto hasSelection = pane.list.selectedIndex >= 0
        && pane.list.selectedIndex < itemCount;

    pane.selectionPath = hasSelection
        ? adapter.getSelectionPathViewModel(pane.list.selectedIndex)
        : adapter.getEmptySelectionPathViewModel();
    pane.detail = hasSelection
        ? adapter.getDetailViewModel(pane.list.selectedIndex)
        : adapter.getEmptyDetailViewModel();

    return pane;
}
} // namespace drs::app::authoring
