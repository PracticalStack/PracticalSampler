ComplexGroupManager::setActiveGroup(Number layerIndex, Number groupIndex) -> undefined

Thread safety: SAFE
Sets the active group filter for the specified layer. On the next noteOn, only samples
matching this group value are considered for voice start. Zero-based index converted to
one-based internally. Rejects IgnoreFlag (255) with a script error. Uses async
notification for thread-safe state updates from the scripting thread.

Dispatch/mechanics:
  Rejects IgnoreFlag -> bumpGroupIndexFromZeroBased(groupIndex)
    -> gm->applyFilter(layerIndex, gi, sendNotificationAsync)
    -> sets layer's filter value for next noteOn sound collection

Anti-patterns:
  - Do NOT pass IgnoreFlag (255) -- rejected with a script error. Use a valid
    zero-based group index (0 to getNumGroupsInLayer()-1).

Pair with:
  getNumGroupsInLayer -- query valid group index range
  registerGroupStartCallback -- observe which group was actually started

Source:
  ScriptingApiObjects.cpp  ScriptingComplexGroupManager::setActiveGroup()
    -> bumpGroupIndexFromZeroBased() -> applyFilter(sendNotificationAsync)
