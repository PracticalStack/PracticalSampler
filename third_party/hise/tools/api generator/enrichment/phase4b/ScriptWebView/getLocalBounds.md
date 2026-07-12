ScriptWebView::getLocalBounds(Double reduceAmount) -> Array

Thread safety: SAFE
Returns [x, y, w, h] representing the local bounds reduced by the given amount.
Starts at [0, 0, width, height] and insets by reduceAmount from each edge.
Source:
  ScriptingApiContent.cpp  ScriptComponent::getLocalBounds() (base class)
