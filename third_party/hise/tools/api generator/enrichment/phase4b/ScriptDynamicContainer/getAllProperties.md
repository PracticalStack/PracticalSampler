ScriptDynamicContainer::getAllProperties() -> Array

Thread safety: UNSAFE
Returns an array of strings containing all active (non-deactivated) property IDs.
ScriptDynamicContainer deactivates 16 base properties, so this returns a reduced
set: visible, enabled, locked, x, y, width, height, bgColour, itemColour,
itemColour2, textColour, useUndoManager, parentComponent.
Pair with:
  get -- read a property by name
  set -- write a property by name
Source:
  ScriptingApiContent.cpp  ScriptComponent::getAllProperties()
    -> iterates property IDs, skips deactivatedProperties
