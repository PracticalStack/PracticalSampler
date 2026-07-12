ScriptDynamicContainer::getLocalBounds(Double reduceAmount) -> Array

Thread safety: SAFE
Returns [x, y, w, h] representing local bounds reduced by the given pixel amount.
Starts at [0, 0, width, height] and insets from each edge.
Source:
  ScriptingApiContent.cpp  ScriptComponent::getLocalBounds()
