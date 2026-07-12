ContainerChild::changed() -> undefined

Thread safety: UNSAFE
Triggers the control callback (if registered via setControlCallback) and sends a
visual refresh message. Call this after setValue() to fire the callback. Works
during onInit because it uses synchronous ValueTree listeners internally.
Dispatch/mechanics:
  valueListener.sendMessageForAllProperties()
    -> sendMessage(RefreshType::changed)
    -> triggers onValue callback AND visual refresh
Pair with:
  setValue -- set the value first, then call changed() to notify
  setControlCallback -- registers the callback that changed() triggers
Anti-patterns:
  - Do NOT expect setValue() alone to trigger the control callback -- it writes
    silently. Always call changed() afterward.
Source:
  ScriptingApiContent.cpp  ChildReference::changed()
    -> valueListener.sendMessageForAllProperties()
    -> sendMessage(RefreshType::changed) via refreshBroadcaster
