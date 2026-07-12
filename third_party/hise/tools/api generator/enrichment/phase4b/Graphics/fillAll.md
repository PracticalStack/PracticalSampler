Graphics::fillAll(Colour colour) -> undefined

Thread safety: UNSAFE -- allocates a new draw action, converts the colour var
Fills the entire component area with the specified colour. Typically the first call
in a paint routine to set the background. Does NOT require setColour first -- the
colour is passed directly as a parameter.

Source:
  ScriptingGraphics.cpp  GraphicsObject::fillAll()
    -> getCleanedObjectColour() converts var to Colour
    -> new draw action that calls g.fillAll(colour) on UI thread
