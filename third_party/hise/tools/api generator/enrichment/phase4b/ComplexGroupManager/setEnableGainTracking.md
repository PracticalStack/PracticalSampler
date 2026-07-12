ComplexGroupManager::setEnableGainTracking(var layerIdOrIndex, Number groupIndex, Integer shouldBeActive) -> undefined

Thread safety: UNSAFE -- iterates all samples in the sample map and modifies per-sample release tracking state
Enables or disables peak volume tracking for a specific layer/group. Repurposes the
release sample peak tracking mechanism. After enabling, use getCurrentPeak() to read the
tracked value for a specific voice by event ID. Rejects IgnoreFlag for the group index.

Dispatch/mechanics:
  Rejects IgnoreFlag -> records (layerIndex, groupIndex) in gainTrackingGroups
    -> iterates all samples matching layer/group
    -> calls setIsReleaseSample(shouldBeActive) on each sample

Pair with:
  getCurrentPeak -- read peak value after enabling tracking

Source:
  ScriptingApiObjects.cpp  ScriptingComplexGroupManager::setEnableGainTracking()
    -> gainTrackingGroups registration -> iterates samples -> setIsReleaseSample()
