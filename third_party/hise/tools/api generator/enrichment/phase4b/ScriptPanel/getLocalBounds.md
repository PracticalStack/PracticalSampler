ScriptPanel::getLocalBounds(Double reduceAmount) -> Array

Thread safety: SAFE
Returns [x, y, w, h] representing the local bounds reduced by the given amount.
Starts at [0, 0, width, height] and insets each edge by reduceAmount pixels.
Source:
  ScriptingApiContent.cpp  ScriptComponent::getLocalBounds()
