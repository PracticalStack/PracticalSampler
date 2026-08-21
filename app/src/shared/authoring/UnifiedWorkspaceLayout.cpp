#include "shared/authoring/UnifiedWorkspaceLayout.h"

#include <algorithm>

namespace drs::app::authoring
{
UnifiedWorkspaceLayoutResult calculateUnifiedWorkspaceLayout(const UnifiedWorkspaceLayoutInput& input)
{
    auto result = UnifiedWorkspaceLayoutResult {};
    auto area = input.content;
    result.browser = area.removeFromLeft(std::clamp(input.browserWidth, 220, 520));
    area.removeFromLeft(std::max(0, input.gap));
    if (!input.mapVisible)
    {
        result.inspector = area;
        return result;
    }
    result.inspector = area.removeFromRight(std::clamp(input.inspectorWidth, 280, 480));
    area.removeFromRight(std::max(0, input.gap));
    result.map = area;
    return result;
}
} // namespace drs::app::authoring
