ScriptDynamicContainer::getGlobalPositionX() -> Integer

Thread safety: SAFE
Returns the absolute x-position relative to the interface root, computed by
recursively adding parent component x-offsets.
Pair with:
  getGlobalPositionY -- absolute y-position
Source:
  ScriptingApiContent.cpp  ScriptComponent::getGlobalPositionX()
    -> walks parent chain summing x offsets
