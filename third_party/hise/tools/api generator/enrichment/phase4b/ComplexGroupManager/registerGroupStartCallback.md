ComplexGroupManager::registerGroupStartCallback(var layerIdOrIndex, Function callback) -> undefined

Thread safety: UNSAFE -- creates a GroupCallback wrapper object and modifies layer post-processor state
Registers a callback invoked when a voice starts for the specified layer. The callback
receives the zero-based group index, or IgnoreFlag (255) if the sample has no assignment
in this layer. Must be an inline function (realtime-safe); non-inline functions rejected
with a script error. On a Custom LogicType layer, auto-promotes to post-processor.
Callback signature: f(int groupIndex)

Required setup:
  const var cgm = Sampler.getComplexGroupManager();

  inline function onGroupStart(groupIndex)
  {
      if (groupIndex != cgm.IgnoreFlag)
          Console.print(groupIndex);
  }

  cgm.registerGroupStartCallback(0, onGroupStart);

Dispatch/mechanics:
  Checks realtime safety (RealtimeSafetyInfo::check in backend, isRealtimeSafe() in frontend)
    -> creates GroupCallback wrapper -> gm->setVoiceStartCallback()
    -> CustomLayer auto-promoted to post-processor for postVoiceStart() notifications

Anti-patterns:
  - Do NOT use a regular function -- must be an inline function for realtime safety
  - Do NOT treat IgnoreFlag (255) as a valid group index -- handle it explicitly

Pair with:
  setActiveGroup -- control which group is active for the layer

Source:
  ScriptingApiObjects.cpp  ScriptingComplexGroupManager::registerGroupStartCallback()
    -> RealtimeSafetyInfo check -> new GroupCallback -> setVoiceStartCallback()
  ComplexGroupManager.h  GroupCallback::onVoiceStart() converts one-based to zero-based
