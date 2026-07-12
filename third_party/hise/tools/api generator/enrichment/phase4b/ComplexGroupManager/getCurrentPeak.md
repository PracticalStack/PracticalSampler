ComplexGroupManager::getCurrentPeak(Number layerIndex, Number groupIndex, Number eventId) -> Double

Thread safety: SAFE
Returns the current peak volume of the voice matching the specified layer, group, and
event ID. Requires setEnableGainTracking() to have been called first for the matching
layer/group pair; throws a script error if gain tracking was not enabled.

Required setup:
  const var cgm = Sampler.getComplexGroupManager();
  cgm.setEnableGainTracking(0, 1, 1);
  // ... later, in a note-on context:
  var peak = cgm.getCurrentPeak(0, 1, Message.getEventId());

Dispatch/mechanics:
  Validates gainTrackingGroups contains (layerIndex, groupIndex) pair
    -> iterates sampler->activeVoices -> matches eventId + layer/group
    -> returns getCurrentReleasePeak() from the voice's sound reference

Pair with:
  setEnableGainTracking -- must enable tracking first (script error otherwise)

Anti-patterns:
  - Do NOT call without enabling gain tracking first -- throws a script error

Source:
  ScriptingApiObjects.cpp  ScriptingComplexGroupManager::getCurrentPeak()
    -> validates gainTrackingGroups -> iterates activeVoices -> getCurrentReleasePeak()
