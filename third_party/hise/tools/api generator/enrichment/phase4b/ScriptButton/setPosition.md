ScriptButton::setPosition(Integer x, Integer y, Integer w, Integer h) -> undefined

Thread safety: UNSAFE
Sets the component's position and size in one call. Directly sets the x, y, width,
height properties on the property tree.

Pair with:
  getWidth / getHeight -- read back dimensions
  getLocalBounds -- get bounds as [x, y, w, h] array

Source:
  ScriptingApiContent.cpp  ScriptComponent::setPosition()
