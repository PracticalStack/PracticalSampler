ContainerChild::getLocalBounds(Integer margin) -> Array

Thread safety: UNSAFE
Returns the component's bounds as [0, 0, width, height] reduced by the given
margin on all sides. Uses the component's width and height properties (defaults:
128, 50 if not set).
Pair with:
  setBounds -- sets position and size from a rectangle array
Source:
  ScriptingApiContent.cpp  ChildReference::getLocalBounds()
    -> reads width/height from componentData (defaults 128, 50)
    -> returns [0, 0, width - 2*margin, height - 2*margin]
