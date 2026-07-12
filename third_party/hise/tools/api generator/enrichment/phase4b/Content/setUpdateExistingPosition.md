Content::setUpdateExistingPosition(bool shouldUpdateExistingComponents) -> void

Thread safety: SAFE
Controls whether re-calling addXXX() on an already-existing component updates that
component's x/y position. Defaults to true. Set to false when component positions are
managed dynamically at runtime and should not be reset on recompile.

Dispatch/mechanics:
  Sets the updateExistingPositions bool member.
  Checked in addComponent<T>(): if ((x != -1 && y != -1) && updateExistingPositions)
    -> when true, existing component's x/y are overwritten
    -> when false, existing component position is preserved

Pair with:
  All addXXX() component creation methods -- this controls their position-update behavior
  setPropertiesFromJSON -- alternative way to set positions dynamically

Source:
  ScriptingApiContent.cpp:~10129  Content::setUpdateExistingPosition()
  ScriptingApiContent.h:~2994  updateExistingPositions member
  ScriptingApiContent.h:~3521  addComponent<T>() template checks the flag
