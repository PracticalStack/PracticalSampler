#pragma once

#include "shared/authoring/AuthoringViewModels.h"

namespace drs::app::authoring
{
enum class WorkbenchSizeMode
{
    collapsed,
    standard,
    focused
};

// Session-local UI state. It deliberately has no project serialization path:
// resizing the workbench is workspace chrome, not an authored instrument edit.
class WorkbenchLayoutState
{
public:
    static constexpr int collapsedHeight = 38;
    static constexpr int splitterHeight = 6;
    static constexpr int standardMinimumHeight = 220;
    static constexpr int standardDefaultHeight = 232;
    static constexpr int standardMaximumHeight = 240;
    static constexpr int focusedMinimumHeight = 320;
    static constexpr int focusedDefaultHeight = 340;
    static constexpr int focusedMaximumHeight = 360;

    void setOpen(bool shouldOpen) noexcept;
    bool isOpen() const noexcept { return open; }

    void setUserHeight(int height) noexcept;
    void requestStandard() noexcept;
    void requestFocused() noexcept;
    void suggestHeightForTab(DrawerTab tab) noexcept;

    int getRememberedHeight() const noexcept { return rememberedHeight; }
    bool hasUserHeight() const noexcept { return userHeightSet; }
    WorkbenchSizeMode getSizeMode() const noexcept;
    int resolveHeight(int availableHeight,
                      int protectedMapHeight,
                      int mapGap) const noexcept;

private:
    bool open = false;
    bool userHeightSet = false;
    int rememberedHeight = standardDefaultHeight;
};
} // namespace drs::app::authoring
