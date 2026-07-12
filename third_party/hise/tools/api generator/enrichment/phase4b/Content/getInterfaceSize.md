Content::getInterfaceSize() -> Array

Thread safety: SAFE -- returns two integer values from member variables, no allocations beyond array construction.
Returns the current interface dimensions as a [width, height] array.

Pair with:
  setWidth/setHeight -- set dimensions individually
  makeFrontInterface -- set both dimensions at once

Source:
  ScriptingApiContent.cpp  Content::getInterfaceSize()
    -> returns [width, height] from member variables
