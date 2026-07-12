ScriptImage::getAllProperties() -> Array

Thread safety: UNSAFE
Returns an array of strings containing all active (non-deactivated) property IDs
for this component. Includes both base ScriptComponent properties and
ScriptImage-specific properties (alpha, fileName, offset, scale, blendMode,
allowCallbacks, popupMenuItems, popupOnRightClick).
Pair with:
  get -- retrieve a property value by ID
  set -- set a property value by ID
Source:
  ScriptingApiContent.cpp  ScriptComponent::getAllProperties()
    -> iterates propertyIds, skips deactivated -> returns StringArray
