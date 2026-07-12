ScriptImage::getGlobalPositionY() -> Integer

Thread safety: SAFE
Returns the absolute y-position relative to the interface root, computed by
recursively adding parent component y-offsets.
Pair with:
  getGlobalPositionX -- absolute x-position
Source:
  ScriptingApiContent.cpp  ScriptComponent::getGlobalPositionY()
    -> walks parentComponent chain -> sums y offsets
