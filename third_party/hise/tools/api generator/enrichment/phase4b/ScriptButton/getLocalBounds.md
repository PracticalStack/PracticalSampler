ScriptButton::getLocalBounds(Double reduceAmount) -> Array

Thread safety: SAFE
Returns an array [x, y, w, h] representing the local bounds reduced (inset) by
the given pixel amount from each edge. Local bounds start at [0, 0, width, height].

Pair with:
  getWidth / getHeight -- individual dimension accessors
  setPosition -- set position and size

Source:
  ScriptingApiContent.cpp  ScriptComponent::getLocalBounds()
