Content::setUseHighResolutionForPanels(Integer shouldUseDoubleResolution) -> undefined

Thread safety: SAFE -- simple boolean assignment to a member variable.
Enables or disables double-resolution rendering for ScriptPanel paint routines. When
enabled, panels render at 2x resolution for sharper graphics on high-DPI/Retina displays.

Anti-patterns:
  - Not enabling this when using custom paint routines results in blurry rendering
    on Retina/HiDPI displays. Call immediately after makeFrontInterface.

Source:
  ScriptingApiContent.cpp  Content::setUseHighResolutionForPanels()
    -> simple boolean assignment to member variable
