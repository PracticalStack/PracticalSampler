ContainerChild::setBounds(Array area) -> undefined

Thread safety: UNSAFE
Sets the component's position and size from a rectangle array [x, y, width,
height]. Equivalent to setting x, y, width, and height properties individually
but in a single call. Respects the undo manager.
Pair with:
  getLocalBounds -- returns bounds at origin with margin reduction
Source:
  ScriptingApiContent.cpp  ChildReference::setBounds()
    -> sets x, y, width, height properties on componentData from array
