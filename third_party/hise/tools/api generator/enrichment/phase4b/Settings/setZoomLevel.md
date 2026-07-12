Settings::setZoomLevel(Double newLevel) -> undefined

Thread safety: UNSAFE -- calls setGlobalScaleFactor() with sendNotificationAsync, triggers UI listener notifications
Sets the global UI scale factor. Clamped to [0.25, 2.0] (25% to 200% zoom).
A notification is sent asynchronously to update the UI.

Dispatch/mechanics:
  jlimit(0.25, 2.0, newLevel) -> gm->setGlobalScaleFactor(clamped, sendNotificationAsync)
    -> notifies ScaleFactorListeners

Anti-patterns:
  - Values outside [0.25, 2.0] are silently clamped, not rejected
  - Do NOT rely on the Settings 2.0 ceiling alone -- always clamp to screen bounds
    using Content.getScreenBounds(false)[3] / interfaceHeight as the maximum

Pair with:
  getZoomLevel -- read the current zoom
  getUserDesktopSize -- compute screen-aware zoom limits

Source:
  ScriptingApi.cpp  Settings::setZoomLevel()
    -> jlimit(0.25, 2.0, newLevel)
    -> gm->setGlobalScaleFactor(clamped, sendNotificationAsync)
