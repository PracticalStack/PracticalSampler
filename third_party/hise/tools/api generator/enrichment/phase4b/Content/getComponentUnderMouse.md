Content::getComponentUnderMouse() -> String

Thread safety: SAFE -- queries JUCE Desktop for the component under the mouse cursor, no allocations or locks.
Returns the JUCE component ID of whatever JUCE Component is currently under the mouse
cursor. Returns an empty string if nothing is under the mouse.

Anti-patterns:
  - Returns the JUCE component ID, not the HISEScript component name. These may
    differ depending on the component wrapping hierarchy.

Source:
  ScriptingApiContent.cpp:9048  Content::getComponentUnderMouse()
    -> Desktop::getInstance().getMainMouseSource().getComponentUnderMouse()
    -> returns JUCE componentID
