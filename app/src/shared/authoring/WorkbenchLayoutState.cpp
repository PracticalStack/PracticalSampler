#include "shared/authoring/WorkbenchLayoutState.h"

#include <algorithm>

namespace drs::app::authoring
{
void WorkbenchLayoutState::setOpen(const bool shouldOpen) noexcept
{
    open = shouldOpen;
}

void WorkbenchLayoutState::setUserHeight(const int height) noexcept
{
    rememberedHeight = std::clamp(height, standardMinimumHeight, focusedMaximumHeight);
    userHeightSet = true;
    open = true;
}

void WorkbenchLayoutState::requestStandard() noexcept
{
    rememberedHeight = standardDefaultHeight;
    userHeightSet = true;
    open = true;
}

void WorkbenchLayoutState::requestFocused() noexcept
{
    rememberedHeight = focusedDefaultHeight;
    userHeightSet = true;
    open = true;
}

void WorkbenchLayoutState::suggestHeightForTab(const WorkbenchTab tab) noexcept
{
    if (userHeightSet)
        return;

    rememberedHeight = tab == WorkbenchTab::waveform
        ? standardDefaultHeight
        : focusedDefaultHeight;
}

WorkbenchSizeMode WorkbenchLayoutState::getSizeMode() const noexcept
{
    if (!open)
        return WorkbenchSizeMode::collapsed;
    return rememberedHeight >= focusedMinimumHeight
        ? WorkbenchSizeMode::focused
        : WorkbenchSizeMode::standard;
}

int WorkbenchLayoutState::resolveHeight(const int availableHeight,
                                        const int protectedMapHeight,
                                        const int mapGap) const noexcept
{
    if (!open)
        return std::min(collapsedHeight, std::max(0, availableHeight));

    const auto maximumWorkbenchHeight = std::max(
        collapsedHeight,
        availableHeight - std::max(0, protectedMapHeight) - std::max(0, mapGap));
    return std::min(rememberedHeight, maximumWorkbenchHeight);
}
} // namespace drs::app::authoring
