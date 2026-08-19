#include "drs/engine/NativeContent.h"

#include "drs/engine/WorkspacePaths.generated.h"

namespace drs::engine
{
NativeContentRoots getNativeContentRoots()
{
    return {
        generated::workspaceRepoRoot,
        generated::workspaceNativeSamplesRoot,
        generated::workspacePhase1RuntimeRoot
    };
}
} // namespace drs::engine
